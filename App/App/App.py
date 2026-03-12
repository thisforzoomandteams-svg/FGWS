import customtkinter as ctk
import random
import os

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

class GuessingGame(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("Pro Guessing Game")
        self.geometry("500x550")
        
        # Game State
        self.min_val = 1
        self.max_val = 100
        self.target_number = 0
        self.attempts = 0
        self.high_score = self.load_high_score()

        # --- UI Setup ---
        self.label = ctk.CTkLabel(self, text="Select a Level to Start", font=("Roboto", 20, "bold"))
        self.label.pack(pady=20)

        # Level Selector
        self.level_var = ctk.StringVar(value="Normal")
        self.level_switch = ctk.CTkSegmentedButton(self, values=["Easy", "Normal", "Extreme", "Custom"],
                                                   command=self.change_level, variable=self.level_var)
        self.level_switch.pack(pady=10)

        # Custom Range Inputs (Hidden by default)
        self.custom_frame = ctk.CTkFrame(self, fg_color="transparent")
        self.custom_min = ctk.CTkEntry(self.custom_frame, placeholder_text="Min", width=60)
        self.custom_min.pack(side="left", padx=5)
        self.custom_max = ctk.CTkEntry(self.custom_frame, placeholder_text="Max", width=60)
        self.custom_max.pack(side="left", padx=5)

        # Guessing Area
        self.entry = ctk.CTkEntry(self, placeholder_text="Enter your guess", width=200)
        self.entry.pack(pady=20)

        self.submit_btn = ctk.CTkButton(self, text="Submit Guess", command=self.check_guess)
        self.submit_btn.pack(pady=10)

        self.stats_label = ctk.CTkLabel(self, text=f"Attempts: 0  |  Best: {self.high_score}")
        self.stats_label.pack(pady=10)

        self.restart_btn = ctk.CTkButton(self, text="Reset Game", command=self.start_new_round, fg_color="gray")
        self.restart_btn.pack(pady=10)

        self.start_new_round()

    def change_level(self, value):
        if value == "Easy":
            self.min_val, self.max_val = 1, 10
            self.custom_frame.pack_forget()
        elif value == "Normal":
            self.min_val, self.max_val = 1, 100
            self.custom_frame.pack_forget()
        elif value == "Extreme":
            self.min_val, self.max_val = 1, 1000
            self.custom_frame.pack_forget()
        elif value == "Custom":
            self.custom_frame.pack(pady=10)
            return # Don't reset until they pick numbers
        
        self.start_new_round()

    def start_new_round(self):
        if self.level_var.get() == "Custom":
            try:
                self.min_val = int(self.custom_min.get())
                self.max_val = int(self.custom_max.get())
            except ValueError:
                self.min_val, self.max_val = 1, 100 # Fallback
        
        self.target_number = random.randint(self.min_val, self.max_val)
        self.attempts = 0
        self.label.configure(text=f"Guess between {self.min_val} and {self.max_val}", text_color="white")
        self.submit_btn.configure(state="normal")
        self.update_stats()

    def check_guess(self):
        try:
            guess = int(self.entry.get())
            self.attempts += 1
            self.entry.delete(0, 'end')

            if guess == self.target_number:
                self.label.configure(text=f"WINNER! It was {guess}.", text_color="#2ECC71")
                self.submit_btn.configure(state="disabled")
                self.save_high_score()
            elif guess > self.target_number:
                self.label.configure(text="Lower! ↓", text_color="#E74C3C")
            else:
                self.label.configure(text="Higher! ↑", text_color="#F39C12")
            
            self.update_stats()
        except ValueError:
            self.label.configure(text="Invalid Input!", text_color="yellow")

    def update_stats(self):
        self.stats_label.configure(text=f"Attempts: {self.attempts}  |  Best: {self.high_score}")

    def load_high_score(self):
        if os.path.exists("highscore.txt"):
            with open("highscore.txt", "r") as f:
                return int(f.read())
        return 999

    def save_high_score(self):
        if self.attempts < self.high_score:
            self.high_score = self.attempts
            with open("highscore.txt", "w") as f:
                f.write(str(self.high_score))

if __name__ == "__main__":
    app = GuessingGame()
    app.mainloop()