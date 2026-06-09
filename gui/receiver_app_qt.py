# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Sunil Hegde & Mythili Shetty

#!/usr/bin/env python3
"""
AudioSync Receiver - Native macOS Style with PyQt6
"""

import sys
import os
import subprocess
import threading
import socket
import time

try:
    from PyQt6.QtWidgets import *
    from PyQt6.QtCore import *
    from PyQt6.QtGui import *
except ImportError:
    print("PyQt6 not installed. Install with: pip install PyQt6")
    sys.exit(1)

class ProcessWorker(QThread):
    output = pyqtSignal(str)
    finished = pyqtSignal()
    
    def __init__(self, command):
        super().__init__()
        self.command = command
        self.process = None
        
    def run(self):
        try:
            self.process = subprocess.Popen(
                self.command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                cwd=os.path.dirname(os.path.dirname(__file__))
            )
            
            for line in iter(self.process.stdout.readline, ''):
                if line:
                    self.output.emit(line.strip())
                    
        except Exception as e:
            self.output.emit(f"Error: {e}")
        finally:
            self.finished.emit()
    
    def stop(self):
        if self.process:
            self.process.terminate()

class StatusIndicator(QWidget):
    def __init__(self):
        super().__init__()
        self.setFixedSize(12, 12)
        self._active = False
        
    def set_active(self, active):
        self._active = active
        self.update()
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        color = QColor("#30d158") if self._active else QColor("#d1d1d6")
        painter.setBrush(QBrush(color))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawEllipse(2, 2, 8, 8)

class AudioReceiverGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("AudioSync Receiver")
        self.setFixedSize(800, 600)
        
        # Variables
        self.receiver_worker = None
        self.is_running = False
        
        self.setup_ui()
        self.check_network()
        
    def setup_ui(self):
        # Set macOS style
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f6f6f6;
            }
            QWidget {
                font-family: -apple-system, BlinkMacSystemFont, "SF Pro Display";
                color: #1d1d1f;
            }
            QLabel {
                color: #1d1d1f;
                font-size: 13px;
            }
            QLabel#title {
                font-size: 18px;
                font-weight: 600;
                color: #1d1d1f;
            }
            QLabel#subtitle {
                font-size: 11px;
                color: #6e6e73;
            }
            QLabel#section_title {
                font-size: 14px;
                font-weight: 600;
                color: #1d1d1f;
            }
            QLabel#small_label {
                font-size: 10px;
                font-weight: 600;
                color: #6e6e73;
            }
            QLabel#status_label {
                font-size: 12px;
                font-weight: 600;
                color: #1d1d1f;
            }
            QFrame#card {
                background-color: white;
                border: 1px solid #d1d1d6;
                border-radius: 8px;
            }
            QPushButton {
                border: none;
                border-radius: 6px;
                padding: 8px 16px;
                font-weight: 500;
                font-size: 11px;
            }
            QPushButton#primary {
                background-color: #007aff;
                color: white;
            }
            QPushButton#primary:hover {
                background-color: #0056d3;
            }
            QPushButton#secondary {
                background-color: #f2f2f7;
                color: #1d1d1f;
            }
            QPushButton#secondary:hover {
                background-color: #e8e8ed;
            }
            QPushButton#success {
                background-color: #30d158;
                color: white;
            }
            QPushButton#success:hover {
                background-color: #2bb24a;
            }
            QPushButton#danger {
                background-color: #ff3b30;
                color: white;
            }
            QPushButton#danger:hover {
                background-color: #d70015;
            }
            QPushButton:disabled {
                background-color: #e5e5e7;
                color: #8e8e93;
            }
            QTextEdit {
                background-color: #1e1e1e;
                color: #ffffff;
                border: 1px solid #d1d1d6;
                border-radius: 4px;
                font-family: Monaco, monospace;
                font-size: 10px;
            }
            QCheckBox {
                color: #1d1d1f;
                font-size: 11px;
            }
            QCheckBox::indicator {
                width: 16px;
                height: 16px;
                border-radius: 3px;
                border: 1px solid #d1d1d6;
                background-color: white;
            }
            QCheckBox::indicator:checked {
                background-color: #007aff;
                border-color: #007aff;
                image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTIiIGhlaWdodD0iOSIgdmlld0JveD0iMCAwIDEyIDkiIGZpbGw9Im5vbmUiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+CjxwYXRoIGQ9Ik0xMC42IDEuNEw0LjggN0wxLjQgMy42IiBzdHJva2U9IndoaXRlIiBzdHJva2Utd2lkdGg9IjEuNSIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIi8+Cjwvc3ZnPgo=);
            }
        """)
        
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(20, 20, 20, 20)
        main_layout.setSpacing(20)
        
        # Header
        header_layout = QHBoxLayout()
        
        title = QLabel("AudioSync Receiver")
        title.setObjectName("title")
        header_layout.addWidget(title)
        
        header_layout.addStretch()
        
        subtitle = QLabel("Listen for incoming audio streams")
        subtitle.setObjectName("subtitle")
        header_layout.addWidget(subtitle)
        
        main_layout.addLayout(header_layout)
        
        # Content area
        content_layout = QHBoxLayout()
        content_layout.setSpacing(20)
        
        # Left column
        left_widget = QWidget()
        left_widget.setFixedWidth(350)
        left_layout = QVBoxLayout(left_widget)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(15)
        
        # Connection status card
        status_card = QFrame()
        status_card.setObjectName("card")
        status_layout = QVBoxLayout(status_card)
        status_layout.setContentsMargins(20, 20, 20, 20)
        status_layout.setSpacing(15)
        
        status_title = QLabel("Connection")
        status_title.setObjectName("section_title")
        status_layout.addWidget(status_title)
        
        # Status indicator
        status_row = QHBoxLayout()
        
        self.status_indicator = StatusIndicator()
        status_row.addWidget(self.status_indicator)
        
        self.status_label = QLabel("Stopped")
        self.status_label.setObjectName("status_label")
        status_row.addWidget(self.status_label)
        
        status_row.addStretch()
        status_layout.addLayout(status_row)
        
        # Network info
        network_label = QLabel("Network:")
        network_label.setObjectName("small_label")
        status_layout.addWidget(network_label)
        
        network_row = QHBoxLayout()
        
        self.ip_label = QLabel("Getting...")
        self.ip_label.setStyleSheet("font-family: Monaco, monospace; font-size: 10px;")
        network_row.addWidget(self.ip_label)
        
        network_row.addStretch()
        
        self.refresh_btn = QPushButton("↻")
        self.refresh_btn.setObjectName("secondary")
        self.refresh_btn.setFixedSize(30, 24)
        self.refresh_btn.clicked.connect(self.check_network)
        network_row.addWidget(self.refresh_btn)
        
        status_layout.addLayout(network_row)
        
        # Controls
        controls_label = QLabel("Controls:")
        controls_label.setObjectName("small_label")
        status_layout.addWidget(controls_label)
        
        controls_row = QHBoxLayout()
        
        self.start_btn = QPushButton("Start")
        self.start_btn.setObjectName("success")
        self.start_btn.clicked.connect(self.start_receiver)
        controls_row.addWidget(self.start_btn)
        
        self.stop_btn = QPushButton("Stop")
        self.stop_btn.setObjectName("danger")
        self.stop_btn.clicked.connect(self.stop_receiver)
        self.stop_btn.setEnabled(False)
        controls_row.addWidget(self.stop_btn)
        
        controls_row.addStretch()
        status_layout.addLayout(controls_row)
        
        left_layout.addWidget(status_card)
        
        # Instructions card
        instructions_card = QFrame()
        instructions_card.setObjectName("card")
        instructions_layout = QVBoxLayout(instructions_card)
        instructions_layout.setContentsMargins(20, 20, 20, 20)
        instructions_layout.setSpacing(15)
        
        instructions_title = QLabel("Setup")
        instructions_title.setObjectName("section_title")
        instructions_layout.addWidget(instructions_title)
        
        instructions = [
            "Same network as sender",
            "Click 'Start' to listen",
            "Launch sender with audio",
            "Audio plays automatically"
        ]
        
        for i, instruction in enumerate(instructions, 1):
            step_layout = QHBoxLayout()
            
            step_num = QLabel(f"{i}.")
            step_num.setStyleSheet("color: #007aff; font-weight: 600; font-size: 10px;")
            step_num.setFixedWidth(15)
            step_layout.addWidget(step_num)
            
            step_text = QLabel(instruction)
            step_text.setStyleSheet("font-size: 10px;")
            step_layout.addWidget(step_text)
            
            step_layout.addStretch()
            instructions_layout.addLayout(step_layout)
        
        # Audio note
        audio_note = QLabel("🔊 Audio plays through default device")
        audio_note.setStyleSheet("color: #6e6e73; font-size: 9px;")
        instructions_layout.addWidget(audio_note)
        
        left_layout.addWidget(instructions_card)
        left_layout.addStretch()
        
        content_layout.addWidget(left_widget)
        
        # Right column - Console
        console_widget = QWidget()
        console_layout = QVBoxLayout(console_widget)
        console_layout.setContentsMargins(0, 0, 0, 0)
        console_layout.setSpacing(10)
        
        # Console header
        console_header = QHBoxLayout()
        console_title = QLabel("Console")
        console_title.setObjectName("section_title")
        console_header.addWidget(console_title)
        
        console_header.addStretch()
        
        self.auto_scroll_check = QCheckBox("Auto")
        self.auto_scroll_check.setChecked(True)
        console_header.addWidget(self.auto_scroll_check)
        
        self.clear_log_btn = QPushButton("Clear")
        self.clear_log_btn.setObjectName("secondary")
        self.clear_log_btn.clicked.connect(self.clear_log)
        console_header.addWidget(self.clear_log_btn)
        
        console_layout.addLayout(console_header)
        
        # Console text
        self.console_text = QTextEdit()
        self.console_text.setReadOnly(True)
        console_layout.addWidget(self.console_text)
        
        content_layout.addWidget(console_widget)
        
        main_layout.addLayout(content_layout)
        
        self.log("AudioSync Receiver ready")
        
    def check_network(self):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            local_ip = s.getsockname()[0]
            s.close()
            
            self.ip_label.setText(local_ip)
            self.log("✓ Network check: OK")
            
        except Exception as e:
            self.ip_label.setText("Not connected")
            self.log(f"✗ Network check failed: {e}")
            
    def start_receiver(self):
        if not os.path.exists("./build/receiver"):
            QMessageBox.critical(self, "Error", "AudioSync receiver not found! Please build the project first.")
            return
            
        self.receiver_worker = ProcessWorker(["./build/receiver"])
        self.receiver_worker.output.connect(self.log)
        self.receiver_worker.finished.connect(self.on_receiver_finished)
        self.receiver_worker.start()
        
        self.is_running = True
        self.status_indicator.set_active(True)
        self.status_label.setText("Listening")
        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self.log("Started listening for audio streams")
        
    def stop_receiver(self):
        if self.receiver_worker:
            self.receiver_worker.stop()
            
    def on_receiver_finished(self):
        self.is_running = False
        self.status_indicator.set_active(False)
        self.status_label.setText("Stopped")
        self.start_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)
        self.log("Stopped listening")
        
    def log(self, message):
        timestamp = QTime.currentTime().toString()
        self.console_text.append(f"[{timestamp}] {message}")
        
        if self.auto_scroll_check.isChecked():
            cursor = self.console_text.textCursor()
            cursor.movePosition(cursor.MoveOperation.End)
            self.console_text.setTextCursor(cursor)
        
    def clear_log(self):
        self.console_text.clear()
        self.log("Console cleared")
        
    def closeEvent(self, event):
        if self.receiver_worker:
            self.receiver_worker.stop()
        event.accept()

def main():
    app = QApplication(sys.argv)
    
    # Set macOS-specific properties
    app.setApplicationName("AudioSync Receiver")
    app.setOrganizationName("AudioSync")
    app.setApplicationVersion("1.0")
    
    window = AudioReceiverGUI()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()