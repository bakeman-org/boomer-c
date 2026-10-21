#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;
uniform vec2 screenshotSize;

// 计算亮度
float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main() {
    vec2 texel = 1.0 / screenshotSize;
    
    // 简单的 Sobel 边缘检测
    float tl = luma(texture(tex, vTexCoord + vec2(-texel.x, texel.y)).rgb);
    float  l = luma(texture(tex, vTexCoord + vec2(-texel.x, 0.0)).rgb);
    float bl = luma(texture(tex, vTexCoord + vec2(-texel.x, -texel.y)).rgb);
    float  t = luma(texture(tex, vTexCoord + vec2(0.0, texel.y)).rgb);
    float  b = luma(texture(tex, vTexCoord + vec2(0.0, -texel.y)).rgb);
    float tr = luma(texture(tex, vTexCoord + vec2(texel.x, texel.y)).rgb);
    float  r = luma(texture(tex, vTexCoord + vec2(texel.x, 0.0)).rgb);
    float br = luma(texture(tex, vTexCoord + vec2(texel.x, -texel.y)).rgb);

    float gx = -tl - 2.0*l - bl + tr + 2.0*r + br;
    float gy = -tl - 2.0*t - tr + bl + 2.0*b + br;
    float edge = sqrt(gx*gx + gy*gy);

    // 原始颜色转为暗色，方便凸显霓虹
    vec3 baseColor = texture(tex, vTexCoord).rgb * 0.2;
    
    // 霓虹色：青色和品红根据边缘角度混合
    vec3 neonCyan = vec3(0.0, 1.0, 1.0);
    vec3 neonPink = vec3(1.0, 0.0, 0.8);
    vec3 neonColor = mix(neonPink, neonCyan, smoothstep(0.0, 1.0, abs(gx) / (abs(gx) + abs(gy) + 0.001)));

    // 强化边缘发光
    vec3 finalColor = baseColor + neonColor * smoothstep(0.1, 0.6, edge) * 2.0;

    // --- 手电筒逻辑 ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    finalColor = mix(finalColor * (1.0 - flShadow), finalColor, mask);

    FragColor = vec4(finalColor, 1.0);
}