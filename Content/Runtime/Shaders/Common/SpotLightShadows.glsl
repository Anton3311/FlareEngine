#ifndef SPOT_LIGHT_SHADOWS_H
#define SPOT_LIGHT_SHADOWS_H

#include "ShadowMapping.glsl"
#include "Light.glsl"

layout(set = 1, binding = 13) uniform sampler2DShadow u_SpotLightShadowMap;

struct SpotLightShadowsEntry
{
	vec4 UVTransform; // xy - scale, zw - translation
	mat4 Projection;
};

layout(std430, set = 1, binding = 14) readonly buffer SpotLightShadowData
{
	SpotLightShadowsEntry Entries[];
} u_SpotLightShadowData;

float CalculateSpotLightShadow(vec3 spotLightPosition, vec3 surfaceNormal, vec3 position, uint lightIndex)
{
	vec3 directionTowardsLight = normalize(spotLightPosition - position);
	float NoL = dot(surfaceNormal, directionTowardsLight);

	vec3 positionBias = (NoL < 0.0f) ? (-directionTowardsLight * 0.2f) : (directionTowardsLight * 0.2f);

	position += positionBias;

	SpotLightShadowsEntry shadowEntry = u_SpotLightShadowData.Entries[lightIndex];
	vec4 projected = shadowEntry.Projection * vec4(position, 1.0f);
	projected /= projected.w;

	vec2 uv = projected.xy * 0.5f + vec2(0.5f);
	float projectedDepth = projected.z;

	if (any(lessThan(uv, vec2(0.0f))) || any(greaterThan(uv, vec2(1.0f))))
		return 1.0f;

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
