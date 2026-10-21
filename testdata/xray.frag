#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

// X-ray lens: inverted high-contrast grayscale inside the cursor circle
void main() {
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float r = length(gl_FragCoord.xy - mouse);
    float R = 180.0;

    vec3 color = texture(tex, vTexCoord).rgb;
    if (r < R) {
        float g = luma(color);
        color = vec3(clamp((1.0 - g) * 1.5, 0.0, 1.0));
    }

    float ring = smoothstep(R - 2.0, R, r) * (1.0 - smoothstep(R, R + 2.0, r));
    color = mix(color, vec3(0.2, 0.9, 1.0), ring * 0.8);

    // --- flashlight ---
    float mask = smoothstep(flRadius, flRadius * 0.8, r);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
