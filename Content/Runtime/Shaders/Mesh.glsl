Type = Surface
Properties = 
{
	u_Material.Color = { Type = Color }
	u_Material.Roughness = {}
	u_Texture = {}
	u_NormalMap = {}
	u_RoughnessMap = {}
}

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in vec3 i_Tangent;
layout(location = 3) in vec2 i_UV;

#include "Common/Camera.glsl"
#include "Common/Instancing.glsl"

struct VertexData
{
	vec4 Position;
	vec3 Normal;
	vec3 Tangent;
	vec2 UV;
	vec3 ViewSpacePosition;
};

layout(location = 0) out VertexData o_Vertex;

void main()
{
	mat4 transform = GetInstanceTransform();
	o_Vertex.Normal = (transform * vec4(i_Normal, 0.0)).xyz;
	o_Vertex.Tangent = (transform * vec4(i_Tangent, 0.0)).xyz;
    
	vec4 transformed = transform * vec4(i_Position, 1.0);
	vec4 position = u_Camera.ViewProjection * transformed;
	o_Vertex.Position = transformed;

	o_Vertex.UV = i_UV;
	o_Vertex.ViewSpacePosition = (u_Camera.View * transformed).xyz;

    gl_Position = position;
}
#end

#begin pixel
#version 450

// #define DEBUG_CASCADES 1

#include "Common/Camera.glsl"
#include "Common/BRDF.glsl"
#include "Common/ShadowMapping.glsl"
#include "Common/Light.glsl"

layout(std140, push_constant) uniform InstanceData
{
	vec4 Color;
	float Roughness;
} u_Material;


struct VertexData
{
	vec4 Position;
	vec3 Normal;
	vec3 Tangent;
	vec2 UV;
	vec3 ViewSpacePosition;
};

layout(set = 2, binding = 7) uniform sampler2D u_Texture;
layout(set = 2, binding = 8) uniform sampler2D u_NormalMap;
layout(set = 2, binding = 9) uniform sampler2D u_RoughnessMap;

layout(location = 0) in VertexData i_Vertex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_Normal;

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

// #define NORMAL_FORMAT_XYZ

vec3 UnpackNormal(vec3 packedNormal)
{
#ifdef NORMAL_FORMAT_XYZ
	return UnpackNormalXYZ(packedNormal);
#else
	return UnpackNormalXY(packedNormal);
#endif
}

void main()
{
	vec4 color = u_Material.Color * texture(u_Texture, i_Vertex.UV);
	if (color.a == 0.0f)
		discard;

	vec3 vertexNormal = normalize(i_Vertex.Normal);
	vec3 V = normalize(u_Camera.Position - i_Vertex.Position.xyz);
	vec3 H = normalize(V - u_LightDirection);
	vec3 N = vertexNormal;

	vec3 tangent = normalize(i_Vertex.Tangent);
	tangent = normalize(tangent - dot(tangent, N) * N);

	vec3 bitangent = cross(N, tangent);
	mat3 tbn = mat3(tangent, bitangent, N);
	vec3 sampledNormal = UnpackNormal(texture(u_NormalMap, i_Vertex.UV).xyz);

	N = normalize(tbn * sampledNormal);

	float roughness = u_Material.Roughness * texture(u_RoughnessMap, i_Vertex.UV).r;
	float shadow = CalculateShadow(vertexNormal, i_Vertex.Position, i_Vertex.ViewSpacePosition);

	vec3 finalColor = CalculateLight(N, V, H, color.rgb,
		u_LightColor.rgb * u_LightColor.w, -u_LightDirection,
		roughness) * shadow;

	finalColor += CalculatePointLightsContribution(N, V, H, color.rgb, i_Vertex.Position.xyz, roughness);
	finalColor += CalculateSpotLightsContribution(N, V, H, color.rgb, i_Vertex.Position.xyz, roughness);

	finalColor += u_EnvironmentLight.rgb * u_EnvironmentLight.w * color.rgb;

#if DEBUG_CASCADES
	int cascadeIndex = CalculateCascadeIndex(i_Vertex.ViewSpacePosition);
	vec3 cascadeColors[] = { vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(1.0f) };

	finalColor *= cascadeColors[min(cascadeIndex, CASCADES_COUNT)];
#endif

	o_Color = vec4(finalColor, color.a);
	o_Normal = vec4(N * 0.5f + vec3(0.5f), 1.0f);
}

#end
