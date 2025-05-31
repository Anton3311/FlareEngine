#ifndef SPOT_LIGHT_SHADOWS_H
#define SPOT_LIGHT_SHADOWS_H

#include "ShadowMapping.glsl"
#include "Light.glsl"

layout(set = 1, binding = 13) uniform sampler2DShadow u_SpotLightShadowMap;

struct SpotLightShadowsEntry
{
	vec4 UVTransform; // xy - scale, zw - translation
	mat4 Projection;
	float Radius;
	float Bias;
	float NormalBias;
};

layout(std430, set = 1, binding = 14) readonly buffer SpotLightShadowData
{
	SpotLightShadowsEntry Entries[];
} u_SpotLightShadowData;

float ComputeShadowBias(float projectedDepth, float near, float far, float bias)
{
	return -bias * near * far / (projectedDepth * (projectedDepth - bias) * (near - far));
}

float CalculateSpotLightShadow(vec3 spotLightPosition,
	vec3 surfaceNormal,
	vec3 position,
	uint lightIndex,
	vec3 directionFromLight)
{
	SpotLightShadowsEntry shadowEntry = u_SpotLightShadowData.Entries[lightIndex - u_FirstShadowCastingSpotlight];
	vec4 projected = shadowEntry.Projection * vec4(position, 1.0f);
	projected /= projected.w;

	vec2 uv = projected.xy * 0.5f + vec2(0.5f);
	if (any(lessThan(uv, vec2(0.0f))) || any(greaterThan(uv, vec2(1.0f))) || projected.z >= 1.0f)
		return 1.0f;

	float NoL = dot(-directionFromLight, surfaceNormal);
	float bias = max(shadowEntry.NormalBias * (1.0f - NoL), 0.0f) + shadowEntry.Bias;

	float potentialOccluderDepth = FindPotentialOccluder(uv,
		shadowEntry.Projection,
		position,
		surfaceNormal,
		bias,
		directionFromLight,
		spotLightPosition,
		false);

	uv.xy = shadowEntry.UVTransform.xy * uv.xy + shadowEntry.UVTransform.zw;
	return texture(u_SpotLightShadowMap, vec3(uv, potentialOccluderDepth));
}

vec3 ComputeShadowCastingSpotLightsContribution(vec3 V, in SurfaceProperties surface)
{
	vec3 finalContribution = vec3(0.0f);
	uint end = u_FirstShadowCastingSpotlight + u_ShadowCastingSpotlightCount;

	for (uint i = u_FirstShadowCastingSpotlight; i < end; i++)
	{
		SpotLightData spotLight = u_SpotLights[i];

		vec3 directionFromLight = surface.Position - spotLight.Position;
		float shadow = CalculateSpotLightShadow(spotLight.Position,
			surface.Normal,
			surface.Position,
			i, normalize(directionFromLight));

		finalContribution += CalculateSingleSpotLightContricbution(V, surface, i) * shadow;
	}

	return finalContribution;
}

#endif
