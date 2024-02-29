layout(std140, binding = 0) uniform Camera
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
} u_Camera;

vec3 ReconstructWorldSpacePositionFromDepth(vec2 screenPosition, float depth)
{
	vec4 clipSpacePosition = vec4(screenPosition, depth * 2.0f - 1.0f, 1.0f);
	vec4 worldSpacePosition = u_Camera.InverseViewProjection * clipSpacePosition;
	return worldSpacePosition.xyz / worldSpacePosition.w;
}

vec3 ReconstructViewSpacePositionFromDepth(vec2 screenPosition, float depth)
{
	vec4 clipSpacePosition = vec4(screenPosition, depth * 2.0f - 1.0f, 1.0f);
	vec4 viewSpacePosition = u_Camera.InverseProjection * clipSpacePosition;
	return viewSpacePosition.xyz / viewSpacePosition.w;
}
