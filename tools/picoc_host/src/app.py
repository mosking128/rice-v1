import sys

from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import QSettings

from main_window import MainWindow
from theme import get_theme


def main() -> int:
    app = QApplication(sys.argv)

    # Load saved theme preference
    saved_theme = QSettings("MCUStudio", "Theme").value("theme", "dark")
    _, stylesheet = get_theme(saved_theme)
    app.setStyleSheet(stylesheet)

    window = MainWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
