DepthTest = false
DepthWrite = false

#begin vertex
#version 450

layout(location = 0) in vec2 i_Position;

layout(location = 0) out vec2 o_UV;

void main()
{
	gl_Position = vec4(i_Position, 0.0, 1.0);
	o_UV = i_Position / 2.0 + vec2(0.5);
}

#end

#begin pixel
#version 450

layout(location = 0) in vec2 i_UV;

layout(binding = 0) uniform sampler2D u_AOTexture;
layout(binding = 1) uniform sampler2D u_ColorTexture;

layout(location = 0) out vec4 o_Color;

layout(std140, push_constant) uniform Params
{
	vec2 TexelSize;
	float BlurSize;
} u_Params;

void main()
{
	vec3 color = texture(u_ColorTexture, i_UV).rgb;

	float ao = 0;
	for (float y = -1.0f; y <= 1.0f; y++)
	{
		for (float x = -1.0f; x <= 1.0f; x++)
		{
			vec2 offset = vec2(x, y) * u_Params.BlurSize * u_Params.TexelSize;
			ao += texture(u_AOTexture, offset + i_UV).r;
		}
	}

	o_Color = vec4(color * ao / 9.0f, 1.0);
}

#end