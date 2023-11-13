Properties = 
{
	u_InstanceData.Color = {}
	u_InstanceData.Roughness = {}
}

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in mat4 i_Transform;
layout(location = 6) in int i_EntityIndex;

#include "Camera.glsl"

struct VertexData
{
	vec3 Position;
	vec3 Normal;
	vec4 LightSpacePosition;
};

layout(std140, binding = 1) uniform DirLight
{
	vec4 u_LightColor;
	vec3 u_LightDirection;
	mat4 u_LightProjection;
};

layout(location = 0) out VertexData o_Vertex;
layout(location = 3) out flat int o_EntityIndex;

void main()
{
	mat4 normalTransform = transpose(inverse(i_Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;

	vec4 position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);
	o_Vertex.Position = (i_Transform * vec4(i_Position, 1.0)).xyz;

	o_Vertex.LightSpacePosition = u_LightProjection * vec4(o_Vertex.Position, 1.0);
	
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
	mat4 u_LightProjection;
	float u_LightNear;
	float u_LightFar;
};

struct VertexData
{
	vec3 Position;
	vec3 Normal;
	vec4 LightSpacePosition;
};

layout(binding = 2) uniform sampler2D u_ShadowMap;

layout(location = 0) in VertexData i_Vertex;
layout(location = 3) in flat int i_EntityIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityIndex;

const vec2[] POISSON_POINTS = {
	vec2(0.10827563978276222, 0.37064253399210023),
	vec2(-0.31323933670540816, 0.22370181326225635),
	vec2(0.25472102837260024, 0.01814211041650693),
	vec2(0.13555163367726086, -0.2681591443162248),
	vec2(-0.28004025543551303, -0.09550898604452474),
	vec2(-0.040740049100009834, -0.39211872150455673),
	vec2(-0.12210222682312522, 0.34165670539190596),
	vec2(0.47959532502903524, -0.35256952915265993),
	vec2(-0.024267323115360173, -0.06810634700483997),
	vec2(0.06607420360505722, -0.5736571213226207),
	vec2(-0.07661433331499623, -0.9010361582833213),
	vec2(0.3379914753280737, -0.788177207223832),
	vec2(-0.2554207150842983, -0.681825211128562),
	vec2(0.31560450454481503, -0.557088451101416),
	vec2(0.4652064182162934, 0.1480137363298193),
	vec2(0.4609565436567771, -0.06112472977363281),
	vec2(0.282923275824686, 0.2398336833582484),
	vec2(-0.13633589865221818, 0.627879739655993),
	vec2(0.39456315153312227, 0.5204562903556604),
	vec2(-0.4956386820714891, 0.6573156021081124),
	vec2(-0.14513864395791587, 0.8827630432858808),
	vec2(0.03817257073337377, 0.9818290097061577),
	vec2(0.09800346050315079, 0.7113529073907687),
	vec2(0.6213882182981487, 0.44764960222726957),
	vec2(0.6980810665505919, 0.7042790944225807),
	vec2(0.28500338535070346, 0.8237901868754756),
	vec2(0.5596639207837573, 0.8525992961802431),
	vec2(-0.47945487067402615, 0.10580495652444322),
	vec2(-0.33172102564029415, 0.4392028712003506),
	vec2(-0.5646773825355629, 0.4536347238199887),
	vec2(-0.8158232369893543, 0.4329812725563691),
	vec2(-0.4648064800223728, 0.9372590785095587),
};
const int NUMBER_OF_SAMPLES = 32;
const float TEXEL_SIZE = 1.0 / 1024.0;
const float BIAS = 0.0001;
const float LIGHT_SIZE = 128.0;

float Random(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float CalculateBlockerDistance(vec3 projectedLightSpacePosition)
{
	float receieverDepth = projectedLightSpacePosition.z;
	float blockerDistance = 0.0;
	float samplesCount = 0;
	float searchSize = int((receieverDepth - u_LightNear) * LIGHT_SIZE);

	float random = Random(projectedLightSpacePosition.xy);
	vec2 rotation = vec2(cos(random), sin(random));

	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = searchSize * POISSON_POINTS[i] * TEXEL_SIZE * rotation;
		float depth = texture(u_ShadowMap, projectedLightSpacePosition.xy + offset).r;
		if (depth - BIAS < receieverDepth)
		{
			samplesCount += 1.0;
			blockerDistance += depth;
		}
	}

	if (samplesCount == 0.0)
		return -1.0;
	
	return max(0.01, blockerDistance / samplesCount);
}

float CalculatePCFKernelSize(vec3 projectedLightSpacePosition)
{
	float blockerDistance = CalculateBlockerDistance(projectedLightSpacePosition);

	if (blockerDistance == -1.0)
		return 1.0;

	float receieverDepth = projectedLightSpacePosition.z;
	float penumbraWidth = (receieverDepth - blockerDistance) / blockerDistance;
	return penumbraWidth * LIGHT_SIZE * u_LightNear / receieverDepth;
}

float CalculateShadow(vec4 lightSpacePosition)
{
	vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
	projected = projected * 0.5 + vec3(0.5);

	if (projected.z > 1.0)
		return 1.0;

	if (abs(projected.x) > 1.0 && abs(projected.y) > 1.0)
		return 1.0;

	float PCFKernelSize = max(1.0, CalculatePCFKernelSize(projected));
	float shadow = 0.0;

	float random = Random(projected.xy);
	vec2 rotation = vec2(cos(random), sin(random));
	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		float sampledDepth = texture(u_ShadowMap, projected.xy + POISSON_POINTS[i] * TEXEL_SIZE * PCFKernelSize * rotation).r;
		shadow += (projected.z - BIAS > sampledDepth ? 1.0 : 0.0);
	}
	
	shadow = shadow / NUMBER_OF_SAMPLES;
	return 1.0 - shadow;
}

void main()
{
	if (u_InstanceData.Color.a <= 0.0001)
		discard;

	float shadow = CalculateShadow(i_Vertex.LightSpacePosition);

	vec3 N = normalize(i_Vertex.Normal);
	vec3 V = normalize(u_Camera.Position - i_Vertex.Position);
	vec3 H = normalize(V + u_LightDirection);

	vec3 incomingLight = u_LightColor.rgb * u_LightColor.w;
	float alpha = max(0.04, u_InstanceData.Roughness * u_InstanceData.Roughness);

	vec3 kS = Fresnel_Shlick(baseReflectivity, V, H);
	vec3 kD = vec3(1.0) - kS;

	vec3 diffuse = Diffuse_Lambertian(u_InstanceData.Color.rgb);
	vec3 specular = Specular_CookTorence(alpha, N, V, u_LightDirection);
	vec3 brdf = kD * diffuse + specular;

	o_Color = vec4(shadow * brdf * incomingLight * max(0.0, dot(u_LightDirection, N)), u_InstanceData.Color.a);
	o_EntityIndex = i_EntityIndex;
}

#end
