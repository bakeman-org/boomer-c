#version 130
out mediump vec4 color;
in mediump vec2 texcoord;

// 核心Uniform（仅保留准星/相机/窗口相关）
uniform sampler2D tex;          
uniform vec2 cursorPos;         // 光标位置（窗口像素坐标，X11）
uniform vec2 windowSize;        // 窗口尺寸（像素）
uniform float flShadow;         // 外区域暗化强度
uniform float flRadius;         // 准星圆环外半径（像素，滚轮调整）
uniform float cameraScale;      // 仅作用于屏幕截图纹理，不影响准星

// CS准星可配置参数（像素单位）
const float circleInnerRadius = 0.92; // 圆环内半径比例（控制环的粗细）
const float lineWidth = 2.0;          // 十字线/圆环宽度（像素，固定）
const float aimBrightness = 1.0;      // 主准星红色亮度
const float edgeSmooth = 1.0;         // 边缘平滑度（1.0=硬边，CS风格）
const vec3 pureRed = vec3(1.0, 0.05, 0.05); // 主准星纯正红色（避免偏白）
// 新增：中心淡红点配置
const float centerDotRadius = 1.5;    // 中心圆点半径（像素）
const float centerDotBrightness = 0.6;// 中心圆点亮度（<1.0 实现淡红色）
const vec3 centerDotColor = vec3(1.0, 0.1, 0.1); // 中心圆点淡红色

void main() {
    // 1. 采样原始屏幕纹理（cameraScale仅作用于纹理显示）
    vec4 baseColor = texture(tex, texcoord);
    
    // 2. 准星坐标校正（仅翻转Y轴，无缩放偏移）
    vec2 cursorFix = vec2(cursorPos.x, windowSize.y - cursorPos.y); 
    vec2 distXY = abs(gl_FragCoord.xy - cursorFix);                        
    float distLen = length(gl_FragCoord.xy - cursorFix);                   

    // 3. CS准星绘制（红色圆环 + 红色十字线 + 中心淡红点）
    float circleOuter = flRadius;                                   // 圆环外半径（滚轮调整）
    float circleInner = circleOuter * circleInnerRadius;            // 圆环内半径
    float halfLineW = lineWidth / 2.0;                              // 线宽的一半
    float crossLineLen = circleOuter;                               // 十字线长度=圆环半径

    vec3 aimColor = vec3(0.0); 
    // 3.1 红色圆环（仅轮廓）
    float inCircle = smoothstep(circleOuter - edgeSmooth, circleOuter + edgeSmooth, distLen) 
                   - smoothstep(circleInner - edgeSmooth, circleInner + edgeSmooth, distLen);
    // 3.2 红色中心竖线（长度=圆环半径）
    float inVLine = step(distXY.x, halfLineW) * step(distXY.y, crossLineLen);
    // 3.3 红色中心横线（长度=圆环半径）
    float inHLine = step(distXY.y, halfLineW) * step(distXY.x, crossLineLen);
    // 3.4 新增：中心淡红色圆点
    float inCenterDot = step(distLen, centerDotRadius); // 判断是否在中心圆点范围内
    
    // 3.5 合并准星颜色
    float mainAimIntensity = max(max(inCircle, inVLine), inHLine); // 主准星强度
    vec3 mainAimColor = mainAimIntensity * pureRed * aimBrightness; // 主准星红色
    vec3 centerDotAimColor = inCenterDot * centerDotColor * centerDotBrightness; // 中心淡红点
    
    aimColor = mainAimColor + centerDotAimColor; // 合并主准星和中心淡红点

    // 4. 外区域暗化（适配缩放后的纹理范围）
    vec2 fragPosWorld = gl_FragCoord.xy / cameraScale;
    vec2 cursorFixWorld = vec2(cursorPos.x, windowSize.y - cursorPos.y) / cameraScale;
    float distLenWorld = length(fragPosWorld - cursorFixWorld);
    vec4 finalColor = mix(baseColor, vec4(0.0, 0.0, 0.0, 0.0), 
                          distLenWorld < flRadius ? 0.0 : flShadow);

    // 5. 叠加准星颜色（主准星+中心淡红点）
    finalColor.rgb = finalColor.rgb + aimColor;

    // 输出最终颜色
    color = finalColor;
}