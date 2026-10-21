#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;
uniform vec2 screenshotSize;

// Subtle unsharp-mask sharpening: text looks crisper, no visual artifacts
void main() {
    vec2 texel = 1.0 / screenshotSize;
    vec3 c  = texture(tex, vTexCoord).rgb;
    vec3 blur = (
        texture(tex, vTexCoord + vec2( texel.x, 0.0)).rgb +
        texture(tex, vTexCoord + vec2(-texel.x, 0.0)).rgb +
        texture(tex, vTexCoord + vec2(0.0,  texel.y)).rgb +
        texture(tex, vTexCoord + vec2(0.0, -texel.y)).rgb
    ) * 0.25;

    float amount = 0.6; // gentle sharpening strength
    vec3 color = clamp(c + (c - blur) * amount, 0.0, 1.0);

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
