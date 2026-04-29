#shader vertex
#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

out vec2 v_TexCoord;

layout(std140) uniform PerObjectData
{
    mat4 u_Model;
    mat4 u_MVP;
    vec4 u_Color;
};

uniform vec2 u_UVoffset;
uniform vec2 u_UVscale;

void main()
{
	gl_Position = u_MVP * vec4(position,1.0);
	v_TexCoord = texCoord * u_UVscale + u_UVoffset;
};

#shader fragment
#version 450 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

layout(std140) uniform PerObjectData
{
    mat4 u_Model;
    mat4 u_MVP;
    vec4 u_Color;
};

uniform sampler2D u_AlbedoMap;

void main()
{
	vec4 texColor = texture(u_AlbedoMap,v_TexCoord);
	if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1)
          texColor.a = 0.0;
	color = texColor * u_Color;
};