-- snake.lua
-- Snake mini-game (logic + optional ASCII rendering) for a custom engine.
--
-- Required: `update(dt)` called every frame.
--
-- Input (polling): expects a global `Input` table with one of these functions:
--   Input.is_key_down(key) / Input.key_down(key)
--   Input.is_key_pressed(key) / Input.key_pressed(key)
-- Keys can be GLFW-style ints (262-265) or strings ("Up","Down","Left","Right").
--
-- Input (event-driven alternative): your engine can call `on_key("Up")` etc.
--
-- Rendering:
--   If your engine provides global `SnakeRender` with:
--     SnakeRender.clear()
--     SnakeRender.cell(x, y, kind)  -- kind: "head"|"body"|"food"
--     SnakeRender.hud(text)         -- optional
--   it will be used.
--   Otherwise (CONFIG.print_board=true) it logs an ASCII board.

local CONFIG = {
  width = 20,
  height = 14,
  step_time = 0.14,      -- seconds per move
  grow_per_food = 1,
  start_len = 4,
  score_per_food = 10,
  print_board = false,   -- set true to see ASCII in console
}

-- "Assets" (self-contained). Optional file-path assets listed for your engine to hook up later.
local ASSETS = {
  glyph = { head = "@", body = "o", food = "*", empty = " ", wall = "#" },
  -- sprites = { head="Assets/Snake/head.png", body="Assets/Snake/body.png", food="Assets/Snake/food.png" },
  -- sounds = { eat="Assets/Snake/eat.wav", die="Assets/Snake/die.wav" },
}

local state = {
  initialized = false,
  acc = 0,
  alive = true,
  game_over_t = 0,
  score = 0,
  dir = { x = 1, y = 0 },
  next_dir = { x = 1, y = 0 },
  snake = {},
  food = { x = 1, y = 1 },
  last_print_t = 0,
}

local function log(msg)
  msg = "[Snake] " .. tostring(msg)
  if this and type(this.log) == "function" then pcall(this.log, msg) end
  if type(_G.AetherLog) == "function" then pcall(_G.AetherLog, msg) end
end

local function seed_rng()
  local addr = tostring({}):match("0x(%x+)")
  local seed = tonumber(addr or "1", 16) or 1
  math.randomseed(seed)
  for _ = 1, 4 do math.random() end
end

local function vec(x, y) return { x = x, y = y } end

local function in_bounds(x, y)
  return x >= 1 and x <= CONFIG.width and y >= 1 and y <= CONFIG.height
end

local function occupies(x, y, max_i)
  max_i = max_i or #state.snake
  for i = 1, max_i do
    local s = state.snake[i]
    if s.x == x and s.y == y then return true end
  end
  return false
end

local function spawn_food()
  local free = (CONFIG.width * CONFIG.height) - #state.snake
  if free <= 0 then
    log("You win. Final score: " .. state.score)
    state.alive = false
    state.game_over_t = 0
    return
  end

  for _ = 1, 2000 do
    local x = math.random(1, CONFIG.width)
    local y = math.random(1, CONFIG.height)
    if not occupies(x, y) then
      state.food = vec(x, y)
      return
    end
  end

  state.food = vec(1, 1)
end

local function apply_dir_request(dx, dy)
  if dx == -state.dir.x and dy == -state.dir.y then return end -- no 180 reversal
  state.next_dir = { x = dx, y = dy }
end

-- Event-driven input hook (optional)
function on_key(key)
  key = tostring(key):lower()
  if key == "up" or key == "arrowup" then apply_dir_request(0, 1) end
  if key == "down" or key == "arrowdown" then apply_dir_request(0, -1) end
  if key == "left" or key == "arrowleft" then apply_dir_request(-1, 0) end
  if key == "right" or key == "arrowright" then apply_dir_request(1, 0) end
end

local function input_table()
  local i = _G.Input
  if type(i) == "table" then return i end
  i = _G.input
  if type(i) == "table" then return i end
  return nil
end

local function input_call(fn, self_tbl, key)
  local ok, res = pcall(fn, key)
  if ok and res then return true end
  ok, res = pcall(fn, self_tbl, key) -- for colon-style bindings
  return ok and res
end

local function key_down(key)
  local I = input_table()
  if not I then return false end

  local candidates = {
    I.is_key_down, I.key_down,
    I.is_key_pressed, I.key_pressed,
  }
  for _, fn in ipairs(candidates) do
    if type(fn) == "function" and input_call(fn, I, key) then
      return true
    end
  end
  return false
end

local KEY = {
  UP = { "Up", "UP", "ArrowUp", 265 },
  DOWN = { "Down", "DOWN", "ArrowDown", 264 },
  LEFT = { "Left", "LEFT", "ArrowLeft", 263 },
  RIGHT = { "Right", "RIGHT", "ArrowRight", 262 },
}

local function any_down(list)
  for _, k in ipairs(list) do
    if key_down(k) then return true end
  end
  return false
end

local function poll_input()
  if any_down(KEY.UP) then apply_dir_request(0, 1) end
  if any_down(KEY.DOWN) then apply_dir_request(0, -1) end
  if any_down(KEY.LEFT) then apply_dir_request(-1, 0) end
  if any_down(KEY.RIGHT) then apply_dir_request(1, 0) end
end

local function reset()
  seed_rng()

  state.acc = 0
  state.alive = true
  state.game_over_t = 0
  state.score = 0
  state.dir = { x = 1, y = 0 }
  state.next_dir = { x = 1, y = 0 }
  state.snake = {}

  local cx = math.floor(CONFIG.width / 2)
  local cy = math.floor(CONFIG.height / 2)
  for i = 0, (CONFIG.start_len - 1) do
    table.insert(state.snake, vec(cx - i, cy))
  end

  spawn_food()
  state.initialized = true
  log("Snake reset. Use arrow keys to move.")
end

local function step()
  state.dir = state.next_dir

  local head = state.snake[1]
  local nx = head.x + state.dir.x
  local ny = head.y + state.dir.y

  if not in_bounds(nx, ny) then
    state.alive = false
    state.game_over_t = 0
    log("Game over (wall). Score: " .. state.score)
    return
  end

  local will_grow = (nx == state.food.x and ny == state.food.y)
  local max_check = #state.snake
  if not will_grow then max_check = max_check - 1 end -- tail moves away
  if max_check >= 1 and occupies(nx, ny, max_check) then
    state.alive = false
    state.game_over_t = 0
    log("Game over (self). Score: " .. state.score)
    return
  end

  table.insert(state.snake, 1, vec(nx, ny))

  if will_grow then
    state.score = state.score + CONFIG.score_per_food
    for _ = 2, CONFIG.grow_per_food do
      local tail = state.snake[#state.snake]
      table.insert(state.snake, vec(tail.x, tail.y))
    end
    spawn_food()
    log("Score: " .. state.score)
  else
    table.remove(state.snake)
  end

  -- Minimal 3D visualization: move the attached entity as the head (optional)
  if this and type(this.set_position) == "function" then
    local wx = (nx - 1) - (CONFIG.width * 0.5)
    local wz = (ny - 1) - (CONFIG.height * 0.5)
    pcall(this.set_position, wx, 0.25, wz)
  end
end

local function render()
  if type(_G.SnakeRender) == "table" and type(_G.SnakeRender.clear) == "function" and type(_G.SnakeRender.cell) == "function" then
    pcall(_G.SnakeRender.clear)
    pcall(_G.SnakeRender.cell, state.food.x, state.food.y, "food")
    for i = 1, #state.snake do
      local s = state.snake[i]
      pcall(_G.SnakeRender.cell, s.x, s.y, (i == 1) and "head" or "body")
    end
    if type(_G.SnakeRender.hud) == "function" then
      pcall(_G.SnakeRender.hud, "Score: " .. state.score)
    end
    return
  end

  if not CONFIG.print_board then return end

  state.last_print_t = state.last_print_t + CONFIG.step_time
  if state.last_print_t < 0.6 then return end
  state.last_print_t = 0

  local grid = {}
  for y = 1, CONFIG.height do
    grid[y] = {}
    for x = 1, CONFIG.width do
      grid[y][x] = ASSETS.glyph.empty
    end
  end

  grid[state.food.y][state.food.x] = ASSETS.glyph.food
  for i = #state.snake, 1, -1 do
    local s = state.snake[i]
    grid[s.y][s.x] = (i == 1) and ASSETS.glyph.head or ASSETS.glyph.body
  end

  local lines = {}
  table.insert(lines, ("Score: %d"):format(state.score))
  local border = ASSETS.glyph.wall:rep(CONFIG.width + 2)
  table.insert(lines, border)
  for y = CONFIG.height, 1, -1 do
    local row = { ASSETS.glyph.wall }
    for x = 1, CONFIG.width do table.insert(row, grid[y][x]) end
    table.insert(row, ASSETS.glyph.wall)
    table.insert(lines, table.concat(row))
  end
  table.insert(lines, border)

  log("\n" .. table.concat(lines, "\n"))
end

function update(dt)
  if not state.initialized then reset() end

  if not state.alive then
    state.game_over_t = state.game_over_t + dt
    if state.game_over_t >= 1.0 then reset() end
    return
  end

  poll_input()

  state.acc = state.acc + dt
  while state.acc >= CONFIG.step_time do
    state.acc = state.acc - CONFIG.step_time
    step()
    if not state.alive then break end
  end

  render()
end
