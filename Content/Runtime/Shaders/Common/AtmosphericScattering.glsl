#ifndef ATMOSPHERIC_SCATTERING
#define ATMOSPHERIC_SCATTERING

#include "Math.glsl"

float CornetteShanksMiePhaseFunction(float g, float cosTheta)
{
	float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
	return k * (1.0 + cosTheta * cosTheta) / pow(1.0 + g * g - 2.0 * g * -cosTheta, 1.5);
}

#define USE_CORNETTE_SHANKS

float MiePhaseFunction(float g, float cosTheta)
{
#ifdef USE_CORNETTE_SHANKS
	return CornetteShanksMiePhaseFunction(g, cosTheta);
#else
	// Reference implementation (i.e. not schlick approximation). 
	// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
	float numer = 1.0f - g * g;
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	return numer / (4.0f * PI * denom * sqrt(denom));
#endif
}

float RayleighPhaseFunction(float cosTheta)
{
	return 3.0f / (16.0f * PI) * (1 + cosTheta * cosTheta);
}

struct AtmosphereProperties
{
	float MieHeight;
	float RayleighHeight;

	float MieCoefficient;
	float MieAbsorbtion;
	float RayleighAbsorbtion;
	vec3 OzoneAbsorbtion;
	vec3 RayleighCoefficient;
};

struct ScatteringCoefficients
{
	float Mie;
	vec3 Rayleigh;
	vec3 Extinction;
};

bool FindRaySphereIntersection(vec3 rayOrigin, vec3 rayDirection, float sphereRadius, out float minDistance, out float maxDistance)
{
	float a = dot(rayDirection, rayDirection);
	float b = 2.0f * dot(rayOrigin, rayDirection);
	float c = dot(rayOrigin, rayOrigin) - sphereRadius * sphereRadius;

	float descriminant = b * b - 4.0f * a * c;

	if (descriminant < 0.0f)
		return false;

	float root = sqrt(descriminant);

	minDistance = (-b - root) / (2.0f * a);
	maxDistance = (-b + root) / (2.0f * a);

	return true;
}

bool FindNearestRaySphereIntersection(vec3 rayOrigin, vec3 rayDirection, float sphereRadius, out float distance)
{
	float minDistance = 0.0f;
	float maxDistance = 0.0f;

	if (!FindRaySphereIntersection(rayOrigin, rayDirection, sphereRadius, minDistance, maxDistance))
		return false;

	if (minDistance < 0.0f)
		return false;

	distance = minDistance;
	return true;
}

// Height is in kilometers
ScatteringCoefficients ComputeScatteringCoefficients(float height, in AtmosphereProperties properties)
{
	float rayleighDensity = exp(-height / properties.RayleighHeight);
	float mieDensity = exp(-height / properties.MieHeight);
	float ozoneDensity = max(0.0f, 1.0f - abs(height - 25.0f) / 15.0f);

	ScatteringCoefficients coefficients;
	coefficients.Rayleigh = properties.RayleighCoefficient * rayleighDensity;
	coefficients.Mie = properties.MieCoefficient * mieDensity;

	vec3 rayleighAbsorbtion = vec3(properties.RayleighAbsorbtion * rayleighDensity);
	vec3 mieAbsorbtion = vec3(properties.MieAbsorbtion * mieDensity);
	vec3 ozoneAbsorbtion = properties.OzoneAbsorbtion * ozoneDensity;

	coefficients.Extinction = coefficients.Rayleigh + rayleighAbsorbtion
		+ vec3(coefficients.Mie) + mieAbsorbtion
		+ ozoneAbsorbtion;

	return coefficients;
}

vec3 SampleSunTransmittanceLUT(sampler2D lut, vec3 lightDirection, vec3 rayOrigin, float planetRadius, float atmosphereThickness)
{
	float height = rayOrigin.y;
	vec3 up = rayOrigin / height;

	float sunZenithAngle = dot(up, -lightDirection);

	float height01 = (height - planetRadius) / atmosphereThickness;

	// uv.x - height
	// uv.y - sun zenith angle
	vec2 uv = vec2(
		height01,
		sunZenithAngle * 0.5f + 0.5f);

	return texture(lut, uv).rgb;
}

vec3 ComputeSunTransmittance(vec3 rayOrigin,
	int raySteps,
	float planetRadius,
	float atmosphereThickness,
	vec3 lightDirection,
	in AtmosphereProperties properties)
{
	float minDistance = 0.0f;
	float maxDistance = 0.0f;

	float atmosphereDistance = 0.0f;

	if (!FindRaySphereIntersection(rayOrigin, -lightDirection, planetRadius + atmosphereThickness, minDistance, maxDistance))
		return vec3(1.0f);

	if (minDistance < 0.0f)
		atmosphereDistance = maxDistance;

	// TODO: Handle the case when the point is outside the atmosphere

	float t = 0.0f;
	vec3 transmittance = vec3(1.0f);
	for (int i = 0; i < raySteps; i++)
	{
		float newT = (float(i) + 0.3f) / float(raySteps) * atmosphereDistance;
		float dt = newT - t;
		t = newT;

		vec3 rayPoint = rayOrigin - lightDirection * t;
		float height = length(rayPoint) - planetRadius;

		ScatteringCoefficients scatteringCoefficients = ComputeScatteringCoefficients(height, properties);
		transmittance *= exp(-scatteringCoefficients.Extinction * dt);
	}

	return transmittance;
}

#endif
