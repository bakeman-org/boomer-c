#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

// Hue rotation: colors shift but brightness/contrast stay intact
vec3 hueShift(vec3 c, float a) {
    const vec3 k = vec3(0.57735);
    float cosA = cos(a);
    return c * cosA + cross(k, c) * sin(a) + k * dot(k, c) * (1.0 - cosA);
}

void main() {
    vec3 color = hueShift(texture(tex, vTexCoord).rgb, 2.094); // 120 degrees
    color = clamp(color, 0.0, 1.0);

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
