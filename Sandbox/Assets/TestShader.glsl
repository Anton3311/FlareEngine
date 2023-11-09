#begin vertex
#version 450

#include "Camera.glsl"

layout(std140, binding = 0) uniform Camera
{
	CameraData u_Camera;
};

layout(location = 0) in vec3 i_Position;
layout(location = 2) in vec2 i_UV;
layout(location = 4) in int i_EntityIndex;

layout(location = 0) out vec2 UV;
layout(location = 1) out flat int EntityIndex;

void main()
{
	gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
	UV = i_UV;
	EntityIndex = i_EntityIndex;
}

#end

#begin pixel
#version 450

layout(std140, push_constant) uniform Constants
{
	vec4 Color;
} Consts;

layout(location = 0) in vec2 UV;
layout(location = 1) in flat int EntityIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityIndex;

void main()
{
	float dist = length((UV - vec2(0.5)) * 2.0);
	float alpha = 1.0 - smoothstep(0.98, 1.0, dist);

	o_EntityIndex = EntityIndex;

	if (alpha <= 0.0001)
		discard;

	o_Color = vec4(Consts.Color.rgb, alpha);
}

#end
