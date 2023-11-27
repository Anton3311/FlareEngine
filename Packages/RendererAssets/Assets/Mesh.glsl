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
	vec4 LightSpacePosition;
};

layout(std140, binding = 1) uniform DirLight
{
	vec4 u_LightColor;
	vec3 u_LightDirection;
	mat4 u_LightProjection;
};

layout(location = 0) out VertexData o_Vertex;
layout(location = 4) out flat int o_EntityIndex;

void main()
{
	mat4 normalTransform = transpose(inverse(i_Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;

	vec4 position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);
	o_Vertex.Position = (i_Transform * vec4(i_Position, 1.0)).xyz;

	o_Vertex.LightSpacePosition = u_LightProjection * vec4(o_Vertex.Position, 1.0);
	o_Vertex.UV = i_UV;
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

layout(std140, binding = 2) uniform ShadowData
{
	float u_Smoothness;
	float u_Bias;
	float u_Resolution;
	float u_TexelSize;
};

struct VertexData
{
	vec3 Position;
	vec3 Normal;
	vec2 UV;
	vec4 LightSpacePosition;
};

layout(binding = 2) uniform sampler2D u_ShadowMap;
layout(binding = 3) uniform sampler2D u_Texture;

layout(location = 0) in VertexData i_Vertex;
layout(location = 4) in flat int i_EntityIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityIndex;

const vec2[] POISSON_POINTS = {
	vec2(0.19137895211891331, 0.2541560153271236),
	vec2(-0.3610552914162688, -0.1702133872981514),
	vec2(-0.11285099426109912, 0.3053419369744086),
	vec2(0.03029878555042398, -0.22921089302199993),
	vec2(0.3263746737232833, -0.09706210535805015),
	vec2(-0.22290176128994843, 0.4981360156236345),
	vec2(-0.008763882096664122, 0.07380710686652425),
	vec2(0.06167897888966056, 0.5694560824272383),
	vec2(-0.43546438936118137, 0.46329117305967493),
	vec2(-0.20269715730566173, 0.783715139127624),
	vec2(-0.5820990691342114, 0.7260681853770539),
	vec2(-0.4479702972171161, 0.19505505917790833),
	vec2(-0.7648568317588637, 0.46856292070223793),
	vec2(-0.4816936403781329, -0.37351504884005227),
	vec2(-0.21383923947483052, -0.5228608180044612),
	vec2(-0.14427805343779843, -0.329473333253935),
	vec2(0.12475286264671825, -0.4517984878039243),
	vec2(-0.25000129358630996, 0.05183526451844611),
	vec2(-0.4499968784249042, -0.61341810367348),
	vec2(-0.6759281883717762, -0.0319065835615302),
	vec2(0.3010726068861467, -0.33524066559587495),
	vec2(0.6544362299583781, -0.27887096922024535),
	vec2(0.53537492110925, 0.14476148488731178),
	vec2(-0.2363757220376479, -0.9415006587542523),
	vec2(-0.7797866173167972, -0.6032284641392093),
	vec2(-0.03627367298818418, -0.8231481461144745),
	vec2(-0.7884171108426258, 0.22492389251147582),
	vec2(-0.8051533726824254, -0.382929276121336),
};

const int NUMBER_OF_SAMPLES = 24;
const float LIGHT_SIZE = 16.0;

float Random(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float CalculateBlockerDistance(vec3 projectedLightSpacePosition, vec2 rotation)
{
	float receieverDepth = projectedLightSpacePosition.z;
	float blockerDistance = 0.0;
	float samplesCount = 0;
	float searchSize = ((receieverDepth - u_LightNear) * LIGHT_SIZE * u_Smoothness);

	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = vec2(
			rotation.x * POISSON_POINTS[i].x - rotation.y * POISSON_POINTS[i].y,
			rotation.y * POISSON_POINTS[i].x + rotation.x * POISSON_POINTS[i].y
		);

		float depth = texture(u_ShadowMap, projectedLightSpacePosition.xy + offset * searchSize * u_TexelSize).r;
		if (depth - u_Bias < receieverDepth)
		{
			samplesCount += 1.0;
			blockerDistance += depth;
		}
	}

	if (samplesCount == 0.0)
		return -1.0;
	
	return max(0.01, blockerDistance / samplesCount);
}

float CalculatePCFKernelSize(vec3 projectedLightSpacePosition, vec2 rotation)
{
	float blockerDistance = CalculateBlockerDistance(projectedLightSpacePosition, rotation);

	if (blockerDistance == -1.0)
		return 1.0;

	float receieverDepth = projectedLightSpacePosition.z;
	float penumbraWidth = (receieverDepth - blockerDistance) * LIGHT_SIZE * u_Smoothness / blockerDistance;
	return penumbraWidth;
}

float CalculateShadow(vec4 lightSpacePosition)
{
	vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
	projected = projected * 0.5 + vec3(0.5);

	if (projected.z > 1.0)
		return 1.0;

	if (projected.x > 1.0 || projected.y > 1.0 || projected.x < 0 || projected.y < 0)
		return 1.0;

	float random = Random(projected.xy);
	vec2 rotation = vec2(cos(random), sin(random));

	float PCFKernelSize = max(0.1, CalculatePCFKernelSize(projected, rotation));
	float shadow = 0.0;

	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = vec2(
			rotation.x * POISSON_POINTS[i].x - rotation.y * POISSON_POINTS[i].y,
			rotation.y * POISSON_POINTS[i].x + rotation.x * POISSON_POINTS[i].y
		);

		float sampledDepth = texture(u_ShadowMap, projected.xy + offset * u_TexelSize * PCFKernelSize).r;
		shadow += (projected.z - u_Bias > sampledDepth ? 1.0 : 0.0);
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

	vec4 color = u_InstanceData.Color * texture(u_Texture, i_Vertex.UV);

	vec3 diffuse = Diffuse_Lambertian(color.rgb);
	vec3 specular = Specular_CookTorence(alpha, N, V, u_LightDirection);
	vec3 brdf = kD * diffuse + specular;

	o_Color = vec4(shadow * brdf * incomingLight * max(0.0, dot(u_LightDirection, N)), color.a);
	o_EntityIndex = i_EntityIndex;
}

#end
