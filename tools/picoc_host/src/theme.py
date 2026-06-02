"""PicoC Host Tool — Multi-theme support (VS Code / Keil MDK style)"""

# ── Dark Theme Colors ────────────────────────────────────────
DARK_COLORS = {
    "BG_PRIMARY": "#1e1e1e",
    "BG_SIDEBAR": "#252526",
    "BG_TOOLBAR": "#2d2d2d",
    "BG_INPUT": "#3c3c3c",
    "BG_HOVER": "#2a2d2e",
    "BG_SELECTED": "#094771",
    "TEXT_PRIMARY": "#e0e0e0",
    "TEXT_SECONDARY": "#b0b0b0",
    "TEXT_DISABLED": "#6a6a6a",
    "TEXT_ACCENT": "#569cd6",
    "BORDER": "#3c3c3c",
    "BORDER_FOCUS": "#007acc",
    "ACCENT": "#0078d4",
    "ACCENT_HOVER": "#1a8cff",
    "SUCCESS": "#4ec9b0",
    "ERROR": "#f44747",
    "WARNING": "#dcdcaa",
    "INFO": "#969696",
    "CONSOLE_TEXT": "#e0e0e0",
    "CONSOLE_PROMPT": "#569cd6",
    "CONSOLE_ERROR": "#f44747",
    "CONSOLE_SUCCESS": "#4ec9b0",
    "CONSOLE_INFO": "#969696",
    "CONSOLE_SEP": "#dcdcaa",
    "CONSOLE_SOURCE": "#e0e0e0",
    "SCROLLBAR_HANDLE": "rgba(121, 121, 121, 0.4)",
    "SCROLLBAR_HOVER": "rgba(121, 121, 121, 0.7)",
}

# ── Light Theme Colors (VS Code Light) ───────────────────────
LIGHT_COLORS = {
    "BG_PRIMARY": "#ffffff",
    "BG_SIDEBAR": "#f3f3f3",
    "BG_TOOLBAR": "#f0f0f0",
    "BG_INPUT": "#ffffff",
    "BG_HOVER": "#e8e8e8",
    "BG_SELECTED": "#d6ebff",
    "TEXT_PRIMARY": "#1e1e1e",
    "TEXT_SECONDARY": "#616161",
    "TEXT_DISABLED": "#cccccc",
    "TEXT_ACCENT": "#0066cc",
    "BORDER": "#d4d4d4",
    "BORDER_FOCUS": "#0090ff",
    "ACCENT": "#0066cc",
    "ACCENT_HOVER": "#0077e6",
    "SUCCESS": "#008000",
    "ERROR": "#e51400",
    "WARNING": "#bf8803",
    "INFO": "#616161",
    "CONSOLE_TEXT": "#1e1e1e",
    "CONSOLE_PROMPT": "#0066cc",
    "CONSOLE_ERROR": "#e51400",
    "CONSOLE_SUCCESS": "#008000",
    "CONSOLE_INFO": "#616161",
    "CONSOLE_SEP": "#bf8803",
    "CONSOLE_SOURCE": "#1e1e1e",
    "SCROLLBAR_HANDLE": "rgba(0, 0, 0, 0.15)",
    "SCROLLBAR_HOVER": "rgba(0, 0, 0, 0.35)",
}

# ── Editor lexer colors ──────────────────────────────────────
DARK_LEXER = {
    "default": "#d4d4d4",
    "keyword": "#569cd6",
    "keyword2": "#4ec9b0",
    "string": "#ce9178",
    "comment": "#6a9955",
    "number": "#b5cea8",
    "identifier": "#9cdcfe",
    "operator": "#d4d4d4",
    "preproc": "#808080",
    "background": "#1e1e1e",
    "margin_bg": "#252526",
    "margin_fg": "#858585",
}

LIGHT_LEXER = {
    "default": "#000000",
    "keyword": "#0000ff",
    "keyword2": "#267f99",
    "string": "#a31515",
    "comment": "#008000",
    "number": "#098658",
    "identifier": "#001080",
    "operator": "#000000",
    "preproc": "#808000",
    "background": "#ffffff",
    "margin_bg": "#f0f0f0",
    "margin_fg": "#237893",
}

# ── Fonts ───────────────────────────────────────────────────
FONT_CONSOLE = "Cascadia Code, 13px"
FONT_TABLE = "Cascadia Code, 12px"
FONT_UI = "'Segoe UI', 'Microsoft YaHei UI', 13px"
FONT_CODE = "'Cascadia Code', Consolas, 'Courier New', monospace"



def get_theme(name: str = "dark"):
    """Return (colors_dict, stylesheet) for the given theme."""
    colors_map = {"dark": DARK_COLORS, "light": LIGHT_COLORS}
    colors = colors_map.get(name, DARK_COLORS)
    return colors, _build_stylesheet(colors)


def get_lexer_theme(name: str = "dark"):
    """Return lexer color dict for the given theme."""
    return DARK_LEXER if name == "dark" else LIGHT_LEXER


def _build_stylesheet(c):
    return f"""
/* ── Global ─────────────────────────────────────────── */
QWidget {{
    background-color: {c["BG_PRIMARY"]};
    color: {c["TEXT_PRIMARY"]};
}}

/* ── ToolBar ────────────────────────────────────────── */
QToolBar {{
    background-color: {c["BG_TOOLBAR"]};
    border-bottom: 1px solid {c["BORDER"]};
    padding: 2px;
    spacing: 4px;
}}
QToolBar::separator {{
    width: 1px;
    background-color: {c["BORDER"]};
    margin: 4px 6px;
}}

/* ── MenuBar & Menu ─────────────────────────────────── */
QMenuBar {{
    background-color: {c["BG_SIDEBAR"]};
    color: {c["TEXT_PRIMARY"]};
    border-bottom: 1px solid {c["BORDER"]};
}}
QMenuBar::item:selected {{
    background-color: {c["BG_SELECTED"]};
}}
QMenu {{
    background-color: {c["BG_TOOLBAR"]};
    color: {c["TEXT_PRIMARY"]};
    border: 1px solid {c["BORDER"]};
}}
QMenu::item:selected {{
    background-color: {c["BG_SELECTED"]};
}}
QMenu::separator {{
    height: 1px;
    background-color: {c["BORDER"]};
    margin: 4px 8px;
}}

/* ── Push Buttons ───────────────────────────────────── */
QPushButton {{
    background-color: {c["BG_INPUT"]};
    color: {c["TEXT_PRIMARY"]};
    border: 1px solid {c["BORDER"]};
    border-radius: 3px;
    padding: 4px 12px;
    min-height: 22px;
}}
QPushButton:hover {{
    background-color: {c["BG_HOVER"]};
    border-color: {c["BORDER_FOCUS"]};
}}
QPushButton:pressed {{
    background-color: {c["BG_SELECTED"]};
}}
QPushButton:disabled {{
    color: {c["TEXT_DISABLED"]};
    background-color: {c["BG_TOOLBAR"]};
    border-color: {c["BORDER"]};
}}

/* ── Combo Boxes ────────────────────────────────────── */
QComboBox {{
    background-color: {c["BG_INPUT"]};
    color: {c["TEXT_PRIMARY"]};
    border: 1px solid {c["BORDER"]};
    border-radius: 3px;
    padding: 3px 8px;
    min-height: 22px;
}}
QComboBox:hover {{
    border-color: {c["BORDER_FOCUS"]};
}}
QComboBox::drop-down {{
    border: none;
    width: 20px;
}}
QComboBox QAbstractItemView {{
    background-color: {c["BG_INPUT"]};
    color: {c["TEXT_PRIMARY"]};
    border: 1px solid {c["BORDER"]};
    selection-background-color: {c["BG_SELECTED"]};
}}

/* ── Line Edits ─────────────────────────────────────── */
QLineEdit {{
    background-color: {c["BG_INPUT"]};
    color: {c["TEXT_PRIMARY"]};
    border: 1px solid {c["BORDER"]};
    border-radius: 3px;
    padding: 3px 8px;
    min-height: 22px;
}}
QLineEdit:focus {{
    border-color: {c["BORDER_FOCUS"]};
}}

/* ── Text Edit (Console) ────────────────────────────── */
QTextEdit {{
    background-color: {c["BG_PRIMARY"]};
    color: {c["CONSOLE_TEXT"]};
    border: 1px solid {c["BORDER"]};
    font: {FONT_CONSOLE};
    selection-background-color: {c["BG_SELECTED"]};
}}
QPlainTextEdit {{
    background-color: {c["BG_PRIMARY"]};
    color: {c["CONSOLE_SOURCE"]};
    border: 1px solid {c["BORDER"]};
    font: {FONT_CONSOLE};
    selection-background-color: {c["BG_SELECTED"]};
}}

/* ── List Widget (File List) ────────────────────────── */
QListWidget {{
    background-color: {c["BG_SIDEBAR"]};
    color: {c["TEXT_PRIMARY"]};
    border: 1px solid {c["BORDER"]};
    outline: none;
}}
QListWidget::item {{
    padding: 4px 8px;
    border-bottom: 1px solid {c["BORDER"]};
}}
QListWidget::item:selected {{
    background-color: {c["BG_SELECTED"]};
    color: {c["TEXT_PRIMARY"]};
}}
QListWidget::item:hover {{
    background-color: {c["BG_HOVER"]};
}}

/* ── Table Widget (Variable / Watch) ────────────────── */
QTableWidget {{
    background-color: {c["BG_PRIMARY"]};
    color: {c["TEXT_PRIMARY"]};
    border: 1px solid {c["BORDER"]};
    gridline-color: {c["BORDER"]};
    font: {FONT_TABLE};
    selection-background-color: {c["BG_SELECTED"]};
}}
QTableWidget::item {{
    padding: 2px 6px;
}}
QTableWidget::item:selected {{
    background-color: {c["BG_SELECTED"]};
}}
QHeaderView::section {{
    background-color: {c["BG_TOOLBAR"]};
    color: {c["TEXT_SECONDARY"]};
    border: 1px solid {c["BORDER"]};
    padding: 3px 6px;
    font-weight: bold;
}}

/* ── Group Box ──────────────────────────────────────── */
QGroupBox {{
    border: 1px solid {c["BORDER"]};
    border-radius: 4px;
    margin-top: 8px;
    padding-top: 12px;
    color: {c["TEXT_PRIMARY"]};
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
    background-color: {c["BG_TOOLBAR"]};
    color: {c["TEXT_PRIMARY"]};
    border-top: 1px solid {c["BORDER"]};
}}
QStatusBar::item {{
    border: none;
}}

/* ── Splitter ───────────────────────────────────────── */
QSplitter::handle {{
    background-color: {c["BORDER"]};
}}
QSplitter::handle:horizontal {{
    width: 3px;
}}
QSplitter::handle:vertical {{
    height: 3px;
}}

/* ── Labels ─────────────────────────────────────────── */
QLabel {{
    color: {c["TEXT_PRIMARY"]};
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
    background-color: {c["SCROLLBAR_HANDLE"]};
    min-height: 30px;
    border-radius: 5px;
    border: 2px solid transparent;
    background-clip: padding-box;
}}
QScrollBar::handle:vertical:hover {{
    background-color: {c["SCROLLBAR_HOVER"]};
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
    background-color: {c["SCROLLBAR_HANDLE"]};
    min-width: 30px;
    border-radius: 5px;
    border: 2px solid transparent;
    background-clip: padding-box;
}}
QScrollBar::handle:horizontal:hover {{
    background-color: {c["SCROLLBAR_HOVER"]};
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
    color: {c["BORDER"]};
}}
"""

# ── Backward-compatible module-level constants (dark theme) ──
_c = DARK_COLORS
BG_PRIMARY = _c["BG_PRIMARY"]
BG_SIDEBAR = _c["BG_SIDEBAR"]
BG_TOOLBAR = _c["BG_TOOLBAR"]
BG_INPUT = _c["BG_INPUT"]
BG_HOVER = _c["BG_HOVER"]
BG_SELECTED = _c["BG_SELECTED"]
TEXT_PRIMARY = _c["TEXT_PRIMARY"]
TEXT_SECONDARY = _c["TEXT_SECONDARY"]
TEXT_DISABLED = _c["TEXT_DISABLED"]
TEXT_ACCENT = _c["TEXT_ACCENT"]
BORDER = _c["BORDER"]
BORDER_FOCUS = _c["BORDER_FOCUS"]
ACCENT = _c["ACCENT"]
ACCENT_HOVER = _c["ACCENT_HOVER"]
SUCCESS = _c["SUCCESS"]
ERROR = _c["ERROR"]
WARNING = _c["WARNING"]
INFO = _c["INFO"]
CONSOLE_TEXT = _c["CONSOLE_TEXT"]
CONSOLE_PROMPT = _c["CONSOLE_PROMPT"]
CONSOLE_ERROR = _c["CONSOLE_ERROR"]
CONSOLE_SUCCESS = _c["CONSOLE_SUCCESS"]
CONSOLE_INFO = _c["CONSOLE_INFO"]
CONSOLE_SEP = _c["CONSOLE_SEP"]
CONSOLE_SOURCE = _c["CONSOLE_SOURCE"]
STYLESHEET = _build_stylesheet(DARK_COLORS)
