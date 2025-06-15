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

layout(std140, push_constant) uniform Constants
{
	bool u_ReduceDynamicRange;
};

layout(location = 0) in vec2 i_UV;

layout(location = 0) out vec3 o_Color;

vec3 KarisAverage(vec3 color)
{
	float luminance = LuminanceFromRGB(color);
	return color / (1.0f + luminance);
}

void main()
{
	vec3 samples[3][3];

	samples[0][0] = textureOffset(u_Color, i_UV, ivec2(-2, -2), 0).rgb;
	samples[1][0] = textureOffset(u_Color, i_UV, ivec2(0, -2), 0).rgb;
	samples[2][0] = textureOffset(u_Color, i_UV, ivec2(2, -2), 0).rgb;

	samples[0][1] = textureOffset(u_Color, i_UV, ivec2(-2, 0), 0).rgb;
	samples[1][1] = textureOffset(u_Color, i_UV, ivec2(0, 0), 0).rgb;
	samples[2][1] = textureOffset(u_Color, i_UV, ivec2(2, 0), 0).rgb;

	samples[0][2] = textureOffset(u_Color, i_UV, ivec2(-2, 2), 0).rgb;
	samples[1][2] = textureOffset(u_Color, i_UV, ivec2(0, 2), 0).rgb;
	samples[2][2] = textureOffset(u_Color, i_UV, ivec2(2, 2), 0).rgb;

	vec3 centerTopLeft = textureOffset(u_Color, i_UV, ivec2(-1, -1), 0).rgb;
	vec3 centerTopRight = textureOffset(u_Color, i_UV, ivec2(1, -1), 0).rgb;
	vec3 centerBottomLeft = textureOffset(u_Color, i_UV, ivec2(-1, 1), 0).rgb;
	vec3 centerBottomRight = textureOffset(u_Color, i_UV, ivec2(1, 1), 0).rgb;

	vec3 topLeft = samples[0][0] + samples[1][0] + samples[0][1] + samples[1][1];
	vec3 topRight = samples[1][0] + samples[2][0] + samples[1][1] + samples[2][1];

	vec3 bottomLeft = samples[0][1] + samples[1][1] + samples[0][2] + samples[1][2];
	vec3 bottomRight = samples[1][1] + samples[2][1] + samples[1][2] + samples[2][2];

	vec3 center = centerTopLeft + centerTopRight + centerBottomLeft + centerBottomRight;

	if (u_ReduceDynamicRange)
	{
		topLeft = KarisAverage(topLeft);
		topRight = KarisAverage(topRight);
		bottomLeft = KarisAverage(bottomLeft);
		bottomRight = KarisAverage(bottomRight);
		center = KarisAverage(center);
	}

	o_Color = center * 0.125f + (topLeft + topRight + bottomLeft + bottomRight) * 0.03125f;

}

#end
