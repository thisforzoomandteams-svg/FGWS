import sys
import random
from PyQt6.QtWidgets import (QApplication, QWidget, QGridLayout, 
                             QPushButton, QLabel, QVBoxLayout, QComboBox)
from PyQt6.QtCore import Qt, QTimer

class TicTacToe(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("PyQt6 Smart Tic-Tac-Toe")
        self.setFixedSize(400, 500)
        
        # Game State
        self.board = [""] * 9
        self.buttons = []
        self.current_player = "X"
        self.game_active = True

        self.init_ui()

    def init_ui(self):
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)

        # Status Label
        self.status_label = QLabel("Your Turn (X)")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_label.setStyleSheet("font-size: 24px; font-weight: bold; color: #3498db;")
        self.layout.addWidget(self.status_label)

        # Difficulty Selector
        self.difficulty_box = QComboBox()
        self.difficulty_box.addItems(["PvP", "Easy AI", "Smart AI"])
        self.difficulty_box.currentIndexChanged.connect(self.reset_game)
        self.layout.addWidget(self.difficulty_box)

        # Grid for the board
        self.grid_layout = QGridLayout()
        self.layout.addLayout(self.grid_layout)

        for i in range(9):
            btn = QPushButton("")
            btn.setFixedSize(100, 100)
            btn.setStyleSheet("font-size: 32px; font-weight: bold; background-color: #2c3e50; color: white; border-radius: 10px;")
            btn.clicked.connect(lambda checked, i=i: self.player_click(i))
            self.grid_layout.addWidget(btn, i // 3, i % 3)
            self.buttons.append(btn)

        # Reset Button
        self.reset_btn = QPushButton("Restart Game")
        self.reset_btn.setStyleSheet("padding: 10px; background-color: #7f8c8d; color: white; border-radius: 5px;")
        self.reset_btn.clicked.connect(self.reset_game)
        self.layout.addWidget(self.reset_btn)

    def player_click(self, index):
        if self.board[index] == "" and self.game_active:
            self.make_move(index, "X")
            
            # If AI is enabled and game isn't over, trigger AI move
            if self.game_active and "AI" in self.difficulty_box.currentText():
                self.status_label.setText("AI is thinking...")
                # QTimer.singleShot acts like self.after in Tkinter
                QTimer.singleShot(600, self.ai_move)

    def make_move(self, index, player):
        self.board[index] = player
        color = "#3498db" if player == "X" else "#e74c3c"
        self.buttons[index].setText(player)
        self.buttons[index].setStyleSheet(f"font-size: 32px; font-weight: bold; background-color: #2c3e50; color: {color}; border-radius: 10px;")

        if self.check_winner(player):
            self.status_label.setText(f"Player {player} Wins!")
            self.status_label.setStyleSheet("font-size: 24px; font-weight: bold; color: #2ecc71;")
            self.game_active = False
        elif "" not in self.board:
            self.status_label.setText("It's a Draw!")
            self.status_label.setStyleSheet("font-size: 24px; font-weight: bold; color: #f1c40f;")
            self.game_active = False
        else:
            self.current_player = "O" if player == "X" else "X"
            if "AI" not in self.difficulty_box.currentText() or player == "O":
                self.status_label.setText(f"Player {self.current_player}'s Turn")

    def ai_move(self):
        if not self.game_active: return
        
        move = -1
        empty_cells = [i for i, val in enumerate(self.board) if val == ""]
        
        if self.difficulty_box.currentText() == "Smart AI":
            move = self.get_best_move()
            
        if move == -1 and empty_cells:
            move = random.choice(empty_cells)
            
        if move != -1:
            self.make_move(move, "O")

    def get_best_move(self):
        # 1. Win if possible
        for i in range(9):
            if self.board[i] == "":
                self.board[i] = "O"
                if self.check_winner("O"):
                    self.board[i] = ""
                    return i
                self.board[i] = ""
        # 2. Block player win
        for i in range(9):
            if self.board[i] == "":
                self.board[i] = "X"
                if self.check_winner("X"):
                    self.board[i] = ""
                    return i
                self.board[i] = ""
        return -1

    def check_winner(self, p):
        wins = [(0,1,2), (3,4,5), (6,7,8), (0,3,6), (1,4,7), (2,5,8), (0,4,8), (2,4,6)]
        return any(self.board[a] == self.board[b] == self.board[c] == p for a, b, c in wins)

    def reset_game(self):
        self.board = [""] * 9
        self.game_active = True
        self.current_player = "X"
        self.status_label.setText("Your Turn (X)")
        self.status_label.setStyleSheet("font-size: 24px; font-weight: bold; color: #3498db;")
        for btn in self.buttons:
            btn.setText("")
            btn.setStyleSheet("font-size: 32px; font-weight: bold; background-color: #2c3e50; color: white; border-radius: 10px;")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = TicTacToe()
    window.show()
    sys.exit(app.exec())