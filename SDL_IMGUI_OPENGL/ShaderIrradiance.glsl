#shader vertex
#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec3 v_RayDir;

uniform mat4 u_InvProjView;

void main()
{
    gl_Position = vec4(aPos.xy, 1.0, 1.0);
    vec4 farPoint = u_InvProjView * vec4(aPos.xy, 1.0, 1.0);
    v_RayDir = normalize(farPoint.xyz / farPoint.w);
}

#shader fragment
#version 450 core

in vec3 v_RayDir;

out vec4 FragColor;

uniform samplerCube u_EnvironmentCubemap;

const float PI = 3.14159265359;

void main()
{
    vec3 normal = normalize(v_RayDir);

    // 建立切线空间：以 normal 为 z 轴
    vec3 up = abs(normal.y) < 0.999 ? vec3(0,1,0) : vec3(0,0,1);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    vec3 irradiance = vec3(0.0);
    float sampleCount = 0.0;

    // 半球卷积，步长根据采样数调整
    // 0.025 ≈ PI/2 / 64 步 × PI / 128 步 ≈ 64×128 = 8192 采样
    // 0.05  ≈ 32×64 = 2048 采样（更快，视觉差异不大）
    float delta = 0.05;

    for (float phi = 0.0; phi < 2.0 * PI; phi += delta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += delta)
        {
            // 球坐标 → 切线空间方向
            vec3 tangentSample = vec3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );

            // 切线空间 → 世界空间
            vec3 sampleVec = tangentSample.x * right +
                            tangentSample.y * up +
                            tangentSample.z * normal;

            irradiance += texture(u_EnvironmentCubemap, sampleVec).rgb
                        * cos(theta)    // Lambert 余弦权重
                        * sin(theta);   // 球面积元素雅可比
            sampleCount += 1.0;
        }
    }

    irradiance = PI * irradiance / sampleCount;

    FragColor = vec4(irradiance, 1.0);
}