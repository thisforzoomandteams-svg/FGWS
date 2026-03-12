import pygame
import sys
import random
import os
import time
import math

# --- Central Configuration ---
class Config:
    WIDTH, HEIGHT = 1000, 700
    FPS = 120
    SAVE_FILE = "overkill_save.txt"
    # Settings
    SHAKE_ENABLED = True
    LIGHTING_ENABLED = True
    PARTICLE_DENSITY = 1.0
    STAR_COUNT = 100

# --- Advanced Math Utilities ---
def lerp(start, end, t):
    return start + (end - start) * t

# --- Initialization ---
pygame.init()
screen = pygame.display.set_mode((Config.WIDTH, Config.HEIGHT), pygame.RESIZABLE)
pygame.display.set_caption("SURVIVAL: OVERKILL EDITION")
clock = pygame.time.Clock()

# Fonts
try:
    font_main = pygame.font.SysFont("Impact", 28)
    font_ui = pygame.font.SysFont("Verdana", 18, bold=True)
    font_huge = pygame.font.SysFont("Impact", 80)
    font_small = pygame.font.SysFont("Verdana", 14, bold=True)
except:
    font_main = pygame.font.get_default_font()

# --- Background Stars ---
stars = []
for _ in range(Config.STAR_COUNT):
    stars.append([random.randint(0, Config.WIDTH), random.randint(0, Config.HEIGHT), random.uniform(1, 3)])

# --- Save/Load Logic ---
def get_default_data():
    return {"money": 0, "multi": 1, "auto": 0, "pierce": 0, "drone": 0, "special_charge": 0}

def load_game():
    if os.path.exists(Config.SAVE_FILE):
        try:
            with open(Config.SAVE_FILE, "r") as f:
                d = [float(x) for x in f.read().split(',')]
                return {
                    "money": d[0], "multi": d[1], "auto": d[2], 
                    "pierce": d[3], "drone": d[4]
                }
        except: return get_default_data()
    return get_default_data()

data = load_game()
data["special_charge"] = 0 

# --- Admin/Cheat States ---
admin_god_mode = False
admin_inf_special = False
admin_one_shot = False

# --- Game Entities & Systems ---
state = "PLAYING" # PLAYING, SHOP, SETTINGS, GAMEOVER, ADMIN
score = 0
dt = 0
time_scale = 1.0 
last_time = time.time()
shake_mag = 0
flash_alpha = 0
auto_fire_timer = 0

bullets = []   # [pos, vel, life, pierce, color]
enemies = []   # [pos, hp, type, speed, size]
particles = [] # [pos, vel, life, color, size]
popups = []    # [pos, text, life, color]
drones = []    # [pos, angle]

# --- UI Assets ---
def draw_button(surf, rect, text, color, hover=False):
    col = tuple(min(255, c + 30) for c in color) if hover else color
    pygame.draw.rect(surf, col, rect, border_radius=12)
    pygame.draw.rect(surf, (255, 255, 255), rect, 2, border_radius=12)
    # Glass effect
    glass = pygame.Surface((rect.width, rect.height//2), pygame.SRCALPHA)
    glass.fill((255, 255, 255, 30))
    surf.blit(glass, rect.topleft)
    
    txt_surf = font_ui.render(text, True, (255, 255, 255))
    surf.blit(txt_surf, (rect.centerx - txt_surf.get_width()//2, rect.centery - txt_surf.get_height()//2))

def spawn_bullet(pos, target_pos, is_drone=False, color=(255, 255, 0)):
    # Performance Cap
    if len(bullets) > 1200:
        return
        
    direction = (target_pos - pos)
    if direction.length() == 0: direction = pygame.Vector2(0, -1)
    else: direction = direction.normalize()
    
    speed = 900 if not is_drone else 600
    p_val = 999 if admin_one_shot else (1 + data["pierce"])
    bullets.append([pygame.Vector2(pos), direction * speed, 2.5, p_val, color])

def save_game():
    with open(Config.SAVE_FILE, "w") as f:
        f.write(f"{data['money']},{data['multi']},{data['auto']},{data['pierce']},{data['drone']}")

def create_explosion(pos, color, count=15, size=4):
    actual_count = count if len(particles) < 600 else 3
    for _ in range(actual_count):
        particles.append([
            pygame.Vector2(pos), 
            pygame.Vector2(random.uniform(-7, 7), random.uniform(-7, 7)), 
            random.randint(10, 40), 
            color,
            random.randint(1, size)
        ])

# --- Main Engine ---
while True:
    now = time.time()
    raw_dt = now - last_time
    last_time = now
    
    # State-based delta time
    if state in ["PLAYING"]:
        dt = raw_dt * time_scale
    else:
        dt = 0
    
    W, H = screen.get_size()
    mouse_pos = pygame.Vector2(pygame.mouse.get_pos())
    
    # UI Rects
    admin_btn_rect = pygame.Rect(W - 120, 20, 100, 40)
    
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            save_game(); pygame.quit(); sys.exit()
        
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_F12: 
                data["money"] += 10000; save_game()
                popups.append([pygame.Vector2(W//2, H//2), "OVERKILL: +$10k", 80, (0, 255, 255)])
            
            if event.key == pygame.K_s:
                state = "SHOP" if state == "PLAYING" else "PLAYING"
            
            if event.key == pygame.K_ESCAPE:
                if state == "ADMIN": state = "PLAYING"
                else: state = "SETTINGS" if state != "SETTINGS" else "PLAYING"
            
            # NUKE mechanic
            if event.key == pygame.K_n and data["money"] >= 2000 and state == "PLAYING":
                data["money"] -= 2000; enemies.clear(); flash_alpha = 255; shake_mag = 60
                create_explosion((W//2, H//2), (255, 255, 255), 100, 20)
            
            if event.key == pygame.K_q and (data["special_charge"] >= 100 or admin_inf_special):
                data["special_charge"] = 0
                time_scale = 0.2 

        if event.type == pygame.MOUSEBUTTONDOWN:
            if admin_btn_rect.collidepoint(event.pos):
                state = "ADMIN" if state != "ADMIN" else "PLAYING"
            
            if state == "PLAYING":
                player_pos = pygame.Vector2(W//2, H-50)
                # Hyper Multi-Shot
                spawn_bullet(player_pos, mouse_pos)
                if data["multi"] >= 2:
                    # Visual spread for high levels
                    spread_range = min(int(data["multi"]), 15)
                    for i in range(1, spread_range + 1):
                        offset_x = i * 35
                        spawn_bullet(player_pos, mouse_pos + pygame.Vector2(offset_x, 0), color=(200, 255, 100))
                        spawn_bullet(player_pos, mouse_pos + pygame.Vector2(-offset_x, 0), color=(200, 255, 100))
            
            elif state == "SHOP":
                costs = {"multi": 500, "auto": 300, "pierce": 400, "drone": 1000}
                for i, key in enumerate(costs.keys()):
                    btn_rect = pygame.Rect(W//2 - 170, 280 + i*70, 340, 55)
                    if btn_rect.collidepoint(event.pos) and data["money"] >= costs[key]:
                        data["money"] -= costs[key]
                        data[key] += 1
                        save_game()
                        create_explosion(event.pos, (255, 215, 0), 10)
            
            elif state == "ADMIN":
                toggles = [pygame.Rect(W//2 - 90, 300 + i*55, 180, 45) for i in range(3)]
                if toggles[0].collidepoint(event.pos): admin_god_mode = not admin_god_mode
                if toggles[1].collidepoint(event.pos): admin_inf_special = not admin_inf_special
                if toggles[2].collidepoint(event.pos): admin_one_shot = not admin_one_shot

    # --- Gameplay Logic ---
    if state == "PLAYING":
        if time_scale < 1.0: time_scale += 0.4 * raw_dt
        if time_scale > 1.0: time_scale = 1.0

        # Parallax Stars
        for s in stars:
            s[1] += s[2] * dt * 20
            if s[1] > H: s[1] = 0; s[0] = random.randint(0, W)

        # Auto Fire (Level 1000 Compatible)
        if data["auto"] > 0:
            auto_fire_timer += dt
            rate = max(0.005, 0.4 - (data["auto"] * 0.002))
            if auto_fire_timer > rate:
                spawn_bullet(pygame.Vector2(W//2, H-50), mouse_pos, color=(255, 150, 0))
                auto_fire_timer = 0

        # Drones
        if data["drone"] > 0:
            if not drones: drones.append([pygame.Vector2(W//2, H-100), 0])
            drones[0][1] += 5 * dt 
            drones[0][0].x = W//2 + math.cos(drones[0][1]) * 150
            drones[0][0].y = (H-100) + math.sin(drones[0][1]) * 100
            
            if random.random() < min(0.95, 0.05 * data["drone"]):
                if enemies: spawn_bullet(drones[0][0], enemies[0][0], True, color=(0, 255, 255))

        # Enemy Spawning
        spawn_rate = 0.04 + (score * 0.0002)
        if random.random() < min(0.4, spawn_rate):
            size = random.randint(30, 60)
            enemies.append([pygame.Vector2(random.randint(0, W), -60), size//10, "grunt", 150 + (score//4), size])

        # Entity Updates
        for e in enemies[:]:
            e[0].y += e[3] * dt
            e_rect = pygame.Rect(e[0].x - e[4]//2, e[0].y - e[4]//2, e[4], e[4])
            if e_rect.y > H + 100: enemies.remove(e)
            
            for b in bullets[:]:
                if e_rect.collidepoint(b[0]):
                    e[1] -= 1; b[3] -= 1
                    create_explosion(b[0], e[4] > 45 and (255, 255, 0) or (255, 50, 50))
                    if b[3] <= 0: bullets.remove(b)
                    if e[1] <= 0:
                        enemies.remove(e); data["money"] += 25; score += 1
                        data["special_charge"] = min(100, data["special_charge"] + 3)
                        shake_mag = min(40, 15 if Config.SHAKE_ENABLED else 0)
                        break
            
            if e_rect.colliderect(pygame.Rect(W//2-25, H-70, 50, 50)) and not admin_god_mode:
                state = "GAMEOVER"

        for b in bullets[:]:
            b[0] += b[1] * dt
            b[2] -= dt
            if b[2] <= 0 or not (0 < b[0].x < W and 0 < b[0].y < H):
                if b in bullets: bullets.remove(b)

    # --- Final Render Pass ---
    screen.fill((2, 2, 8))
    
    # Shake logic
    offset = pygame.Vector2(0, 0)
    if shake_mag > 0:
        offset = pygame.Vector2(random.uniform(-shake_mag, shake_mag), random.uniform(-shake_mag, shake_mag))
        shake_mag = lerp(shake_mag, 0, 0.15)

    # Background
    for s in stars:
        pygame.draw.circle(screen, (100, 100, 150), (s[0], s[1]), 1)

    # Projectiles with Glow
    for b in bullets:
        # Simple Glow Trace
        pygame.draw.line(screen, b[4], b[0] + offset, b[0] - b[1]*0.02 + offset, 3)
        pygame.draw.circle(screen, (255, 255, 255), b[0] + offset, 4)
    
    # Enemies
    for e in enemies:
        e_rect = (e[0].x - e[4]//2 + offset.x, e[0].y - e[4]//2 + offset.y, e[4], e[4])
        pygame.draw.rect(screen, (200, 30, 30), e_rect, border_radius=8)
        pygame.draw.rect(screen, (255, 100, 100), e_rect, 2, border_radius=8)

    # Drones
    for d in drones: 
        pygame.draw.circle(screen, (0, 200, 255), d[0] + offset, 14)
        pygame.draw.circle(screen, (255, 255, 255), d[0] + offset, 6)
        # Orbit Trail
        pygame.draw.circle(screen, (0, 255, 255), d[0] + offset, 20, 1)
    
    # Player Ship
    p_rect = (W//2-25+offset.x, H-75+offset.y, 50, 60)
    p_col = (0, 255, 200) if not admin_god_mode else (255, 255, 255)
    pygame.draw.polygon(screen, p_col, [
        (p_rect[0] + 25, p_rect[1]), 
        (p_rect[0], p_rect[1] + 60), 
        (p_rect[0] + 50, p_rect[1] + 60)
    ])
    pygame.draw.rect(screen, (0, 150, 255), (p_rect[0]+15, p_rect[1]+40, 20, 10)) # Cockpit

    # Global Lighting Effect
    if Config.LIGHTING_ENABLED:
        overlay = pygame.Surface((W, H), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 100))
        # Player light
        pygame.draw.circle(overlay, (0, 50, 100, 0), (W//2, H-45), 200)
        # Mouse cursor light
        pygame.draw.circle(overlay, (50, 50, 0, 0), (int(mouse_pos.x), int(mouse_pos.y)), 150)
        screen.blit(overlay, (0,0))

    # Particles
    for p in particles[:]:
        p[0] += p[1] * 60 * raw_dt
        p[2] -= 1
        if p[2] <= 0: particles.remove(p)
        else:
            alpha = min(255, p[2] * 10)
            p_surf = pygame.Surface((p[4]*2, p[4]*2), pygame.SRCALPHA)
            pygame.draw.circle(p_surf, (*p[3], alpha), (p[4], p[4]), p[4])
            screen.blit(p_surf, p[0] + offset)

    # Popups
    for p in popups[:]:
        p[0].y -= 50 * raw_dt
        p[2] -= 1
        if p[2] <= 0: popups.remove(p)
        else: screen.blit(font_ui.render(p[1], True, p[3]), p[0])

    # HUD
    draw_button(screen, admin_btn_rect, "SYSTEM", (80, 0, 0), admin_btn_rect.collidepoint(mouse_pos))
    
    hud_panel = pygame.Surface((280, 120), pygame.SRCALPHA)
    hud_panel.fill((0, 0, 0, 150))
    screen.blit(hud_panel, (10, 10))
    screen.blit(font_main.render(f"CREDITS: ${int(data['money'])}", True, (255, 215, 0)), (25, 20))
    screen.blit(font_ui.render(f"ELIMINATIONS: {score}", True, (255, 255, 255)), (25, 60))
    screen.blit(font_small.render("[S] SHOP | [N] NUKE $2K", True, (180, 180, 180)), (25, 90))

    # Special Charge UI
    spec_rect = pygame.Rect(W-220, H-40, 200, 25)
    pygame.draw.rect(screen, (30, 30, 30), spec_rect, border_radius=5)
    charge = data["special_charge"] if not admin_inf_special else 100
    bar_w = int((charge / 100) * 200)
    if charge >= 100:
        bar_col = (random.randint(0,255), 200, 255) # Flashy
    else:
        bar_col = (0, 120, 200)
    pygame.draw.rect(screen, bar_col, (spec_rect.x, spec_rect.y, bar_w, 25), border_radius=5)
    screen.blit(font_ui.render("HYPER-DRIVE [Q]", True, (255, 255, 255)), (W-215, H-75))

    # Overlays
    if state == "SHOP":
        s = pygame.Surface((W, H), pygame.SRCALPHA); s.fill((5, 5, 15, 230)); screen.blit(s, (0,0))
        screen.blit(font_huge.render("REQUISITION", True, (0, 200, 255)), (W//2-220, 80))
        
        upgrades = [("Multi-Shot", "multi", 500), ("Auto-Fire", "auto", 300), ("Hyper-Pierce", "pierce", 400), ("Drone Wing", "drone", 1000)]
        for i, (name, key, cost) in enumerate(upgrades):
            rect = pygame.Rect(W//2 - 170, 280 + i*70, 340, 55)
            draw_button(screen, rect, f"{name} Lv.{int(data[key])} (${cost})", (30, 30, 60), rect.collidepoint(mouse_pos))
        
        screen.blit(font_ui.render("PRESS [S] TO ENGAGE", True, (200, 200, 200)), (W//2-110, H-60))

    elif state == "ADMIN":
        win_rect = pygame.Rect(W//2 - 200, H//2 - 200, 400, 400)
        pygame.draw.rect(screen, (15, 15, 20), win_rect, border_radius=20)
        pygame.draw.rect(screen, (0, 255, 150), win_rect, 2, border_radius=20)
        
        screen.blit(font_main.render("DEBUG INTERFACE", True, (0, 255, 150)), (win_rect.centerx - 90, win_rect.y + 30))
        
        opts = [("GOD MODE", admin_god_mode), ("INF CHARGE", admin_inf_special), ("INSTA-KILL", admin_one_shot)]
        for i, (label, val) in enumerate(opts):
            rect = pygame.Rect(win_rect.x + 110, win_rect.y + 120 + i*65, 180, 45)
            col = (0, 100, 50) if val else (60, 60, 60)
            draw_button(screen, rect, f"{label}: {'[ON]' if val else '[OFF]'}", col, rect.collidepoint(mouse_pos))
        
        screen.blit(font_small.render("PRESS ESC TO RETURN", True, (100, 100, 100)), (win_rect.centerx - 75, win_rect.bottom - 40))

    if flash_alpha > 0:
        f = pygame.Surface((W, H)); f.fill((255,255,255)); f.set_alpha(flash_alpha)
        screen.blit(f, (0,0)); flash_alpha = lerp(flash_alpha, 0, 0.12)

    pygame.display.flip()
    clock.tick(Config.FPS)