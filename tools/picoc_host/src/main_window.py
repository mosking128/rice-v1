from __future__ import annotations

from pathlib import Path

from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QColor, QFont, QTextCharFormat, QTextCursor
from PyQt5.QtWidgets import (
    QCheckBox,
    QComboBox,
    QFileDialog,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMenu,
    QMessageBox,
    QPushButton,
    QShortcut,
    QSplitter,
    QStackedWidget,
    QStatusBar,
    QTableWidget,
    QTableWidgetItem,
    QPlainTextEdit,
    QToolBar,
    QVBoxLayout,
    QWidget,
)
from PyQt5.Qsci import QsciScintilla, QsciLexerCPP

from picoc_session import PicocSession
from serial_manager import SerialManager
from theme import (
    BG_PRIMARY,
    BG_SIDEBAR,
    BORDER_FOCUS,
    CONSOLE_ERROR,
    CONSOLE_INFO,
    CONSOLE_PROMPT,
    CONSOLE_SEP,
    CONSOLE_SOURCE,
    CONSOLE_SUCCESS,
    FONT_CONSOLE,
    FONT_TABLE,
    FONT_UI,
    TEXT_PRIMARY,
    TEXT_SECONDARY,
    SUCCESS,
    ERROR,
    WARNING,
)

FILE_ITEM_ROLE = 256
BREAKPOINT_MARKER = 0
EXECUTION_MARKER = 1

ERROR_KEYWORDS = (
    "parse error",
    "is undefined",
    "file input not supported",
    "not connected",
    "timed out",
    "failed",
    "abort",
)

TYPE_MAP = {
    'i': 'int', 's': 'short', 'c': 'char', 'l': 'long',
    'I': 'unsigned int', 'S': 'unsigned short', 'C': 'unsigned char',
    'L': 'unsigned long', 'f': 'float', 'p': 'pointer',
}


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("MCUStudio 单片机开发调试系统")
        self.resize(1200, 800)
        self.setMinimumSize(900, 600)

        self._console_line_buffer = ""
        self._upload_active = False
        self._execution_separator_pending = False
        self._serial_manager = SerialManager()
        self._session = PicocSession()

        self._batch_queue: list = []
        self._batch_results: list = []
        self._batch_active = False
        self._current_upload_path = None
        self._single_step_mode = False
        self._populating = False
        self._watch_vars: set = set()
        self._watch_prev: dict = {}
        self._watch_cache: dict = {}
        self._breakpoints: set = set()
        self._editor_dirty = False
        self._auto_save_enabled = True
        self._auto_save_timer = QTimer()
        self._auto_save_timer.setSingleShot(True)
        self._auto_save_timer.setInterval(500)
        self._auto_save_timer.timeout.connect(self._auto_save)

        self._build_ui()
        self._connect_signals()
        self._refresh_ports()
        self._update_ui_state()

    # ── Layout ──────────────────────────────────────────────

    def _build_ui(self) -> None:
        self._build_menubar()
        self._build_toolbar()
        self._build_central_area()
        self._build_status_bar()

    def _build_toolbar(self) -> None:
        toolbar = QToolBar("工具栏")
        toolbar.setMovable(False)
        toolbar.setFloatable(False)
        self.addToolBar(toolbar)

        toolbar.addWidget(QLabel(" 串口 "))
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(100)
        toolbar.addWidget(self.port_combo)

        self.refresh_button = QPushButton("刷新")
        toolbar.addWidget(self.refresh_button)

        toolbar.addWidget(QLabel(" 波特率 "))
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["115200", "230400", "460800", "921600"])
        self.baud_combo.setCurrentText("115200")
        self.baud_combo.setMinimumWidth(90)
        toolbar.addWidget(self.baud_combo)

        self.connect_button = QPushButton("连接")
        self.disconnect_button = QPushButton("断开")
        toolbar.addWidget(self.connect_button)
        toolbar.addWidget(self.disconnect_button)

        toolbar.addSeparator()

        self.debug_info_label = QLabel("调试未激活")
        self.debug_info_label.setStyleSheet(f"color: {TEXT_SECONDARY}; padding-left: 8px;")
        toolbar.addWidget(self.debug_info_label)

        toolbar.addSeparator()

        self.debug_continue_btn = QPushButton("继续")
        self.debug_step_btn = QPushButton("单步")
        toolbar.addWidget(self.debug_continue_btn)
        toolbar.addWidget(self.debug_step_btn)

        toolbar.addWidget(QLabel(" "))
        self.debug_eval_input = QLineEdit()
        self.debug_eval_input.setPlaceholderText("表达式...")
        self.debug_eval_input.setFixedWidth(120)
        toolbar.addWidget(self.debug_eval_input)
        self.debug_eval_btn = QPushButton("求值")
        toolbar.addWidget(self.debug_eval_btn)

    def _build_menubar(self) -> None:
        menubar = self.menuBar()
        menubar.setStyleSheet(f"""
            QMenuBar {{
                background-color: {BG_SIDEBAR};
                color: {TEXT_PRIMARY};
                border-bottom: 1px solid #3c3c3c;
            }}
            QMenuBar::item:selected {{
                background-color: #094771;
            }}
            QMenu {{
                background-color: #2d2d2d;
                color: {TEXT_PRIMARY};
                border: 1px solid #3c3c3c;
            }}
            QMenu::item:selected {{
                background-color: #094771;
            }}
            QMenu::separator {{
                height: 1px;
                background-color: #3c3c3c;
                margin: 4px 8px;
            }}
        """)

        # ── 文件菜单 ──
        file_menu = menubar.addMenu("文件(&F)")

        open_action = file_menu.addAction("选择文件夹(&O)")
        open_action.setShortcut("Ctrl+O")
        open_action.triggered.connect(self._browse_folder)

        save_action = file_menu.addAction("保存(&S)")
        save_action.setShortcut("Ctrl+S")
        save_action.triggered.connect(self._save_current_file)

        file_menu.addSeparator()

        new_c_action = file_menu.addAction("新建 C 文件(&N)")
        new_c_action.setShortcut("Ctrl+N")
        new_c_action.triggered.connect(self._new_c_file)

        new_txt_action = file_menu.addAction("新建文本文档(&T)")
        new_txt_action.setShortcut("Ctrl+Shift+N")
        new_txt_action.triggered.connect(self._new_text_file)

        new_folder_action = file_menu.addAction("新建文件夹(&F)")
        new_folder_action.setShortcut("Ctrl+Shift+F")
        new_folder_action.triggered.connect(self._new_folder)

        file_menu.addSeparator()

        self.auto_save_menu_action = file_menu.addAction("自动保存")
        self.auto_save_menu_action.setCheckable(True)
        self.auto_save_menu_action.setChecked(True)
        self.auto_save_menu_action.toggled.connect(self._on_auto_save_toggled)

        file_menu.addSeparator()

        exit_action = file_menu.addAction("退出(&X)")
        exit_action.setShortcut("Alt+F4")
        exit_action.triggered.connect(self.close)

        # ── 视图菜单 ──
        view_menu = menubar.addMenu("视图(&V)")

        clear_console_action = view_menu.addAction("清空控制台")
        clear_console_action.triggered.connect(self._clear_console)

        save_log_action = view_menu.addAction("保存日志")
        save_log_action.triggered.connect(self._save_log)

    def _build_central_area(self) -> None:
        central = QWidget()
        self.setCentralWidget(central)
        root_layout = QVBoxLayout(central)
        root_layout.setContentsMargins(0, 0, 0, 0)
        root_layout.setSpacing(0)

        main_splitter = QSplitter(Qt.Horizontal)
        main_splitter.addWidget(self._build_sidebar())
        main_splitter.addWidget(self._build_right_area())
        main_splitter.setStretchFactor(0, 0)
        main_splitter.setStretchFactor(1, 1)
        main_splitter.setSizes([220, 980])

        root_layout.addWidget(main_splitter)

    def _build_sidebar(self) -> QWidget:
        sidebar = QWidget()
        sidebar.setObjectName("sidebar")
        sidebar.setStyleSheet(f"""
            QWidget#sidebar {{
                background-color: {BG_SIDEBAR};
                border-right: 1px solid #3c3c3c;
            }}
        """)
        layout = QVBoxLayout(sidebar)
        layout.setContentsMargins(6, 6, 6, 6)
        layout.setSpacing(6)

        explorer_label = QLabel("资源管理器")
        explorer_label.setStyleSheet(f"color: {TEXT_SECONDARY}; font-weight: bold; font: {FONT_UI};")
        layout.addWidget(explorer_label)

        self.file_path_edit = QLineEdit()
        self.file_path_edit.setPlaceholderText("选择文件夹或文件...")
        self.file_path_edit.setReadOnly(True)
        layout.addWidget(self.file_path_edit)

        list_label = QLabel("待测文件")
        list_label.setStyleSheet(f"color: {TEXT_SECONDARY}; font-weight: bold; font: {FONT_UI}; margin-top: 4px;")
        layout.addWidget(list_label)

        self.file_list = QListWidget()
        self.file_list.setSelectionMode(QListWidget.SingleSelection)
        self.file_list.setContextMenuPolicy(Qt.CustomContextMenu)
        self.file_list.customContextMenuRequested.connect(self._show_file_context_menu)
        self.file_list.setStyleSheet("""
            QListWidget {
                background-color: #1e1e1e;
                color: #e0e0e0;
                border: 1px solid #3c3c3c;
                font-size: 13px;
            }
            QListWidget::item {
                padding: 3px 6px;
            }
            QListWidget::item:selected {
                background-color: #094771;
                color: #ffffff;
            }
            QListWidget::item:hover {
                background-color: #2a2d2e;
            }
        """)
        layout.addWidget(self.file_list, 1)

        btn_layout = QGridLayout()
        btn_layout.setSpacing(4)

        self.upload_button = QPushButton("执行选中")
        self.run_all_button = QPushButton("全部执行")
        self.abort_button = QPushButton("中止")
        self.clear_list_button = QPushButton("清空列表")

        btn_layout.addWidget(self.upload_button, 0, 0)
        btn_layout.addWidget(self.run_all_button, 0, 1)
        btn_layout.addWidget(self.abort_button, 1, 0)
        btn_layout.addWidget(self.clear_list_button, 1, 1)

        layout.addLayout(btn_layout)
        return sidebar

    def _build_right_area(self) -> QWidget:
        right = QWidget()
        layout = QVBoxLayout(right)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # ── File tab bar (VS Code style) ──
        self.tab_bar = QWidget()
        self.tab_bar.setFixedHeight(35)
        self.tab_bar.setStyleSheet(f"""
            QWidget {{
                background-color: {BG_SIDEBAR};
                border-bottom: 1px solid #3c3c3c;
            }}
        """)
        tab_layout = QHBoxLayout(self.tab_bar)
        tab_layout.setContentsMargins(8, 0, 0, 0)
        tab_layout.setSpacing(0)

        self.tab_label = QLabel("  未打开文件")
        self.tab_label.setStyleSheet(f"""
            QLabel {{
                color: {TEXT_PRIMARY};
                background-color: {BG_PRIMARY};
                border: 1px solid #3c3c3c;
                border-bottom: none;
                border-top: 2px solid {BORDER_FOCUS};
                padding: 6px 16px;
                font: {FONT_UI};
            }}
        """)
        tab_layout.addWidget(self.tab_label)
        tab_layout.addStretch()

        layout.addWidget(self.tab_bar)

        # ── Stacked widget: welcome / editor ──
        self.editor_stack = QStackedWidget()

        # Welcome page
        welcome = QWidget()
        welcome.setStyleSheet(f"background-color: {BG_PRIMARY};")
        welcome_layout = QVBoxLayout(welcome)
        welcome_layout.setAlignment(Qt.AlignCenter)

        welcome_layout.addStretch()

        title = QLabel("MCUStudio")
        title.setStyleSheet(f"color: {TEXT_PRIMARY}; font-size: 28px; font-weight: bold; background: transparent;")
        title.setAlignment(Qt.AlignCenter)
        welcome_layout.addWidget(title)

        subtitle = QLabel("单片机开发调试系统")
        subtitle.setStyleSheet(f"color: {TEXT_SECONDARY}; font-size: 14px; background: transparent;")
        subtitle.setAlignment(Qt.AlignCenter)
        welcome_layout.addWidget(subtitle)

        welcome_layout.addSpacing(30)

        hint = QLabel("从左侧资源管理器选择文件夹或文件打开")
        hint.setStyleSheet(f"color: {TEXT_SECONDARY}; font-size: 13px; background: transparent;")
        hint.setAlignment(Qt.AlignCenter)
        welcome_layout.addWidget(hint)

        shortcut_hint = QLabel("Ctrl+O 选择文件夹    Ctrl+S 保存")
        shortcut_hint.setStyleSheet(f"color: #6a6a6a; font-size: 12px; background: transparent;")
        shortcut_hint.setAlignment(Qt.AlignCenter)
        welcome_layout.addWidget(shortcut_hint)

        welcome_layout.addStretch()

        self.editor_stack.addWidget(welcome)

        # Editor page
        editor_page = QWidget()
        editor_layout = QVBoxLayout(editor_page)
        editor_layout.setContentsMargins(0, 0, 0, 0)
        editor_layout.setSpacing(0)

        vertical_splitter = QSplitter(Qt.Vertical)

        # ── QScintilla code editor ──
        self.editor = QsciScintilla()
        self._setup_editor()
        vertical_splitter.addWidget(self.editor)

        # ── Bottom: output + variable tables ──
        bottom = QWidget()
        bottom_layout = QVBoxLayout(bottom)
        bottom_layout.setContentsMargins(4, 4, 4, 4)
        bottom_layout.setSpacing(4)

        # Command input
        input_row = QHBoxLayout()
        input_row.setSpacing(4)
        self.manual_input = QLineEdit()
        self.manual_input.setPlaceholderText("输入 PicoC 命令或表达式...")
        self.send_button = QPushButton("发送")
        self.send_button.setFixedWidth(60)
        input_row.addWidget(self.manual_input, 1)
        input_row.addWidget(self.send_button)
        bottom_layout.addLayout(input_row)

        # Output console
        self.console_view = QPlainTextEdit()
        self.console_view.setReadOnly(True)
        self.console_view.setFont(QFont("Cascadia Code", 12))
        self.console_view.setPlaceholderText("执行结果输出...")
        self.console_view.setStyleSheet(f"""
            QPlainTextEdit {{
                background-color: {BG_PRIMARY};
                color: {CONSOLE_SOURCE};
                border: 1px solid #3c3c3c;
                font-family: 'Cascadia Code', Consolas, monospace;
                font-size: 12px;
            }}
        """)
        bottom_layout.addWidget(self.console_view, 1)

        # Variable tables
        var_splitter = QSplitter(Qt.Horizontal)
        var_splitter.addWidget(self._build_variable_watch_group())
        var_splitter.addWidget(self._build_watch_group())
        var_splitter.setSizes([480, 480])
        bottom_layout.addWidget(var_splitter)

        vertical_splitter.addWidget(bottom)
        vertical_splitter.setStretchFactor(0, 3)
        vertical_splitter.setStretchFactor(1, 1)
        vertical_splitter.setSizes([500, 300])

        editor_layout.addWidget(vertical_splitter)

        self.editor_stack.addWidget(editor_page)

        # Start on welcome page
        self.editor_stack.setCurrentIndex(0)

        layout.addWidget(self.editor_stack)
        return right

    def _setup_editor(self) -> None:
        """Configure QScintilla editor with C lexer, dark theme, and breakpoint margins."""
        # Lexer for C syntax highlighting
        lexer = QsciLexerCPP()
        lexer.setDefaultFont(QFont("Cascadia Code", 13))

        # Dark theme colors for lexer
        lexer.setColor(QColor("#d4d4d4"))                    # Default text
        lexer.setColor(QColor("#569cd6"), QsciLexerCPP.Keyword)
        lexer.setColor(QColor("#4ec9b0"), QsciLexerCPP.KeywordSet2)
        lexer.setColor(QColor("#ce9178"), QsciLexerCPP.SingleQuotedString)
        lexer.setColor(QColor("#ce9178"), QsciLexerCPP.DoubleQuotedString)
        lexer.setColor(QColor("#ce9178"), QsciLexerCPP.RawString)
        lexer.setColor(QColor("#6a9955"), QsciLexerCPP.Comment)
        lexer.setColor(QColor("#6a9955"), QsciLexerCPP.CommentLine)
        lexer.setColor(QColor("#6a9955"), QsciLexerCPP.CommentDoc)
        lexer.setColor(QColor("#b5cea8"), QsciLexerCPP.Number)
        lexer.setColor(QColor("#9cdcfe"), QsciLexerCPP.Identifier)
        lexer.setColor(QColor("#d4d4d4"), QsciLexerCPP.Operator)
        lexer.setColor(QColor("#808080"), QsciLexerCPP.PreProcessor)

        # Background for all lexer styles
        for i in range(20):
            lexer.setPaper(QColor("#1e1e1e"), i)
        lexer.setDefaultPaper(QColor("#1e1e1e"))

        self.editor.setLexer(lexer)
        self.editor.setPaper(QColor("#1e1e1e"))

        # Font
        self.editor.setFont(QFont("Cascadia Code", 13))

        # Line numbers (margin 0)
        self.editor.setMarginType(0, QsciScintilla.NumberMargin)
        self.editor.setMarginWidth(0, "00000")
        self.editor.setMarginsBackgroundColor(QColor("#252526"))
        self.editor.setMarginsForegroundColor(QColor("#858585"))
        self.editor.setMarginsFont(QFont("Cascadia Code", 11))

        # Breakpoint margin (margin 1)
        self.editor.setMarginType(1, QsciScintilla.SymbolMargin)
        self.editor.setMarginWidth(1, 20)
        self.editor.setMarginSensitivity(1, True)
        self.editor.markerDefine(QsciScintilla.Circle, BREAKPOINT_MARKER)
        self.editor.setMarkerBackgroundColor(QColor("#f44747"), BREAKPOINT_MARKER)
        self.editor.setMarkerForegroundColor(QColor("#ffffff"), BREAKPOINT_MARKER)

        # Execution arrow marker (margin 1)
        self.editor.markerDefine(QsciScintilla.SC_MARK_ARROW, EXECUTION_MARKER)
        self.editor.setMarkerBackgroundColor(QColor("#dcdcaa"), EXECUTION_MARKER)
        self.editor.setMarkerForegroundColor(QColor("#1e1e1e"), EXECUTION_MARKER)

        # Editor settings
        self.editor.setCaretForegroundColor(QColor("#d4d4d4"))
        self.editor.setIndentationGuides(True)
        self.editor.setIndentationsUseTabs(False)
        self.editor.setTabWidth(4)
        self.editor.setAutoIndent(True)
        self.editor.setFolding(QsciScintilla.BoxedTreeFoldStyle)
        self.editor.setFoldMarginColors(QColor("#252526"), QColor("#252526"))

        # Brace matching
        self.editor.setBraceMatching(QsciScintilla.SloppyBraceMatch)
        self.editor.setMatchedBraceBackgroundColor(QColor("#264f78"))
        self.editor.setMatchedBraceForegroundColor(QColor("#ffffff"))

        # Connect margin click for breakpoint toggle
        self.editor.marginClicked.connect(self._on_margin_clicked)

        # Track modification state
        self.editor.textChanged.connect(self._on_editor_text_changed)

    def _build_variable_watch_group(self) -> QGroupBox:
        group = QGroupBox("变量监视")
        layout = QVBoxLayout(group)
        layout.setContentsMargins(4, 4, 4, 4)

        toolbar = QHBoxLayout()
        self.watch_add_btn = QPushButton("加监视")
        self.watch_add_btn.setMinimumWidth(60)
        self.watch_add_btn.clicked.connect(self._on_watch_add)
        self.watch_clear_btn = QPushButton("清监视")
        self.watch_clear_btn.setMinimumWidth(60)
        self.watch_clear_btn.clicked.connect(self._on_watch_clear_all)
        toolbar.addWidget(self.watch_add_btn)
        toolbar.addWidget(self.watch_clear_btn)
        toolbar.addStretch()
        layout.addLayout(toolbar)

        self.var_table = QTableWidget(0, 3)
        self.var_table.setHorizontalHeaderLabels(["名称", "类型", "值"])
        self.var_table.horizontalHeader().setStretchLastSection(True)
        self.var_table.verticalHeader().setVisible(False)
        self.var_table.setSelectionBehavior(QTableWidget.SelectRows)
        self.var_table.setEditTriggers(
            QTableWidget.DoubleClicked | QTableWidget.EditKeyPressed
        )
        self.var_table.setFont(QFont("Cascadia Code", 12))
        self.var_table.itemChanged.connect(self._on_var_item_changed)
        self.var_table.setStyleSheet("""
            QTableWidget {
                background-color: #1e1e1e;
                color: #e0e0e0;
                border: 1px solid #3c3c3c;
                gridline-color: #3c3c3c;
            }
            QTableWidget::item {
                padding: 2px 4px;
                color: #e0e0e0;
            }
            QTableWidget::item:selected {
                background-color: #094771;
                color: #ffffff;
            }
            QHeaderView::section {
                background-color: #2d2d2d;
                color: #b0b0b0;
                border: 1px solid #3c3c3c;
                padding: 3px 6px;
                font-weight: bold;
            }
        """)

        layout.addWidget(self.var_table)
        return group

    def _build_watch_group(self) -> QGroupBox:
        group = QGroupBox("Watch")
        layout = QVBoxLayout(group)
        layout.setContentsMargins(4, 4, 4, 4)

        self.watch_table = QTableWidget(0, 3)
        self.watch_table.setHorizontalHeaderLabels(["名称", "类型", "值"])
        self.watch_table.horizontalHeader().setStretchLastSection(True)
        self.watch_table.verticalHeader().setVisible(False)
        self.watch_table.setSelectionBehavior(QTableWidget.SelectRows)
        self.watch_table.setEditTriggers(QTableWidget.NoEditTriggers)
        self.watch_table.setFont(QFont("Cascadia Code", 12))
        self.watch_table.setStyleSheet("""
            QTableWidget {
                background-color: #1e1e1e;
                color: #e0e0e0;
                border: 1px solid #3c3c3c;
                gridline-color: #3c3c3c;
            }
            QTableWidget::item {
                padding: 2px 4px;
                color: #e0e0e0;
            }
            QTableWidget::item:selected {
                background-color: #094771;
                color: #ffffff;
            }
            QHeaderView::section {
                background-color: #2d2d2d;
                color: #b0b0b0;
                border: 1px solid #3c3c3c;
                padding: 3px 6px;
                font-weight: bold;
            }
        """)

        remove_row = QHBoxLayout()
        self.watch_remove_btn = QPushButton("移除选中")
        self.watch_remove_btn.clicked.connect(self._on_watch_remove)
        remove_row.addWidget(self.watch_remove_btn)
        remove_row.addStretch()
        layout.addLayout(remove_row)
        layout.addWidget(self.watch_table)
        return group

    def _build_status_bar(self) -> None:
        status_bar = QStatusBar()
        self.setStatusBar(status_bar)

        self.status_label = QLabel("未连接")
        self.mode_label = QLabel("未连接")
        self.debug_status_label = QLabel("")

        status_bar.addWidget(self.status_label, 1)
        self.mode_label.setAlignment(Qt.AlignCenter)
        status_bar.addWidget(self.mode_label)
        self.debug_status_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        status_bar.addPermanentWidget(self.debug_status_label)

    # ── Signals ─────────────────────────────────────────────

    def _connect_signals(self) -> None:
        self.refresh_button.clicked.connect(self._refresh_ports)
        self.connect_button.clicked.connect(self._connect_serial)
        self.disconnect_button.clicked.connect(self._disconnect_serial)
        self.send_button.clicked.connect(self._send_manual_input)
        self.manual_input.returnPressed.connect(self._send_manual_input)

        self.upload_button.clicked.connect(self._upload_file)
        self.run_all_button.clicked.connect(self._run_all_files)
        self.clear_list_button.clicked.connect(self._clear_file_list)
        self.abort_button.clicked.connect(self._abort_action)
        self.file_list.currentItemChanged.connect(self._on_file_selected)

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
        self._session.var_info.connect(self._on_var_info)
        self._session.vars_complete.connect(self._on_vars_complete)
        self._session.set_result.connect(self._on_set_result)
        self.debug_continue_btn.clicked.connect(lambda: self._session.send_debug_continue())
        self.debug_step_btn.clicked.connect(lambda: self._session.send_debug_step())
        self.debug_eval_btn.clicked.connect(self._on_eval_clicked)
        self.debug_eval_input.returnPressed.connect(self._on_eval_clicked)

    # ── Connection ──────────────────────────────────────────

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

    # ── File Management ─────────────────────────────────────

    def _new_c_file(self) -> None:
        """Create a new C source file in the current folder."""
        current_folder = self._get_current_folder()
        if current_folder is None:
            self._show_warning("未选择文件夹", "请先选择一个文件夹。")
            return

        filename, ok = QFileDialog.getSaveFileName(
            self, "新建 C 文件", str(current_folder / "untitled.c"),
            "C source (*.c);;All files (*.*)",
        )
        if not ok or not filename:
            return

        file_path = Path(filename)
        if file_path.exists():
            self._show_warning("文件已存在", f"文件 {file_path.name} 已存在。")
            return

        try:
            file_path.write_text("#include <stdio.h>\n\nint main() {\n    return 0;\n}\n", encoding="utf-8")
        except Exception as exc:
            self._show_warning("创建文件失败", str(exc))
            return

        self._add_files_to_list([file_path])
        self._set_status(f"已创建: {file_path.name}")

    def _new_text_file(self) -> None:
        """Create a new text file in the current folder."""
        current_folder = self._get_current_folder()
        if current_folder is None:
            self._show_warning("未选择文件夹", "请先选择一个文件夹。")
            return

        filename, ok = QFileDialog.getSaveFileName(
            self, "新建文本文档", str(current_folder / "untitled.txt"),
            "Text files (*.txt);;All files (*.*)",
        )
        if not ok or not filename:
            return

        file_path = Path(filename)
        if file_path.exists():
            self._show_warning("文件已存在", f"文件 {file_path.name} 已存在。")
            return

        try:
            file_path.write_text("", encoding="utf-8")
        except Exception as exc:
            self._show_warning("创建文件失败", str(exc))
            return

        self._set_status(f"已创建: {file_path.name}")

    def _new_folder(self) -> None:
        """Create a new folder in the current folder."""
        current_folder = self._get_current_folder()
        if current_folder is None:
            self._show_warning("未选择文件夹", "请先选择一个文件夹。")
            return

        folder_name, ok = QFileDialog.getSaveFileName(
            self, "新建文件夹", str(current_folder / "new_folder"),
            "All files (*.*)",
        )
        if not ok or not folder_name:
            return

        folder_path = Path(folder_name)
        if folder_path.exists():
            self._show_warning("文件夹已存在", f"文件夹 {folder_path.name} 已存在。")
            return

        try:
            folder_path.mkdir(parents=True, exist_ok=True)
        except Exception as exc:
            self._show_warning("创建文件夹失败", str(exc))
            return

        self._set_status(f"已创建文件夹: {folder_path.name}")

    def _get_current_folder(self) -> Path:
        """Get the current folder path from the file list or path edit."""
        # Try to get from file path edit
        current_path = self.file_path_edit.text().strip()
        if current_path:
            path = Path(current_path)
            if path.is_dir():
                return path
            return path.parent

        # Try to get from first file in list
        if self.file_list.count() > 0:
            item = self.file_list.item(0)
            path_str = item.data(FILE_ITEM_ROLE)
            if isinstance(path_str, str) and path_str:
                return Path(path_str).parent

        return None

    def _browse_files(self) -> None:
        filenames, _ = QFileDialog.getOpenFileNames(
            self, "选择 C 文件", "", "C source (*.c);;All files (*.*)",
        )
        if filenames:
            self._add_files_to_list([Path(name) for name in filenames])

    def _browse_folder(self) -> None:
        folder = QFileDialog.getExistingDirectory(self, "选择文件夹")
        if folder:
            folder_path = Path(folder)
            c_files = sorted(folder_path.rglob("*.c"))
            if c_files:
                self._add_files_to_list(c_files)
            else:
                self._show_warning("未找到文件", f"文件夹中没有 .c 文件:\n{folder}")

    def _add_files_to_list(self, paths: list) -> None:
        existing = {path.resolve() for path in self._get_all_listed_files()}
        added_items = []
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

    def _show_file_context_menu(self, pos) -> None:
        """Show context menu for file list."""
        item = self.file_list.itemAt(pos)
        if item is None:
            return

        file_path = item.data(FILE_ITEM_ROLE)
        if not isinstance(file_path, str) or not file_path:
            return

        path = Path(file_path)
        if not path.exists():
            return

        menu = QMenu(self)
        menu.setStyleSheet("""
            QMenu {
                background-color: #2d2d2d;
                color: #e0e0e0;
                border: 1px solid #3c3c3c;
            }
            QMenu::item:selected {
                background-color: #094771;
            }
        """)

        # 打开文件
        open_action = menu.addAction("打开文件")
        open_action.triggered.connect(lambda: self._open_file_in_editor(path))

        # 在资源管理器中打开
        explorer_action = menu.addAction("在资源管理器中打开")
        explorer_action.triggered.connect(lambda: self._open_in_explorer(path))

        menu.addSeparator()

        # 复制文件路径
        copy_path_action = menu.addAction("复制文件路径")
        copy_path_action.triggered.connect(lambda: self._copy_file_path(path))

        # 复制文件名
        copy_name_action = menu.addAction("复制文件名")
        copy_name_action.triggered.connect(lambda: self._copy_file_name(path))

        menu.addSeparator()

        # 重命名
        rename_action = menu.addAction("重命名")
        rename_action.triggered.connect(lambda: self._rename_file(path, item))

        # 删除文件
        delete_action = menu.addAction("删除文件")
        delete_action.triggered.connect(lambda: self._delete_file(path, item))

        menu.addSeparator()

        # 执行文件
        run_action = menu.addAction("执行文件")
        run_action.triggered.connect(lambda: self._run_single_file(path))

        # 从列表中移除
        remove_action = menu.addAction("从列表中移除")
        remove_action.triggered.connect(lambda: self._remove_from_list(item))

        menu.exec_(self.file_list.mapToGlobal(pos))

    def _open_file_in_editor(self, file_path: Path) -> None:
        """Open file in editor."""
        if file_path.suffix.lower() in ('.c', '.h', '.txt', '.py', '.md'):
            self._load_source_to_editor(file_path)

    def _open_in_explorer(self, file_path: Path) -> None:
        """Open file location in Windows Explorer."""
        import subprocess
        try:
            subprocess.Popen(['explorer', '/select,', str(file_path)])
        except Exception as exc:
            self._show_warning("打开失败", str(exc))

    def _copy_file_path(self, file_path: Path) -> None:
        """Copy file path to clipboard."""
        from PyQt5.QtWidgets import QApplication
        clipboard = QApplication.clipboard()
        clipboard.setText(str(file_path))
        self._set_status(f"已复制路径: {file_path.name}")

    def _copy_file_name(self, file_path: Path) -> None:
        """Copy file name to clipboard."""
        from PyQt5.QtWidgets import QApplication
        clipboard = QApplication.clipboard()
        clipboard.setText(file_path.name)
        self._set_status(f"已复制文件名: {file_path.name}")

    def _rename_file(self, old_path: Path, item: QListWidgetItem) -> None:
        """Rename file."""
        from PyQt5.QtWidgets import QInputDialog
        new_name, ok = QInputDialog.getText(self, "重命名", "新文件名:", text=old_path.name)
        if not ok or not new_name or new_name == old_path.name:
            return

        new_path = old_path.parent / new_name
        if new_path.exists():
            self._show_warning("文件已存在", f"文件 {new_name} 已存在。")
            return

        try:
            old_path.rename(new_path)
        except Exception as exc:
            self._show_warning("重命名失败", str(exc))
            return

        item.setText(new_name)
        item.setData(FILE_ITEM_ROLE, str(new_path.resolve()))
        item.setToolTip(str(new_path.resolve()))
        self._set_status(f"已重命名: {old_path.name} -> {new_name}")

    def _delete_file(self, file_path: Path, item: QListWidgetItem) -> None:
        """Delete file."""
        reply = QMessageBox.question(
            self, "确认删除",
            f"确定要删除文件 {file_path.name} 吗？",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No
        )
        if reply != QMessageBox.Yes:
            return

        try:
            file_path.unlink()
        except Exception as exc:
            self._show_warning("删除失败", str(exc))
            return

        row = self.file_list.row(item)
        self.file_list.takeItem(row)
        self._set_status(f"已删除: {file_path.name}")

    def _run_single_file(self, file_path: Path) -> None:
        """Run a single file."""
        if not self._serial_manager.is_connected():
            self._show_warning("未连接", "请先连接串口。")
            return
        self._batch_active = False
        self._batch_queue.clear()
        self._batch_results.clear()
        self._single_step_mode = True
        self._start_upload_for_path(file_path)

    def _remove_from_list(self, item: QListWidgetItem) -> None:
        """Remove file from list without deleting the actual file."""
        row = self.file_list.row(item)
        self.file_list.takeItem(row)
        self._set_status("已从列表中移除")

    def _clear_file_list(self) -> None:
        self.file_list.clear()
        self.file_path_edit.clear()
        self.editor.clear()
        self._breakpoints.clear()
        self._editor_dirty = False
        self._update_title_dirty()
        self.tab_label.setText("  未打开文件")
        self.editor_stack.setCurrentIndex(0)
        self._update_ui_state()

    def _on_file_selected(self, current, previous) -> None:
        """When file list selection changes, load source into editor."""
        if current is None:
            self.file_path_edit.clear()
            self.tab_label.setText("  未打开文件")
            self.editor_stack.setCurrentIndex(0)
            return
        if self._editor_dirty and self._auto_save_enabled:
            self._auto_save()
        path = current.data(FILE_ITEM_ROLE)
        if not isinstance(path, str) or not path:
            return
        self.file_path_edit.setText(path)
        self._load_source_to_editor(Path(path))

    def _load_source_to_editor(self, file_path: Path) -> None:
        """Read file and display in QScintilla editor, clearing old breakpoints."""
        source = self._read_source_file(file_path)
        if source is None:
            return
        self._breakpoints.clear()
        self._editor_dirty = False
        self.editor.clear()
        self.editor.setText(source)
        self.tab_label.setText(f"  {file_path.name}")
        self.editor_stack.setCurrentIndex(1)

    def _get_selected_file_path(self) -> Path:
        item = self.file_list.currentItem()
        if item is None:
            return None
        path = item.data(FILE_ITEM_ROLE)
        if not isinstance(path, str) or not path:
            return None
        return Path(path)

    def _get_all_listed_files(self) -> list:
        paths = []
        for index in range(self.file_list.count()):
            item = self.file_list.item(index)
            path = item.data(FILE_ITEM_ROLE)
            if isinstance(path, str) and path:
                paths.append(Path(path))
        return paths

    # ── Breakpoints via margin click ────────────────────────

    def _on_margin_clicked(self, margin, line, state) -> None:
        """Toggle breakpoint when clicking the symbol margin."""
        if margin != 1:
            return
        # QScintilla line is 0-based, our protocol is 1-based
        line_no = line + 1
        if line_no in self._breakpoints:
            self._breakpoints.discard(line_no)
            self.editor.markerDelete(line, BREAKPOINT_MARKER)
            self._session.send_breakpoint_clear(self._bp_filename(), line_no)
        else:
            self._breakpoints.add(line_no)
            self.editor.markerAdd(line, BREAKPOINT_MARKER)
            self._session.send_breakpoint_set(self._bp_filename(), line_no)

    def _on_editor_text_changed(self) -> None:
        if not self._editor_dirty:
            self._editor_dirty = True
            self._update_title_dirty()
        if self._auto_save_enabled:
            self._auto_save_timer.start()

    def _update_title_dirty(self) -> None:
        title = "MCUStudio 单片机开发调试系统"
        if self._editor_dirty and not self._auto_save_enabled:
            title = "* " + title
        self.setWindowTitle(title)

    def _save_current_file(self) -> None:
        file_path = self._get_selected_file_path()
        if file_path is None:
            self._show_warning("无文件", "请先选择一个文件。")
            return
        try:
            file_path.write_text(self.editor.text(), encoding="utf-8")
        except Exception as exc:
            self._show_warning("保存失败", str(exc))
            return
        self._editor_dirty = False
        self._update_title_dirty()
        self._set_status(f"已保存: {file_path.name}")

    def _on_auto_save_toggled(self, checked: bool) -> None:
        self._auto_save_enabled = checked
        # Sync menu action if triggered from somewhere else
        if self.auto_save_menu_action.isChecked() != checked:
            self.auto_save_menu_action.setChecked(checked)
        if not checked:
            self._auto_save_timer.stop()

    def _auto_save(self) -> None:
        file_path = self._get_selected_file_path()
        if file_path is None:
            return
        try:
            file_path.write_text(self.editor.text(), encoding="utf-8")
        except Exception:
            return
        self._editor_dirty = False
        self._update_title_dirty()

    # ── Upload & Batch ──────────────────────────────────────

    def _upload_file(self) -> None:
        file_path = self._get_selected_file_path()
        if file_path is None:
            raw_path = self.file_path_edit.text().strip()
            if raw_path:
                file_path = Path(raw_path)

        if file_path is None or not file_path.exists():
            self._show_warning("文件无效", "请先从列表中选中一个 .c 文件。")
            return

        self._batch_active = False
        self._batch_queue.clear()
        self._batch_results.clear()
        self._single_step_mode = True
        self._start_upload_for_path(file_path)

    def _run_all_files(self) -> None:
        self._single_step_mode = False
        self._start_batch(self._get_all_listed_files(), "文件批量测试")

    def _start_batch(self, paths: list, title: str) -> None:
        valid_paths = [p for p in paths if p.exists() and p.is_file()]
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
        # Use editor content if this file is currently displayed
        current_displayed = self._get_selected_file_path()
        if current_displayed is not None and current_displayed == file_path and self.editor.text():
            source_text = self.editor.text()
        else:
            source_text = self._read_source_file(file_path)
        if source_text is None:
            if self._batch_active:
                self._batch_results.append((file_path, False, "读取文件失败"))
                self._continue_batch_or_finish()
            return

        self._current_upload_path = file_path
        self.file_path_edit.setText(str(file_path))
        self._load_source_to_editor(file_path)
        self._execution_separator_pending = True
        if self._batch_active:
            tested = len(self._batch_results)
            total = len(self._batch_results) + len(self._batch_queue) + 1
            self._append_separator_line(f"测试 [{tested}/{total}]: {file_path.name}")
        else:
            self._append_separator_line(f"上传: {file_path.name}")

        if not self._session.start_upload(source_text):
            self._execution_separator_pending = False
            self._append_info_line(f"启动上传失败: {file_path.name}")
            if self._batch_active:
                self._batch_results.append((file_path, False, "启动上传失败"))
                self._continue_batch_or_finish()
        self._update_ui_state()

    def _read_source_file(self, file_path: Path) -> str:
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
            self, "保存日志", "picoc_console_log.txt",
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
        self.debug_status_label.setText("")
        self.editor.markerDeleteAll(EXECUTION_MARKER)

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
            self.debug_status_label.setText("")
            self.editor.markerDeleteAll(EXECUTION_MARKER)
        self._update_ui_state()

    # ── Debug ───────────────────────────────────────────────

    def _on_debug_break(self, filename: str, line_no: int) -> None:
        self.debug_info_label.setText(f"已中断: 第{line_no}行")
        self.debug_status_label.setText(f"调试: 第{line_no}行")
        # Show execution arrow in editor (0-based)
        self.editor.markerDeleteAll(EXECUTION_MARKER)
        self.editor.markerAdd(line_no - 1, EXECUTION_MARKER)
        self.editor.ensureLineVisible(line_no - 1)
        self._update_ui_state()
        self.var_table.setRowCount(0)
        self._watch_cache.clear()
        self._set_watch_values_to_pending()
        QTimer.singleShot(50, lambda: self._session.send_debug_vars())

    def _on_debug_resumed(self) -> None:
        self.debug_info_label.setText("调试未激活")
        self.debug_status_label.setText("")
        self.editor.markerDeleteAll(EXECUTION_MARKER)
        self.var_table.setRowCount(0)
        self._set_watch_values_to_pending()
        self._update_ui_state()

    def _on_eval_clicked(self) -> None:
        expr = self.debug_eval_input.text().strip()
        if not expr:
            return
        if self._session.send_debug_eval(expr):
            self.debug_eval_input.clear()

    def _on_var_info(self, name: str, type_char: str, value: str) -> None:
        self._populating = True
        row = self.var_table.rowCount()
        self.var_table.insertRow(row)

        name_item = QTableWidgetItem(name)
        name_item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
        name_item.setForeground(QColor(TEXT_PRIMARY))
        self.var_table.setItem(row, 0, name_item)

        type_name = TYPE_MAP.get(type_char, type_char)
        type_item = QTableWidgetItem(type_name)
        type_item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
        type_item.setForeground(QColor("#569cd6"))
        self.var_table.setItem(row, 1, type_item)

        value_item = QTableWidgetItem(value)
        value_item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled | Qt.ItemIsEditable)
        value_item.setForeground(QColor(TEXT_PRIMARY))
        self.var_table.setItem(row, 2, value_item)

        self._populating = False

        if name in self._watch_vars:
            self._watch_cache[name] = (type_char, value)

    def _on_vars_complete(self) -> None:
        self.var_table.resizeColumnsToContents()
        count = self.var_table.rowCount()
        self.status_label.setText(f"变量监视: {count} 个变量")
        self._refresh_watch_table()

    def _on_set_result(self, success: bool, message: str) -> None:
        if success:
            self.status_label.setText("变量修改成功")
            self._session.send_debug_vars()
        else:
            self.status_label.setText(f"变量修改失败: {message}")

    def _on_var_item_changed(self, item: QTableWidgetItem) -> None:
        if self._populating:
            return
        if item.column() != 2:
            return
        name_item = self.var_table.item(item.row(), 0)
        if name_item is None:
            return
        self._session.send_debug_set(name_item.text(), item.text())

    def _on_watch_add(self) -> None:
        selected = self.var_table.selectedItems()
        if not selected:
            return
        name = self.var_table.item(selected[0].row(), 0)
        if name is None:
            return
        var_name = name.text()
        if var_name in self._watch_vars:
            return
        self._watch_vars.add(var_name)
        self._watch_prev[var_name] = ""
        self._rebuild_watch_rows()

    def _on_watch_remove(self) -> None:
        selected = self.watch_table.selectedItems()
        if not selected:
            return
        name_item = self.watch_table.item(selected[0].row(), 0)
        if name_item is None:
            return
        var_name = name_item.text()
        self._watch_vars.discard(var_name)
        self._watch_prev.pop(var_name, None)
        self._watch_cache.pop(var_name, None)
        self._rebuild_watch_rows()

    def _on_watch_clear_all(self) -> None:
        self._watch_vars.clear()
        self._watch_prev.clear()
        self._watch_cache.clear()
        self._rebuild_watch_rows()

    def _set_watch_values_to_pending(self) -> None:
        for row in range(self.watch_table.rowCount()):
            item = self.watch_table.item(row, 2)
            if item is not None:
                item.setText("—")
                item.setForeground(QColor("gray"))

    def _rebuild_watch_rows(self) -> None:
        self.watch_table.setRowCount(0)
        for var_name in sorted(self._watch_vars):
            row = self.watch_table.rowCount()
            self.watch_table.insertRow(row)

            name_item = QTableWidgetItem(var_name)
            name_item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
            name_item.setForeground(QColor(TEXT_PRIMARY))
            self.watch_table.setItem(row, 0, name_item)

            type_item = QTableWidgetItem("—")
            type_item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
            type_item.setForeground(QColor("#569cd6"))
            self.watch_table.setItem(row, 1, type_item)

            value_item = QTableWidgetItem("—")
            value_item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
            value_item.setForeground(QColor(TEXT_PRIMARY))
            self.watch_table.setItem(row, 2, value_item)

    def _refresh_watch_table(self) -> None:
        for row in range(self.watch_table.rowCount()):
            name_item = self.watch_table.item(row, 0)
            if name_item is None:
                continue
            var_name = name_item.text()

            if var_name in self._watch_cache:
                type_char, cur_value = self._watch_cache[var_name]
                prev_value = self._watch_prev.get(var_name, "")

                type_item = self.watch_table.item(row, 1)
                if type_item is not None:
                    type_item.setText(TYPE_MAP.get(type_char, type_char))

                value_item = self.watch_table.item(row, 2)
                if value_item is not None:
                    changed = (prev_value != "" and prev_value != cur_value)
                    if changed:
                        value_item.setText(f"{prev_value} -> {cur_value}")
                        value_item.setForeground(QColor(ERROR))
                    else:
                        value_item.setText(cur_value)
                        value_item.setForeground(QColor(TEXT_PRIMARY))

                self._watch_prev[var_name] = cur_value
            else:
                value_item = self.watch_table.item(row, 2)
                if value_item is not None:
                    value_item.setText("—")
                    value_item.setForeground(QColor("gray"))
        self.watch_table.resizeColumnsToContents()

    # ── Breakpoints ─────────────────────────────────────────

    @staticmethod
    def _bp_filename() -> str:
        return "serial_load"

    # ── Error & Status ──────────────────────────────────────

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
        has_selection = self.file_list.currentItem() is not None

        self.connect_button.setEnabled(not connected)
        self.disconnect_button.setEnabled(connected)
        self.refresh_button.setEnabled(not connected)
        self.port_combo.setEnabled(not connected)
        self.baud_combo.setEnabled(not connected)

        self.manual_input.setEnabled(connected and not busy and not debug_active)
        self.send_button.setEnabled(connected and not busy and not debug_active)

        self.clear_list_button.setEnabled(has_files)
        # File list is always enabled
        self.file_list.setEnabled(True)
        self.file_path_edit.setEnabled(True)

        self.upload_button.setEnabled(connected and not busy and has_selection)
        self.run_all_button.setEnabled(connected and not busy and has_files)
        self.abort_button.setEnabled(connected)

        self.debug_continue_btn.setEnabled(debug_active)
        self.debug_step_btn.setEnabled(debug_active)
        self.debug_eval_input.setEnabled(debug_active)
        self.debug_eval_btn.setEnabled(debug_active)

        mode = self._session.mode
        if mode == "REPL":
            self.mode_label.setStyleSheet(f"color: {SUCCESS}; padding: 0 8px;")
        elif mode in ("LOAD", "BUSY"):
            self.mode_label.setStyleSheet(f"color: {WARNING}; padding: 0 8px;")
        else:
            self.mode_label.setStyleSheet(f"color: {TEXT_SECONDARY}; padding: 0 8px;")

    # ── Console Output ──────────────────────────────────────

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

    def _append_remote_line(self, line: str) -> None:
        if not line.strip():
            return
        if self._should_filter_control_echo(line):
            return

        lowered = line.lower()
        if any(keyword in lowered for keyword in ERROR_KEYWORDS):
            color = CONSOLE_ERROR
        elif "ready for next file" in lowered:
            color = CONSOLE_SUCCESS
        elif "picoc>" in line or "load>" in line:
            color = CONSOLE_PROMPT
        else:
            color = CONSOLE_SOURCE

        self._append_colored_line(line, color)

    def _append_info_line(self, text: str) -> None:
        self._append_colored_line(text, CONSOLE_INFO)

    def _append_separator_line(self, title: str) -> None:
        self._append_colored_line(f"---------------- {title} ----------------", CONSOLE_SEP)

    def _append_colored_line(self, text: str, color: str) -> None:
        cursor = self.console_view.textCursor()
        cursor.movePosition(QTextCursor.End)
        fmt = QTextCharFormat()
        fmt.setForeground(QColor(color))
        cursor.insertText(text + "\n", fmt)
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
                        ":ok eval", ":ok bkpt", ":ok bkptclear", ":ok vars", ":ok set", "load>"}:
            return True
        if stripped.startswith(":load") or stripped.startswith(":err"):
            return True
        if stripped.startswith(":break") or stripped.startswith(":step"):
            return True
        if stripped in {":cont", ":step"}:
            return True
        if stripped.startswith(":eval ") or stripped.startswith(":var "):
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
