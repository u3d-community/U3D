#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

#ifndef D3D11
uniform float4 cTAAParams;
uniform float4x4 cPrevViewProj;
uniform float4 cPrevCameraPos;
uniform float4 cPrevCameraDir;
#else
#ifdef COMPILEPS
cbuffer CustomPS : register(b6)
{
    float4 cTAAParams;
    float4x4 cPrevViewProj;
    float4 cPrevCameraPos;
    float4 cPrevCameraDir;
}
#endif
#endif

void VS(float4 iPos : POSITION, out float2 oScreenPos : TEXCOORD0, out float3 oFarRay : TEXCOORD1,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oScreenPos = GetScreenPosPreDiv(oPos);
    oFarRay = GetFarRay(oPos);
}

void PS(float2 iScreenPos : TEXCOORD0, float3 iFarRay : TEXCOORD1, out float4 oColor : OUTCOLOR0)
{
    float3 current = Sample2DLod0(DiffMap, iScreenPos).rgb;
    float currentDepth = DecodeDepth(Sample2DLod0(DepthBuffer, iScreenPos).rgb);
    float3 worldPos = cCameraPosPS + iFarRay * currentDepth;
    float4 previousClip = mul(float4(worldPos, 1.0), cPrevViewProj);
    float2 historyUV = previousClip.xy / previousClip.w * float2(0.5, -0.5) + 0.5;
    if (cTAAParams.x <= 0.0 || previousClip.w <= 0.0 || any(historyUV < 0.0) || any(historyUV > 1.0))
    {
        oColor = float4(current, 1.0);
        return;
    }

    float previousDepth = DecodeDepth(Sample2DLod0(SpecMap, historyUV).rgb);
    float expectedPreviousDepth = dot(worldPos - cPrevCameraPos.xyz, cPrevCameraDir.xyz) / cPrevCameraPos.w;
    if (abs(previousDepth - expectedPreviousDepth) > cTAAParams.y * max(expectedPreviousDepth, 0.01))
    {
        oColor = float4(current, 1.0);
        return;
    }

    float3 history = Sample2DLod0(NormalMap, historyUV).rgb;
    float currentLuma = dot(current, float3(0.2126, 0.7152, 0.0722));
    float historyLuma = dot(history, float3(0.2126, 0.7152, 0.0722));
    float relativeLumaChange = abs(currentLuma - historyLuma) / max(max(currentLuma, historyLuma), 0.1);
    float historyWeight = cTAAParams.x * (1.0 - smoothstep(0.1, 0.5, relativeLumaChange));
    float3 neighborhoodMin = current;
    float3 neighborhoodMax = current;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float3 sampleColor = Sample2DLod0(DiffMap, iScreenPos + float2(x, y) * cGBufferInvSize.xy).rgb;
            neighborhoodMin = min(neighborhoodMin, sampleColor);
            neighborhoodMax = max(neighborhoodMax, sampleColor);
        }
    }
    history = clamp(history, neighborhoodMin, neighborhoodMax);
    oColor = float4(lerp(current, history, historyWeight), 1.0);
}
