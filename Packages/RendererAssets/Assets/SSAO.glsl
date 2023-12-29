DepthTest = false
DepthWrite = false

#begin vertex
#version 450

layout(location = 0) in vec2 i_Position;

layout(location = 0) out vec2 o_UV;

void main()
{
	gl_Position = vec4(i_Position, 0.0, 1.0);
	o_UV = i_Position / 2.0 + vec2(0.5);
}

#end

#begin pixel
#version 450

#include "Camera.glsl"

layout(location = 0) in vec2 i_UV;

layout(binding = 0) uniform sampler2D u_NormalsTexture;
layout(binding = 1) uniform sampler2D u_DepthTexure;

layout(std140, push_constant) uniform Params
{
	float Bias;
	float SampleRadius;
} u_Params;

layout(location = 0) out vec4 o_Color;

vec3 ReconstructWorldSpacePositionFromDepth(vec2 screenPosition, float depth)
{
	vec4 clipSpacePosition = vec4(screenPosition, depth * 2.0f - 1.0f, 1.0f);
	vec4 worldSpacePosition = u_Camera.InverseViewProjection * clipSpacePosition;
	return worldSpacePosition.xyz / worldSpacePosition.w;
}

const vec3[] RANDOM_VECTORS = 
{
	vec3(0.8307551671871758, 0.366047105457987, 0.47209442360044285),
	vec3(0.3494271669190782, -0.35913606883495625, 0.9451131246875603),
	vec3(9.718594137867516, -8.414752818747923, 5.088015420896713),
	vec3(0.38158420707063007, 0.8783220251591102, 0.5595656147592295),
	vec3(0.9650177012358351, 0.8301578635222607, 0.9166236686103235),
	vec3(-0.5482077436022013, -0.5352162098396661, 0.663479330040359),
	vec3(3.6143037900887514, -3.6573004687940975, 3.3000045158656532),
	vec3(0.1692641649926853, -0.8809682593789004, 0.5035157253106457),
};

const int SAMPLES_COUNT = 8;

void main()
{
	float depth = texture(u_DepthTexure, i_UV).r;
	vec3 worldSpacePosition = ReconstructWorldSpacePositionFromDepth(i_UV * 2.0f - vec2(1.0f), depth);
	vec3 normal = texture(u_NormalsTexture, i_UV).xyz * 2.0f - vec3(1.0f);

	vec3 tangent = cross(normal, vec3(0.0f, 1.0f, 0.0f));
	vec3 bitangent = cross(normal, tangent);

	mat3 TBN = mat3(tangent, bitangent, normal);

	float aoFactor = 0.0f;
	for (int i = 0; i < SAMPLES_COUNT; i++)
	{
		vec3 offset = TBN * (RANDOM_VECTORS[i] * u_Params.SampleRadius);
		vec4 projected = u_Camera.ViewProjection * vec4(worldSpacePosition + offset, 1.0f);
		projected.xyz /= projected.w;
		projected.xyz = projected.xyz / 2.0f + vec3(0.5f);

		float sampleDepth = texture(u_DepthTexure, projected.xy).r;
		float rangeCheck = smoothstep(0.0, 1.0, u_Params.SampleRadius / abs(depth - sampleDepth));
		if (projected.z > sampleDepth + u_Params.Bias)
			aoFactor += rangeCheck;
	}

	o_Color = vec4(vec3(1.0f - aoFactor / float(SAMPLES_COUNT)), 1.0);
}

#end
