Culling = Front
DepthTest = true

Properties =
{
	u_InstanceData.Color = { Display = "Color", Type = HDR }
}

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 2) in mat4 i_Transform;

#include "Camera.glsl"

void main()
{
    gl_Position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);
}

#end

#begin pixel
#version 450

layout(std140, push_constant) uniform InstanceData
{
	vec4 Color;
} u_InstanceData;

layout(location = 0) out vec4 o_Color;

void main()
{
	o_Color = vec4(1.0);
}
#end
