#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

// Camera viewfinder: rule-of-thirds grid + corner brackets, content untouched
void main() {
    vec3 color = texture(tex, vTexCoord).rgb;
    vec2 p = gl_FragCoord.xy;

    // rule-of-thirds lines (thin, subtle)
    float line = 0.0;
    line += 1.0 - smoothstep(0.5, 1.0, abs(p.x - windowSize.x / 3.0));
    line += 1.0 - smoothstep(0.5, 1.0, abs(p.x - windowSize.x * 2.0 / 3.0));
    line += 1.0 - smoothstep(0.5, 1.5, abs(p.y - windowSize.y / 3.0));
    line += 1.0 - smoothstep(0.5, 1.5, abs(p.y - windowSize.y * 2.0 / 3.0));
    color = mix(color, vec3(1.0), clamp(line, 0.0, 1.0) * 0.25);

    // corner brackets
    float m = 20.0, len = 45.0, th = 3.0;
    vec2 q = min(p, windowSize - p);           // distance to nearest corner
    float inX = step(m, q.x) * step(q.x, m + len);
    float inY = step(m, q.y) * step(q.y, m + len);
    float barH = inX * step(q.y, m + th);      // horizontal bar
    float barV = inY * step(q.x, m + th);      // vertical bar
    color = mix(color, vec3(0.2, 1.0, 0.2), clamp(barH + barV, 0.0, 1.0) * 0.9);

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
