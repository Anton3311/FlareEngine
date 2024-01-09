#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;

#include "Camera.glsl"

struct InstanceData
{
	mat4 Transform;
	int EntityIndex;
};

layout(std140, binding = 3) buffer InstacesData
{
	InstanceData Data[];
} u_InstancesData;

struct VertexData
{
	vec3 Normal;
};

layout(location = 0) out VertexData o_Vertex;

void main()
{
	mat4 normalTransform = transpose(inverse(u_InstancesData.Data[gl_InstanceIndex].Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;

    gl_Position = u_Camera.ViewProjection * u_InstancesData.Data[gl_InstanceIndex].Transform * vec4(i_Position, 1.0);
}

#end

#begin pixel
#version 450

#include "Camera.glsl"

struct VertexData
{
	vec3 Normal;
};

layout(location = 0) in VertexData i_Vertex;
layout(location = 0) out vec4 o_Color;


void main()
{
	o_Color = vec4(i_Vertex.Normal, 1.0);
}

#end
