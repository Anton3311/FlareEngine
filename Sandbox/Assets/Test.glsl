Culling = Back
DepthTest = true

Properties =
{
	u_InstanceData.Color = {}
	u_InstanceData.Value = {}
	u_Texture = {}
}

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 2) in mat4 i_Transform;

layout(location = 0) out vec2 o_UV;

#include "Camera.glsl"

void main()
{
	o_UV = i_Position.xz;
    gl_Position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);
}

#end

#begin pixel
#version 450

layout(binding = 10) uniform sampler2D u_Texture;

layout(std140, push_constant) uniform InstanceData
{
	vec4 Color;
	float Value;
} u_InstanceData;

layout(location = 0) in vec2 i_UV;
layout(location = 0) out vec4 o_Color;

void main()
{
	vec4 color = texture(u_Texture, i_UV);
	o_Color = u_InstanceData.Color * u_InstanceData.Value * color;
}
#end
