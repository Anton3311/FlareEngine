Type = FullscreenQuad
Culling = None
BlendMode = Opaque
DepthTest = false
DepthWrite = false

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;

layout(location = 0) out vec2 o_UV;

void main()
{
	gl_Position = vec4(i_Position.xy, 0.0f, 1.0f);
	o_UV = i_Position.xy * 0.5f + vec2(0.5f);
}
#end

#begin pixel
#version 450

#include "Common/AtmosphericScattering.glsl"

layout(push_constant, std140) uniform Params
{
	float PlanetRadius;
	float AtmosphereThickness;
	int SunTransmittanceSteps;

	float MieHeight;
	float RayleighHeight;

	float MieCoefficient;
	float MieAbsorbtion;
	float RayleighAbsorbtion;
	vec3 OzoneAbsorbtion;
	vec3 RayleighCoefficient;
} u_Params;

layout(location = 0) in vec2 i_UV;

layout(location = 0) out vec4 o_Transmittance;

void main()
{
	float cosTheta = clamp(i_UV.y * 2.0f - 1.0f, -1.0f, 1.0f);
	float theta = acos(cosTheta);
	float height = i_UV.x * u_Params.AtmosphereThickness + u_Params.PlanetRadius;

	vec3 rayOrigin = vec3(0.0f, height, 0.0f);
	vec3 lightDirection = -normalize(vec3(0.0f, cosTheta, -sin(theta)));

	const float scale = pow(10, -3);

	AtmosphereProperties properties;
	properties.MieHeight = u_Params.MieHeight;
	properties.RayleighHeight = u_Params.RayleighHeight;
	properties.MieCoefficient = u_Params.MieCoefficient * scale;
	properties.MieAbsorbtion = u_Params.MieAbsorbtion * scale;
	properties.RayleighAbsorbtion = u_Params.RayleighAbsorbtion * scale;
	properties.OzoneAbsorbtion = u_Params.OzoneAbsorbtion * scale;
	properties.RayleighCoefficient = u_Params.RayleighCoefficient * scale;

	vec3 transmittance = ComputeSunTransmittance(rayOrigin,
		u_Params.SunTransmittanceSteps,
		u_Params.PlanetRadius,
		u_Params.AtmosphereThickness,
		lightDirection,
		properties);

	o_Transmittance = vec4(transmittance, 1.0f);
}
#end
