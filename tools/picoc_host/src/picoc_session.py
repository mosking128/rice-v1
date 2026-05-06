from __future__ import annotations

from typing import Optional

from PySide6.QtCore import QObject, Signal

from upload_job import UploadJob, UploadPhase

EXEC_ERROR_PATTERNS = (
    ("语法错误", "parse error"),
    ("语法错误", "brackets not closed"),
    ("语法错误", "operator not expected"),
    ("未定义", "is undefined"),
    ("重复定义", "is already defined"),
    ("重复定义", "redefinition"),
    ("嵌套函数", "nested function"),
    ("返回值错误", "value in return"),
)


class PicocSession(QObject):
    send_requested = Signal(str)
    console_text = Signal(str)
    mode_changed = Signal(str)
    status_changed = Signal(str)
    upload_finished = Signal(bool, str)
    upload_state_changed = Signal(bool)
    debug_break = Signal(str, int)
    debug_resumed = Signal()

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self._connected = False
        self._mode = "DISCONNECTED"
        self._observed_mode = "UNKNOWN"
        self._prompt_window = ""
        self._response_line_buffer = ""
        self._last_ready_marker_index = -1
        self._upload_job: Optional[UploadJob] = None
        self._ping_acknowledged = False
        self._debug_active = False
        self._debug_file = ""
        self._debug_line = 0

    @property
    def mode(self) -> str:
        return self._mode

    def set_connected(self, connected: bool) -> None:
        self._connected = connected
        self._prompt_window = ""
        self._response_line_buffer = ""
        self._last_ready_marker_index = -1
        self._observed_mode = "UNKNOWN" if connected else "DISCONNECTED"
        self._set_mode("UNKNOWN" if connected else "DISCONNECTED")
        self._ping_acknowledged = False
        self._debug_active = False
        self._debug_file = ""
        self._debug_line = 0
        if connected:
            self.status_changed.emit("已连接，等待 PicoC 提示符...")
        else:
            self.status_changed.emit("已断开连接。")
            self._cancel_upload("连接断开")

    def handle_incoming_text(self, text: str) -> None:
        self.console_text.emit(text)
        self._prompt_window = (self._prompt_window + text)[-512:]

        self._response_line_buffer += text
        *complete_lines, self._response_line_buffer = self._response_line_buffer.split("\n")
        for line in complete_lines:
            stripped = line.strip()
            if self._parse_structured_line(stripped):
                continue
            if self._upload_job is not None and self._upload_job.phase.name in ("SENDING_LINES", "WAIT_EXEC_COMPLETE"):
                lowered = stripped.lower()
                if self._upload_job.error_type is None:
                    for category, kw in EXEC_ERROR_PATTERNS:
                        if kw in lowered:
                            self._upload_job.error_type = category
                            break

        if "load buffer full" in self._prompt_window.lower():
            if self._upload_job is not None:
                self._cancel_upload("缓冲区满，跳过")
                self._prompt_window = ""
                return

        ready_marker = "ready for next file"
        ready_index = self._prompt_window.lower().rfind(ready_marker)
        if ready_index >= 0 and ready_index != self._last_ready_marker_index:
            self._last_ready_marker_index = ready_index
            self.status_changed.emit("设备已准备好接收下一个文件。")
            if self._upload_job is not None:
                self._upload_job.observe_ready_for_next_file()

        detected_mode: Optional[str] = None
        last_picoc = self._prompt_window.rfind("picoc> ")
        last_load = self._prompt_window.rfind("load> ")
        if last_picoc >= 0 or last_load >= 0:
            detected_mode = "REPL" if last_picoc > last_load else "LOAD"

        if detected_mode is not None:
            self._observed_mode = detected_mode
            if self._upload_job is None:
                self._set_mode(detected_mode)
            elif detected_mode == "REPL" and self._upload_job.phase == UploadPhase.SENDING_LINES:
                pass  # stale prompt from before :load, structured parser already transitioned
            else:
                self._upload_job.observe_mode(detected_mode)

    def send_manual(self, text: str) -> bool:
        if not self._connected:
            self.status_changed.emit("当前未连接。")
            return False
        if self._upload_job is not None and not self._debug_active:
            self.status_changed.emit("正在上传文件，已禁止手工发送。")
            return False

        payload = text.replace("\r\n", "\n").replace("\r", "\n")
        if not payload.endswith("\n"):
            payload += "\n"
        self.send_requested.emit(payload.replace("\n", "\r\n"))
        self.status_changed.emit("手工命令已发送。")
        return True

    def send_ping(self) -> bool:
        if not self._connected:
            return False
        self._ping_acknowledged = False
        self.send_requested.emit(":ping\r\n")
        return True

    def start_upload(self, source_text: str) -> bool:
        if not self._connected:
            self.status_changed.emit("当前未连接。")
            return False
        if self._upload_job is not None:
            self.status_changed.emit("已有上传任务正在运行。")
            return False

        job = UploadJob(source_text, self)
        self._upload_job = job
        job.send_requested.connect(self.send_requested)
        job.progress_changed.connect(self.status_changed)
        job.finished.connect(self._handle_upload_finished)
        self.upload_state_changed.emit(True)
        self._set_mode("BUSY")
        job.start(self._observed_mode)
        return True

    def send_debug_continue(self) -> bool:
        if not self._debug_active:
            return False
        self.send_requested.emit(":cont\r\n")
        self.status_changed.emit("已发送继续执行。")
        self._on_debug_command_sent()
        return True

    def send_debug_step(self) -> bool:
        if not self._debug_active:
            return False
        self.send_requested.emit(":step\r\n")
        self.status_changed.emit("已发送单步执行。")
        self._on_debug_command_sent()
        return True

    def send_debug_eval(self, expr: str) -> bool:
        if not self._debug_active:
            return False
        payload = expr.strip().replace("\r\n", " ").replace("\r", " ").replace("\n", " ")
        self.send_requested.emit(f":eval {payload}\r\n")
        self.status_changed.emit(f"已发送求值: {payload}")
        return True

    def send_breakpoint_set(self, filename: str, line: int) -> bool:
        if not self._connected:
            self.status_changed.emit("当前未连接。")
            return False
        self.send_requested.emit(f":bkpt {filename} {line}\r\n")
        info = f"设置断点: 第{line}行"
        self.console_text.emit(f"{info}\r\n")
        self.status_changed.emit(info)
        return True

    def send_breakpoint_clear(self, filename: str, line: int) -> bool:
        if not self._connected:
            self.status_changed.emit("当前未连接。")
            return False
        self.send_requested.emit(f":bkptclear {filename} {line}\r\n")
        info = f"清除断点: 第{line}行"
        self.console_text.emit(f"{info}\r\n")
        self.status_changed.emit(info)
        return True

    def _on_debug_command_sent(self) -> None:
        if self._upload_job is not None:
            self._upload_job.debug_resume()

    def _enter_debug(self, filename: str, line_no: int) -> None:
        self._debug_active = True
        self._debug_file = filename
        self._debug_line = line_no
        if self._upload_job is not None:
            self._upload_job.debug_pause()
        self.debug_break.emit(filename, line_no)

    def _leave_debug(self) -> None:
        if self._debug_active:
            self._debug_active = False
            self._debug_file = ""
            self._debug_line = 0
            self.debug_resumed.emit()

    def abort(self) -> bool:
        if not self._connected:
            self.status_changed.emit("当前未连接。")
            return False

        self.send_requested.emit(":abort\r\n")
        self.status_changed.emit("已发送中止命令。")
        if self._upload_job is not None:
            self._cancel_upload("已取消")
        return True

    def _parse_structured_line(self, line: str) -> bool:
        if not line:
            return False

        if line.startswith(":break"):
            self._handle_break_line(line, "break")
            return True

        if line.startswith(":step"):
            self._handle_break_line(line, "step")
            return True

        if line.startswith(":ok"):
            data = line[3:].strip()
            self._dispatch_structured(":ok", data)
            return True

        if line.startswith(":err"):
            data = line[4:].strip()
            self._dispatch_structured(":err", data)
            return True

        if line == ":pong":
            self._dispatch_structured(":pong", "")
            return True

        if line.startswith(":result"):
            return True

        return False

    def _handle_break_line(self, line: str, kind: str) -> None:
        parts = line.split(" ", 3)
        filename = parts[1] if len(parts) > 1 else "(none)"
        try:
            line_no = int(parts[2]) if len(parts) > 2 else 0
        except ValueError:
            line_no = 0
        if filename == "(none)":
            return
        self._enter_debug(filename, line_no)
        info = f"单步至: 第{line_no}行" if kind == "step" else f"断点触发: 第{line_no}行"
        self.console_text.emit(f"{info}\r\n")
        self.status_changed.emit(info)

    def _dispatch_structured(self, prefix: str, data: str) -> None:
        if prefix == ":pong":
            self._ping_acknowledged = True
            self.status_changed.emit("设备心跳响应正常。")
            return

        if prefix == ":ok" and data in ("eval", "bkpt", "bkptclear"):
            self.status_changed.emit(f"调试操作完成: {data}")
            return

        if prefix == ":err" and data.startswith("debug "):
            self.status_changed.emit(f"调试错误: {data[6:]}")
            return

        if prefix == ":err" and data.startswith("eval "):
            self.status_changed.emit(f"表达式求值失败: {data[5:]}")
            return

        if self._upload_job is not None:
            if prefix == ":ok" and self._upload_job.phase == UploadPhase.WAIT_LOAD_ACK:
                self._observed_mode = "LOAD"
            self._upload_job.handle_response(prefix, data)

    def _handle_upload_finished(self, success: bool, message: str) -> None:
        if self._upload_job is not None:
            self._upload_job = None
        self._leave_debug()
        self.upload_state_changed.emit(False)
        self._set_mode(self._observed_mode)
        self.status_changed.emit(message)
        self.upload_finished.emit(success, message)

    def _cancel_upload(self, message: str) -> None:
        if self._upload_job is not None:
            job = self._upload_job
            self._upload_job = None
            job.finished.disconnect(self._handle_upload_finished)
            self.upload_state_changed.emit(False)
            job.cancel()
        self._leave_debug()
        self._set_mode(self._observed_mode if self._connected else "DISCONNECTED")
        self.upload_finished.emit(False, message)

    def _set_mode(self, mode: str) -> None:
        if self._mode != mode:
            self._mode = mode
            self.mode_changed.emit(mode)
