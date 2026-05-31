import sys

from PyQt5.QtWidgets import QApplication

from main_window import MainWindow
from theme import STYLESHEET


def main() -> int:
    app = QApplication(sys.argv)
    app.setStyleSheet(STYLESHEET)
    window = MainWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
