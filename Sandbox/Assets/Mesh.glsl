#type vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;

#include "Camera.glsl"

layout(std140, push_constant) uniform InstanceData
{
	mat4 Transform;
	vec4 Color;
} u_InstanceData;

struct VertexData
{
	vec3 Normal;
	vec4 Color;
};

layout(location = 0) out VertexData o_Vertex;

void main()
{
	mat4 normalTransform = transpose(inverse(u_InstanceData.Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;
	o_Vertex.Color = u_InstanceData.Color;
    gl_Position = u_Camera.ViewProjection * u_InstanceData.Transform * vec4(i_Position, 1.0);
}

#type fragment
#version 450

layout(std140, binding = 1) uniform DirLight
{
	vec4 u_LightColor;
	vec3 u_LightDirection;
};

struct VertexData
{
	vec3 Normal;
	vec4 Color;
};

layout(location = 0) in VertexData i_Vertex;
layout(location = 0) out vec4 o_Color;

void main()
{
	vec3 normal = normalize(i_Vertex.Normal);
	vec3 diffuseColor = i_Vertex.Color.rgb * u_LightColor.rgb * u_LightColor.w;
	o_Color = vec4(diffuseColor * max(0.0, dot(u_LightDirection, normal)), i_Vertex.Color.a);
}
