import sys, subprocess, threading, time, os, math
from datetime import datetime
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QLabel, QPushButton, QSlider, QGridLayout, 
                             QTabWidget, QTextEdit, QFileDialog, QMessageBox, QFrame)
from PyQt6.QtCore import Qt, pyqtSignal, QObject

# --- CONFIG ---
ADB_PATH = r"C:\adb\platform-tools\adb.exe"
SCRCPY_PATH = "scrcpy" 
SCREENSHOT_DIR = os.path.join(os.path.expanduser("~"), "Desktop", "S7_Screenshots")
if not os.path.exists(SCREENSHOT_DIR): os.makedirs(SCREENSHOT_DIR)

# --- THEME ---
THEME = """
QMainWindow { background-color: #08080E; }
QTabWidget::pane { border: 1px solid #1A1A24; background: #08080E; border-radius: 10px; }
QTabBar::tab { background: #12121A; color: #565F89; padding: 10px 15px; border-radius: 5px; margin: 2px; }
QTabBar::tab:selected { background: #1A1A24; color: #BB9AF7; border-bottom: 2px solid #7AA2F7; }
QFrame#MetricCard { background-color: #16161E; border: 1px solid #24283B; border-radius: 12px; }
QLabel#Val { color: #7AA2F7; font-size: 26px; font-weight: bold; font-family: 'Consolas'; }
QLabel#Title { color: #565F89; font-size: 10px; text-transform: uppercase; }
QPushButton { background-color: #1A1A24; color: #C0CAF5; border: 1px solid #292E42; border-radius: 6px; padding: 10px; font-weight: bold; }
QPushButton:hover { border: 1px solid #BB9AF7; background-color: #24283B; }
QTextEdit#Console { background-color: #050508; color: #9ECE6A; font-family: 'Consolas'; font-size: 10px; border: 1px solid #1A1A24; }
"""

class ADBWorker(QObject):
    data_received = pyqtSignal(dict)
    log_signal = pyqtSignal(str)

    def run_cmd(self, cmd, silent=False):
        try:
            full_cmd = f"{ADB_PATH} shell \"su -c '{cmd}'\""
            res = subprocess.check_output(full_cmd, shell=True, stderr=subprocess.STDOUT, timeout=2).decode().strip()
            if not silent: self.log_signal.emit(f"EXE: {cmd[:25]}...")
            return res
        except: return "0"

    def poll(self):
        while True:
            data = {}
            temps = []
            for i in range(5): # Scans therm_zone 0-4
                t = self.run_cmd(f"cat /sys/class/thermal/thermal_zone{i}/temp", True)
                if t.isdigit(): temps.append(int(t)/1000)
            data['temp'] = max(temps) if temps else 0.0
            data['freqs'] = self.run_cmd("cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq", True).split()
            data['gpu'] = self.run_cmd("cat /sys/devices/14ac0000.mali/clock", True)
            data['bright'] = self.run_cmd("settings get system screen_brightness", True)
            data['top'] = self.run_cmd("top -n 1 -b | head -n 8", True)
            self.data_received.emit(data)
            time.sleep(1)

class MetricCard(QFrame):
    def __init__(self, title):
        super().__init__()
        self.setObjectName("MetricCard")
        l = QVBoxLayout(self)
        self.t = QLabel(title); self.t.setObjectName("Title")
        self.v = QLabel("--"); self.v.setObjectName("Val")
        l.addWidget(self.t); l.addWidget(self.v)

class MongooseFinal(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("S7 GOD MODE")
        self.setFixedSize(580, 700)
        self.setStyleSheet(THEME)
        self.rainbow_on = False

        main_widget = QWidget(); self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)

        self.tabs = QTabWidget()
        self.tab_mon = QWidget(); mon_lay = QVBoxLayout(self.tab_mon)
        grid = QGridLayout()
        self.c_temp = MetricCard("SOC TEMP"); grid.addWidget(self.c_temp, 0, 0)
        self.c_gpu = MetricCard("MALI GPU"); grid.addWidget(self.c_gpu, 0, 1)
        self.c_cpu_big = MetricCard("MONGOOSE"); grid.addWidget(self.c_cpu_big, 1, 0)
        self.c_cpu_small = MetricCard("CORTEX"); grid.addWidget(self.c_cpu_small, 1, 1)
        mon_lay.addLayout(grid)
        self.proc_view = QTextEdit(); self.proc_view.setObjectName("Console"); self.proc_view.setReadOnly(True)
        mon_lay.addWidget(self.proc_view)

        self.tab_ctrl = QWidget(); ctrl_lay = QVBoxLayout(self.tab_ctrl)
        btn_grid = QGridLayout()
        self.add_b(btn_grid, "BOOST 2.6G", "echo performance > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor", 0, 0, "#7AA2F7")
        # NUCLEAR COOL: Min clocks + kill apps + dim screen
        self.add_b(btn_grid, "FORCE COOL", "echo 442000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq; echo 806000 > /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq; am kill-all; settings put system screen_brightness 10", 0, 1, "#00F2FF")
        self.add_b(btn_grid, "NUKE RAM", "am kill-all; sync; echo 3 > /proc/sys/vm/drop_caches", 1, 0, "#F7768E")
        self.add_b(btn_grid, "HAPTIC TEST", "echo 150 > /sys/class/timed_output/vibrator/enable", 1, 1, "#BB9AF7")
        ctrl_lay.addLayout(btn_grid)

        ctrl_lay.addWidget(QLabel("BRIGHTNESS"))
        self.s_bright = QSlider(Qt.Orientation.Horizontal); self.s_bright.setRange(0, 255)
        self.s_bright.sliderReleased.connect(self.update_br); ctrl_lay.addWidget(self.s_bright)

        tool_grid = QGridLayout()
        self.add_tool(tool_grid, "SCRCPY", self.run_scrcpy, 0, 0)
        self.add_tool(tool_grid, "APK", self.run_apk, 0, 1)
        self.add_tool(tool_grid, "SHOT", self.run_shot, 1, 0)
        self.add_tool(tool_grid, "RAINBOW", self.toggle_rainbow, 1, 1)
        ctrl_lay.addLayout(tool_grid)

        self.tabs.addTab(self.tab_mon, "Monitor"); self.tabs.addTab(self.tab_ctrl, "God Mode")
        layout.addWidget(self.tabs)
        self.log = QTextEdit(); self.log.setObjectName("Console"); self.log.setFixedHeight(80)
        layout.addWidget(self.log)

        self.worker = ADBWorker()
        self.thread = threading.Thread(target=self.worker.poll, daemon=True)
        self.worker.data_received.connect(self.update_ui)
        self.worker.log_signal.connect(lambda m: self.log.append(f"[{datetime.now().strftime('%H:%M:%S')}] {m}"))
        self.thread.start()

    def add_b(self, lay, txt, cmd, r, c, clr):
        b = QPushButton(txt); b.setStyleSheet(f"border: 1px solid {clr}; color: {clr};"); b.clicked.connect(lambda: self.worker.run_cmd(cmd)); lay.addWidget(b, r, c)
    def add_tool(self, lay, txt, func, r, c):
        b = QPushButton(txt); b.clicked.connect(func); lay.addWidget(b, r, c)
    def update_br(self): self.worker.run_cmd(f"settings put system screen_brightness {self.s_bright.value()}")
    def run_scrcpy(self): subprocess.Popen([SCRCPY_PATH, "--always-on-top", "-m", "1024"])
    def run_apk(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select APK", "", "APK Files (*.apk)")
        if path: threading.Thread(target=lambda: subprocess.run([ADB_PATH, "install", "-r", path])).start()
    def run_shot(self):
        p = os.path.join(SCREENSHOT_DIR, f"S7_{int(time.time())}.png")
        self.worker.run_cmd("screencap -p /sdcard/s.png", True); subprocess.run([ADB_PATH, "pull", "/sdcard/s.png", p])
    def toggle_rainbow(self):
        self.rainbow_on = not self.rainbow_on
        if self.rainbow_on: threading.Thread(target=self._rainbow_loop, daemon=True).start()
    def _rainbow_loop(self):
        s = 0
        while self.rainbow_on:
            r = int((math.sin(s) + 1) * 127.5); g = int((math.sin(s+2) + 1) * 127.5); b = int((math.sin(s+4) + 1) * 127.5)
            self.worker.run_cmd(f"echo {r} > /sys/class/sec/led/led_r; echo {g} > /sys/class/sec/led/led_g; echo {b} > /sys/class/sec/led/led_b", True)
            s += 0.3; time.sleep(0.1)
    def update_ui(self, data):
        t = data.get('temp', 0); self.c_temp.v.setText(f"{t:.1f}°C")
        self.c_gpu.v.setText(f"{data.get('gpu', '0')} MHz")
        f = data.get('freqs', []); 
        if len(f) >= 8: self.c_cpu_big.v.setText(f"{int(f[4])//1000} MHz"); self.c_cpu_small.v.setText(f"{int(f[0])//1000} MHz")
        self.proc_view.setText(data.get('top', ''))
        if not self.s_bright.isSliderDown(): self.s_bright.setValue(int(data.get('bright', 0)))
        if not self.rainbow_on: # Thermal Logic: Blue (cold) to Red (hot)
            val = max(0, min(255, int((t - 30) * 6)))
            self.worker.run_cmd(f"echo {val} > /sys/class/sec/led/led_r; echo {255-val} > /sys/class/sec/led/led_b; echo 0 > /sys/class/sec/led/led_g", True)

if __name__ == "__main__":
    app = QApplication(sys.argv); w = MongooseFinal(); w.show(); sys.exit(app.exec())