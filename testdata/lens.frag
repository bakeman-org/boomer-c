#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;
uniform vec2 cameraScale;

// Magnifying lens: 2x zoom inside a circle that follows the cursor
void main() {
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    vec2 d = gl_FragCoord.xy - mouse;
    float r = length(d);
    float R = 180.0;
    vec2 texelPerPixel = 1.0 / (windowSize * cameraScale);

    vec2 uv = vTexCoord;
    if (r < R) {
        float k = 0.5; // 2x magnification inside the lens
        uv = clamp(vTexCoord + d * (k - 1.0) * texelPerPixel, 0.0, 1.0);
    }
    vec3 color = texture(tex, uv).rgb;

    // lens ring
    float ring = smoothstep(R - 3.0, R, r) * (1.0 - smoothstep(R, R + 3.0, r));
    color = mix(color, vec3(1.0), ring * 0.9);

    // --- flashlight ---
    float dist = r;
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
