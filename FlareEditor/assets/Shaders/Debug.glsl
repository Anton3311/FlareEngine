#type vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;

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

out vec4 VertexColor;

void main()
{
	VertexColor = i_Color;
	gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
}

#type fragment
#version 450

in vec4 VertexColor;

out vec4 o_Color;

void main()
{
	o_Color = VertexColor;
}
