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
layout(set = 3, binding = 1) uniform sampler2D u_PreviousMip;

layout(location = 0) in vec2 i_UV;

layout(location = 0) out vec4 o_Color;

layout(std140, push_constant) uniform Constants
{
	float u_Radius;
	float u_AspectRatio;
};

const float FILTER_WEIGHTS[3][3] =
{
	{ 1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f },
	{ 2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f },
	{ 1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f },
};

void main()
{
	vec3 finalColor = vec3(0.0f);
	for (int yIndex = 0; yIndex < 3; yIndex++)
	{
		for (int xIndex = 0; xIndex < 3; xIndex++)
		{
			vec2 offset = vec2(float(xIndex - 1), float(yIndex - 1));
			offset.x /= u_AspectRatio;

			vec2 sampleUV = i_UV + offset * u_Radius;
			finalColor += texture(u_Color, sampleUV).rgb * FILTER_WEIGHTS[yIndex][xIndex];
		}
	}

	o_Color = vec4(finalColor + texture(u_PreviousMip, i_UV).rgb, 1.0f);
}

#end
