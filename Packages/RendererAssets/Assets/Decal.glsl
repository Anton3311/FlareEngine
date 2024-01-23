DepthTest = false

Properties = 
{
	u_ColorTexture = {}
}

#begin vertex
#version 450

#include "Common/Camera.glsl"
#include "Common/Instancing.glsl"

layout(location = 0) in vec3 i_Position;

layout(location = 0) out flat int o_EntityIndex;
layout(location = 1) out flat mat4 o_ViewToObjectSpace;

void main()
{
	mat4 transform = GetInstanceTransform();

	gl_Position = u_Camera.ViewProjection * transform * vec4(i_Position, 1.0f);

	o_EntityIndex = u_InstancesData.Data[gl_InstanceIndex].EntityIndex;
	o_ViewToObjectSpace = inverse(transform) * u_Camera.InverseViewProjection;
}

#end

#begin pixel
#version 450

#include "Common/Camera.glsl"

layout(binding = 0) uniform sampler2D u_ColorTexture;
layout(binding = 1) uniform sampler2D u_DepthTexture;

layout(location = 0) in flat int i_EntityIndex;
layout(location = 1) in flat mat4 i_ViewToObjectSpace;

layout(location = 0) out vec4 o_Color;
layout(location = 2) out int o_EntityIndex;

void main()
{
	vec2 depthUV = vec2(gl_FragCoord.xy) / vec2(u_Camera.ViewportSize);
	float depth = texture(u_DepthTexture, depthUV).r;
	
	vec4 clipSpacePosition = vec4(depthUV * 2.0f - vec2(1.0f), depth * 2.0f - 1.0f, 1.0f);
	vec4 objectSpacePosition= i_ViewToObjectSpace * clipSpacePosition;
	objectSpacePosition.xyz /= objectSpacePosition.w;

	if (abs(objectSpacePosition.x) > 0.5f || abs(objectSpacePosition.y) > 0.5f || abs(objectSpacePosition.z) > 0.5f)
		discard;

	o_Color = texture(u_ColorTexture, objectSpacePosition.xz);
	o_EntityIndex = i_EntityIndex;
}

#end
