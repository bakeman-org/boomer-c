#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D tex;
uniform vec2 windowSize;
uniform vec2 cursorPos;
uniform float flShadow;
uniform float flRadius;
uniform vec2 screenshotSize;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

// Comic look: mild posterize + dark ink outlines on strong edges
void main() {
    vec2 texel = 1.0 / screenshotSize;
    vec3 c = texture(tex, vTexCoord).rgb;

    // mild posterize (16 levels keeps text readable)
    vec3 color = floor(c * 16.0 + 0.5) / 16.0;

    // edge detection on luma
    float t  = luma(texture(tex, vTexCoord + vec2(0.0,  texel.y)).rgb);
    float b  = luma(texture(tex, vTexCoord + vec2(0.0, -texel.y)).rgb);
    float l  = luma(texture(tex, vTexCoord + vec2(-texel.x, 0.0)).rgb);
    float r  = luma(texture(tex, vTexCoord + vec2( texel.x, 0.0)).rgb);
    float edge = clamp(abs(t - b) + abs(l - r), 0.0, 1.0);

    color *= 1.0 - smoothstep(0.15, 0.5, edge) * 0.8; // ink outlines

    // --- flashlight ---
    vec2 mouse = vec2(cursorPos.x, windowSize.y - cursorPos.y);
    float dist = distance(gl_FragCoord.xy, mouse);
    float mask = smoothstep(flRadius, flRadius * 0.8, dist);
    color = mix(color * (1.0 - flShadow), color, mask);

    FragColor = vec4(color, 1.0);
}
