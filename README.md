# boomer-c
用纯 C 语言编写的跨平台（X11 / Wayland）屏幕放大、平移与手电筒工具。

```bash
# 编译
clang -std=c23 one.c $(pkg-config --cflags --libs x11 xrandr gl glew wayland-client) -o boomer-c -lm
```

## 🚀 运行与操作

直接运行编译出的二进制文件（**注意：必须在桌面终端中运行**）：

```bash
./boomer
```

### 快捷键与鼠标操作

| 输入 | 操作 |
|---|---|
| `鼠标左键拖拽` | 平移画布（带惯性滑动） |
| `鼠标滚轮` | 以鼠标位置为中心缩放画布 |
| `方向键` / `h`, `j`, `k`, `l` | 键盘平移画布 |
| `+` / `-` 或 `Ctrl+滚轮` | 缩放画布（以鼠标为中心） |
| `f` | 开关手电筒特效 |
| `r` | 重新加载配置文件和着色器 |
| `0` | 重置视角（缩放归 1，位置归原点） |
| `q` 或 `Esc` | 退出程序 |


## ⚙️ 配置文件

程序启动时会按以下顺序查找配置文件：
1. `$XDG_CONFIG_HOME/boomer-c/config.conf` (通常为 `~/.config/boomer-c/config.conf`)
2. 当前工作目录下的 `config.conf`
3. 如果都没找到，且编译器支持 `#embed`，则使用内置的默认配置。

### 配置项说明

```ini
# 最小缩放倍率 (默认 0.1)
min_scale = 0.1

# 最大缩放倍率 (默认 6.0)
max_scale = 6.0

# 拖拽时的摩擦力 (数值越大停得越快，默认 6.0)
drag_friction = 6.0

# 缩放时的摩擦力 (默认 4.0)
scale_friction = 4.0

# 鼠标滚轮缩放速度 (默认 1.5)
scroll_speed = 1.5

# 键盘平移速度 (默认 400.0)
key_move_speed = 400.0

# 是否以普通窗口模式运行 (默认 false，即全屏无边框)
# true/t or false/f
windowed = false
```

### 自定义着色器

你也可以在同级目录或配置目录下提供 `vertex.glsl` 和 `fragment.glsl`。程序在按下 `r` 键重载时，会优先加载外部文件；如果找不到，则回退到编译时硬编码的默认着色器。

## 📄 许可证

本项目基于 MIT 许可证开源。请自由使用、修改和分发。

# ref links
- https://github.com/nick-bors/zooc