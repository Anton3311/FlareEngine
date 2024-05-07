Type = FullscreenQuad
DepthWrite = false

Properties =
{
	u_Params.RayleighCoefficient = { DisplayName = "Rayleigh Coefficient" }
	u_Params.MieCoefficient = { DisplayName = "Mie Coefficient" }
	u_Params.MieAbsorbtion = { DisplayName = "Mie Absorbtion" }
	u_Params.RayleighAbsorbtion = { DisplayName = "Rayleigh Absorbtion" }
	u_Params.OzoneAbsorbtion = { DisplayName = "Ozone Absorbtion" }

	u_Params.GroundColor =
	{
		Type = Color
		DisplayName = "Ground Color"
	}
}

#begin vertex
#version 450

#ifdef OPENGL
	layout(location = 0) in vec2 i_Position;
#else
	layout(location = 0) in vec3 i_Position;
#endif

void main()
{
	gl_Position = vec4(i_Position.xy, 0.9999999f, 1.0f);
}

#end

#begin pixel
#version 450

#include "Common/BRDF.glsl"
#include "Common/Light.glsl"
#include "Common/Camera.glsl"
#include "Common/AtmosphericScattering.glsl"

layout(std140, push_constant) uniform Sky
{
	float PlanetRadius;
	float AtmosphereThickness;
	int SunTransmittanceSteps;
	int ViewRaySteps;

	float ObserverHeight;

	float MieHeight;
	float RayleighHeight;

	float MieCoefficient;
	float MieAbsorbtion;
	float RayleighAbsorbtion;
	vec3 OzoneAbsorbtion;
	vec3 RayleighCoefficient;

	vec3 GroundColor;

} u_Params;

layout(location = 0) out vec4 o_Color;

#define USE_SUN_TRANSMITTANCE_LUT 1
#if USE_SUN_TRANSMITTANCE_LUT
layout(set = 2, binding = 0) uniform sampler2D u_SunTransmittanceLUT;
#endif

vec3 CalculateViewDirection()
{
	vec2 uv = vec2(gl_FragCoord.xy) / vec2(u_Camera.ViewportSize);
	vec4 projected = u_Camera.InverseViewProjection * vec4(uv * 2.0f - vec2(1.0f), 1.0f, 1.0f);
	projected.xyz /= projected.w;
	return normalize(projected.xyz - u_Camera.Position);
}

void main()
{
	vec3 viewDirection = CalculateViewDirection();
	vec3 viewRayPoint = vec3(0.0f, u_Params.PlanetRadius + u_Params.ObserverHeight, 0.0f);

	float groudDistance = FindSpehereRayIntersection(viewRayPoint, viewDirection, u_Params.PlanetRadius);
	if (groudDistance > 0.0f)
	{
		o_Color = vec4(u_Params.GroundColor, 1.0f);
		return;
	}

	float distanceThroughAtmosphere = FindSpehereRayIntersection(viewRayPoint,
		viewDirection, u_Params.PlanetRadius + u_Params.AtmosphereThickness);

	if (distanceThroughAtmosphere < 0.0f)
		discard;

	vec3 viewRayStep = viewDirection * distanceThroughAtmosphere / max(1, u_Params.ViewRaySteps - 1);
	float viewRayStepLength = length(viewRayStep);

	float rayleighPhase = RayleighPhaseFunction(-dot(viewDirection, -u_LightDirection));
	float miePhase = MiePhaseFunction(-dot(viewDirection, -u_LightDirection), 0.84f);

	vec3 luminance = vec3(0.0f);
	vec3 transmittance = vec3(1.0f);

	const float scale = pow(10, -3);

	AtmosphereProperties properties;
	properties.MieHeight = u_Params.MieHeight;
	properties.RayleighHeight = u_Params.RayleighHeight;
	properties.MieCoefficient = u_Params.MieCoefficient * scale;
	properties.MieAbsorbtion = u_Params.MieAbsorbtion * scale;
	properties.RayleighAbsorbtion = u_Params.RayleighAbsorbtion * scale;
	properties.OzoneAbsorbtion = u_Params.OzoneAbsorbtion * scale;
	properties.RayleighCoefficient = u_Params.RayleighCoefficient * scale;

	for (int i = 0; i < u_Params.ViewRaySteps; i++)
	{
		float height = length(viewRayPoint) - u_Params.PlanetRadius;

		ScatteringCoefficients scatteringCoefficients = ComputeScatteringCoefficients(height, properties);
		vec3 sampleTransmittance = exp(-scatteringCoefficients.Extinction * viewRayStepLength);
		vec3 sunTransmittance;
#if USE_SUN_TRANSMITTANCE_LUT
		sunTransmittance = SampleSunTransmittanceLUT(u_SunTransmittanceLUT,
			u_LightDirection,
			viewRayPoint,
			u_Params.PlanetRadius,
			u_Params.AtmosphereThickness);
#else
		sunTransmittance = ComputeSunTransmittance(
			viewRayPoint,
			u_Params.SunTransmittanceSteps,
			u_Params.PlanetRadius,
			u_Params.AtmosphereThickness,
			u_LightDirection,
			properties);
#endif

		vec3 rayleighInScatter = scatteringCoefficients.Rayleigh * rayleighPhase * sunTransmittance;
		vec3 mieInScatter = scatteringCoefficients.Mie * miePhase * sunTransmittance;
		vec3 inScatter = rayleighInScatter + mieInScatter;

		vec3 scatteringIntegral = (inScatter - inScatter * sampleTransmittance) / scatteringCoefficients.Extinction;
		luminance += scatteringIntegral * transmittance;
		transmittance *= sampleTransmittance;

		viewRayPoint += viewRayStep;
	}

	o_Color = vec4(u_LightColor.w * luminance * 10, 1.0f);
}

#end
