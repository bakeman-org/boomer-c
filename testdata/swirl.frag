#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;
uniform vec2 cameraScale;

// Swirl lens: content inside the cursor circle is twisted
void main() {
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    vec2 d = gl_FragCoord.xy - mouse;
    float r = length(d);
    float R = 200.0;
    vec2 texelPerPixel = 1.0 / (windowSize * cameraScale);

    vec2 uv = vTexCoord;
    if (r < R) {
        float t = 1.0 - r / R;
        float a = t * t * 4.0; // twist angle, strongest at center
        float cs = cos(a), sn = sin(a);
        vec2 rot = vec2(d.x * cs - d.y * sn, d.x * sn + d.y * cs);
        uv = clamp(vTexCoord + (rot - d) * texelPerPixel, 0.0, 1.0);
    }
    vec3 color = texture(tex, uv).rgb;

    float ring = smoothstep(R - 2.0, R, r) * (1.0 - smoothstep(R, R + 2.0, r));
    color = mix(color, vec3(1.0), ring * 0.5);

    // --- flashlight ---
    float mask = smoothstep(flRadius, flRadius * 0.8, r);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
