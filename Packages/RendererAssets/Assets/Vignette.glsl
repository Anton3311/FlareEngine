#begin vertex
#version 450

layout(location = 0) in vec2 i_Position;

layout(location = 0) out vec2 o_UV;

void main()
{
	gl_Position = vec4(i_Position, 0.0, 1.0);
	o_UV = i_Position;
}

#end

#begin pixel
#version 450

layout(std140, push_constant) uniform Params
{
	vec4 Color;
	float Radius;
	float Smoothness;
} u_Params;

layout(location = 0) in vec2 i_UV;
layout(location = 0) out vec4 o_Color;

void main()
{
	float alpha = smoothstep(u_Params.Radius, u_Params.Radius + u_Params.Smoothness, length(i_UV));
	o_Color = vec4(u_Params.Color.rgb, u_Params.Color.a * alpha);
}

#end
