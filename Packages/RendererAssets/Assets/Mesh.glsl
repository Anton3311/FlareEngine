Properties = 
{
	u_InstanceData.Color = { Type = Color }
	u_InstanceData.Roughness = {}
	u_Texture = {}
}

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in vec2 i_UV;

#include "Camera.glsl"
#include "Instancing.glsl"

struct VertexData
{
	vec4 Position;
	vec3 Normal;
	vec2 UV;
	vec3 ViewSpacePosition;
	vec2 ScreenSpacePosition;
};

layout(std140, binding = 1) uniform LightData
{
	vec4 u_LightColor;
	vec3 u_LightDirection;
};

layout(location = 0) out VertexData o_Vertex;
layout(location = 5) out flat int o_EntityIndex;

void main()
{
	mat4 transform = GetInstanceTransform();
	mat4 normalTransform = transpose(inverse(transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;
    
	vec4 position = u_Camera.ViewProjection * transform * vec4(i_Position, 1.0);
	vec4 transformed = transform * vec4(i_Position, 1.0);
	o_Vertex.Position = transformed;

	o_Vertex.UV = i_UV;
	o_Vertex.ViewSpacePosition = (u_Camera.View * transformed).xyz;
	o_EntityIndex = u_InstancesData.Data[gl_InstanceIndex].EntityIndex;

	o_Vertex.ScreenSpacePosition = (position.xy / position.w) * 0.5f + vec2(0.5f);

    gl_Position = position;
}
#end

#begin pixel
#version 450

#include "Camera.glsl"
#include "BRDF.glsl"

layout(std140, push_constant) uniform InstanceData
{
	vec4 Color;
	float Roughness;
} u_InstanceData;

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

const int CASCADES_COUNT = 4;

layout(std140, binding = 2) uniform ShadowData
{
	float u_Bias;
	float u_LightFrustumSize;
	float u_LightSize;

	int u_MaxCascadeIndex;

	vec4 u_CascadeSplits;

	mat4 u_CascadeProjection0;
	mat4 u_CascadeProjection1;
	mat4 u_CascadeProjection2;
	mat4 u_CascadeProjection3;

	float u_ShadowResolution;
};

struct VertexData
{
	vec4 Position;
	vec3 Normal;
	vec2 UV;
	vec3 ViewSpacePosition;
	vec2 ScreenSpacePosition;
};

layout(binding = 2) uniform sampler2D u_ShadowMap0;
layout(binding = 3) uniform sampler2D u_ShadowMap1;
layout(binding = 4) uniform sampler2D u_ShadowMap2;
layout(binding = 5) uniform sampler2D u_ShadowMap3;

layout(binding = 7) uniform sampler2D u_Texture;

layout(location = 0) in VertexData i_Vertex;
layout(location = 5) in flat int i_EntityIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_Normal;
layout(location = 2) out int o_EntityIndex;

// Vogel disk points
const vec2[] SAMPLE_POINTS = {
	vec2(0.1767766952966369, 0.0),
	vec2(-0.22577979282638214, 0.20681751654846825),
	vec2(0.03458701007725054, -0.3937686360464939),
	vec2(0.28453027372165623, 0.3712041531770346),
	vec2(-0.5222095951380513, -0.09245073685889445),
	vec2(0.4947532383785339, -0.3145937588604606),
	vec2(-0.16560172116300248, 0.6154884807596737),
	vec2(-0.3154050676060234, -0.6076756069881691),
	vec2(0.6845685825311724, 0.2502316043413811),
	vec2(-0.7123533443287733, 0.29377323367456754),
	vec2(0.34362426953239034, -0.7336023182817316),
	vec2(0.2534030290286406, 0.8090345511034185),
	vec2(-0.7645502647498922, -0.4435232718481295),
	vec2(0.8972281608231768, -0.19680352493250616),
	vec2(-0.5479077316191127, 0.778490281013192),
	vec2(-0.1259483874642235, -0.9761593126611875),
};

const int NUMBER_OF_SAMPLES = 16;
#define LIGHT_SIZE (u_LightSize / u_LightFrustumSize)

float CalculateBlockerDistance(sampler2D shadowMap, vec3 projectedLightSpacePosition, vec2 rotation, float bias, float scale)
{
	float receieverDepth = projectedLightSpacePosition.z;
	float blockerDistance = 0.0;
	float samplesCount = 0;
	float searchSize = LIGHT_SIZE * (receieverDepth - u_LightNear) / receieverDepth * scale;

	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = vec2(
			rotation.x * SAMPLE_POINTS[i].x - rotation.y * SAMPLE_POINTS[i].y,
			rotation.y * SAMPLE_POINTS[i].x + rotation.x * SAMPLE_POINTS[i].y
		);

		float depth = texture(shadowMap, projectedLightSpacePosition.xy + offset * searchSize).r;
		if (depth < receieverDepth - bias)
		{
			samplesCount += 1.0;
			blockerDistance += depth;
		}
	}

	if (samplesCount == 0.0)
		return -1.0;
	
	return max(0.01, blockerDistance / samplesCount);
}

float PCF(sampler2D shadowMap, vec2 uv, float receieverDepth, float filterRadius, vec2 rotation, float bias)
{
	float shadow = 0.0f;
	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = vec2(
			rotation.x * SAMPLE_POINTS[i].x - rotation.y * SAMPLE_POINTS[i].y,
			rotation.y * SAMPLE_POINTS[i].x + rotation.x * SAMPLE_POINTS[i].y
		);

		float sampledDepth = texture(shadowMap, uv + offset * filterRadius).r;
		shadow += (receieverDepth - bias > sampledDepth ? 1.0 : 0.0);
	}
	
	return shadow / NUMBER_OF_SAMPLES;
}

float CalculateShadow(sampler2D shadowMap, vec4 lightSpacePosition, float bias, float poissonPointsRotationAngle, float scale)
{
	vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
	projected = projected * 0.5 + vec3(0.5);

	vec2 uv = projected.xy;
	float receieverDepth = projected.z;

	if (receieverDepth > 1.0)
		return 1.0;

	if (projected.x > 1.0 || projected.y > 1.0 || projected.x < 0 || projected.y < 0)
		return 1.0;

	vec2 rotation = vec2(cos(poissonPointsRotationAngle), sin(poissonPointsRotationAngle));

	float blockerDistance = CalculateBlockerDistance(shadowMap, projected, rotation, bias, scale);
	if (blockerDistance == -1.0f)
		return 1.0f;

	float penumbraWidth = (receieverDepth - blockerDistance) / blockerDistance;
	float filterRadius = penumbraWidth * LIGHT_SIZE * u_LightNear / receieverDepth;
	return 1.0f - PCF(shadowMap, uv, receieverDepth, max(3.0f / u_ShadowResolution, filterRadius * scale), rotation, bias);
}

#define DEBUG_CASCADES 0

// From Next Generation Post Processing in Call of Duty Advancded Warfare
float InterleavedGradientNoise(vec2 screenSpacePosition)
{
	const float scale = 64.0;
	vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
	return -scale + 2.0 * scale * fract(magic.z * fract(dot(screenSpacePosition, magic.xy)));
}

vec3 CalculateLight(vec3 N, vec3 V, vec3 H, vec4 color, vec3 incomingLight, vec3 lightDirection)
{
	float alpha = max(0.04, u_InstanceData.Roughness * u_InstanceData.Roughness);

	vec3 kS = Fresnel_Shlick(baseReflectivity, V, H);
	vec3 kD = vec3(1.0) - kS;

	vec3 diffuse = Diffuse_Lambertian(color.rgb);
	vec3 specular = Specular_CookTorence(alpha, N, V, lightDirection);
	vec3 brdf = kD * diffuse + specular;

	return brdf * incomingLight * max(0.0, dot(lightDirection, N));
}

void main()
{
	vec3 N = normalize(i_Vertex.Normal);
	vec3 V = normalize(u_Camera.Position - i_Vertex.Position.xyz);
	vec3 H = normalize(V - u_LightDirection);

	vec4 color = u_InstanceData.Color * texture(u_Texture, i_Vertex.UV);

	float shadow = 1.0f;
	float viewSpaceDistance = abs(i_Vertex.ViewSpacePosition.z);

	int cascadeIndex = CASCADES_COUNT;
	for (int i = 0; i < CASCADES_COUNT; i++)
	{
		if (viewSpaceDistance <= u_CascadeSplits[i])
		{
			cascadeIndex = i;
			break;
		}
	}

#if DEBUG_CASCADES
	switch (cascadeIndex)
	{
	case 0:
		color.xyz *= vec3(1.0f, 0.f, 0.0f);
		break;
	case 1:
		color.xyz *= vec3(0.0f, 1.0f, 0.0f);
		break;
	case 2:
		color.xyz *= vec3(0.0f, 0.0f, 1.0f);
		break;
	case 3:
		color.xyz *= vec3(1.0f, 0.0f, 0.0f);
		break;
	}
#endif

	float NoL = dot(N, -u_LightDirection);
	float bias = max(u_Bias * (1.0f - NoL), 0.0025f);

	bias /= float(cascadeIndex + 1);

	float poissonPointsRotationAngle = 2.0f * pi * InterleavedGradientNoise(gl_FragCoord.xy);

	switch (cascadeIndex)
	{
	case 0:
		shadow = CalculateShadow(u_ShadowMap0, (u_CascadeProjection0 * i_Vertex.Position), bias, poissonPointsRotationAngle, 1.0f);
		break;
	case 1:
		shadow = CalculateShadow(u_ShadowMap1, (u_CascadeProjection1 * i_Vertex.Position), bias, poissonPointsRotationAngle, 0.5f);
		break;
	case 2:
		shadow = CalculateShadow(u_ShadowMap2, (u_CascadeProjection2 * i_Vertex.Position), bias, poissonPointsRotationAngle, 0.25f);
		break;
	case 3:
		shadow = CalculateShadow(u_ShadowMap3, (u_CascadeProjection3 * i_Vertex.Position), bias, poissonPointsRotationAngle, 0.125f);
		break;
	}

	vec3 finalColor = CalculateLight(N, V, H, color, u_LightColor.rgb * u_LightColor.w, -u_LightDirection) * shadow;

	for (uint i = 0; i < u_PointLightsCount; i++)
	{
		vec3 direction = u_LightsData.PointLights[i].Position - i_Vertex.Position.xyz;
		float distance = length(direction);
		float attenuation = 1.0f / (distance * distance);

		finalColor += CalculateLight(N, V, H, color,
			u_LightsData.PointLights[i].Color.rgb * u_LightsData.PointLights[i].Color.w * attenuation,
			direction / distance);
	}

	finalColor += u_EnvironmentLight.rgb * u_EnvironmentLight.w * color.rgb;

	o_Normal = vec4(N * 0.5f + vec3(0.5f), 1.0f);
	o_Color = vec4(finalColor, color.a);
	o_EntityIndex = i_EntityIndex;
}

#end
