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

#endif
