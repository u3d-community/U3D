#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"

varying vec2 vScreenPos;
varying vec3 vFarRay;

#ifdef COMPILEPS
uniform vec4 cTAAParams;
uniform mat4 cPrevViewProj;
uniform vec4 cPrevCameraPos;
uniform vec4 cPrevCameraDir;
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
    vFarRay = GetFarRay(gl_Position);
}

void PS()
{
    vec3 current = texture2D(sDiffMap, vScreenPos).rgb;
    float currentDepth = DecodeDepth(texture2D(sDepthBuffer, vScreenPos).rgb);
    vec3 worldPos = cCameraPosPS + vFarRay * currentDepth;
    vec4 previousClip = vec4(worldPos, 1.0) * cPrevViewProj;
    vec2 historyUV = previousClip.xy / previousClip.w * 0.5 + 0.5;
    if (cTAAParams.x <= 0.0 || previousClip.w <= 0.0 || any(lessThan(historyUV, vec2(0.0))) ||
        any(greaterThan(historyUV, vec2(1.0))))
    {
        gl_FragColor = vec4(current, 1.0);
        return;
    }

    float previousDepth = DecodeDepth(texture2D(sSpecMap, historyUV).rgb);
    float expectedPreviousDepth = dot(worldPos - cPrevCameraPos.xyz, cPrevCameraDir.xyz) / cPrevCameraPos.w;
    if (abs(previousDepth - expectedPreviousDepth) > cTAAParams.y * max(expectedPreviousDepth, 0.01))
    {
        gl_FragColor = vec4(current, 1.0);
        return;
    }

    vec3 history = texture2D(sNormalMap, historyUV).rgb;
    float currentLuma = dot(current, vec3(0.2126, 0.7152, 0.0722));
    float historyLuma = dot(history, vec3(0.2126, 0.7152, 0.0722));
    float relativeLumaChange = abs(currentLuma - historyLuma) / max(max(currentLuma, historyLuma), 0.1);
    float historyWeight = cTAAParams.x * (1.0 - smoothstep(0.1, 0.5, relativeLumaChange));
    vec3 neighborhoodMin = current;
    vec3 neighborhoodMax = current;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec3 sampleColor = texture2D(sDiffMap,
                vScreenPos + vec2(float(x), float(y)) * cGBufferInvSize.xy).rgb;
            neighborhoodMin = min(neighborhoodMin, sampleColor);
            neighborhoodMax = max(neighborhoodMax, sampleColor);
        }
    }
    history = clamp(history, neighborhoodMin, neighborhoodMax);
    gl_FragColor = vec4(mix(current, history, historyWeight), 1.0);
}
