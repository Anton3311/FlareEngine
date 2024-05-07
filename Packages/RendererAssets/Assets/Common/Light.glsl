#ifndef LIGHT_H
#define LIGHT_H

#include "BRDF.glsl"

layout(std140, set = 0, binding = 1) uniform LightData
{
	vec4 u_LightColor;
	vec3 u_LightDirection;
	float u_LightNear;

	vec4 u_EnvironmentLight;

	uint u_PointLightsCount;
	uint u_SpotLightsCount;
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

layout(std140, set = 0, binding = 4) buffer PointLightsData
{
	PointLightData[] Lights;
} u_PointLights;

layout(std140, set = 0, binding = 5) buffer SpotLightsData
{
	SpotLightData[] Lights;
} u_SpotLights;

vec3 CalculateLight(vec3 N, vec3 V, vec3 H, vec3 color, vec3 incomingLight, vec3 lightDirection, float roughness)
{
	float alpha = max(0.04, roughness * roughness);

	vec3 kS = Fresnel_Shlick(BASE_REFLECTIVITY, V, H);
	vec3 kD = vec3(1.0) - kS;

	vec3 diffuse = Diffuse_Lambertian(color);
	vec3 specular = Specular_CookTorence(alpha, N, V, lightDirection);
	vec3 brdf = kD * diffuse + specular;

	return brdf * incomingLight * max(0.0, dot(lightDirection, N));
}

vec3 CalculatePointLightsContribution(vec3 N, vec3 V, vec3 H, vec3 color, vec3 position, float roughness)
{
	vec3 finalColor = vec3(0.0);
	for (uint i = 0; i < u_PointLightsCount; i++)
	{
		vec3 direction = u_PointLights.Lights[i].Position - position;
		float distance = length(direction);
		float attenuation = 1.0f / (distance * distance);

		vec3 incomingLight = u_PointLights.Lights[i].Color.rgb * u_PointLights.Lights[i].Color.w;

		finalColor += CalculateLight(N, V, H, color,
			incomingLight * attenuation,
			direction / distance, roughness);
	}

	return finalColor;
}

vec3 CalculateSpotLightsContribution(vec3 N, vec3 V, vec3 H, vec3 color, vec3 position, float roughness)
{
	vec3 finalColor = vec3(0.0);
	for (uint i = 0; i < u_SpotLightsCount; i++)
	{
		SpotLightData spotLight = u_SpotLights.Lights[i];

		vec3 direction = u_SpotLights.Lights[i].Position - position;
		float distance = length(direction);
		float attenuation = 1.0f / (distance * distance);

		direction /= distance;

		float angleCos = dot(direction, spotLight.Direction);

		float fade = 1.0f - smoothstep(spotLight.InnerrRadiusCos, spotLight.OuterRadiusCos, angleCos);
		vec3 incomingLight = spotLight.Color.rgb * spotLight.Color.w * fade;

		finalColor += CalculateLight(N, V, H, color,
			incomingLight * attenuation,
			direction, roughness);
	}

	return finalColor;
}

#endif
