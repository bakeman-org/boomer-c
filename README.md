# this is my modify zooc code
- I rename as boomer-c
- use clang std=23 #embed feature that helps bake resource into executable

# ref links
- original repo: https://github.com/nick-bors/zooc

# bugs
1. 图片发虚
原因：Hyprland 开启了分数缩放（估计 scale = 1.5 或类似）。你的 X11 窗口是逻辑尺寸（比如 1707×960），而 Wayland screencopy 抓回来的是物理尺寸（比如 2560×1440）。X11 窗口的 1707×960 buffer 被合成器放大 1.5 倍显示到屏幕 → 糊。放大到 scale>1 时，纹理直接以物理像素采样，所以清晰。

修法：让 XWayland 使用物理像素坐标。 在 ~/.config/hypr/hyprland.conf 里加：

```conf
xwayland {
    force_zero_scaling = true
}
```