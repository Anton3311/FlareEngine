#ifndef LIGHT_H
#define LIGHT_H

layout(std140, binding = 1) uniform LightData
{
	vec4 u_LightColor;
	vec3 u_LightDirection;

	vec4 u_EnvironmentLight;
	float u_LightNear;

	uint u_PointLightsCount;
};

struct PointLightData
{
	vec3 Position;
	vec4 Color;
};

layout(std140, binding = 4) buffer LightsData
{
	PointLightData[] PointLights;
} u_LightsData;

vec3 CalculateLight(vec3 N, vec3 V, vec3 H, vec3 color, vec3 incomingLight, vec3 lightDirection, float roughness)
{
	float alpha = max(0.04, roughness * roughness);

	vec3 kS = Fresnel_Shlick(baseReflectivity, V, H);
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
		vec3 direction = u_LightsData.PointLights[i].Position - position;
		float distance = length(direction);
		float attenuation = 1.0f / (distance * distance);

		vec3 incomingLight = u_LightsData.PointLights[i].Color.rgb * u_LightsData.PointLights[i].Color.w;

		finalColor += CalculateLight(N, V, H, color,
			incomingLight * attenuation,
			direction / distance, roughness);
	}

	return finalColor;
}

#endif
