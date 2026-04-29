#shader vertex
#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

void main() {
    gl_Position = vec4(aPos, 1.0);
    vUV = aUV;
}

#shader fragment
#version 450 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D u_SceneMap;

const float EDGE_THRESH_MIN = 0.0312;
const float EDGE_THRESH_MAX = 0.125;
const float SUBPIXEL_QUALITY = 0.75;

float rgb2luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_SceneMap, 0));

    vec3 rgbN = texture(u_SceneMap, vUV + vec2( 0.0,  texelSize.y)).rgb;
    vec3 rgbS = texture(u_SceneMap, vUV + vec2( 0.0, -texelSize.y)).rgb;
    vec3 rgbE = texture(u_SceneMap, vUV + vec2( texelSize.x,  0.0)).rgb;
    vec3 rgbW = texture(u_SceneMap, vUV + vec2(-texelSize.x,  0.0)).rgb;
    vec3 rgbM = texture(u_SceneMap, vUV).rgb;

    float lumaN = rgb2luma(rgbN);
    float lumaS = rgb2luma(rgbS);
    float lumaE = rgb2luma(rgbE);
    float lumaW = rgb2luma(rgbW);
    float lumaM = rgb2luma(rgbM);

    float rangeMin = min(min(min(lumaN, lumaS), min(lumaE, lumaW)), lumaM);
    float rangeMax = max(max(max(lumaN, lumaS), max(lumaE, lumaW)), lumaM);
    float range = rangeMax - rangeMin;

    if (range < max(EDGE_THRESH_MIN, rangeMax * EDGE_THRESH_MAX))
    {
        FragColor = vec4(rgbM, 1.0);
        return;
    }

    vec3 rgbNW = texture(u_SceneMap, vUV + vec2(-texelSize.x,  texelSize.y)).rgb;
    vec3 rgbNE = texture(u_SceneMap, vUV + vec2( texelSize.x,  texelSize.y)).rgb;
    vec3 rgbSW = texture(u_SceneMap, vUV + vec2(-texelSize.x, -texelSize.y)).rgb;
    vec3 rgbSE = texture(u_SceneMap, vUV + vec2( texelSize.x, -texelSize.y)).rgb;

    float lumaNW = rgb2luma(rgbNW);
    float lumaNE = rgb2luma(rgbNE);
    float lumaSW = rgb2luma(rgbSW);
    float lumaSE = rgb2luma(rgbSE);

    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float edgeHorz1 = abs(-2.0 * lumaM + lumaNS) * 2.0 + abs(-2.0 * lumaE + lumaNE + lumaSE);
    float edgeHorz2 = abs(-2.0 * lumaM + lumaNS) * 2.0 + abs(-2.0 * lumaW + lumaNW + lumaSW);
    float edgeVert1 = abs(-2.0 * lumaM + lumaWE) * 2.0 + abs(-2.0 * lumaN + lumaNE + lumaNW);
    float edgeVert2 = abs(-2.0 * lumaM + lumaWE) * 2.0 + abs(-2.0 * lumaS + lumaSE + lumaSW);

    bool isHorizontal = (edgeHorz1 + edgeHorz2) >= (edgeVert1 + edgeVert2);

    float luma1, luma2;
    float stepLength;
    if (isHorizontal) {
        luma1 = lumaN;
        luma2 = lumaS;
        stepLength = texelSize.y;
    } else {
        luma1 = lumaE;
        luma2 = lumaW;
        stepLength = texelSize.x;
    }

    float gradient = luma1 - luma2;
    float stepSign = gradient < 0.0 ? -1.0 : 1.0;

    // 沿边缘搜索端点
    vec2 offset;
    if (isHorizontal)
        offset = vec2(0.0, stepLength * stepSign);
    else
        offset = vec2(stepLength * stepSign, 0.0);

    vec2 uvP = vUV + offset;
    vec2 uvN = vUV - offset;
    float lumaEndP = rgb2luma(texture(u_SceneMap, uvP).rgb);
    float lumaEndN = rgb2luma(texture(u_SceneMap, uvN).rgb);
    uvP += offset;
    uvN -= offset;

    // 双向搜索
    float lumaPos = lumaEndP;
    float lumaNeg = lumaEndN;
    for (int i = 0; i < 12; i++)
    {
        lumaPos = rgb2luma(texture(u_SceneMap, uvP).rgb);
        lumaNeg = rgb2luma(texture(u_SceneMap, uvN).rgb);
        bool posDone = abs(lumaPos - lumaM) >= abs(gradient) * 0.25;
        bool negDone = abs(lumaNeg - lumaM) >= abs(gradient) * 0.25;
        if (posDone && negDone) break;
        if (!posDone) { lumaEndP = lumaPos; uvP += offset; }
        if (!negDone) { lumaEndN = lumaNeg; uvN -= offset; }
    }

    // 判断边方向并计算混合权重
    float distP = abs(lumaEndP - lumaM);
    float distN = abs(lumaEndN - lumaM);
    bool facingLeft = distP >= distN;
    float edgeBlend = facingLeft
        ? distN / (distP + distN)
        : distP / (distP + distN);

    // 子像素 AA
    float lumaAvg = (lumaN + lumaS + lumaE + lumaW) / 4.0;
    float subpix = abs(lumaAvg - lumaM) / range;
    float subpixBlend = clamp(subpix * SUBPIXEL_QUALITY * 2.0, 0.0, 1.0);

    float finalBlend = max(edgeBlend, subpixBlend);

    // 沿法线方向偏移采样
    float finalOffset = stepLength * stepSign * finalBlend;
    if (facingLeft) finalOffset = -finalOffset;

    vec2 blendUV;
    if (isHorizontal)
        blendUV = vUV + vec2(0.0, finalOffset);
    else
        blendUV = vUV + vec2(finalOffset, 0.0);

    vec3 result = texture(u_SceneMap, blendUV).rgb;
    FragColor = vec4(result, 1.0);
}