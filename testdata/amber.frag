#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

// Retro amber phosphor terminal look
void main() {
    float t = luma(texture(tex, vTexCoord).rgb);
    vec3 color = vec3(t * 1.0, t * 0.65, t * 0.15);

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
