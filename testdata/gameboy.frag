#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

// Game Boy 4-tone palette
void main() {
    float t = luma(texture(tex, vTexCoord).rgb);
    int band = int(clamp(t * 3.999, 0.0, 3.0));
    vec3 palette[4];
    palette[0] = vec3(0.059, 0.220, 0.059);
    palette[1] = vec3(0.188, 0.384, 0.188);
    palette[2] = vec3(0.545, 0.675, 0.059);
    palette[3] = vec3(0.608, 0.737, 0.059);
    vec3 color = palette[band];

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
