#type vertex
#version 450

#include "Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in mat4 i_Transform;

layout(std140, push_constant) uniform InstanceData
{
	vec4 Color;
	float Roughness;
} u_InstanceData;

struct VertexData
{
	vec3 Normal;
	vec4 Color;
	float Roughness;
};

layout(location = 0) out VertexData o_Vertex;

void main()
{
	mat4 normalTransform = transpose(inverse(i_Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;
	o_Vertex.Color = u_InstanceData.Color;
	o_Vertex.Roughness = u_InstanceData.Roughness;

	vec4 position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);

    gl_Position = position;
}

#type fragment
#version 450

#include "Camera.glsl"

struct VertexData
{
	vec3 Normal;
	vec4 Color;
	float Roughness;
};

layout(location = 0) in VertexData i_Vertex;
layout(location = 0) out vec4 o_Color;

void main()
{
	o_Color = i_Vertex.Color;
}
