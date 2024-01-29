Culling = Front

Properties =
{
	u_Sky.AtmoshpereHeight = {}
	u_Sky.PlanetRadius = {}
	u_Sky.AtmoshpericDensity = {}
	u_Sky.RaySteps = {}
	u_Sky.ViewRaySteps = {}
	u_Sky.ScatteringStrength = {}
	u_Sky.SunColor = {}
}

#begin vertex
#version 450

#include "Packages/RendererAssets/Assets/Common/Camera.glsl"
#include "Packages/RendererAssets/Assets/Common/Instancing.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 0) out vec3 o_Position;

void main()
{
	mat4 transform = GetInstanceTransform();
	vec4 transformed = transform * vec4(i_Position, 1.0f);
	o_Position = transformed.xyz;
	gl_Position = u_Camera.ViewProjection * transformed;
}

#end

#begin pixel
#version 450

#include "Packages/RendererAssets/Assets/Common/BRDF.glsl"
#include "Packages/RendererAssets/Assets/Common/Light.glsl"
#include "Packages/RendererAssets/Assets/Common/Camera.glsl"

layout(std140, push_constant) uniform Sky
{
	float AtmoshpereHeight;
	float AtmoshpericDensity;
	float PlanetRadius;
	float ScatteringStrength;
	int RaySteps;
	int ViewRaySteps;
	vec3 SunColor;
} u_Sky;

layout(location = 0) in vec3 i_Position;

layout(location = 0) out vec4 o_Color;

float CalculatePhase(float angleCos, float g)
{
	float g2 = g * g;
	float a = (1 + g2 - 2 * g * angleCos);
	return (3 * (1 - g2)) / (2 * (2 + g2)) * (1 + angleCos * angleCos) / sqrt(pow(a, 3.0f));
}

float CalculateOpticalDepth(vec3 rayOrigin, vec3 rayDirection, vec3 waveLength)
{
	float opticalDepth = 0.0f;
	vec3 step = rayDirection / max(1, u_Sky.RaySteps - 1);
	float stepLength = length(step);
	vec3 pointOnRay = rayOrigin;
	for (int i = 0; i < u_Sky.RaySteps; i++)
	{
		float height = length(pointOnRay);
		opticalDepth += exp(-height / u_Sky.AtmoshpereHeight) * stepLength;
		pointOnRay += step;
	}

	return opticalDepth;
}

float FindSpehereRayIntersection(vec3 rayOrigin, vec3 rayDirection)
{
	vec3 d = rayOrigin;
	float p1 = -dot(rayDirection, d);
	float p2sqr = p1 * p1 - dot(d, d) + u_Sky.AtmoshpereHeight * u_Sky.AtmoshpereHeight;

	if (p2sqr < 0)
		return -1.0f;

	float p2 = sqrt(p2sqr);
	float t1 = p1 - p2;
	float t2 = p1 + p2;

	return max(t1, t2);
}

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

	float distanceThroughAtmosphere = FindSpehereRayIntersection(
		vec3(0.0f, u_Sky.PlanetRadius, 0.0f),
		viewDirection);
 
	if (distanceThroughAtmosphere == -1.0f)
		discard;

	vec3 waveLength = u_Sky.SunColor * u_Sky.ScatteringStrength;
	vec3 waveLengthPower4 = pow(waveLength, vec3(4.0f));
	vec3 scatteringCoefficients = vec3(1.1111327521701591e-30);

	vec3 viewRayStep = viewDirection * distanceThroughAtmosphere / max(1, u_Sky.ViewRaySteps - 1);
	vec3 viewRayPoint = u_Camera.Position;
	float viewRayStepLength = length(viewRayStep);
	
	vec3 finalColor = vec3(0.0f);
	float viewOpticalDepth = 0.0f;
	for (int i = 0; i < u_Sky.ViewRaySteps; i++)
	{
		float distanceToSun = FindSpehereRayIntersection(viewRayPoint, -u_LightDirection);
		float opticalDepth = CalculateOpticalDepth(viewRayPoint, -u_LightDirection * distanceToSun, waveLength);

		float height = length(viewRayPoint);
		float density = exp(-height / u_Sky.AtmoshpereHeight);

		finalColor += density * exp(vec3(-opticalDepth - viewOpticalDepth) * 4.0f * pi * scatteringCoefficients / waveLengthPower4);

		viewOpticalDepth += density * viewRayStepLength;
		viewRayPoint += viewRayStep;
	}

	float phase = CalculatePhase(dot(viewDirection, -u_LightDirection), 0.25f);
	o_Color = vec4(u_LightColor.w * finalColor * phase, 1.0f);
}

/*
8 * pi^3 * (n^2 - 1)^2/3 / N
*/
#end
