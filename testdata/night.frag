#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;

// Night mode: blue-light filter, shifts toward warm amber; content readable
void main() {
    vec3 c = texture(tex, vTexCoord).rgb;
    vec3 color = vec3(c.r, c.g * 0.85, c.b * 0.60); // cut blue, slightly cut green
    color *= 1.05; // small brightness compensation

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
