#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform vec2 cameraPos;
uniform float cameraScale;
uniform vec2 windowSize;

void main() {
    // 1. 将屏幕像素坐标转为中心点相对的坐标
    vec2 pos = aPos.xy - windowSize * 0.5;
    
    // 2. 应用相机缩放和位移
    pos = (pos / cameraScale) + cameraPos;
    
    // 3. 转回 NDC (Normalized Device Coordinates: -1.0 到 1.0)
    vec2 ndc = pos / (windowSize * 0.5);
    
    gl_Position = vec4(ndc, 0.0, 1.0);
    vTexCoord = aTexCoord;
}