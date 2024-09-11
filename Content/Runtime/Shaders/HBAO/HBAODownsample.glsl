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

layout(location = 0) out float o_LinearDepth;

void main()
{
	float depth = texture(u_DepthTexture, i_UV).r;
	float linearDepth = LinearizeDepth(depth, u_Camera.Near, u_Camera.Far);

	o_LinearDepth = linearDepth;
}

#end
