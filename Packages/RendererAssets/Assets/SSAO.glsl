DepthTest = false

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

#include "Common/Camera.glsl"
#include "Common/Math.glsl"

layout(location = 0) in vec2 i_UV;

layout(binding = 0) uniform sampler2D u_NormalsTexture;
layout(binding = 1) uniform sampler2D u_DepthTexture;

layout(std140, push_constant) uniform Params
{
	float Bias;
	float SampleRadius;
	vec2 NoiseScale;
} u_Params;

layout(location = 0) out vec4 o_Color;

const vec3[] RANDOM_VECTORS = 
{
	vec3(-0.32830159515586244, -0.5659756934644117, 0.18806013865195345),
	vec3(-0.4465635062735347, 0.2579993447467218, 0.7279607998464577),
	vec3(0.304332749930374, 0.30976256436359373, 0.5135590529250091),
	vec3(-0.15128878659992248, -0.08828689834084624, 0.22414899035747424),
	vec3(0.3028487602163618, 0.12821095596802518, 0.5595310390583169),
	vec3(0.19662413887257293, 0.17548092269332788, 0.03780357646602708),
	vec3(0.15408213039715665, -0.2688993572571084, 0.43020078010230556),
	vec3(-0.1799459131466272, -0.4041520412254783, 0.028206220496327216),
	vec3(-0.049972621335896264, -0.08712259590750068, 0.049464826160264694),
	vec3(0.05893827025365321, -0.8811405588247998, 0.2800080982064954),
	vec3(0.20404991132045303, 0.3764613303980451, 0.08024874513309072),
	vec3(-0.11132449077585153, -0.12520962163100177, 0.07490508507233512),
	vec3(0.26312491785104736, -0.049557374293251744, 0.2059565395548251),
	vec3(0.33456474503711886, 0.35425366671690617, 0.3274672202693679),
	vec3(0.13919347344746355, 0.28379020040501185, 0.384424063745891),
	vec3(-0.046311766621635726, 0.1885066735933092, 0.1376767239610456),
	vec3(0.044920209904863714, 0.18543807149403047, 0.15751214300781977),
	vec3(-0.11368147614465629, -0.028662159275677727, 0.005189818336276572),
	vec3(-0.5925498478715127, -0.49752390658872586, 0.4152325931733292),
	vec3(0.5900340531909183, -0.5574801175679978, 0.5700064151776859),
	vec3(0.5185900979995376, 0.29528879240201356, 0.5521965418412842),
	vec3(-0.0014242920879751317, 0.0031706818576986806, 0.003819974195405997),
	vec3(0.47608258814028925, -0.5386513130772225, 0.5687281619530685),
	vec3(0.00024469063577047293, 0.0013420844316311617, 0.001000465199831852),
	vec3(0.3985758161340294, -0.50918064221192, 0.5404378357665571),
	vec3(0.3727806855258153, 0.09418129732765709, 0.1591035828580539),
	vec3(0.3683471953609148, 0.34614301294760297, 0.6971910721382938),
	vec3(0.0327440215994156, 0.14072620502544989, 0.25035001893165193),
	vec3(-0.4911624277812166, -0.17246988989715084, 0.09859245608493722),
	vec3(-0.013925161443861338, 0.08319067059003313, 0.06282406425959092),
	vec3(0.3508417729352578, 0.040505327778209886, 0.23807991540712356),
	vec3(0.3204879598135104, 0.025779750036101525, 0.07015850178948604),
};

const int SAMPLES_COUNT = 32;

void main()
{
	float depth = texture(u_DepthTexture, i_UV).r;
	vec3 viewSpacePosition = ReconstructViewSpacePositionFromDepth(i_UV * 2.0f - vec2(1.0f), depth);

	mat4 worldNormalToView = transpose(u_Camera.InverseView);
	vec3 sampledNormal = texture(u_NormalsTexture, i_UV).xyz;

	if (length(sampledNormal) < 0.01)
	{
		o_Color = vec4(1.0f); // No AO
	 	return;
	}

	vec3 worldSpaceNormal = normalize(sampledNormal * 2.0f - vec3(1.0f));
	vec3 normal = (worldNormalToView * vec4(worldSpaceNormal, 1.0)).xyz;

	float angle = 2.0f * PI * InterleavedGradientNoise(gl_FragCoord.xy);
	vec3 randomVector = vec3(cos(angle), sin(angle), 0.0f);
	randomVector = normalize(randomVector.xyz * 2.0f - vec3(1.0f));

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
		float sampleDepth = texture(u_DepthTexture, uv).r;
		vec3 sampleViewSpace = ReconstructViewSpacePositionFromDepth(uv, sampleDepth);

		float rangeCheck = smoothstep(0.0, 1.0, u_Params.SampleRadius / abs(sampleViewSpace.z - samplePosition.z));
		if (samplePosition.z + u_Params.Bias <= sampleViewSpace.z)
			aoFactor += rangeCheck;
	}

	o_Color = vec4(vec3(1.0 - aoFactor / float(SAMPLES_COUNT)), 1.0);
}

#end
