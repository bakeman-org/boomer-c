#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

// 4x4 Bayer 矩阵用于抖动
float bayer4x4[16] = float[](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

void main() {
    // 像素化分辨率 (数值越大越糊)
    float pixelSize = 300.0; 
    vec2 pixelated_uv = floor(vTexCoord * pixelSize) / pixelSize;
    
    vec3 color = texture(tex, pixelated_uv).rgb;
    
    // Bayer 抖动
    int x = int(mod(gl_FragCoord.x, 4.0));
    int y = int(mod(gl_FragCoord.y, 4.0));
    int index = x + y * 4;
    float threshold = bayer4x4[index] / 16.0 - 0.5; // 范围 -0.5 到 0.5
    
    // 减少颜色层级 (比如缩减到 8 色)
    color = floor(color * 8.0 + threshold) / 8.0;
    
    // 提高对比度，让像素感更强
    color = (color - 0.5) * 1.2 + 0.5;

    // --- 手电筒逻辑 ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}