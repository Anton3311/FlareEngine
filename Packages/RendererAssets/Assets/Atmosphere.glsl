Properties =
{
	u_Params.PlanetRadius = { DisplayName = "Planet Radius" }
	u_Params.AtmosphereThickness = { DisplayName = "Thickness" }
	u_Params.RaySteps = { DisplayName = "Ray Steps" }
	u_Params.ViewRaySteps = { DisplayName = "View Ray Steps" }

	u_Params.ObserverHeight = { DisplayName = "Observer Height" }

	u_Params.MieHeight = { DisplayName = "Mie Height" }
	u_Params.RayleighHeight = { DisplayName = "Rayleigh Height" }
	u_Params.RayleighCoefficient = { DisplayName = "Rayleigh Coefficient" }
	u_Params.MieCoefficient = { DisplayName = "Mie Coefficient" }
	u_Params.MieAbsorbtion = { DisplayName = "Mie Absorbtion" }
	u_Params.RayleighAbsobtion = { DisplayName = "Rayleigh Absobtion" }
	u_Params.OzoneAbsorbtion = { DisplayName = "Ozone Absorbtion" }

	u_Params.GroundColor =
	{
		Type = Color
		DisplayName = "Ground Color"
	}
}

#begin vertex
#version 450

layout(location = 0) in vec2 i_Position;

void main()
{
	gl_Position = vec4(i_Position, 0.9999999f, 1.0f);
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
	int RaySteps;
	int ViewRaySteps;

	float ObserverHeight;

	float MieHeight;
	float RayleighHeight;

	float MieCoefficient;
	float MieAbsorbtion;
	float RayleighAbsobtion;
	vec3 OzoneAbsorbtion;
	vec3 RayleighCoefficient;

	vec3 GroundColor;

} u_Params;

layout(location = 0) out vec4 o_Color;

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
	float scaledViewRayStepLength = viewRayStepLength / u_Params.AtmosphereThickness;

	float rayleighPhase = RayleighPhaseFunction(-dot(viewDirection, -u_LightDirection));
	float miePhase = MiePhaseFunction(dot(viewDirection, -u_LightDirection), 0.84f);

	vec3 luminance = vec3(0.0f);
	vec3 transmittance = vec3(1.0f);

	AtmosphereProperties properties;
	properties.MieHeight = u_Params.MieHeight;
	properties.RayleighHeight = u_Params.RayleighHeight;
	properties.MieCoefficient = u_Params.MieCoefficient;
	properties.MieAbsorbtion = u_Params.MieAbsorbtion;
	properties.RayleighAbsobtion = u_Params.RayleighAbsobtion;
	properties.OzoneAbsorbtion = u_Params.OzoneAbsorbtion;
	properties.RayleighCoefficient = u_Params.RayleighCoefficient;

	for (int i = 0; i < u_Params.ViewRaySteps; i++)
	{
		float height = length(viewRayPoint) - u_Params.PlanetRadius;

		ScatteringCoefficients scatteringCoefficients = ComputeScatteringCoefficients(height, properties);
		vec3 sampleTransmittance = exp(-scatteringCoefficients.Extinction * scaledViewRayStepLength);
		vec3 sunTransmittance = ComputeSunTransmittance(
			viewRayPoint,
			u_Params.RaySteps,
			u_Params.PlanetRadius,
			u_Params.AtmosphereThickness,
			u_LightDirection,
			properties);

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
