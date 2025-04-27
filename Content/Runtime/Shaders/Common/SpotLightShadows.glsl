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
	float Near;
	float Far;
	float Bias;
};

layout(std430, set = 1, binding = 14) readonly buffer SpotLightShadowData
{
	SpotLightShadowsEntry Entries[];
} u_SpotLightShadowData;

float ComputeShadowBias(float projectedDepth, float near, float far, float bias)
{
	return -bias * near * far / (projectedDepth * (projectedDepth - bias) * (near - far));
}

float CalculateSpotLightShadow(vec3 spotLightPosition, vec3 surfaceNormal, vec3 position, uint lightIndex)
{
	SpotLightShadowsEntry shadowEntry = u_SpotLightShadowData.Entries[lightIndex];
	vec4 projected = shadowEntry.Projection * vec4(position, 1.0f);
	projected /= projected.w;

	vec2 uv = projected.xy * 0.5f + vec2(0.5f);
	if (any(lessThan(uv, vec2(0.0f))) || any(greaterThan(uv, vec2(1.0f))) || projected.z >= 1.0f)
		return 1.0f;

	float bias = ComputeShadowBias(projected.z, shadowEntry.Near, shadowEntry.Far, shadowEntry.Bias);
	float projectedDepth = projected.z - bias;

	uv.xy = shadowEntry.UVTransform.xy * uv.xy + shadowEntry.UVTransform.zw;
	return texture(u_SpotLightShadowMap, vec3(uv, projectedDepth));
}

vec3 ComputeShadowCastingSpotLightsContribution(vec3 V, in SurfaceProperties surface)
{
	vec3 finalContribution = vec3(0.0f);
	uint end = u_FirstShadowCastingSpotlight + u_ShadowCastingSpotlightCount;

	for (uint i = u_FirstShadowCastingSpotlight; i < end; i++)
	{
		SpotLightData spotLight = u_SpotLights[i];

		float shadow = CalculateSpotLightShadow(spotLight.Position, surface.Normal, surface.Position, i);

		finalContribution += CalculateSingleSpotLightContricbution(V, surface, i) * shadow;
	}

	return finalContribution;
}

#endif
