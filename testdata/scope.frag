#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

// Sniper scope: world dimmed, clear circle with crosshair follows the cursor
void main() {
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    vec2 d = gl_FragCoord.xy - mouse;
    float r = length(d);
    float R = 220.0;

    vec3 color = texture(tex, vTexCoord).rgb;

    float inside = 1.0 - smoothstep(R - 2.0, R, r);
    color *= mix(0.25, 1.0, inside); // dark outside, bright inside

    // scope ring
    float ring = smoothstep(R - 3.0, R, r) * (1.0 - smoothstep(R, R + 3.0, r));
    color = mix(color, vec3(0.0), ring);

    // crosshair + mil-dots (only inside scope)
    float line = (1.0 - smoothstep(0.5, 1.5, abs(d.x))) + (1.0 - smoothstep(0.5, 1.5, abs(d.y)));
    line *= inside * step(8.0, r); // keep center open
    color = mix(color, vec3(1.0, 0.15, 0.15), clamp(line, 0.0, 1.0) * 0.85);

    // --- flashlight ---
    float mask = smoothstep(flRadius, flRadius * 0.8, r);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
