#ifndef BRDF_H
#define BRDF_H

const float pi = 3.1415926535897932384626433832795;
const vec3 baseReflectivity = vec3(0.04);

vec3 Fresnel_Shlick(vec3 baseReflectivity, vec3 V, vec3 H)
{
	return baseReflectivity + (vec3(1.0) - baseReflectivity) * pow(1.0 - max(0.0, dot(V, H)), 5);
}

vec3 Diffuse_Lambertian(vec3 color)
{
	return color / pi;
}

float NDF_GGXThrowbridgeReitz(float alpha, vec3 N, vec3 H)
{
	float NdotH = max(dot(N, H), 0.0);
	NdotH *= NdotH;

	float numer = alpha * alpha;
	float denom = NdotH * (numer - 1.0) + 1.0;
	return numer / (denom * denom * pi);
}

float ShlickBeckmann(vec3 N, vec3 X, float k)
{
	float NdotX = max(0.0, dot(N, X));
	return NdotX / max(0.000001, NdotX * (1.0 - k) + k);
}

float ShlickGGX(float alpha, vec3 N, vec3 V, vec3 L)
{
	float k = alpha / 2.0;
	return ShlickBeckmann(N, V, k) * ShlickBeckmann(N, L, k);
}

vec3 Specular_CookTorence(float alpha, vec3 N, vec3 V, vec3 L)
{
	vec3 H = normalize(V + L);
	vec3 numer = NDF_GGXThrowbridgeReitz(alpha, N, H) * ShlickGGX(alpha, N, V, L) * Fresnel_Shlick(baseReflectivity, V, H);
	float denom = 4 * max(0.0, dot(V, N)) * max(0.0, dot(L, N));

	return numer / max(denom, 0.000001);
}

#endif
