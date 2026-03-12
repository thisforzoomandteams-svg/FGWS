-- move.lua
local v = 0
function update(dt)
  v = v + dt
  if this then
    local pos = this.get_position()
    pos.x = pos.x + math.sin(v) * dt * 0.5
    this.set_position(pos)
  end
end