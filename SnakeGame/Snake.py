import pygame
import sys
import random
import os

# --- Initialization ---
pygame.init()
WIDTH, HEIGHT = 800, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT), pygame.RESIZABLE)
clock = pygame.time.Clock()
FPS = 80 
font = pygame.font.SysFont("Verdana", 20, bold=True)
popup_font = pygame.font.SysFont("Verdana", 16, bold=True)

# --- Save/Load System ---
def save_data():
    with open("savedata.txt", "w") as f:
        f.write(f"{money},{multi_shot_lvl},{auto_fire_lvl},{pierce_lvl}")

def load_data():
    if os.path.exists("savedata.txt"):
        try:
            with open("savedata.txt", "r") as f:
                return [int(x) for x in f.read().split(',')]
        except: return [0, 1, 0, 0]
    return [0, 1, 0, 0]

# --- Game Variables ---
money, multi_shot_lvl, auto_fire_lvl, pierce_lvl = load_data()
score = 0
state = "PLAYING"
shake_intensity = 0
auto_timer = 0

player_rect = pygame.Rect(WIDTH//2, HEIGHT - 70, 50, 50)
bullets = []   # [x, y, dx, health]
enemies = []   # [Rect, health]
bosses = []    # [Rect, hp, max_hp]
particles = [] # [x, y, dx, dy, life, color]
popups = []    # [x, y, text, life, color]

def create_popup(x, y, text, color=(0, 255, 100)):
    # x, y, text, life (frames), color
    popups.append([x, y, text, 60, color])

def spawn_bullet(x, y, dx=0):
    bullets.append([x, y, dx, 1 + pierce_lvl])

def create_particles(x, y, color=(200, 200, 200)):
    for _ in range(10):
        particles.append([x, y, random.uniform(-4, 4), random.uniform(-4, 4), 20, color])

# --- Main Loop ---
while True:
    W, H = screen.get_size()
    mouse_pos = pygame.mouse.get_pos()
    
    # 1. Screen Shake Offset
    render_offset = [0, 0]
    if shake_intensity > 0:
        render_offset = [random.randint(-shake_intensity, shake_intensity), 
                         random.randint(-shake_intensity, shake_intensity)]
        shake_intensity -= 1

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            save_data()
            pygame.quit()
            sys.exit()
        
        if event.type == pygame.MOUSEBUTTONDOWN:
            if state == "PLAYING":
                spawn_bullet(player_rect.centerx, player_rect.top)
                if multi_shot_lvl >= 2: spawn_bullet(player_rect.centerx, player_rect.top, -3)
                if multi_shot_lvl >= 3: spawn_bullet(player_rect.centerx, player_rect.top, 3)
            elif state == "SHOP":
                if 50 < mouse_pos[0] < 300:
                    if 150 < mouse_pos[1] < 200 and money >= 50:
                        money -= 50; multi_shot_lvl += 1; save_data(); create_popup(mouse_pos[0], mouse_pos[1], "UPGRADED!", (255, 215, 0))
                    elif 210 < mouse_pos[1] < 260 and money >= 100:
                        money -= 100; auto_fire_lvl += 1; save_data(); create_popup(mouse_pos[0], mouse_pos[1], "SPEED UP!", (255, 215, 0))
                    elif 270 < mouse_pos[1] < 320 and money >= 150:
                        money -= 150; pierce_lvl += 1; save_data(); create_popup(mouse_pos[0], mouse_pos[1], "PIERCE!", (255, 215, 0))

        if event.type == pygame.KEYDOWN and event.key == pygame.K_s:
            state = "SHOP" if state == "PLAYING" else "PLAYING"

    screen.fill((10, 10, 20)) 

    if state == "PLAYING":
        player_rect.centerx = mouse_pos[0]
        player_rect.bottom = H - 10

        # Auto-fire
        if auto_fire_lvl > 0:
            auto_timer += 1
            if auto_timer > max(4, 50 - (auto_fire_lvl * 4)):
                spawn_bullet(player_rect.centerx, player_rect.top)
                auto_timer = 0

        # Boss Logic
        if score > 0 and score % 50 == 0 and not bosses:
            bosses.append([pygame.Rect(W//2-50, -100, 100, 100), 30 + (score//2), 30 + (score//2)])
            shake_intensity = 15
            create_popup(W//2, 100, "BOSS INCOMING!", (255, 0, 0))

        # --- Updates ---
        for b in bullets[:]:
            b[0] += b[2]; b[1] -= 12
            if b[1] < 0: bullets.remove(b)
            else: pygame.draw.circle(screen, (255, 255, 0), (int(b[0]+render_offset[0]), int(b[1]+render_offset[1])), 5)

        for b_data in bosses[:]:
            rect, hp, m_hp = b_data
            rect.y += 1
            pygame.draw.rect(screen, (150, 0, 255), rect.move(render_offset[0], render_offset[1]), border_radius=12)
            pygame.draw.rect(screen, (255,0,0), (rect.x, rect.y-20, rect.width, 10))
            pygame.draw.rect(screen, (0,255,0), (rect.x, rect.y-20, rect.width * (hp/m_hp), 10))
            for b in bullets[:]:
                if rect.collidepoint(b[0], b[1]):
                    b_data[1] -= 1; bullets.remove(b); shake_intensity = 3
                    if b_data[1] <= 0:
                        bosses.remove(b_data); money += 500; score += 1; save_data()
                        create_popup(rect.centerx, rect.centery, "+$500", (255, 215, 0))

        if random.random() < 0.04:
            enemies.append([pygame.Rect(random.randint(0, W-40), -50, 40, 40), 1])

        for e_data in enemies[:]:
            e = e_data[0]
            e.y += 4 + (score // 30)
            for b in bullets[:]:
                if e.collidepoint(b[0], b[1]):
                    create_particles(e.centerx, e.centery, (255, 50, 50))
                    create_popup(e.centerx, e.centery, "+$10")
                    b[3] -= 1
                    if b[3] <= 0: bullets.remove(b)
                    enemies.remove(e_data); money += 10; score += 1; break
            if player_rect.colliderect(e): state = "GAMEOVER"

        # --- Popup Animation ---
        for p in popups[:]:
            p[1] -= 1 # Float up
            p[3] -= 1 # Life decrease
            if p[3] <= 0: popups.remove(p)
            else:
                alpha_val = min(255, p[3] * 5)
                # Note: True transparency in text requires a bit more code, 
                # but we'll keep it simple for now
                txt = popup_font.render(p[2], True, p[4])
                screen.blit(txt, (p[0] + render_offset[0], p[1] + render_offset[1]))

        # --- Particles ---
        for p in particles[:]:
            p[0]+=p[2]; p[1]+=p[3]; p[4]-=1
            if p[4]<=0: particles.remove(p)
            else: pygame.draw.circle(screen, p[5], (int(p[0]+render_offset[0]), int(p[1]+render_offset[1])), 2)

        for e_data in enemies:
            pygame.draw.rect(screen, (255, 50, 50), e_data[0].move(render_offset[0], render_offset[1]), border_radius=6)
        
        pygame.draw.rect(screen, (0, 255, 150), player_rect.move(render_offset[0], render_offset[1]), border_radius=10)

        screen.blit(font.render(f"Cash: ${money}  Score: {score}", True, (255, 215, 0)), (20, 20))

    elif state == "SHOP":
        screen.blit(font.render("=== UPGRADE SHOP ===", True, (255, 215, 0)), (W//2-120, 50))
        items = [("Multi-Shot ($50)", 150), ("Auto-Fire ($100)", 210), ("Piercing ($150)", 270)]
        for text, y in items:
            pygame.draw.rect(screen, (40, 40, 60), (50, y, 250, 50), border_radius=8)
            screen.blit(font.render(text, True, (255, 255, 255)), (60, y+10))
        screen.blit(font.render(f"Wallet: ${money}", True, (255, 255, 255)), (W-200, 50))
        screen.blit(font.render("Press [S] to Return", True, (100, 100, 100)), (W//2-100, H-50))

    pygame.display.flip()
    clock.tick(FPS)