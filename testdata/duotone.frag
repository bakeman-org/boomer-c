#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

// Duotone: map brightness to indigo -> cyan, contrast preserved
void main() {
    float t = luma(texture(tex, vTexCoord).rgb);
    vec3 color = mix(vec3(0.05, 0.02, 0.25), vec3(0.35, 0.95, 1.0), t);

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
