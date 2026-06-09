# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Sunil Hegde & Mythili Shetty

#!/usr/bin/env python3
"""
AudioSync Sender - Native macOS Style with PyQt6
"""

import sys
import os
import subprocess
import threading
import tempfile
import shutil
from pathlib import Path

try:
    from PyQt6.QtWidgets import *
    from PyQt6.QtCore import *
    from PyQt6.QtGui import *
except ImportError:
    print("PyQt6 not installed. Install with: pip install PyQt6")
    sys.exit(1)

class ProgressWorker(QThread):
    progress = pyqtSignal(int)
    finished = pyqtSignal()
    
    def __init__(self):
        super().__init__()
        self._running = False
        
    def run(self):
        self._running = True
        progress = 0
        while self._running:
            progress = (progress + 1) % 101
            self.progress.emit(progress)
            self.msleep(50)
        self.finished.emit()
    
    def stop(self):
        self._running = False

class ConversionWorker(QThread):
    finished = pyqtSignal(bool, str, str)  # success, message, output_file
    
    def __init__(self, input_file):
        super().__init__()
        self.input_file = input_file
        
    def run(self):
        try:
            # Create temporary directory
            temp_dir = tempfile.mkdtemp(prefix="audiosync_")
            
            # Generate raw file path
            original_path = Path(self.input_file)
            raw_filename = f"{original_path.stem}.raw"
            raw_path = os.path.join(temp_dir, raw_filename)
            
            # FFmpeg command
            cmd = [
                "ffmpeg", "-i", self.input_file,
                "-f", "s16le",
                "-ar", "44100",
                "-ac", "2",
                "-y",
                raw_path
            ]
            
            process = subprocess.run(cmd, capture_output=True, text=True)
            
            if process.returncode == 0:
                self.finished.emit(True, f"Converted successfully! ({os.path.getsize(raw_path):,} bytes)", raw_path)
            else:
                self.finished.emit(False, f"Conversion failed: {process.stderr}", "")
                
        except Exception as e:
            self.finished.emit(False, f"Error: {str(e)}", "")

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

class AudioSenderGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("AudioSync Sender")
        self.setFixedSize(900, 650)
        
        # Variables
        self.original_file = ""
        self.raw_file = ""
        self.temp_dir = None
        self.conversion_worker = None
        self.progress_worker = None
        self.sender_worker = None
        
        self.setup_ui()
        
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
            QFrame#card {
                background-color: white;
                border: 1px solid #d1d1d6;
                border-radius: 8px;
            }
            QFrame#drop_zone {
                background-color: #f0f9ff;
                border: 2px dashed #007aff;
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
            QPushButton#warning {
                background-color: #ff9500;
                color: white;
            }
            QPushButton#warning:hover {
                background-color: #e6851a;
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
            QLineEdit {
                background-color: #f2f2f7;
                border: 1px solid #d1d1d6;
                border-radius: 4px;
                padding: 6px 8px;
                font-family: Monaco, monospace;
                font-size: 9px;
            }
            QTextEdit {
                background-color: #1e1e1e;
                color: #ffffff;
                border: 1px solid #d1d1d6;
                border-radius: 4px;
                font-family: Monaco, monospace;
                font-size: 10px;
            }
            QProgressBar {
                border: none;
                border-radius: 2px;
                background-color: #e5e5e7;
                text-align: center;
            }
            QProgressBar::chunk {
                background-color: #007aff;
                border-radius: 2px;
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
        
        title = QLabel("AudioSync Sender")
        title.setObjectName("title")
        header_layout.addWidget(title)
        
        header_layout.addStretch()
        
        subtitle = QLabel("Stream audio to multiple devices")
        subtitle.setObjectName("subtitle")
        header_layout.addWidget(subtitle)
        
        main_layout.addLayout(header_layout)
        
        # Content area
        content_layout = QHBoxLayout()
        content_layout.setSpacing(20)
        
        # Left column
        left_widget = QWidget()
        left_widget.setFixedWidth(400)
        left_layout = QVBoxLayout(left_widget)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(15)
        
        # File selection card
        file_card = QFrame()
        file_card.setObjectName("card")
        file_layout = QVBoxLayout(file_card)
        file_layout.setContentsMargins(20, 20, 20, 20)
        file_layout.setSpacing(15)
        
        file_title = QLabel("Audio File")
        file_title.setObjectName("section_title")
        file_layout.addWidget(file_title)
        
        # Drop zone
        self.drop_zone = QFrame()
        self.drop_zone.setObjectName("drop_zone")
        self.drop_zone.setFixedHeight(80)
        self.drop_zone.setAcceptDrops(True)
        
        drop_layout = QHBoxLayout(self.drop_zone)
        drop_layout.setContentsMargins(15, 0, 15, 0)
        
        drop_icon = QLabel("⬆")
        drop_icon.setStyleSheet("font-size: 16px; color: #007aff;")
        drop_layout.addWidget(drop_icon)
        
        drop_text = QLabel("Drop audio files here or click Browse")
        drop_text.setStyleSheet("color: #6e6e73; font-size: 11px;")
        drop_layout.addWidget(drop_text)
        drop_layout.addStretch()
        
        file_layout.addWidget(self.drop_zone)
        
        # File paths
        orig_label = QLabel("Original:")
        orig_label.setObjectName("small_label")
        file_layout.addWidget(orig_label)
        
        self.orig_entry = QLineEdit()
        self.orig_entry.setReadOnly(True)
        file_layout.addWidget(self.orig_entry)
        
        conv_label = QLabel("Converted:")
        conv_label.setObjectName("small_label")
        file_layout.addWidget(conv_label)
        
        self.raw_entry = QLineEdit()
        self.raw_entry.setReadOnly(True)
        file_layout.addWidget(self.raw_entry)
        
        # Buttons
        button_layout = QHBoxLayout()
        
        self.browse_btn = QPushButton("Browse")
        self.browse_btn.setObjectName("primary")
        self.browse_btn.clicked.connect(self.browse_file)
        button_layout.addWidget(self.browse_btn)
        
        self.clear_btn = QPushButton("Clear")
        self.clear_btn.setObjectName("secondary")
        self.clear_btn.clicked.connect(self.clear_files)
        button_layout.addWidget(self.clear_btn)
        
        button_layout.addStretch()
        file_layout.addLayout(button_layout)
        
        left_layout.addWidget(file_card)
        
        # Controls card
        controls_card = QFrame()
        controls_card.setObjectName("card")
        controls_layout = QVBoxLayout(controls_card)
        controls_layout.setContentsMargins(20, 20, 20, 20)
        controls_layout.setSpacing(15)
        
        controls_title = QLabel("Controls")
        controls_title.setObjectName("section_title")
        controls_layout.addWidget(controls_title)
        
        # Status
        status_layout = QHBoxLayout()
        status_layout.addWidget(QLabel("Status:"))
        
        self.status_label = QLabel("Ready")
        self.status_label.setStyleSheet("color: #007aff; font-weight: 500;")
        status_layout.addWidget(self.status_label)
        status_layout.addStretch()
        
        controls_layout.addLayout(status_layout)
        
        # Progress bar
        self.progress_bar = QProgressBar()
        self.progress_bar.setFixedHeight(4)
        self.progress_bar.setVisible(False)
        controls_layout.addWidget(self.progress_bar)
        
        # Control buttons
        control_button_layout = QHBoxLayout()
        
        self.convert_btn = QPushButton("Convert")
        self.convert_btn.setObjectName("warning")
        self.convert_btn.clicked.connect(self.convert_audio)
        self.convert_btn.setEnabled(False)
        control_button_layout.addWidget(self.convert_btn)
        
        self.start_btn = QPushButton("Start Stream")
        self.start_btn.setObjectName("success")
        self.start_btn.clicked.connect(self.start_sender)
        self.start_btn.setEnabled(False)
        control_button_layout.addWidget(self.start_btn)
        
        self.stop_btn = QPushButton("Stop")
        self.stop_btn.setObjectName("danger")
        self.stop_btn.clicked.connect(self.stop_sender)
        self.stop_btn.setEnabled(False)
        control_button_layout.addWidget(self.stop_btn)
        
        control_button_layout.addStretch()
        controls_layout.addLayout(control_button_layout)
        
        left_layout.addWidget(controls_card)
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
        
        # Enable drag and drop
        self.setAcceptDrops(True)
        
        self.log("AudioSync Sender ready")
        
    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
            
    def dropEvent(self, event):
        files = [url.toLocalFile() for url in event.mimeData().urls()]
        if files and self.is_audio_file(files[0]):
            self.set_original_file(files[0])
        
    def is_audio_file(self, file_path):
        audio_extensions = {'.mp3', '.wav', '.flac', '.m4a', '.aac', '.ogg', '.wma', '.mp4', '.mov', '.avi', '.mkv'}
        return Path(file_path).suffix.lower() in audio_extensions
        
    def browse_file(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Select Audio File",
            "",
            "Audio Files (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.wma *.mp4 *.mov *.avi *.mkv);;All Files (*)"
        )
        if file_path:
            self.set_original_file(file_path)
            
    def set_original_file(self, file_path):
        self.original_file = file_path
        self.orig_entry.setText(file_path)
        self.convert_btn.setEnabled(True)
        self.log(f"Selected: {os.path.basename(file_path)}")
        
    def clear_files(self):
        self.original_file = ""
        self.raw_file = ""
        self.orig_entry.clear()
        self.raw_entry.clear()
        self.convert_btn.setEnabled(False)
        self.start_btn.setEnabled(False)
        self.cleanup_temp_files()
        self.log("Files cleared")
        
    def convert_audio(self):
        if not self.original_file:
            return
            
        if not shutil.which("ffmpeg"):
            QMessageBox.critical(self, "Error", "FFmpeg not found! Please install FFmpeg.")
            return
            
        self.status_label.setText("Converting...")
        self.progress_bar.setVisible(True)
        self.convert_btn.setEnabled(False)
        
        # Start progress animation
        self.progress_worker = ProgressWorker()
        self.progress_worker.progress.connect(self.progress_bar.setValue)
        self.progress_worker.start()
        
        # Start conversion
        self.conversion_worker = ConversionWorker(self.original_file)
        self.conversion_worker.finished.connect(self.on_conversion_finished)
        self.conversion_worker.start()
        
    def on_conversion_finished(self, success, message, output_file):
        self.progress_worker.stop()
        self.progress_worker.wait()
        self.progress_bar.setVisible(False)
        
        if success:
            self.raw_file = output_file
            self.raw_entry.setText(output_file)
            self.status_label.setText("Ready to stream")
            self.start_btn.setEnabled(True)
            self.log(f"✓ {message}")
        else:
            self.status_label.setText("Conversion failed")
            self.log(f"✗ {message}")
            QMessageBox.critical(self, "Conversion Error", message)
            
        self.convert_btn.setEnabled(bool(self.original_file))
        
    def start_sender(self):
        if not self.raw_file or not os.path.exists("./build/sender"):
            QMessageBox.critical(self, "Error", "AudioSync sender not found! Please build the project first.")
            return
            
        self.sender_worker = ProcessWorker(["./build/sender", self.raw_file])
        self.sender_worker.output.connect(self.log)
        self.sender_worker.finished.connect(self.on_sender_finished)
        self.sender_worker.start()
        
        self.status_label.setText("Streaming...")
        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self.log("Started streaming audio")
        
    def stop_sender(self):
        if self.sender_worker:
            self.sender_worker.stop()
            
    def on_sender_finished(self):
        self.status_label.setText("Stopped")
        self.start_btn.setEnabled(bool(self.raw_file))
        self.stop_btn.setEnabled(False)
        self.log("Stopped streaming")
        
    def log(self, message):
        self.console_text.append(f"[{QTime.currentTime().toString()}] {message}")
        
    def clear_log(self):
        self.console_text.clear()
        
    def cleanup_temp_files(self):
        if self.temp_dir and os.path.exists(self.temp_dir):
            try:
                shutil.rmtree(self.temp_dir)
            except:
                pass
        self.temp_dir = None
        
    def closeEvent(self, event):
        if self.sender_worker:
            self.sender_worker.stop()
        if self.progress_worker:
            self.progress_worker.stop()
        self.cleanup_temp_files()
        event.accept()

def main():
    app = QApplication(sys.argv)
    
    # Set macOS-specific properties
    app.setApplicationName("AudioSync Sender")
    app.setOrganizationName("AudioSync")
    app.setApplicationVersion("1.0")
    
    window = AudioSenderGUI()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()