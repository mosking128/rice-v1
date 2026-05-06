from __future__ import annotations

from enum import Enum, auto

from PySide6.QtCore import QObject, QTimer, Signal


class UploadPhase(Enum):
    IDLE = auto()
    WAIT_LOAD_ACK = auto()
    SENDING_LINES = auto()
    WAIT_EXEC_COMPLETE = auto()
    FINISHED = auto()


class UploadJob(QObject):
    send_requested = Signal(str)
    progress_changed = Signal(str)
    finished = Signal(bool, str)

    def __init__(self, source_text: str, parent: QObject | None = None) -> None:
        super().__init__(parent)
        normalized = source_text.replace("\r\n", "\n").replace("\r", "\n")
        self._lines = normalized.split("\n")
        self._line_index = 0
        self._phase = UploadPhase.IDLE
        self.error_type: str | None = None
        self._timeout_timer = QTimer(self)
        self._timeout_timer.setSingleShot(True)
        self._timeout_timer.timeout.connect(self._handle_timeout)
        self._send_timer = QTimer(self)
        self._send_timer.setSingleShot(True)
        self._send_timer.timeout.connect(self._send_next_line)

    @property
    def phase(self) -> UploadPhase:
        return self._phase

    def start(self, current_mode: str) -> None:
        total_bytes = self._compute_total_bytes()
        self._progress(f"正在请求上传 ({total_bytes} 字节)...")
        self._phase = UploadPhase.WAIT_LOAD_ACK
        self.send_requested.emit(f":load {total_bytes}\r\n")
        self._timeout_timer.start(4000)

    def handle_response(self, prefix: str, data: str) -> None:
        if self._phase == UploadPhase.WAIT_LOAD_ACK:
            if prefix == ":ok":
                self._timeout_timer.stop()
                self._progress("设备已接受，开始发送文件...")
                self._phase = UploadPhase.SENDING_LINES
                self._send_timer.start(0)
            elif prefix == ":err":
                self._finish(False, "文件过大，跳过")
        elif self._phase == UploadPhase.SENDING_LINES:
            if prefix == ":err":
                self._finish(False, "执行错误")
        elif self._phase == UploadPhase.WAIT_EXEC_COMPLETE:
            if prefix == ":ok" and data == "ready":
                if self.error_type is not None:
                    self._finish(False, self.error_type)
                else:
                    self._finish(True, "执行完成")
            elif prefix == ":err":
                self._finish(False, "执行错误")

    def observe_mode(self, mode: str) -> None:
        if self._phase == UploadPhase.SENDING_LINES and mode == "REPL":
            self._finish(False, "通信异常")

    def observe_ready_for_next_file(self) -> None:
        if self._phase == UploadPhase.WAIT_EXEC_COMPLETE:
            self._finish(True, "执行完成")

    def debug_pause(self) -> None:
        """Pause timeout timer when debugger hits a breakpoint."""
        if self._phase == UploadPhase.WAIT_EXEC_COMPLETE:
            self._timeout_timer.stop()

    def debug_resume(self) -> None:
        """Resume timeout timer after debug continue/step."""
        if self._phase == UploadPhase.WAIT_EXEC_COMPLETE:
            self._timeout_timer.start(120000)

    def cancel(self) -> None:
        if self._phase != UploadPhase.FINISHED:
            self._finish(False, "已取消")

    def _compute_total_bytes(self) -> int:
        return sum(len(line) + 1 for line in self._lines)

    def _send_next_line(self) -> None:
        if self._phase != UploadPhase.SENDING_LINES:
            return

        if self._line_index < len(self._lines):
            self.send_requested.emit(self._lines[self._line_index] + "\r\n")
            self._line_index += 1
            self._send_timer.start(5)
            return

        self._phase = UploadPhase.WAIT_EXEC_COMPLETE
        self._progress("正在执行文件...")
        self.send_requested.emit(":end\r\n")
        self._timeout_timer.start(120000)

    def _handle_timeout(self) -> None:
        self._finish(False, "超时")

    def _finish(self, success: bool, message: str) -> None:
        self._timeout_timer.stop()
        self._send_timer.stop()
        self._phase = UploadPhase.FINISHED
        self.finished.emit(success, message)

    def _progress(self, message: str) -> None:
        self.progress_changed.emit(message)
