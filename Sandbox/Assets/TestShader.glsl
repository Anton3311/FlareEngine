#type vertex
#version 450

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

layout(location = 0) in vec3 i_Position;

void main()
{
	gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
}

#type fragment
#version 450

layout(std140, push_constant) uniform Constants
{
	vec4 Color;
} Consts;

layout(location = 0) out vec4 o_Color;

void main()
{
	o_Color = Consts.Color;
}
