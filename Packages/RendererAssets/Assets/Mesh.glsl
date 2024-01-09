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

struct InstanceData
{
	mat4 Transform;
	int EntityIndex;
};

layout(std140, binding = 3) buffer InstacesData
{
	InstanceData Data[];
} u_InstancesData;

layout(location = 0) out VertexData o_Vertex;
layout(location = 5) out flat int o_EntityIndex;

void main()
{
	mat4 normalTransform = transpose(inverse(u_InstancesData.Data[gl_InstanceIndex].Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;
    
	vec4 position = u_Camera.ViewProjection * u_InstancesData.Data[gl_InstanceIndex].Transform * vec4(i_Position, 1.0);
	vec4 transformed = u_InstancesData.Data[gl_InstanceIndex].Transform * vec4(i_Position, 1.0);
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

layout(std140, binding = 1) uniform DirLight
{
	vec4 u_LightColor;
	vec3 u_LightDirection;

	vec4 u_EnvironmentLight;
	float u_LightNear;
};

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

const vec2[] POISSON_POINTS = {
	vec2(0.19720058313715616, -0.126486154070558),
	vec2(0.0009889608544477646, -0.22664507218369004),
	vec2(-0.2734364188916016, 0.16998219873785095),
	vec2(-0.05627888319129203, 0.3417174487591548),
	vec2(0.17630187577796308, 0.25626249198305373),
	vec2(-0.22133066411936145, -0.11169131389810796),
	vec2(0.07874947085941063, -0.49784888851498144),
	vec2(0.3367516311138403, 0.061844425249618036),
	vec2(0.24206212071523217, -0.3434060421929901),
	vec2(-0.4584196133795251, -0.11921633394838857),
	vec2(-0.610318625227714, 0.10216782955315853),
	vec2(-0.40762380258089803, 0.44255622309867276),
	vec2(-0.638870484170101, 0.29887844090022253),
	vec2(-0.147488672409682, -0.6626000933377793),
	vec2(-0.24754946856053187, -0.39087749108560194),
	vec2(0.4395031222358684, -0.5345429932350132),
	vec2(0.17424177636283178, -0.8615742535163032),
	vec2(0.6296239472407246, -0.31454151036644407),
	vec2(0.36219610480587505, -0.7430911110009584),
	vec2(0.7109629465467581, -0.5879441048415303),
	vec2(0.12880022486006107, 0.5033381465367552),
	vec2(0.40976015405429883, 0.4739540494059923),
	vec2(0.9059886417498543, -0.4233019974205821),
	vec2(0.6973919978044718, -0.0006410703053913907),
	vec2(0.40689966031564423, -0.1946107102756708),
	vec2(-0.17072221121859749, 0.5096286422582593),
	vec2(0.06862033329541031, 0.7072732106703834),
	vec2(0.3946848418824804, 0.7895068805292053),
	vec2(0.49510509842148803, 0.24741745331806023),
	vec2(0.07581696369151561, 0.05748883597413473),
	vec2(-0.7665664598386808, -0.06648209971836988),
	vec2(-0.8026373723638186, -0.3079872104229894),
};

const int NUMBER_OF_SAMPLES = 32;
#define LIGHT_SIZE (u_LightSize / u_LightFrustumSize)

float CalculateBlockerDistance(sampler2D shadowMap, vec3 projectedLightSpacePosition, vec2 rotation, float bias, float scale)
{
	float receieverDepth = projectedLightSpacePosition.z;
	float blockerDistance = 0.0;
	float samplesCount = 0;
	float searchSize = LIGHT_SIZE * (receieverDepth - u_LightNear) / receieverDepth;

	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = vec2(
			rotation.x * POISSON_POINTS[i].x - rotation.y * POISSON_POINTS[i].y,
			rotation.y * POISSON_POINTS[i].x + rotation.x * POISSON_POINTS[i].y
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
			rotation.x * POISSON_POINTS[i].x - rotation.y * POISSON_POINTS[i].y,
			rotation.y * POISSON_POINTS[i].x + rotation.x * POISSON_POINTS[i].y
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

	vec3 incomingLight = u_LightColor.rgb * u_LightColor.w;
	float alpha = max(0.04, u_InstanceData.Roughness * u_InstanceData.Roughness);

	vec3 kS = Fresnel_Shlick(baseReflectivity, V, H);
	vec3 kD = vec3(1.0) - kS;

	vec3 diffuse = Diffuse_Lambertian(color.rgb);
	vec3 specular = Specular_CookTorence(alpha, N, V, -u_LightDirection);
	vec3 brdf = kD * diffuse + specular;

	vec3 finalColor = shadow * brdf * incomingLight * max(0.0, dot(-u_LightDirection, N));
	finalColor += u_EnvironmentLight.rgb * u_EnvironmentLight.w * color.rgb;

	o_Normal = vec4(N * 0.5f + vec3(0.5f), 1.0f);
	o_Color = vec4(finalColor, color.a);
	o_EntityIndex = i_EntityIndex;
}

#end
