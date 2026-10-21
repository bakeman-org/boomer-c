-- boomer-c configuration (Lua)
-- Every field supports hot reload: press the reload key to re-read this file.

min_scale        = 0.7
max_scale        = 8.0
drag_friction    = 6.0
scale_friction   = 4.0
scroll_speed     = 1.5
key_move_speed   = 400.0
windowed         = false

-- Popup (OSD) settings
popup_timeout    = 10.0          -- seconds a popup stays visible
popup_margin     = 8             -- px from screen bottom edge
font_name        = "monospace-14" -- Xft font pattern

-- Output / debug
output_path      = "."            -- where screenshots go (default: $HOME)
debug            = false          -- debug logging to terminal

-- Custom shaders (paths; embedded shaders are used if files are absent)
vertex_shader    = "./vertex.glsl"
fragment_shader  = "./fragment.glsl"

-- Custom keybinds (define all actions or none).
-- Each action takes a list of key names.
keys = {
  move_left  = { "h", "left" },
  move_down  = { "j", "down" },
  move_up    = { "k", "up" },
  move_right = { "l", "right" },
  zoom_out   = { "minus" },
  zoom_in    = { "equal" },
  quit       = { "q", "escape" },
  reset      = { "0" },
  reload     = { "r" },
  flashlight = { "f" },
  save       = { "s" },
  help       = { "question" },
}

-- Custom help text (define all actions or none).
help = {
  move_left  = "Pan view left",
  move_down  = "Pan view down",
  move_up    = "Pan view up",
  move_right = "Pan view right",
  zoom_out   = "Zoom out",
  zoom_in    = "Zoom in",
  quit       = "Quit boomer",
  reset      = "Reset zoom and position",
  reload     = "Reload shaders and config",
  flashlight = "Toggle flashlight mode",
  save       = "Save screenshot",
  help       = "Toggle help",
}
