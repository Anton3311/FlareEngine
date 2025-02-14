#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;

layout(location = 0) out vec2 o_UV;

void main()
{
	gl_Position = vec4(i_Position, 1.0f);
	o_UV = i_Position.xy * 0.5f + vec2(0.5f);
}

#end

#begin pixel
#version 450

#include "../Common/Camera.glsl"

layout(set = 3, binding = 0) uniform sampler2D u_DepthTexture;

layout(location = 0) in vec2 i_UV;

layout(location = 0) out vec4 o_LinearDepth;

#if 1
vec3 MinDifference(vec3 position, vec3 left, vec3 right)
{
	vec3 leftDirection = position - left;
	vec3 rightDirection = right - position;

	float leftDistance = dot(leftDirection, leftDirection);
	float rightDistance = dot(rightDirection, rightDirection);

	return (leftDistance < rightDistance) ? leftDirection : rightDirection;
}

vec3 GetVSPosition(vec2 uv)
{
	ivec2 textureSize = textureSize(u_DepthTexture, 0);
	vec2 u_InverseDepthTextureSize = vec2(1.0f) / vec2(textureSize);

	float linearDepth = texture(u_DepthTexture, uv).r;

	uv = uv * 2.0f - vec2(1.0f);

	vec4 clipSpacePos = vec4(uv, linearDepth, 1.0f);
	vec4 VSPosition = u_Camera.InverseProjection * clipSpacePos;
	return VSPosition.xyz / VSPosition.w;
}

vec3 FetchVSPosition(ivec2 texelPosition)
{
	float depth = texelFetch(u_DepthTexture, texelPosition, 0).r;

	ivec2 textureSize = textureSize(u_DepthTexture, 0);
	vec2 uv = vec2(texelPosition) / vec2(textureSize) * 2.0f - vec2(1.0f);

	vec4 clipSpacePos = vec4(uv, depth, 1.0f);
	vec4 VSPosition = u_Camera.InverseProjection * clipSpacePos;

	return VSPosition.xyz / VSPosition.w;
}
#endif

void main()
{
	ivec2 textureSize = textureSize(u_DepthTexture, 0);
	vec2 texelSize = vec2(1.0f) / vec2(textureSize);

	float depth = texture(u_DepthTexture, i_UV).r;
	float linearDepth = LinearizeDepth(depth, u_Camera.Near, u_Camera.Far);

#if 0
	vec2 uv = i_UV;
	vec3 VSPosition = GetVSPosition(uv);
#if 1
	vec3 left = GetVSPosition(uv + vec2(-texelSize.x, 0.0f));
	vec3 right = GetVSPosition(uv + vec2(+texelSize.x, 0.0f));

	vec3 top = GetVSPosition(uv + vec2(0.0f, +texelSize.y));
	vec3 bottom = GetVSPosition(uv + vec2(0.0f, -texelSize.y));
#else
	ivec2 texelPosition = ivec2(gl_FragCoord.xy * 2);

	vec3 left = FetchVSPosition(texelPosition - ivec2(1, 0));
	vec3 right = FetchVSPosition(texelPosition + ivec2(1, 0));

	vec3 top = FetchVSPosition(texelPosition + ivec2(0, 1));
	vec3 bottom = FetchVSPosition(texelPosition - ivec2(0, 1));
#endif

	vec3 du = MinDifference(VSPosition, left, right);
	vec3 dv = MinDifference(VSPosition, bottom, top);

	vec3 normalVS = normalize(cross(du, dv));
#endif

#if 1
	o_LinearDepth = vec4(vec3(depth), 1.0f);
#else
	o_LinearDepth = vec4(normalVS, 1.0f);
#endif
}

#end
