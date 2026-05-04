#shader vertex
#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec3 v_RayDir;

uniform mat4 u_InvProjView;

void main()
{
    gl_Position = vec4(aPos.xy, 1.0, 1.0);  // 全屏四边形，深度最远
    vec4 farPoint = u_InvProjView * vec4(aPos.xy, 1.0, 1.0);
    v_RayDir = normalize(farPoint.xyz / farPoint.w);
}

#shader fragment
#version 450 core

in vec3 v_RayDir;

out vec4 FragColor;

uniform sampler2D u_SkyboxMap;

const float PI = 3.14159265359;

vec2 DirectionToEquirectangular(vec3 dir)
{
    dir = normalize(dir);
    float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    float v = asin(dir.y) / PI + 0.5;
    return vec2(u, v);
}

void main()
{
    vec2 uv = DirectionToEquirectangular(v_RayDir);
    vec3 color = texture(u_SkyboxMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}