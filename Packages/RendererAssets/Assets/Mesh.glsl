Properties = 
{
	u_InstanceData.Color = {}
	u_InstanceData.Roughness = {}
	u_Texture = {}
}

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in mat4 i_Transform;
layout(location = 7) in int i_EntityIndex;

#include "Camera.glsl"

struct VertexData
{
	vec3 Position;
	vec3 Normal;
	vec2 UV;
	vec3 ViewSpacePosition;
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
	mat4 normalTransform = transpose(inverse(i_Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;

	vec4 position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);

	vec4 transformed = i_Transform * vec4(i_Position, 1.0);
	o_Vertex.Position = transformed.xyz;

	o_Vertex.UV = i_UV;
	o_Vertex.ViewSpacePosition = (u_Camera.View * transformed).xyz;
	o_EntityIndex = i_EntityIndex;

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
};

struct VertexData
{
	vec3 Position;
	vec3 Normal;
	vec2 UV;
	vec3 ViewSpacePosition;
};

layout(binding = 2) uniform sampler2D u_ShadowMap0;
layout(binding = 3) uniform sampler2D u_ShadowMap1;
layout(binding = 4) uniform sampler2D u_ShadowMap2;
layout(binding = 5) uniform sampler2D u_ShadowMap3;

layout(binding = 6) uniform sampler3D u_RandomAngles;
layout(binding = 7) uniform sampler2D u_Texture;

layout(location = 0) in VertexData i_Vertex;
layout(location = 5) in flat int i_EntityIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityIndex;

const vec2[] POISSON_POINTS = {
	vec2(0.06202323374764562, 0.030134247767888755),
	vec2(-0.045215927419343105, 0.005259890307770521),
	vec2(0.0291242311326263, -0.04890114852957095),
	vec2(0.07166083913765608, -0.02294852802288938),
	vec2(0.02930951265990056, 0.06235823231873705),
	vec2(-0.03439345609698008, -0.0644393210968266),
	vec2(0.008414144111760224, 0.0005320839441136371),
	vec2(0.07192577830745317, -0.07552349918894519),
	vec2(0.14685174257602096, -0.019889734709092388),
	vec2(0.13896582071098118, -0.08256178359367672),
	vec2(0.03488065847596844, -0.11782778487036805),
	vec2(0.07961454571719973, -0.11842360357039117),
	vec2(0.10188776480776296, 0.07044442536707529),
	vec2(0.012440147980056215, 0.11589377626618025),
	vec2(-0.04031003929448962, 0.07670902584296702),
	vec2(0.058458178450993215, 0.12609988056611532),
	vec2(0.16579407300606075, 0.09322781011405978),
	vec2(0.10130612713134224, 0.11809713989856352),
	vec2(0.13300349357681007, 0.04236448836072215),
	vec2(-0.022269761146856748, -0.10558780201875617),
	vec2(-0.04065826190642696, 0.1534486420249026),
	vec2(0.03388887109888783, 0.16181425524795356),
	vec2(-0.0993917409083892, 0.005977622350578526),
	vec2(-0.07345402862775141, 0.039063582091554805),
	vec2(-0.08493171557268842, -0.09935454388262643),
	vec2(-0.06982783591943231, -0.03532949248364259),
	vec2(-0.05641266424985536, -0.13722979133071522),
	vec2(0.11495995731214825, -0.15831987898120004),
	vec2(0.018152298528749666, -0.16331966023686706),
	vec2(0.06826126670050803, -0.18496855551761537),
	vec2(0.17632452763123485, -0.06704455776180784),
	vec2(0.22072341483722613, -0.048569591547511504),
};

const int NUMBER_OF_SAMPLES = 32;
#define LIGHT_SIZE (u_LightSize / u_LightFrustumSize)

float CalculateBlockerDistance(sampler2D shadowMap, vec3 projectedLightSpacePosition, vec2 rotation, float bias)
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

float CalculateShadow(sampler2D shadowMap, vec4 lightSpacePosition, float bias)
{
	vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
	projected = projected * 0.5 + vec3(0.5);

	vec2 uv = projected.xy;
	float receieverDepth = projected.z;

	if (receieverDepth > 1.0)
		return 1.0;

	if (projected.x > 1.0 || projected.y > 1.0 || projected.x < 0 || projected.y < 0)
		return 1.0;

	float angle = texture(u_RandomAngles, lightSpacePosition.xyz).r;
	vec2 rotation = vec2(cos(angle), sin(angle));

	float blockerDistance = CalculateBlockerDistance(shadowMap, projected, rotation, bias);
	if (blockerDistance == -1.0f)
		return 1.0f;

	float penumbraWidth = (receieverDepth - blockerDistance) / blockerDistance;
	float filterRadius = penumbraWidth * LIGHT_SIZE * u_LightNear / receieverDepth;
	return 1.0f - PCF(shadowMap, uv, receieverDepth, filterRadius, rotation, bias);
}

#define DEBUG_CASCADES 0

void main()
{
	vec3 N = normalize(i_Vertex.Normal);
	vec3 V = normalize(u_Camera.Position - i_Vertex.Position);
	vec3 H = normalize(V - u_LightDirection);

	vec4 color = u_InstanceData.Color * texture(u_Texture, i_Vertex.UV);

	float shadow = 1.0f;
	float viewSpaceDistance = abs(i_Vertex.ViewSpacePosition.z);

	int cascadeIndex = u_MaxCascadeIndex;
	for (int i = 0; i < u_MaxCascadeIndex; i++)
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
#else

	float NoL = dot(N, -u_LightDirection);
	float bias = max(u_Bias * (1.0f - NoL), 0.0025f);

	bias /= float(cascadeIndex + 1);
	switch (cascadeIndex)
	{
	case 0:
		shadow = CalculateShadow(u_ShadowMap0, (u_CascadeProjection0 * vec4(i_Vertex.Position, 1.0f)), bias);
		break;
	case 1:
		shadow = CalculateShadow(u_ShadowMap1, (u_CascadeProjection1 * vec4(i_Vertex.Position, 1.0f)), bias);
		break;
	case 2:
		shadow = CalculateShadow(u_ShadowMap2, (u_CascadeProjection2 * vec4(i_Vertex.Position, 1.0f)), bias);
		break;
	case 3:
		shadow = CalculateShadow(u_ShadowMap3, (u_CascadeProjection3 * vec4(i_Vertex.Position, 1.0f)), bias);
		break;
	}
#endif

	vec3 incomingLight = u_LightColor.rgb * u_LightColor.w;
	float alpha = max(0.04, u_InstanceData.Roughness * u_InstanceData.Roughness);

	vec3 kS = Fresnel_Shlick(baseReflectivity, V, H);
	vec3 kD = vec3(1.0) - kS;

	vec3 diffuse = Diffuse_Lambertian(color.rgb);
	vec3 specular = Specular_CookTorence(alpha, N, V, -u_LightDirection);
	vec3 brdf = kD * diffuse + specular;

	vec3 finalColor = shadow * brdf * incomingLight * max(0.0, dot(-u_LightDirection, N));
	finalColor += u_EnvironmentLight.rgb * u_EnvironmentLight.w * color.rgb;

	o_Color = vec4(finalColor, color.a);
	o_EntityIndex = i_EntityIndex;
}

#end
