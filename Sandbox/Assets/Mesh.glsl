#type vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;

struct CameraData
{
	vec3 Position;

	mat4 Projection;
	mat4 View;
	mat4 ViewProjection;

	mat4 InverseProjection;
	mat4 InverseView;
	mat4 InverseViewProjection;
};

layout(std140, binding = 0) uniform Camera
{
	CameraData u_Camera;
};

struct VertexData
{
	vec3 Normal;
};

layout(location = 0) out VertexData o_Vertex;

void main()
{
	o_Vertex.Normal = i_Normal;
    gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
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
};

layout(location = 0) in VertexData i_Vertex;
layout(location = 0) out vec4 o_Color;

layout(std140, push_constant) uniform Input
{
	vec4 Color;
} u_Input;

void main()
{
	vec3 normal = normalize(i_Vertex.Normal);
	vec3 diffuseColor = u_Input.Color.rgb * u_LightColor.rgb * u_LightColor.w;
	o_Color = vec4(diffuseColor * max(0.0, dot(u_LightDirection, normal)), u_Input.Color.a);
}
