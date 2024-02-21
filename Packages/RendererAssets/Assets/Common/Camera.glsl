struct CameraData
{
	vec3 Position;
	float Near;
	vec3 ViewDirection;
	float Far;

	mat4 Projection;
	mat4 View;
	mat4 ViewProjection;

	mat4 InverseProjection;
	mat4 InverseView;
	mat4 InverseViewProjection;

	ivec2 ViewportSize;
	float FOV;
};

layout(std140, binding = 0) uniform Camera
{
	CameraData u_Camera;
};
