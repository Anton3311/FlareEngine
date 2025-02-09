#ifndef LIGHT_H
#define LIGHT_H

#include "BRDF.glsl"

layout(set = 1, binding = 12) uniform sampler2D u_AO;

layout(std140, set = 1, binding = 1) uniform LightData
{
	vec4 u_LightColor;
	vec3 u_LightDirection;
	float u_LightNear;

	vec4 u_EnvironmentLight;

	uint u_PointLightsCount;
	uint u_SpotLightsCount;

	uint u_FirstShadowCastingSpotlight;
	uint u_ShadowCastingSpotlightCount;

	bool u_AOEnabled;
};

struct PointLightData
{
	vec3 Position;
	vec4 Color;
};

struct SpotLightData
{
	vec3 Position;
	float InnerrRadiusCos;
	vec3 Direction;
	float OuterRadiusCos;
	vec4 Color;
};

layout(std140, set = 1, binding = 2) buffer PointLightsData
{
	PointLightData[] u_PointLights;
};

layout(std140, set = 1, binding = 3) buffer SpotLightsData
{
	SpotLightData[] u_SpotLights;
};

struct SurfaceProperties
{
	vec3 Position;
	vec3 Color;
	float Roughness;
	vec3 Normal;
	float Metallic;
};

vec3 CalculateLight(vec3 V, vec3 H, vec3 incomingLight, vec3 lightDirection, in SurfaceProperties surface)
{
	float alpha = max(0.04, surface.Roughness * surface.Roughness);
	vec3 F0 = mix(BASE_REFLECTIVITY, surface.Color, surface.Metallic);

	vec3 kS = Fresnel_Shlick(F0, V, H);
	vec3 kD = vec3(1.0) - kS;

	vec3 diffuse = Diffuse_Lambertian(surface.Color);
	vec3 specular = Specular_CookTorence(alpha, surface.Normal, V, lightDirection);
	vec3 brdf = kD * diffuse + specular;

	return brdf * incomingLight * max(0.0, dot(lightDirection, surface.Normal));
}

vec3 CalculatePointLightsContribution(vec3 V, in SurfaceProperties surface)
{
	vec3 finalColor = vec3(0.0);
	for (uint i = 0; i < u_PointLightsCount; i++)
	{
		vec3 direction = u_PointLights[i].Position - surface.Position;
		float distance = length(direction);
		float attenuation = 1.0f / (distance * distance);

		direction /= distance;

		vec3 halfWayVector = normalize(V + direction);
		vec3 incomingLight = u_PointLights[i].Color.rgb * u_PointLights[i].Color.w;

		finalColor += CalculateLight(V, halfWayVector, incomingLight * attenuation, direction, surface);
	}

	return finalColor;
}

vec3 CalculateSingleSpotLightContricbution(vec3 V, in SurfaceProperties surface, uint lightIndex)
{
	SpotLightData spotLight = u_SpotLights[lightIndex];

	vec3 direction = spotLight.Position - surface.Position;
	float distance = length(direction);
	float attenuation = 1.0f / (distance * distance);

	direction /= distance;

	vec3 halfWayVector = normalize(V + direction);

	float angleCos = dot(direction, spotLight.Direction);
	float fade = 1.0f - smoothstep(spotLight.InnerrRadiusCos, spotLight.OuterRadiusCos, angleCos);

	vec3 incomingLight = spotLight.Color.rgb * spotLight.Color.w * fade;
	return CalculateLight(V, halfWayVector, incomingLight * attenuation, direction, surface);
}

vec3 CalculateSpotLightsContribution(vec3 V, in SurfaceProperties surface)
{
	vec3 finalColor = vec3(0.0);
	for (uint i = 0; i < u_SpotLightsCount; i++)
	{
		finalColor += CalculateSingleSpotLightContricbution(V, surface, i);
	}

	return finalColor;
}

float SampleAO(ivec2 pixelPosition)
{
	return u_AOEnabled ? (texelFetch(u_AO, pixelPosition, 0).r) : 1.0f;
}

#endif
