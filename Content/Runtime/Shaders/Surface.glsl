Type = Surface
Properties = 
{
	u_Material.Color = { Type = Color }
	u_Material.Roughness = {}
	u_Material.Metallic = {}
	u_Material.Emission = { Type = HDR }
	u_Texture = { Default = White }
	u_NormalMap = { Default = DefaultNormals }
	u_RoughnessMap = { Default = White }
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
	vec3 Position;
	float UVx;
	vec3 Normal;
	float UVy;
	vec3 Tangent;
};

layout(location = 0) out VertexData o_Vertex;

void main()
{
	mat4 transform = GetInstanceTransform();
	o_Vertex.Normal = (transform * vec4(i_Normal, 0.0)).xyz;
	o_Vertex.Tangent = (transform * vec4(i_Tangent, 0.0)).xyz;
    
	vec4 transformed = transform * vec4(i_Position, 1.0);
	o_Vertex.Position = transformed.xyz;

	o_Vertex.UVx = i_UV.x;
	o_Vertex.UVy = i_UV.y;

    gl_Position = u_Camera.ViewProjection * transformed;
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
	float Metallic;
	vec3 Emission;
} u_Material;

struct VertexData
{
	vec3 Position;
	float UVx;
	vec3 Normal;
	float UVy;
	vec3 Tangent;
};

layout(set = 3, binding = 0) uniform sampler2D u_Texture;
layout(set = 3, binding = 1) uniform sampler2D u_NormalMap;
layout(set = 3, binding = 2) uniform sampler2D u_RoughnessMap;

layout(location = 0) in VertexData i_Vertex;

layout(location = 0) out vec4 o_Color;

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
	vec2 uv = vec2(i_Vertex.UVx, i_Vertex.UVy);
	vec4 color = u_Material.Color * texture(u_Texture, uv);
	if (color.a == 0.0f)
		discard;

	vec3 vertexNormal = normalize(i_Vertex.Normal);
	vec3 V = normalize(u_Camera.Position - i_Vertex.Position);
	vec3 H = normalize(V - u_LightDirection);
	vec3 N = vertexNormal;

	vec3 tangent = normalize(i_Vertex.Tangent);
	tangent = normalize(tangent - dot(tangent, N) * N);

	vec3 bitangent = cross(N, tangent);
	mat3 tbn = mat3(tangent, bitangent, N);
	vec3 sampledNormal = UnpackNormal(texture(u_NormalMap, uv).xyz);

	N = normalize(tbn * sampledNormal);

	SurfaceProperties surface;
	surface.Position = i_Vertex.Position;
	surface.Normal = N;
	surface.Color = color.rgb;
	surface.Roughness = u_Material.Roughness * texture(u_RoughnessMap, uv).r;;
	surface.Metallic = u_Material.Metallic;

	float shadow = CalculateShadow(vertexNormal, i_Vertex.Position);

	vec3 finalColor = CalculateLight(V, H, u_LightColor.rgb * u_LightColor.w, -u_LightDirection, surface);

	finalColor *= shadow;

	finalColor += CalculatePointLightsContribution(V, surface);
	finalColor += CalculateSpotLightsContribution(V, surface);

	finalColor += u_EnvironmentLight.rgb * u_EnvironmentLight.w * color.rgb;

	finalColor += u_Material.Emission;

#if DEBUG_CASCADES
	int cascadeIndex = CalculateCascadeIndex(i_Vertex.ViewSpacePosition);
	vec3 cascadeColors[] = { vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(1.0f) };

	finalColor *= cascadeColors[min(cascadeIndex, CASCADES_COUNT)];
#endif

	o_Color = vec4(finalColor, color.a);
}

#end
