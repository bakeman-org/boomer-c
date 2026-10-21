#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

// Thermal camera: ironbow false-color ramp, brightness order preserved
vec3 ironbow(float t) {
    vec3 c = vec3(0.0);
    c = mix(c, vec3(0.30, 0.00, 0.55), smoothstep(0.0, 0.25, t));
    c = mix(c, vec3(0.95, 0.10, 0.10), smoothstep(0.25, 0.55, t));
    c = mix(c, vec3(1.00, 0.90, 0.10), smoothstep(0.55, 0.80, t));
    c = mix(c, vec3(1.00, 1.00, 1.00), smoothstep(0.80, 1.00, t));
    return c;
}

void main() {
    vec3 color = ironbow(luma(texture(tex, vTexCoord).rgb));

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
