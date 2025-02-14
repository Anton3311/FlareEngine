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

#include "../Common/Camera.glsl"

layout(set = 3, binding = 0) uniform sampler2D u_DepthTexture;

layout(location = 0) in vec2 i_UV;
layout(location = 0) out vec4 o_LinearDepth;

void main()
{
	float linearDepth = LinearizeDepth(texture(u_DepthTexture, i_UV).r, u_Camera.Near, u_Camera.Far);
	o_LinearDepth = vec4(vec3(linearDepth), 1.0f);
}

#end
