"""PicoC Host Tool — Dark IDE Theme (VS Code / Keil MDK style)"""

# ── Background ──────────────────────────────────────────────
BG_PRIMARY = "#1e1e1e"       # 主区域背景
BG_SIDEBAR = "#252526"       # 侧边栏背景
BG_TOOLBAR = "#2d2d2d"       # 工具栏背景
BG_INPUT   = "#3c3c3c"       # 输入框背景
BG_HOVER   = "#2a2d2e"       # 悬停态
BG_SELECTED = "#094771"      # 选中态

# ── Text ────────────────────────────────────────────────────
TEXT_PRIMARY   = "#e0e0e0"   # 正文 (提高亮度)
TEXT_SECONDARY = "#b0b0b0"   # 次要文字 (原 #808080 太暗)
TEXT_DISABLED  = "#6a6a6a"   # 禁用态 (原 #5a5a5a 太暗)
TEXT_ACCENT    = "#569cd6"   # 强调文字 (蓝色)

# ── Border ──────────────────────────────────────────────────
BORDER       = "#3c3c3c"
BORDER_FOCUS = "#007acc"

# ── Accent ──────────────────────────────────────────────────
ACCENT       = "#0078d4"     # 按钮/链接
ACCENT_HOVER = "#1a8cff"

# ── Status ──────────────────────────────────────────────────
SUCCESS = "#4ec9b0"
ERROR   = "#f44747"
WARNING = "#dcdcaa"
INFO    = "#969696"

# ── Console ─────────────────────────────────────────────────
CONSOLE_TEXT    = "#e0e0e0"
CONSOLE_PROMPT  = "#569cd6"
CONSOLE_ERROR   = "#f44747"
CONSOLE_SUCCESS = "#4ec9b0"
CONSOLE_INFO    = "#969696"
CONSOLE_SEP     = "#dcdcaa"
CONSOLE_SOURCE  = "#e0e0e0"

# ── Fonts ───────────────────────────────────────────────────
FONT_CONSOLE = "Cascadia Code, 13px"
FONT_TABLE   = "Cascadia Code, 12px"
FONT_UI      = "'Segoe UI', 'Microsoft YaHei UI', 13px"
FONT_CODE    = "'Cascadia Code', Consolas, 'Courier New', monospace"

# ── QSS Stylesheet ──────────────────────────────────────────
STYLESHEET = f"""
/* ── Global ─────────────────────────────────────────── */
QWidget {{
    background-color: {BG_PRIMARY};
    color: {TEXT_PRIMARY};
    font: {FONT_UI};
}}

/* ── ToolBar ────────────────────────────────────────── */
QToolBar {{
    background-color: {BG_TOOLBAR};
    border-bottom: 1px solid {BORDER};
    padding: 2px;
    spacing: 4px;
}}
QToolBar::separator {{
    width: 1px;
    background-color: {BORDER};
    margin: 4px 6px;
}}

/* ── Push Buttons ───────────────────────────────────── */
QPushButton {{
    background-color: {BG_INPUT};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    border-radius: 3px;
    padding: 4px 12px;
    min-height: 22px;
}}
QPushButton:hover {{
    background-color: {BG_HOVER};
    border-color: {BORDER_FOCUS};
}}
QPushButton:pressed {{
    background-color: {BG_SELECTED};
}}
QPushButton:disabled {{
    color: {TEXT_DISABLED};
    background-color: {BG_TOOLBAR};
    border-color: {BORDER};
}}

/* ── Combo Boxes ────────────────────────────────────── */
QComboBox {{
    background-color: {BG_INPUT};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    border-radius: 3px;
    padding: 3px 8px;
    min-height: 22px;
}}
QComboBox:hover {{
    border-color: {BORDER_FOCUS};
}}
QComboBox::drop-down {{
    border: none;
    width: 20px;
}}
QComboBox QAbstractItemView {{
    background-color: {BG_INPUT};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    selection-background-color: {BG_SELECTED};
}}

/* ── Line Edits ─────────────────────────────────────── */
QLineEdit {{
    background-color: {BG_INPUT};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    border-radius: 3px;
    padding: 3px 8px;
    min-height: 22px;
}}
QLineEdit:focus {{
    border-color: {BORDER_FOCUS};
}}

/* ── Text Edit (Console) ────────────────────────────── */
QTextEdit {{
    background-color: {BG_PRIMARY};
    color: {CONSOLE_TEXT};
    border: 1px solid {BORDER};
    font: {FONT_CONSOLE};
    selection-background-color: {BG_SELECTED};
}}

/* ── List Widget (File List) ────────────────────────── */
QListWidget {{
    background-color: {BG_SIDEBAR};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    outline: none;
    font: {FONT_UI};
}}
QListWidget::item {{
    padding: 4px 8px;
    border-bottom: 1px solid {BORDER};
}}
QListWidget::item:selected {{
    background-color: {BG_SELECTED};
    color: {TEXT_PRIMARY};
}}
QListWidget::item:hover {{
    background-color: {BG_HOVER};
}}

/* ── Table Widget (Variable / Watch) ────────────────── */
QTableWidget {{
    background-color: {BG_PRIMARY};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    gridline-color: {BORDER};
    font: {FONT_TABLE};
    selection-background-color: {BG_SELECTED};
}}
QTableWidget::item {{
    padding: 2px 6px;
}}
QTableWidget::item:selected {{
    background-color: {BG_SELECTED};
}}
QHeaderView::section {{
    background-color: {BG_TOOLBAR};
    color: {TEXT_SECONDARY};
    border: 1px solid {BORDER};
    padding: 3px 6px;
    font-weight: bold;
}}

/* ── Group Box ──────────────────────────────────────── */
QGroupBox {{
    border: 1px solid {BORDER};
    border-radius: 4px;
    margin-top: 8px;
    padding-top: 12px;
    color: {TEXT_PRIMARY};
    font-weight: bold;
    font-size: 13px;
}}
QGroupBox::title {{
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
}}

/* ── Status Bar ─────────────────────────────────────── */
QStatusBar {{
    background-color: {BG_TOOLBAR};
    color: {TEXT_PRIMARY};
    border-top: 1px solid {BORDER};
    font: {FONT_UI};
}}
QStatusBar::item {{
    border: none;
}}

/* ── Splitter ───────────────────────────────────────── */
QSplitter::handle {{
    background-color: {BORDER};
}}
QSplitter::handle:horizontal {{
    width: 3px;
}}
QSplitter::handle:vertical {{
    height: 3px;
}}

/* ── Labels ─────────────────────────────────────────── */
QLabel {{
    color: {TEXT_PRIMARY};
    background: transparent;
}}

/* ── ScrollBar (VS Code style) ──────────────────────── */
QScrollBar:vertical {{
    background-color: transparent;
    width: 12px;
    border: none;
    margin: 0;
}}
QScrollBar::handle:vertical {{
    background-color: rgba(121, 121, 121, 0.4);
    min-height: 30px;
    border-radius: 5px;
    border: 2px solid transparent;
    background-clip: padding-box;
}}
QScrollBar::handle:vertical:hover {{
    background-color: rgba(121, 121, 121, 0.7);
}}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0;
    background: none;
}}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {{
    background: none;
}}
QScrollBar:horizontal {{
    background-color: transparent;
    height: 12px;
    border: none;
    margin: 0;
}}
QScrollBar::handle:horizontal {{
    background-color: rgba(121, 121, 121, 0.4);
    min-width: 30px;
    border-radius: 5px;
    border: 2px solid transparent;
    background-clip: padding-box;
}}
QScrollBar::handle:horizontal:hover {{
    background-color: rgba(121, 121, 121, 0.7);
}}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {{
    width: 0;
    background: none;
}}
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {{
    background: none;
}}

/* ── Frame ──────────────────────────────────────────── */
QFrame[frameShape="6"] {{
    color: {BORDER};
}}
"""
