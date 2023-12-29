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
	vec3(0.40636955683783693, -0.32225178565910606, 0.9687690070046615),
	vec3(1.8294227025319691, 2.3612833160558186, 1.3974106610868675),
	vec3(-0.4565198813802882, -1.0312994760557594, 0.1845851912978406),
	vec3(1.986230712460333, -0.44215256598148805, 2.9899495989379816),
	vec3(-1.0741159103074374, -0.19367617003838744, 2.011362137559878),
	vec3(1.222501985754348, -2.771981964606179, 2.7468334854703866),
	vec3(-0.26110405258621006, -1.0109680765968878, 0.1593568851359476),
	vec3(-0.8758363380675994, -1.1299983463835663, 0.5880219902902034),
	vec3(1.9962415932614566, 0.627131604843255, 0.8183897870266011),
	vec3(5.003835472362996, -3.1797444809531203, 1.7290124055705396),
	vec3(0.8522821181090796, -0.7116103055661687, 0.4589231816748502),
	vec3(-2.1859342935051536, 0.7568147773087225, 0.16902066232180035),
	vec3(-0.8771823199124096, 0.49209199482988625, 1.3114859792638351),
	vec3(-0.8169852264137277, -0.6274795585718771, 0.5493707280514889),
	vec3(-1.570921636954846, -0.3831998984805417, 0.9237872225582945),
	vec3(2.8610599687010247, -1.4166415178307539, 2.746448799003694)
};

const int SAMPLES_COUNT = 16;

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
