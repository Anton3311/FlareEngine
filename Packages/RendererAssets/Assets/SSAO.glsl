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
layout(binding = 2) uniform sampler2D u_RandomVectors;

layout(std140, push_constant) uniform Params
{
	float Bias;
	float SampleRadius;
	vec2 NoiseScale;
} u_Params;

layout(location = 0) out vec4 o_Color;

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

const vec3[] RANDOM_VECTORS = 
{
	vec3(0.2701420796672004, -0.16259540000848002, 0.14812016705548542),
	vec3(-0.29048289380746367, 0.22307651233002976, 0.06497384172182932),
	vec3(0.058883346362995574, -0.22294025027645392, 0.09686273442760837),
	vec3(0.06692157068902395, -0.34362418702022024, 0.14927095145553365),
	vec3(-0.16390731640895612, -0.14369652440860842, 0.045185039305914866),
	vec3(0.05691037792560343, -0.2609889031713489, 0.18884381785457055),
	vec3(-0.7235091568876681, 0.345426230040939, 0.0019103741191988425),
	vec3(0.043712800235472, 0.033724546160143094, 0.040889340305886986),
	vec3(-0.3178434227062447, -0.14312449449793735, 0.05905526934599541),
	vec3(-0.09529351953754467, -0.21416338438538018, 0.04426958078246395),
	vec3(-0.33116130122859977, 0.5923736046998866, 0.4941597460765092),
	vec3(-0.5793342101387434, -0.35019345595183804, 0.4136550115620087),
	vec3(0.034995080774583215, 0.34896247442824213, 0.7065582751974434),
	vec3(0.12961814597406235, 0.099584814107574, 0.1107984233903666),
	vec3(-0.24310780429748582, -0.5801881090519327, 0.5612803401610399),
	vec3(-0.083850823850024, 0.13994988724867344, 0.2713956194382487),
};

const int SAMPLES_COUNT = 16;

void main()
{
	float depth = texture(u_DepthTexure, i_UV).r;
	vec3 viewSpacePosition = ReconstructViewSpacePositionFromDepth(i_UV * 2.0f - vec2(1.0f), depth);

	mat4 worldNormalToView = transpose(u_Camera.InverseView);

	vec3 worldSpaceNormal = normalize(texture(u_NormalsTexture, i_UV).xyz * 2.0f - vec3(1.0f));
	vec3 normal = (worldNormalToView * vec4(worldSpaceNormal, 1.0)).xyz;
	vec3 randomVector = texture(u_RandomVectors, i_UV * u_Params.NoiseScale).xyz;
	randomVector.xy = randomVector.xy * 2.0f - vec2(1.0f);

	randomVector = normalize(randomVector);

	vec3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
	vec3 bitangent = cross(normal, tangent);

	mat3 tbn = mat3(tangent, bitangent, normal);

	float aoFactor = 0.0f;
	for (int i = 0; i < SAMPLES_COUNT; i++)
	{
		vec3 samplePosition = viewSpacePosition + tbn * (RANDOM_VECTORS[i] * u_Params.SampleRadius);
		vec4 projected = u_Camera.Projection * vec4(samplePosition, 1.0);
		projected.xyz /= projected.w;
		
		vec2 uv = projected.xy * 0.5f + vec2(0.5f);
		float sampleDepth = texture(u_DepthTexure, uv).r;
		vec3 sampleViewSpace = ReconstructViewSpacePositionFromDepth(uv, sampleDepth);

		float rangeCheck = smoothstep(0.0, 1.0, u_Params.SampleRadius / abs(sampleViewSpace.z - samplePosition.z));
		if (samplePosition.z + u_Params.Bias <= sampleViewSpace.z)
			aoFactor += rangeCheck;
	}

	o_Color = vec4(vec3(1.0f - aoFactor / float(SAMPLES_COUNT)), 1.0);
}

#end
