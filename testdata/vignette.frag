#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

// Vignette: gently darkened corners, center stays fully clear
void main() {
    vec3 color = texture(tex, vTexCoord).rgb;

    vec2 uv = vTexCoord - 0.5;
    float vig = 1.0 - dot(uv, uv) * 0.6; // subtle falloff
    color *= clamp(vig, 0.0, 1.0);

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
