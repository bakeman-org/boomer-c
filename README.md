# boomer-c

用纯 C 语言编写的跨平台（X11 / Wayland）屏幕放大、平移与手电筒工具。

## 编译

依赖：`libX11 libXrandr GLEW GL libpng libXft lua5.4`（Wayland 可选）。

```bash
bash build-one.sh
# 等价于：
clang-22 -std=c23 boomer-c.c \
  $(pkg-config --cflags --libs x11 xrandr gl glew libpng xft lua5.4) \
  -o boomer-c -lm -lwayland-client
```

> 编译器需支持 C23 `#embed`（clang 16+）。配置文件与着色器会被烘焙进二进制，因此即使外部配置文件缺失也能运行。

## 运行

**必须在桌面终端中运行**：

```bash
./boomer-c                       # 使用 config.lua 配置
./boomer-c -d                    # 终端打印调试信息
./boomer-c -o ~/Pictures/shot.png   # 指定截图保存路径
./boomer-c vertex.glsl frag.frag # 命令行指定着色器（顶点在前，片元在后）
```

命令行参数会覆盖配置文件中的对应字段。

## 快捷键与鼠标操作

| 输入 | 操作 |
|---|---|
| `鼠标左键拖拽` | 平移画布（带惯性滑动） |
| `鼠标滚轮` | 以鼠标位置为中心缩放画布 |
| `h j k l` / 方向键 | 键盘平移画布 |
| `-` / `=` | 缩放画布 |
| `f` | 开关手电筒特效 |
| `s` | 保存当前画面截图 |
| `r` | 热重载配置与着色器 |
| `0` | 重置视角（缩放归 1，位置归原点） |
| `?` | 显示/隐藏居中的快捷键帮助面板 |
| `q` / `Esc` | 退出程序 |

> 所有快捷键均支持在 `config.lua` 中自定义（见下文）。

## 配置文件

配置格式为 **Lua**，启动时按以下顺序查找：

1. `$XDG_CONFIG_HOME/boomer-c/config.lua`（通常为 `~/.config/boomer-c/config.lua`）
2. 当前工作目录下的 `config.lua`
3. 若都不存在，使用编译时 `#embed` 烘焙的默认配置

参考 `config.lua`：

```lua
min_scale        = 0.7
max_scale        = 8.0
drag_friction    = 6.0
scale_friction   = 4.0
scroll_speed     = 1.5
key_move_speed   = 400.0
windowed         = false     -- true 为普通窗口，false 为全屏无边框

-- OSD 弹层设置
popup_timeout    = 10.0      -- 弹层持续秒数
popup_margin     = 8         -- 距屏幕底部像素
font_name        = "monospace-14"   -- Xft 字体

output_path      = "."       -- 截图保存目录
debug            = false     -- 终端输出调试信息

-- 自定义着色器路径（文件不存在则回退到内置）
vertex_shader    = "./vertex.glsl"
fragment_shader  = "./fragment.glsl"
```

### 热重载

按 `r` 键可热重载 **全部** 配置字段（缩放参数、摩擦、窗口模式、字体、输出路径、着色器路径、快捷键、帮助文案、调试开关等），无需重启程序。

### 自定义快捷键

在 `config.lua` 中定义 `keys` 表，每个动作对应一个按键名数组。支持多键绑定：

```lua
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
```

可用按键名包括字母 `a–z`、数字 `0–9`、`left/right/up/down`、`minus/equal/plus`、`question/slash/space/tab/enter/escape/backspace/delete/home/end/pageup/pagedown`、`f1`–`f12`。

> 若定义了 `keys` 表，**12 个动作必须全部定义**，否则程序启动报错。同理，定义了 `help` 表也必须给出全部 12 条帮助文案。

### 自定义帮助文案

```lua
help = {
  move_left  = "Pan view left",
  ...
  help       = "Toggle help",
}
```

## 自定义着色器

启动或按 `r` 重载时，着色器路径解析顺序为：

1. `config.lua` 中 `vertex_shader` / `fragment_shader` 指定的路径
2. `~/.config/boomer-c/vertex.glsl`、`~/.config/boomer-c/fragment.glsl`
3. 当前目录下的 `vertex.glsl`、`fragment.glsl`
4. 编译时烘焙的内置着色器

### 可用 uniform

片元着色器可使用以下 uniform（顶点着色器需透传 `vTexCoord`）：

| uniform | 类型 | 说明 |
|---|---|---|
| `tex` | sampler2D | 屏幕截图纹理 |
| `cameraPos` | vec2 | 相机位置 |
| `cameraScale` | vec2 | 缩放比例 |
| `screenshotSize` | vec2 | 截图分辨率 |
| `windowSize` | vec2 | 窗口分辨率 |
| `cursorPos` | vec2 | 鼠标位置（像素，y 向下） |
| `flShadow` | float | 手电筒阴影强度（0–1） |
| `flRadius` | float | 手电筒半径 |

### testdata 示例着色器

`testdata/` 目录内置一组有趣且清晰的片元着色器，需搭配 `testdata/vertex.glsl` 使用：

| 着色器 | 效果 |
|---|---|
| `grayscale.frag` | 灰度转换 |
| `sepia.frag` | 棕褐色复古 |
| `invert.frag` | 颜色反相 |
| `night.frag` | 夜间护眼（滤蓝） |
| `sharpen.frag` | 锐化 |
| `vignette.frag` | 暗角 |
| `hue.frag` | 色相旋转 120° |
| `duotone.frag` | 靛蓝→青色双调 |
| `amber.frag` | 琥珀色终端复古 |
| `gameboy.frag` | Game Boy 4 色调色板 |
| `thermal.frag` | 热成像伪彩色 |
| `comic.frag` | 漫画轮廓 |
| `solarize.frag` | 中途曝光反转 |
| `blueprint.frag` | 蓝图风格 |
| `lens.frag` | 放大镜（跟随鼠标） |
| `scope.frag` | 狙击镜 |
| `privacy.frag` | 马赛克隐私镜 |
| `swirl.frag` | 漩涡 |
| `xray.frag` | X 光镜 |
| `viewfinder.frag` | 取景框 |

运行示例：

```bash
./boomer-c testdata/vertex.glsl testdata/lens.frag
```

所有示例着色器都保留了屏幕内容的可读性，并兼容手电筒功能。

## 许可证

MIT

## 参考

- https://github.com/nick-bors/zooc
