#shader vertex
#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec2 v_UV;

void main()
{
    gl_Position = vec4(aPos.xy, 1.0, 1.0);
    v_UV = aUV;
}

#shader fragment
#version 450 core

in vec2 v_UV;

out vec2 FragColor;  // 注意：输出是 vec2，不是 vec4

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

// 几何遮蔽函数（Schlick-GGX，IBL 版本，用 roughness remap）
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;  // IBL 用 k = a²/2，直接光照用 k = (a+1)²/8

    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith 方法：遮蔽 + 自遮挡
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel-Schlick 近似
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // UV → (NdotV, roughness)
    float NdotV = v_UV.x;
    float roughness = v_UV.y;

    // 构造一个让 NdotV = dot(N, V) 恰好等于 UV.x 的几何配置
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);  // sin(angle)
    V.y = 0.0;
    V.z = NdotV;                        // cos(angle)

    vec3 N = vec3(0.0, 0.0, 1.0);       // 法线沿 Z 轴

    vec3 F0 = vec3(1.0);  // 积分时令 F0 = 1，后续拆出 scale/bias
    // 因为 F = F0 + (1-F0)*(1-cos)^5 = F0*(1-(1-cos)^5) + (1-cos)^5
    // 所以 ∫F*G*V dH = F0 × ∫(1-(1-cos)^5)*G*V dH + ∫(1-cos)^5*G*V dH
    //                  = F0 × scale          +        bias

    float scale = 0.0;
    float bias = 0.0;

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);       // N = (0,0,1)，所以 NdotL = L.z
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV + 0.0001);
            float Fc = pow(1.0 - VdotH, 5.0);

            scale += (1.0 - Fc) * G_Vis;
            bias += Fc * G_Vis;
        }
    }

    FragColor = vec2(scale, bias) / float(SAMPLE_COUNT);
}