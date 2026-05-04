#shader vertex
#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 tangent;

out vec3 v_FragPos;
out mat3 v_TBN;
out vec2 v_TexCoord;

layout(std140) uniform PerFrameData
{
    mat4 u_ProjView;
    vec3 u_ViewPos;
    float _pad0;
};

layout(std140) uniform PerObjectData
{
    mat4 u_Model;
    mat4 u_MVP;
    vec4 u_Color;
    float u_MetallicFactor;
    float u_RoughnessFactor;
    float u_AlphaCutoff;
    float _pad2;
    vec4 u_EmissiveFactor;
};

void main()
{
    v_FragPos = vec3(u_Model * vec4(position, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    vec3 N = normalize(normalMatrix * normal);
    vec3 T = normalize(normalMatrix * tangent.xyz);
    // 重正交化：确保 T 垂直于 N
    T = normalize(T - N * dot(T, N));
    vec3 B = cross(N, T) * tangent.w;

    v_TBN = mat3(T, B, N);

    v_TexCoord = texCoord;
    gl_Position = u_MVP * vec4(position, 1.0);
}



#shader fragment
#version 450 core

layout(location = 0) out vec4 FragColor;    // HDR 场景颜色
layout(location = 1) out vec4 FragNormal;   // 世界空间法线

in vec3 v_FragPos;
in mat3 v_TBN;
in vec2 v_TexCoord;

layout(std140) uniform PerFrameData
{
    mat4 u_ProjView;
    vec3 u_ViewPos;
    float _pad0;
};

layout(std140) uniform PerObjectData
{
    mat4 u_Model;
    mat4 u_MVP;
    vec4 u_Color;
    float u_MetallicFactor;
    float u_RoughnessFactor;
    float u_AlphaCutoff;
    float _pad2;
    vec4 u_EmissiveFactor;
};

struct LightInfo
{
    vec3  position;
    float _lpad1;
    vec3  color;
    float intensity;
    mat4  lightSpaceMatrix;
    ivec4 flags;        // x = castShadow, y = shadowLayer
};

layout(std140, binding = 2) uniform LightArrayData
{
    ivec4     u_NumLightsPad;   // x = count
    LightInfo u_Lights[8];
};

uniform sampler2D               u_AlbedoMap;
uniform sampler2DArray          u_ShadowMaps;
uniform sampler2D               u_NormalMap;
uniform sampler2D               u_MetallicRoughnessMap;
uniform sampler2D               u_EmissiveMap;
uniform sampler2D               u_OcclusionMap;
uniform samplerCube             u_IrradianceMap;
uniform samplerCube             u_PrefilterMap;
uniform sampler2D               u_BRDFLUT;

uniform float u_MaxReflectionLOD = 4.0f;

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int layer)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    float bias = max(0.002 * (1.0 - dot(normal, lightDir)), 0.001);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMaps, 0).xy);
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(u_ShadowMaps, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;

    if (projCoords.z > 1.0) shadow = 0.0;
    return shadow;
}

void main()
{
    // 法线贴图：采样并从切线空间转到世界空间
    vec3 norm = texture(u_NormalMap, v_TexCoord).rgb;
    norm = normalize(norm * 2.0 - 1.0); 
    vec3 N = normalize(v_TBN * norm);

    vec3 albedo = texture(u_AlbedoMap, v_TexCoord).rgb * u_Color.rgb;
    float alpha = texture(u_AlbedoMap, v_TexCoord).a * u_Color.a;
    if (alpha < u_AlphaCutoff) discard;

    vec2 mr = texture(u_MetallicRoughnessMap, v_TexCoord).bg;
    float roughness = mr.x * u_RoughnessFactor;
    float metallic  = mr.y * u_MetallicFactor;

    vec3 emissive = texture(u_EmissiveMap, v_TexCoord).rgb * u_EmissiveFactor.rgb;

    float ao = texture(u_OcclusionMap, v_TexCoord).r;

    // PBR 公共计算
    vec3 V = normalize(u_ViewPos - v_FragPos);
    float NdotV = max(dot(N, V), 0.001);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // 多光源累加
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_NumLightsPad.x; i++)
    {
        vec3 L = normalize(u_Lights[i].position - v_FragPos);
        vec3 H = normalize(V + L);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // 菲涅尔
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

        // 法线分布 (GGX)
        float a2 = roughness * roughness * roughness * roughness;
        float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
        float D = a2 / (3.14159265 * denom * denom);

        // 几何遮蔽 (Smith)
        float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
        float G1 = NdotV / (NdotV * (1.0 - k) + k);
        float G2 = NdotL / (NdotL * (1.0 - k) + k);
        float G = G1 * G2;

        // Cook-Torrance BRDF
        vec3 specBRDF = (D * F * G) / (4.0 * NdotV * NdotL + 0.001);
        vec3 kD = (1.0 - F) * (1.0 - metallic);

        // 阴影
        float shadow = 0.0;
        if (u_Lights[i].flags.x != 0)
        {
            vec4 fragPosLightSpace = u_Lights[i].lightSpaceMatrix * vec4(v_FragPos, 1.0);
            shadow = ShadowCalculation(fragPosLightSpace, N, L, u_Lights[i].flags.y);
        }

        float dist = length(u_Lights[i].position - v_FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        vec3 radiance = u_Lights[i].color * u_Lights[i].intensity * attenuation;
        Lo += (kD * albedo / 3.14159265 + specBRDF) * radiance * NdotL * (1.0 - shadow);
    }

    // 环境光 + AO
    vec3 F_ibl = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD = (1.0 - F_ibl) * (1.0 - metallic);

    // 漫反射 IBL
    vec3 irradiance = texture(u_IrradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo * kD;

    // 镜面 IBL
    vec3 R = reflect(-V, N);
    // u_MaxReflectionLOD = log2(128) = 7.0，对应 5 个 mip level (0-4)
    vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * u_MaxReflectionLOD).rgb;
    vec2 envBRDF = texture(u_BRDFLUT, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor * (F_ibl * envBRDF.x + envBRDF.y);

    vec3 ambient = (diffuse + specular) * ao;

    vec3 result = ambient + Lo + emissive;

    FragColor = vec4(result, alpha);
    FragNormal = vec4(N, 1.0);
}