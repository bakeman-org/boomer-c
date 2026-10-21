#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;
uniform vec2 cameraScale;

// Privacy lens: everything pixelated except a clear circle around the cursor
void main() {
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    vec2 d = gl_FragCoord.xy - mouse;
    float r = length(d);
    float R = 200.0;
    vec2 texelPerPixel = 1.0 / (windowSize * cameraScale);

    vec2 uv = vTexCoord;
    if (r > R) {
        float grid = 24.0; // pixelation block size (screen px)
        vec2 cell = floor(gl_FragCoord.xy / grid) * grid + grid * 0.5;
        uv = clamp(vTexCoord + (cell - gl_FragCoord.xy) * texelPerPixel, 0.0, 1.0);
    }
    vec3 color = texture(tex, uv).rgb;

    // soft lens edge
    float edge = smoothstep(R - 6.0, R + 6.0, r);
    float ring = smoothstep(R - 2.0, R, r) * (1.0 - smoothstep(R, R + 2.0, r));
    color = mix(color, vec3(0.2), ring * 0.8);

    // --- flashlight ---
    float mask = smoothstep(flRadius, flRadius * 0.8, r);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
