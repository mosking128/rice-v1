from __future__ import annotations

import threading
import time
from typing import Optional

import serial
import serial.tools.list_ports
from PyQt5.QtCore import QObject, pyqtSignal as Signal


class SerialReaderThread(threading.Thread):
    def __init__(self, port: serial.Serial, on_text, on_error):
        super().__init__(daemon=True)
        self._port = port
        self._on_text = on_text
        self._on_error = on_error
        self._stop_event = threading.Event()

    def stop(self) -> None:
        self._stop_event.set()

    def run(self) -> None:
        while not self._stop_event.is_set():
            try:
                waiting = self._port.in_waiting
                if waiting:
                    data = self._port.read(waiting)
                else:
                    data = self._port.read(1)
            except Exception as exc:  # pragma: no cover - hardware dependent
                if not self._stop_event.is_set():
                    self._on_error(str(exc))
                return

            if data:
                self._on_text(data.decode("utf-8", errors="replace"))
            else:
                time.sleep(0.01)


class SerialManager(QObject):
    text_received = Signal(str)
    error_occurred = Signal(str)
    connection_changed = Signal(bool, str)

    def __init__(self) -> None:
        super().__init__()
        self._port: Optional[serial.Serial] = None
        self._reader: Optional[SerialReaderThread] = None
        self._lock = threading.Lock()

    @staticmethod
    def list_ports() -> list[str]:
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def is_connected(self) -> bool:
        return self._port is not None and self._port.is_open

    def open(self, port_name: str, baudrate: int) -> bool:
        self.close()

        try:
            self._port = serial.Serial(
                port=port_name,
                baudrate=baudrate,
                timeout=0.1,
                write_timeout=1.0,
            )
        except Exception as exc:
            self.error_occurred.emit(f"打开串口失败: {exc}")
            self.connection_changed.emit(False, "")
            return False

        self._reader = SerialReaderThread(
            self._port,
            self.text_received.emit,
            self._handle_reader_error,
        )
        self._reader.start()
        self.connection_changed.emit(True, port_name)
        return True

    def close(self) -> None:
        reader = self._reader
        port = self._port
        self._reader = None
        self._port = None

        if reader is not None:
            reader.stop()
            reader.join(timeout=0.5)

        if port is not None:
            try:
                if port.is_open:
                    port.close()
            except Exception:
                pass

        self.connection_changed.emit(False, "")

    def send_text(self, text: str) -> bool:
        if not self.is_connected():
            self.error_occurred.emit("串口未连接。")
            return False

        assert self._port is not None

        try:
            payload = text.encode("utf-8")
            with self._lock:
                self._port.write(payload)
                self._port.flush()
            return True
        except Exception as exc:
            self.error_occurred.emit(f"串口发送失败: {exc}")
            self.close()
            return False

    def _handle_reader_error(self, message: str) -> None:
        self.error_occurred.emit(f"串口接收失败: {message}")
        self.close()
