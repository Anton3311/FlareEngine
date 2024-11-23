BlendMode = Additive

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;

layout(location = 0) out vec2 o_UV;

void main()
{
	gl_Position = vec4(i_Position, 1.0f);
	o_UV = i_Position.xy * 0.5f + vec2(0.5f);
}

#end

#begin pixel
#version 450

#include "../Common/Math.glsl"

layout(set = 3, binding = 0) uniform sampler2D u_Color;

layout(location = 0) in vec2 i_UV;

layout(location = 0) out vec4 o_Color;

layout(std140, push_constant) uniform Constants
{
	float u_Intensity;
};

void main()
{
	o_Color = vec4(texture(u_Color, i_UV).rgb * u_Intensity, 1.0f);
}

#end
