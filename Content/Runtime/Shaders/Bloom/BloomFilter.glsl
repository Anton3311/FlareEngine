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

layout(location = 0) out vec3 o_Color;

layout(std140, push_constant) uniform Constants
{
	vec2 u_TexelSize;
};

void main()
{
	vec3 centerSample = texture(u_Color, i_UV).rgb;

	vec3 topLeftSample = texture(u_Color, i_UV - vec2(-u_TexelSize.x, u_TexelSize.y)).rgb;
	vec3 topRightSample = texture(u_Color, i_UV + vec2(u_TexelSize.x, u_TexelSize.y)).rgb;

	vec3 bottomLeftSample = texture(u_Color, i_UV + vec2(-u_TexelSize.x, -u_TexelSize.y)).rgb;
	vec3 bottomRightSample = texture(u_Color, i_UV + vec2(u_TexelSize.x, -u_TexelSize.y)).rgb;

	o_Color = centerSample * 0.5f
		+ (topLeftSample + topRightSample + bottomLeftSample + bottomRightSample) * 0.125f;
}

#end
