# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Sunil Hegde & Mythili Shetty

#!/usr/bin/env python3
"""
AudioSync Launcher - Native macOS Style with PyQt6
"""

import sys
import os
import subprocess

try:
    from PyQt6.QtWidgets import *
    from PyQt6.QtCore import *
    from PyQt6.QtGui import *
except ImportError:
    print("PyQt6 not installed. Install with: pip install PyQt6")
    sys.exit(1)

class AudioSyncLauncher(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("AudioSync")
        self.setFixedSize(600, 500)
        
        self.setup_ui()
        
    def setup_ui(self):
        # Set macOS style with fallback fonts
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f6f6f6;
            }
            QWidget {
                font-family: "SF Pro Display", "Helvetica Neue", Arial, sans-serif;
                color: #1d1d1f;
            }
            QLabel {
                color: #1d1d1f;
                font-size: 13px;
            }
            QLabel#title {
                font-size: 24px;
                font-weight: bold;
                color: #1d1d1f;
            }
            QLabel#subtitle {
                font-size: 14px;
                color: #6e6e73;
            }
            QLabel#feature_title {
                font-size: 16px;
                font-weight: bold;
                color: #1d1d1f;
            }
            QLabel#feature_desc {
                font-size: 12px;
                color: #6e6e73;
            }
            QFrame#card {
                background-color: white;
                border: 1px solid #d1d1d6;
                border-radius: 12px;
                width: 220px;
                height: 200px;
            }
            QPushButton {
                border: none;
                border-radius: 8px;
                padding: 12px 24px;
                font-weight: bold;
                font-size: 13px;
                min-width: 120px;
                min-height: 40px;
            }
            QPushButton#primary {
                background-color: #007aff;
                color: white;
            }
            QPushButton#primary:hover {
                background-color: #0056d3;
            }
            QPushButton#secondary {
                background-color: #30d158;
                color: white;
            }
            QPushButton#secondary:hover {
                background-color: #2bb24a;
            }
            QPushButton#build {
                background-color: #ff9500;
                color: white;
            }
            QPushButton#build:hover {
                background-color: #cc7700;
            }
            QPushButton:disabled {
                background-color: #e5e5e7;
                color: #8e8e93;
            }
        """)
        
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(40, 40, 40, 40)
        main_layout.setSpacing(30)
        
        # Header
        header_layout = QVBoxLayout()
        header_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        title = QLabel("AudioSync")
        title.setObjectName("title")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        header_layout.addWidget(title)
        
        subtitle = QLabel("Real-time synchronized audio streaming")
        subtitle.setObjectName("subtitle")
        subtitle.setAlignment(Qt.AlignmentFlag.AlignCenter)
        header_layout.addWidget(subtitle)
        
        main_layout.addLayout(header_layout)
        
        # Cards container
        cards_layout = QHBoxLayout()
        cards_layout.setSpacing(30)
        
        # Sender card
        sender_card = QFrame()
        sender_card.setObjectName("card")
        sender_card.setFixedSize(220, 250)
        sender_layout = QVBoxLayout(sender_card)
        sender_layout.setContentsMargins(20, 20, 20, 20)
        sender_layout.setSpacing(15)
        
        sender_icon = QLabel("📤")
        sender_icon.setAlignment(Qt.AlignmentFlag.AlignCenter)
        sender_icon.setStyleSheet("font-size: 32px;")
        sender_layout.addWidget(sender_icon)
        
        sender_title = QLabel("Sender")
        sender_title.setObjectName("feature_title")
        sender_title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        sender_layout.addWidget(sender_title)
        
        sender_desc = QLabel("Stream audio files to receivers on your network")
        sender_desc.setObjectName("feature_desc")
        sender_desc.setAlignment(Qt.AlignmentFlag.AlignCenter)
        sender_desc.setWordWrap(True)
        sender_layout.addWidget(sender_desc)
        
        sender_layout.addStretch()
        
        sender_btn = QPushButton("Launch Sender")
        sender_btn.setObjectName("primary")
        sender_btn.clicked.connect(self.launch_sender)
        sender_layout.addWidget(sender_btn)
        
        cards_layout.addWidget(sender_card)
        
        # Receiver card
        receiver_card = QFrame()
        receiver_card.setObjectName("card")
        receiver_card.setFixedSize(220, 250)
        receiver_layout = QVBoxLayout(receiver_card)
        receiver_layout.setContentsMargins(20, 20, 20, 20)
        receiver_layout.setSpacing(15)
        
        receiver_icon = QLabel("📥")
        receiver_icon.setAlignment(Qt.AlignmentFlag.AlignCenter)
        receiver_icon.setStyleSheet("font-size: 32px;")
        receiver_layout.addWidget(receiver_icon)
        
        receiver_title = QLabel("Receiver")
        receiver_title.setObjectName("feature_title")
        receiver_title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        receiver_layout.addWidget(receiver_title)
        
        receiver_desc = QLabel("Listen for and play incoming audio streams")
        receiver_desc.setObjectName("feature_desc")
        receiver_desc.setAlignment(Qt.AlignmentFlag.AlignCenter)
        receiver_desc.setWordWrap(True)
        receiver_layout.addWidget(receiver_desc)
        
        receiver_layout.addStretch()
        
        receiver_btn = QPushButton("Launch Receiver")
        receiver_btn.setObjectName("secondary")
        receiver_btn.clicked.connect(self.launch_receiver)
        receiver_layout.addWidget(receiver_btn)
        
        cards_layout.addWidget(receiver_card)
        
        # Center the cards
        cards_container = QHBoxLayout()
        cards_container.addStretch()
        cards_container.addLayout(cards_layout)
        cards_container.addStretch()
        
        main_layout.addLayout(cards_container)
        
        main_layout.addStretch()
        
        # Check if executables exist
        self.check_build_status()
        
    def check_build_status(self):
        # Just check if executables exist, don't show build dialog
        pass
        
    def launch_sender(self):
        try:
            script_path = os.path.join(os.path.dirname(__file__), "sender_app_qt.py")
            subprocess.Popen([sys.executable, script_path], 
                           cwd=os.path.dirname(os.path.dirname(__file__)))
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to launch sender: {e}")
            
    def launch_receiver(self):
        try:
            script_path = os.path.join(os.path.dirname(__file__), "receiver_app_qt.py")
            subprocess.Popen([sys.executable, script_path], 
                           cwd=os.path.dirname(os.path.dirname(__file__)))
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to launch receiver: {e}")
            
def main():
    app = QApplication(sys.argv)
    
    # Set macOS-specific properties
    app.setApplicationName("AudioSync")
    app.setOrganizationName("AudioSync")
    app.setApplicationVersion("1.0")
    
    window = AudioSyncLauncher()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()