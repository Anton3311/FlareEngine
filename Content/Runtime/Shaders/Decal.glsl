Type = Decal
DepthTest = false

Properties = 
{
	u_Material.Color = { Type = Color }
	u_ColorTexture = {}
}

#begin vertex
#version 450

#include "Common/Camera.glsl"
#include "Common/Instancing.glsl"

layout(location = 0) in vec3 i_Position;

layout(location = 0) out flat mat4 o_ViewToObjectSpace;

void main()
{
	mat4 transform = GetInstanceTransform();

	gl_Position = u_Camera.ViewProjection * transform * vec4(i_Position, 1.0f);

	o_ViewToObjectSpace = inverse(transform) * u_Camera.InverseViewProjection;
}

#end

#begin pixel
#version 450

#include "Common/Camera.glsl"

layout(set = 1, binding = 0) uniform sampler2D u_DepthTexture;
layout(set = 2, binding = 0) uniform sampler2D u_ColorTexture;

layout(location = 0) in flat mat4 i_ViewToObjectSpace;

layout(location = 0) out vec4 o_Color;

layout(std140, push_constant) uniform MaterialData
{
	vec4 Color;
} u_Material;

void main()
{
	vec2 depthUV = vec2(gl_FragCoord.xy) / vec2(u_Camera.ViewportSize);
	float depth = texture(u_DepthTexture, depthUV).r;
	
	vec4 clipSpacePosition = vec4(depthUV * 2.0f - vec2(1.0f), depth, 1.0f);
	vec4 objectSpacePosition= i_ViewToObjectSpace * clipSpacePosition;
	objectSpacePosition.xyz /= objectSpacePosition.w;

	if (abs(objectSpacePosition.x) > 0.5f || abs(objectSpacePosition.y) > 0.5f || abs(objectSpacePosition.z) > 0.5f)
		discard;

	vec4 color = texture(u_ColorTexture, objectSpacePosition.xz) * u_Material.Color;
	if (color.a == 0)
		discard;

	o_Color = color;
}

#end
