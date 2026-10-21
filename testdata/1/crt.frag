#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

// 屏幕弯曲 (Barrel Distortion)
vec2 curve(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(6.0, 4.0);
    uv = uv + uv * offset * offset;
    return uv * 0.5 + 0.5;
}

void main() {
    vec2 uv = vTexCoord;
    vec2 curved_uv = curve(uv);
    
    // 屏幕外部的部分变黑
    if (curved_uv.x < 0.0 || curved_uv.x > 1.0 || curved_uv.y < 0.0 || curved_uv.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 扫描线
    float scanline = sin(curved_uv.y * windowSize.y * 1.5) * 0.1;
    
    // 色差 (Chromatic Aberration)
    float offset = 0.002;
    vec4 color;
    color.r = texture(tex, curved_uv + vec2(offset, 0.0)).r;
    color.g = texture(tex, curved_uv).g;
    color.b = texture(tex, curved_uv - vec2(offset, 0.0)).b;
    color.a = 1.0;

    // 基础颜色减暗
    color.rgb -= scanline;
    color.rgb *= 0.9;

    // --- 手电筒逻辑 ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    
    // 混合黑暗和手电筒照亮区域
    color.rgb = mix(color.rgb * (1.0 - flShadow), color.rgb, mask);

    FragColor = color;
}