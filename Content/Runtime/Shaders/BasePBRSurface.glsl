#ifndef BASE_SURFACE_H
#define BASE_SURFACE_H

#ifndef DEBUG_CASCADES
	#define DEBUG_CASCADES 0
#endif

#ifndef NORMAL_FORMAT_XYZ
	#define NORMAL_FORMAT_XYZ 0
#endif

#include "Common/Camera.glsl"
#include "Common/BRDF.glsl"
#include "Common/ShadowMapping.glsl"
#include "Common/Light.glsl"
#include "Common/SpotLightShadows.glsl"

struct PBRMaterialProperties
{
	vec3 SurfacePosition;

	// Surface normal after applying a normal map
	vec3 SurfaceDetailNormal;

	// Surface normal without the normal map
	vec3 SurfaceNormal;

	vec4 SurfaceColor;
	vec3 SurfaceEmission;

	float Metallic;
	float Roughness;
};

vec3 CASCADE_COLORS[] =
{
	vec3(1.0f, 0.0f, 0.0f),
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, 0.0f, 1.0f),
	vec3(1.0f, 0.0f, 0.0f),
	vec3(1.0f)
};

vec3 UnpackNormalXYZ(vec3 packedNormal)
{
	return packedNormal * 2.0f - vec3(1.0f);
}

vec3 UnpackNormalXY(vec3 packedNormal)
{
	vec2 normalXY = packedNormal.xy * 2.0f - vec2(1.0f);
	float z = sqrt(clamp(1.0f - dot(normalXY, normalXY), 0.0f, 1.0f));
	return vec3(normalXY, z);
}

vec3 UnpackNormal(vec3 packedNormal)
{
#if NORMAL_FORMAT_XYZ
	return UnpackNormalXYZ(packedNormal);
#else
	return UnpackNormalXY(packedNormal);
#endif
}

// Both `normal` and `tangent` must be normalized
mat3 ComputeTangentSpace(vec3 normal, vec3 tangent)
{
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	vec3 bitangent = cross(normal, tangent);
	return mat3(tangent, bitangent, normal);
}

vec3 ShadePBRSurface(PBRMaterialProperties material)
{
	SurfaceProperties surface;
	surface.Color = material.SurfaceColor.rgb;
	surface.Position = material.SurfacePosition;
	surface.Normal = material.SurfaceDetailNormal;
	surface.Metallic = material.Metallic;
	surface.Roughness = material.Roughness;

	vec3 V = normalize(u_Camera.Position - material.SurfacePosition);
	vec3 H = normalize(V - u_LightDirection);

	vec3 finalColor = vec3(0.0f);

	float directionalShadow = CalculateShadow(material.SurfaceNormal, material.SurfacePosition);
	finalColor += CalculateLight(V, H, u_LightColor.rgb * u_LightColor.w, -u_LightDirection, surface) * directionalShadow;
	finalColor += CalculatePointLightsContribution(V, surface);
	finalColor += CalculateSpotLightsContribution(V, surface);
	finalColor += ComputeShadowCastingSpotLightsContribution(V, surface, material.SurfaceNormal);

	float ao = SampleAO(ivec2(gl_FragCoord.xy));
	ao = mix(ao, 1.0f, directionalShadow);

	finalColor += u_EnvironmentLight.rgb * u_EnvironmentLight.w * material.SurfaceColor.rgb * ao;
	finalColor += material.SurfaceEmission;

#if DEBUG_CASCADES
	float distanceToCameraPlane = CalculateDistanceToCameraPlane(surface.Position);

	int cascadeIndex = CASCADES_COUNT;
	for (int i = 0; i < CASCADES_COUNT; i++)
	{
		if (distanceToCameraPlane < u_CascadeSplits[i])
		{
			cascadeIndex = i;
			break;
		}
	}
	finalColor *= CASCADE_COLORS[min(cascadeIndex, CASCADES_COUNT)];
#endif

	return finalColor;
}

#endif
