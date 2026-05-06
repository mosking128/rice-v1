from __future__ import annotations

import html
from pathlib import Path

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QComboBox,
    QFileDialog,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from picoc_session import PicocSession
from serial_manager import SerialManager


FILE_ITEM_ROLE = 256

ERROR_KEYWORDS = (
    "parse error",
    "is undefined",
    "file input not supported",
    "not connected",
    "timed out",
    "failed",
    "abort",
)


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("PicoC 上位机工具")
        self.resize(1100, 800)

        self._console_line_buffer = ""
        self._upload_active = False
        self._execution_separator_pending = False
        self._serial_manager = SerialManager()
        self._session = PicocSession()

        self._batch_queue: list[Path] = []
        self._batch_results: list[tuple[Path, bool, str]] = []
        self._batch_active = False
        self._current_upload_path: Path | None = None
        self._single_step_mode = False

        self._build_ui()
        self._connect_signals()
        self._refresh_ports()
        self._update_ui_state()

    def _build_ui(self) -> None:
        central = QWidget(self)
        root = QVBoxLayout(central)

        root.addWidget(self._build_connection_group())
        root.addWidget(self._build_console_group(), 1)
        root.addWidget(self._build_debug_toolbar())
        root.addWidget(self._build_actions_group())

        self.setCentralWidget(central)

    def _build_debug_toolbar(self) -> QFrame:
        self.debug_toolbar = QFrame(self)
        self.debug_toolbar.setFrameStyle(QFrame.StyledPanel | QFrame.Raised)
        layout = QHBoxLayout(self.debug_toolbar)
        layout.setContentsMargins(6, 4, 6, 4)

        self.debug_info_label = QLabel("调试未激活")
        layout.addWidget(self.debug_info_label, 1)

        layout.addWidget(QLabel("行号"))
        self.debug_bp_line = QLineEdit()
        self.debug_bp_line.setPlaceholderText("42")
        self.debug_bp_line.setFixedWidth(50)
        layout.addWidget(self.debug_bp_line)

        self.debug_bp_set_btn = QPushButton("设断点")
        self.debug_bp_clear_btn = QPushButton("清断点")
        layout.addWidget(self.debug_bp_set_btn)
        layout.addWidget(self.debug_bp_clear_btn)

        layout.addWidget(QLabel("  "))
        self.debug_continue_btn = QPushButton("继续")
        self.debug_step_btn = QPushButton("单步")
        self.debug_eval_input = QLineEdit()
        self.debug_eval_input.setPlaceholderText("表达式...")
        self.debug_eval_input.setFixedWidth(120)
        self.debug_eval_btn = QPushButton("求值")

        layout.addWidget(self.debug_continue_btn)
        layout.addWidget(self.debug_step_btn)
        layout.addWidget(self.debug_eval_input)
        layout.addWidget(self.debug_eval_btn)
        return self.debug_toolbar

    def _build_connection_group(self) -> QGroupBox:
        group = QGroupBox("连接")
        layout = QGridLayout(group)

        self.port_combo = QComboBox()
        self.refresh_button = QPushButton("刷新")
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["115200", "230400", "460800", "921600"])
        self.baud_combo.setCurrentText("115200")
        self.connect_button = QPushButton("连接")
        self.disconnect_button = QPushButton("断开")
        self.status_label = QLabel("未连接")
        self.mode_label = QLabel("未连接")

        layout.addWidget(QLabel("串口"), 0, 0)
        layout.addWidget(self.port_combo, 0, 1)
        layout.addWidget(self.refresh_button, 0, 2)
        layout.addWidget(QLabel("波特率"), 0, 3)
        layout.addWidget(self.baud_combo, 0, 4)
        layout.addWidget(self.connect_button, 0, 5)
        layout.addWidget(self.disconnect_button, 0, 6)
        layout.addWidget(QLabel("状态"), 1, 0)
        layout.addWidget(self.status_label, 1, 1, 1, 3)
        layout.addWidget(QLabel("模式"), 1, 4)
        layout.addWidget(self.mode_label, 1, 5, 1, 2)
        return group

    def _build_console_group(self) -> QGroupBox:
        group = QGroupBox("控制台")
        layout = QVBoxLayout(group)

        self.console_view = QTextEdit()
        self.console_view.setReadOnly(True)
        self.console_view.setAcceptRichText(True)
        self.console_view.setLineWrapMode(QTextEdit.NoWrap)
        self.console_view.setPlaceholderText("这里显示 PicoC 代码回显、执行结果、错误信息和批量测试日志。")

        input_row = QHBoxLayout()
        self.manual_input = QLineEdit()
        self.manual_input.setPlaceholderText("输入 PicoC 命令或表达式...")
        self.send_button = QPushButton("发送")
        input_row.addWidget(self.manual_input, 1)
        input_row.addWidget(self.send_button)

        layout.addWidget(self.console_view, 1)
        layout.addLayout(input_row)
        return group

    def _build_actions_group(self) -> QGroupBox:
        group = QGroupBox("操作")
        layout = QGridLayout(group)

        self.file_path_edit = QLineEdit()
        self.file_path_edit.setPlaceholderText("选择一个 .c 文件或从列表中选中...")
        self.browse_button = QPushButton("添加文件")
        self.file_list = QListWidget()
        self.file_list.setSelectionMode(QListWidget.SingleSelection)

        self.upload_button = QPushButton("执行选中")
        self.run_all_button = QPushButton("全部执行")
        self.clear_list_button = QPushButton("清空列表")
        self.abort_button = QPushButton("中止")
        self.clear_button = QPushButton("清空控制台")
        self.save_button = QPushButton("保存日志")

        layout.addWidget(QLabel("当前文件"), 0, 0)
        layout.addWidget(self.file_path_edit, 0, 1, 1, 3)
        layout.addWidget(self.browse_button, 0, 4)

        layout.addWidget(QLabel("待测文件"), 1, 0)
        layout.addWidget(self.file_list, 1, 1, 3, 3)
        layout.addWidget(self.upload_button, 1, 4)
        layout.addWidget(self.run_all_button, 2, 4)
        layout.addWidget(self.abort_button, 3, 4)

        layout.addWidget(self.clear_list_button, 4, 1)
        layout.addWidget(self.save_button, 4, 4)
        layout.addWidget(self.clear_button, 5, 4)
        return group

    def _connect_signals(self) -> None:
        self.refresh_button.clicked.connect(self._refresh_ports)
        self.connect_button.clicked.connect(self._connect_serial)
        self.disconnect_button.clicked.connect(self._disconnect_serial)
        self.send_button.clicked.connect(self._send_manual_input)
        self.manual_input.returnPressed.connect(self._send_manual_input)

        self.browse_button.clicked.connect(self._browse_files)
        self.upload_button.clicked.connect(self._upload_file)
        self.run_all_button.clicked.connect(self._run_all_files)
        self.clear_list_button.clicked.connect(self._clear_file_list)
        self.abort_button.clicked.connect(self._abort_action)
        self.clear_button.clicked.connect(self._clear_console)
        self.save_button.clicked.connect(self._save_log)
        self.file_list.currentItemChanged.connect(self._sync_selected_file_path)

        self._serial_manager.text_received.connect(self._session.handle_incoming_text)
        self._serial_manager.error_occurred.connect(self._handle_error)
        self._serial_manager.connection_changed.connect(self._handle_connection_changed)

        self._session.send_requested.connect(self._serial_manager.send_text)
        self._session.console_text.connect(self._append_console_text)
        self._session.mode_changed.connect(self._handle_mode_changed)
        self._session.status_changed.connect(self._set_status)
        self._session.upload_finished.connect(self._handle_upload_finished)
        self._session.upload_state_changed.connect(self._handle_upload_state_changed)

        self._session.debug_break.connect(self._on_debug_break)
        self._session.debug_resumed.connect(self._on_debug_resumed)
        self.debug_continue_btn.clicked.connect(lambda: self._session.send_debug_continue())
        self.debug_step_btn.clicked.connect(lambda: self._session.send_debug_step())
        self.debug_eval_btn.clicked.connect(self._on_eval_clicked)
        self.debug_eval_input.returnPressed.connect(self._on_eval_clicked)
        self.debug_bp_set_btn.clicked.connect(self._on_bp_set_clicked)
        self.debug_bp_clear_btn.clicked.connect(self._on_bp_clear_clicked)

    def _refresh_ports(self) -> None:
        current = self.port_combo.currentText()
        ports = SerialManager.list_ports()
        self.port_combo.clear()
        self.port_combo.addItems(ports)
        if current and current in ports:
            self.port_combo.setCurrentText(current)

    def _connect_serial(self) -> None:
        port_name = self.port_combo.currentText().strip()
        if not port_name:
            self._show_warning("未选择串口", "请先选择一个串口。")
            return
        baudrate = int(self.baud_combo.currentText())
        self._serial_manager.open(port_name, baudrate)

    def _disconnect_serial(self) -> None:
        self._serial_manager.close()

    def _send_manual_input(self) -> None:
        text = self.manual_input.text().strip("\r\n")
        if not text:
            return
        if self._session.send_manual(text):
            self.manual_input.clear()

    def _browse_files(self) -> None:
        filenames, _ = QFileDialog.getOpenFileNames(
            self,
            "选择 C 文件",
            "",
            "C source (*.c);;All files (*.*)",
        )
        if filenames:
            self._add_files_to_list([Path(name) for name in filenames])

    def _add_files_to_list(self, paths: list[Path]) -> None:
        existing = {path.resolve() for path in self._get_all_listed_files()}
        added_items: list[QListWidgetItem] = []
        for path in paths:
            if not path.exists() or not path.is_file() or path.suffix.lower() != ".c":
                continue
            resolved = path.resolve()
            if resolved in existing:
                continue
            item = QListWidgetItem(path.name)
            item.setData(FILE_ITEM_ROLE, str(resolved))
            item.setToolTip(str(resolved))
            self.file_list.addItem(item)
            added_items.append(item)
            existing.add(resolved)

        if added_items:
            self.file_list.setCurrentItem(added_items[0])
            self._append_info_line(f"已添加 {len(added_items)} 个文件到待测列表。")
        self._update_ui_state()

    def _clear_file_list(self) -> None:
        self.file_list.clear()
        self.file_path_edit.clear()
        self._update_ui_state()

    def _sync_selected_file_path(
        self,
        current: QListWidgetItem | None,
        previous: QListWidgetItem | None,
    ) -> None:
        _ = previous
        if current is None:
            self.file_path_edit.clear()
            return
        path = current.data(FILE_ITEM_ROLE)
        if isinstance(path, str):
            self.file_path_edit.setText(path)

    def _get_selected_file_path(self) -> Path | None:
        item = self.file_list.currentItem()
        if item is None:
            return None
        path = item.data(FILE_ITEM_ROLE)
        if not isinstance(path, str) or not path:
            return None
        return Path(path)

    def _get_all_listed_files(self) -> list[Path]:
        paths: list[Path] = []
        for index in range(self.file_list.count()):
            item = self.file_list.item(index)
            path = item.data(FILE_ITEM_ROLE)
            if isinstance(path, str) and path:
                paths.append(Path(path))
        return paths

    def _upload_file(self) -> None:
        file_path = self._get_selected_file_path()
        if file_path is None:
            raw_path = self.file_path_edit.text().strip()
            if raw_path:
                file_path = Path(raw_path)

        if file_path is None or not file_path.exists():
            self._show_warning("文件无效", "请先从列表中选中一个 .c 文件，或输入有效路径。")
            return

        self._batch_active = False
        self._batch_queue.clear()
        self._batch_results.clear()
        self._single_step_mode = True
        self._start_upload_for_path(file_path)

    def _run_all_files(self) -> None:
        self._single_step_mode = False
        self._start_batch(self._get_all_listed_files(), "文件批量测试")

    def _start_batch(self, paths: list[Path], title: str) -> None:
        valid_paths = [path for path in paths if path.exists() and path.is_file()]
        if not valid_paths:
            self._show_warning("没有可测试文件", "请先往待测列表中添加 .c 文件。")
            return

        self._batch_active = True
        self._batch_queue = valid_paths[1:]
        self._batch_results = []
        self._single_step_mode = False
        self._append_separator_line(title)
        self._append_info_line(f"共 {len(valid_paths)} 个文件，失败后继续执行后续文件。")
        self._start_upload_for_path(valid_paths[0])

    def _start_upload_for_path(self, file_path: Path) -> None:
        source_text = self._read_source_file(file_path)
        if source_text is None:
            if self._batch_active:
                self._batch_results.append((file_path, False, "读取文件失败"))
                self._continue_batch_or_finish()
            return

        self._current_upload_path = file_path
        self.file_path_edit.setText(str(file_path))
        self._execution_separator_pending = True
        if self._batch_active:
            tested = len(self._batch_results)
            total = len(self._batch_results) + len(self._batch_queue) + 1
            self._append_separator_line(f"测试 [{tested}/{total}]: {file_path.name}")
        else:
            self._append_separator_line(f"上传: {file_path.name}")
        total_bytes = sum(len(line) + 1 for line in source_text.replace("\r\n", "\n").replace("\r", "\n").split("\n"))
        if total_bytes <= 8191:
            self._append_uploaded_source(file_path, source_text)

        if not self._session.start_upload(source_text):
            self._execution_separator_pending = False
            self._append_info_line(f"启动上传失败: {file_path.name}")
            if self._batch_active:
                self._batch_results.append((file_path, False, "启动上传失败"))
                self._continue_batch_or_finish()
        self._update_ui_state()

    def _read_source_file(self, file_path: Path) -> str | None:
        try:
            return file_path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            try:
                return file_path.read_text(encoding="gbk")
            except Exception as exc:
                self._show_warning("打开文件失败", f"{file_path}\n{exc}")
                return None
        except Exception as exc:
            self._show_warning("打开文件失败", f"{file_path}\n{exc}")
            return None

    def _abort_action(self) -> None:
        self._batch_active = False
        self._batch_queue.clear()
        self._single_step_mode = False
        self._session.abort()

    def _save_log(self) -> None:
        filename, _ = QFileDialog.getSaveFileName(
            self,
            "保存日志",
            "picoc_console_log.txt",
            "Text files (*.txt);;All files (*.*)",
        )
        if not filename:
            return

        try:
            Path(filename).write_text(self.console_view.toPlainText(), encoding="utf-8")
        except Exception as exc:
            self._show_warning("保存日志失败", str(exc))
            return

        self._set_status(f"日志已保存到 {filename}")

    def _handle_connection_changed(self, connected: bool, port_name: str) -> None:
        self._session.set_connected(connected)
        if connected:
            self._append_info_line(f"已连接到 {port_name}")
            QTimer.singleShot(800, lambda: self._session.send_ping())
        else:
            self._append_info_line("已断开连接。")
            self._execution_separator_pending = False
            self._batch_active = False
            self._batch_queue.clear()
            self._single_step_mode = False
        self._update_ui_state()

    def _handle_mode_changed(self, mode: str) -> None:
        self.mode_label.setText(self._translate_mode(mode))
        self._update_ui_state()

    def _handle_upload_finished(self, success: bool, message: str) -> None:
        self._append_info_line(message)
        self._execution_separator_pending = False
        self.debug_info_label.setText("调试未激活")

        if self._batch_active and self._current_upload_path is not None:
            self._batch_results.append((self._current_upload_path, success, message))
            self._continue_batch_or_finish()
        else:
            if not success and "超时" in message:
                self._show_warning("上传超时", message)
            self._advance_to_next_file_selection()
            self._current_upload_path = None
            self._single_step_mode = False

        self._update_ui_state()

    def _continue_batch_or_finish(self) -> None:
        if self._batch_queue:
            next_file = self._batch_queue.pop(0)
            QTimer.singleShot(0, lambda: self._start_upload_for_path(next_file))
            return

        passed = sum(1 for _, success, _ in self._batch_results if success)
        failed = len(self._batch_results) - passed
        self._append_separator_line("批量测试结束")
        self._append_info_line(f"总计 {len(self._batch_results)} 执行完成 {passed} 执行错误 {failed}")
        for path, success, message in self._batch_results:
            result_text = "PASS" if success else "FAIL"
            self._append_info_line(f"  {result_text}  {path.name}  ({message})")
        self._batch_active = False
        self._current_upload_path = None
        self._single_step_mode = False

    def _advance_to_next_file_selection(self) -> None:
        if not self._single_step_mode:
            return

        current_row = self.file_list.currentRow()
        if current_row < 0:
            return

        next_row = current_row + 1
        if next_row < self.file_list.count():
            self.file_list.setCurrentRow(next_row)

    def _handle_upload_state_changed(self, active: bool) -> None:
        self._upload_active = active
        if not active:
            self._execution_separator_pending = False
            self.debug_info_label.setText("调试未激活")
        self._update_ui_state()

    def _on_debug_break(self, filename: str, line_no: int) -> None:
        self.debug_info_label.setText(f"已中断: 第{line_no}行")
        self._update_ui_state()

    def _on_debug_resumed(self) -> None:
        self.debug_info_label.setText("调试未激活")
        self._update_ui_state()

    def _on_eval_clicked(self) -> None:
        expr = self.debug_eval_input.text().strip()
        if not expr:
            return
        if self._session.send_debug_eval(expr):
            self.debug_eval_input.clear()

    @staticmethod
    def _bp_filename() -> str:
        return "serial_load"

    def _read_bp_params(self) -> tuple[str, int] | None:
        try:
            line = int(self.debug_bp_line.text().strip())
        except ValueError:
            self._show_warning("断点", "请输入有效的行号。")
            return None
        if line <= 0:
            self._show_warning("断点", "行号必须大于 0。")
            return None
        return self._bp_filename(), line

    def _on_bp_set_clicked(self) -> None:
        params = self._read_bp_params()
        if params is None:
            return
        filename, line = params
        self._session.send_breakpoint_set(filename, line)

    def _on_bp_clear_clicked(self) -> None:
        params = self._read_bp_params()
        if params is None:
            return
        filename, line = params
        self._session.send_breakpoint_clear(filename, line)

    def _handle_error(self, message: str) -> None:
        self._append_info_line(message)
        self._set_status(message)
        self._update_ui_state()
        self._show_warning("串口错误", message)

    def _set_status(self, message: str) -> None:
        self.status_label.setText(message)
        if self._upload_active and self._execution_separator_pending and message == "正在执行文件...":
            self._append_separator_line("执行结果")
            self._execution_separator_pending = False

    def _update_ui_state(self) -> None:
        connected = self._serial_manager.is_connected()
        busy = self._session.mode == "BUSY"
        debug_active = self._session._debug_active
        has_files = self.file_list.count() > 0
        has_selection = self.file_list.currentItem() is not None or bool(self.file_path_edit.text().strip())

        self.connect_button.setEnabled(not connected)
        self.disconnect_button.setEnabled(connected)
        self.refresh_button.setEnabled(not connected)
        self.port_combo.setEnabled(not connected)
        self.baud_combo.setEnabled(not connected)

        self.manual_input.setEnabled(connected and not busy and not debug_active)
        self.send_button.setEnabled(connected and not busy and not debug_active)

        self.browse_button.setEnabled(connected and not busy)
        self.clear_list_button.setEnabled(not busy and has_files)
        self.file_list.setEnabled(not busy)
        self.file_path_edit.setEnabled(not busy)

        self.upload_button.setEnabled(connected and not busy and has_selection)
        self.run_all_button.setEnabled(connected and not busy and has_files)
        self.abort_button.setEnabled(connected)
        self.save_button.setEnabled(True)
        self.clear_button.setEnabled(True)

        self.debug_continue_btn.setEnabled(debug_active)
        self.debug_step_btn.setEnabled(debug_active)
        self.debug_eval_input.setEnabled(debug_active)
        self.debug_eval_btn.setEnabled(debug_active)
        self.debug_bp_line.setEnabled(connected and not busy)
        self.debug_bp_set_btn.setEnabled(connected and not busy)
        self.debug_bp_clear_btn.setEnabled(connected and not busy)

    def _append_console_text(self, text: str) -> None:
        text = text.replace("\r\n", "\n").replace("\r", "\n")
        self._console_line_buffer += text
        parts = self._console_line_buffer.split("\n")
        self._console_line_buffer = parts.pop()

        for line in parts:
            self._append_remote_line(line)

        if self._console_line_buffer.endswith("picoc> ") or self._console_line_buffer.endswith("load> "):
            self._append_remote_line(self._console_line_buffer)
            self._console_line_buffer = ""

    def _append_uploaded_source(self, file_path: Path, source_text: str) -> None:
        self._append_separator_line(f"上传文件 {file_path.name}")
        self._append_info_line(f"文件路径: {file_path}")
        for line in source_text.replace("\r\n", "\n").replace("\r", "\n").split("\n"):
            self._append_html_line(line, "#202020")

    def _append_local_command(self, text: str) -> None:
        self._append_html_line(text, "#202020")

    def _append_remote_line(self, line: str) -> None:
        if self._should_filter_control_echo(line):
            return

        lowered = line.lower()
        if any(keyword in lowered for keyword in ERROR_KEYWORDS):
            color = "#c0392b"
        elif "ready for next file" in lowered:
            color = "#1f7a1f"
        elif "picoc>" in line or "load>" in line:
            color = "#1f5aa6"
        else:
            color = "#202020"

        self._append_html_line(line, color)

    def _append_info_line(self, text: str) -> None:
        self._append_html_line(text, "#666666")

    def _append_separator_line(self, title: str) -> None:
        self._append_html_line(f"---------------- {title} ----------------", "#8a6d3b")

    def _append_html_line(self, text: str, color: str) -> None:
        escaped = html.escape(text)
        self.console_view.append(
            f'<span style="color:{color}; white-space:pre;">{escaped}</span>'
        )
        scrollbar = self.console_view.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())

    def _clear_console(self) -> None:
        self.console_view.clear()
        self._console_line_buffer = ""
        self._execution_separator_pending = False

    @staticmethod
    def _should_filter_control_echo(line: str) -> bool:
        stripped = line.strip()
        if stripped in {":end", ":abort", ":ping", ":reset", ":pong", ":ok", ":ok ready",
                        ":ok eval", ":ok bkpt", ":ok bkptclear", "load>"}:
            return True
        if stripped.startswith(":load") or stripped.startswith(":err"):
            return True
        if stripped.startswith(":break") or stripped.startswith(":step"):
            return True
        if stripped in {":cont", ":step"}:
            return True
        if stripped.startswith(":eval "):
            return True
        if stripped.startswith(":bkpt ") or stripped.startswith(":bkptclear "):
            return True
        if stripped == ":result" or stripped == ":ok":
            return True
        if line.startswith("load> "):
            return True
        return False

    def _show_warning(self, title: str, message: str) -> None:
        QMessageBox.warning(self, title, message)

    @staticmethod
    def _translate_mode(mode: str) -> str:
        return {
            "DISCONNECTED": "未连接",
            "UNKNOWN": "等待识别",
            "REPL": "交互模式",
            "LOAD": "文件模式",
            "BUSY": "忙碌中",
        }.get(mode, mode)
