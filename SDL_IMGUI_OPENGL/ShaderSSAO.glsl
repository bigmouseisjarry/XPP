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

uniform sampler2D u_DepthMap;
uniform sampler2D u_NoiseMap;

uniform mat4 u_Projection;
uniform mat4 u_InvProjection;
uniform float u_Radius = 0.5;
uniform vec2 u_NoiseScale;

layout(std140) uniform SSAOKernelData
{
    vec4 u_Samples[64]; 
};

void main()
{
    // 从深度重建视图空间位置
    float depth = texture(u_DepthMap, vUV).r;
    vec4 ndc = vec4(vUV * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = u_InvProjection * ndc;
    viewPos /= viewPos.w;

    // 世界空间法线
     vec3 normal = normalize(cross(dFdx(viewPos.xyz), dFdy(viewPos.xyz)));

    // 随机旋转向量
    vec3 randomVec = normalize(texture(u_NoiseMap, vUV * u_NoiseScale).xyz);

    // 构建 TBN（从法线 + 随机向量）
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // 采样累加遮蔽
    float occlusion = 0.0;
    for (int i = 0; i < 64; ++i)
    {
        // 采样方向转到视图空间
        vec3 sampleDir = TBN * u_Samples[i].xyz;
        vec4 sampleView = vec4(viewPos.xyz + sampleDir * u_Radius, 1.0);

        // 投影到屏幕空间获取 UV
        vec4 sampleNDC = u_Projection * sampleView;
        sampleNDC.xy /= sampleNDC.w;
        vec2 sampleUV = sampleNDC.xy * 0.5 + 0.5;

        // 采样实际深度并重建视图空间位置
        float sampleDepth = texture(u_DepthMap, sampleUV).r;
        vec4 sampleViewReal = u_InvProjection * vec4(sampleNDC.xy, sampleDepth * 2.0 - 1.0, 1.0);
        sampleViewReal /= sampleViewReal.w;

        // 范围检测：避免远处的遮挡影响近处
        float rangeCheck = smoothstep(0.0, 1.0, u_Radius / abs(viewPos.z - sampleViewReal.z));
        float bias = 0.01;
        occlusion += (sampleViewReal.z < viewPos.z + sampleDir.z * u_Radius - bias ? 1.0 : 0.0) * rangeCheck;
    }

    float ao = 1.0 - (occlusion / 64.0);
    FragColor = vec4(ao, ao, ao, 1.0);
}