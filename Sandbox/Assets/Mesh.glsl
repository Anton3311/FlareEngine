#type vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;

#include "Camera.glsl"

layout(std140, push_constant) uniform InstanceData
{
	mat4 Transform;
	vec4 Color;
	float Roughness;
} u_InstanceData;

struct VertexData
{
	vec3 Position;
	vec3 Normal;
	vec4 Color;
	float Roughness;
};

layout(location = 0) out VertexData o_Vertex;

void main()
{
	mat4 normalTransform = transpose(inverse(u_InstanceData.Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;
	o_Vertex.Color = u_InstanceData.Color;
	o_Vertex.Roughness = u_InstanceData.Roughness;

	vec4 position = u_Camera.ViewProjection * u_InstanceData.Transform * vec4(i_Position, 1.0);
	o_Vertex.Position = (u_InstanceData.Transform * vec4(i_Position, 1.0)).xyz;

    gl_Position = position;
}

#type fragment
#version 450

#include "Camera.glsl"

layout(std140, binding = 1) uniform DirLight
{
	vec4 u_LightColor;
	vec3 u_LightDirection;
};

struct VertexData
{
	vec3 Position;
	vec3 Normal;
	vec4 Color;
	float Roughness;
};

layout(location = 0) in VertexData i_Vertex;
layout(location = 0) out vec4 o_Color;

vec3 Fresnel_Shlick(vec3 baseReflectivity, vec3 V, vec3 H)
{
	return baseReflectivity + (vec3(1.0) - baseReflectivity) * pow(1.0 - max(0.0, dot(V, H)), 5);
}

const float pi = 3.1415926535897932384626433832795;
const vec3 baseReflectivity = vec3(0.04);

vec3 Diffuse_Lambertian(vec3 color)
{
	return color / pi;
}

float NDF_GGXThrowbridgeReitz(float alpha, vec3 N, vec3 H)
{
	float NdotH = max(dot(N, H), 0.0);
	NdotH *= NdotH;

	float numer = alpha * alpha;
	float denom = NdotH * (numer - 1.0) + 1.0;
	return numer / (denom * denom * pi);
}

float ShlickBeckmann(vec3 N, vec3 X, float k)
{
	float NdotX = max(0.0, dot(N, X));
	return NdotX / max(0.000001, NdotX * (1.0 - k) + k);
}

float ShlickGGX(float alpha, vec3 N, vec3 V, vec3 L)
{
	float k = alpha / 2.0;
	return ShlickBeckmann(N, V, k) * ShlickBeckmann(N, L, k);
}

vec3 Specular_CookTorence(float alpha, vec3 N, vec3 V, vec3 L)
{
	vec3 H = normalize(V + L);
	vec3 numer = NDF_GGXThrowbridgeReitz(alpha, N, H) * ShlickGGX(alpha, N, V, L) * Fresnel_Shlick(baseReflectivity, V, H);
	float denom = 4 * max(0.0, dot(V, N)) * max(0.0, dot(L, N));

	return numer / max(denom, 0.000001);
}

void main()
{
	vec3 N = normalize(i_Vertex.Normal);
	vec3 V = normalize(u_Camera.Position - i_Vertex.Position);
	vec3 H = normalize(V + u_LightDirection);

	vec3 incomingLight = u_LightColor.rgb * u_LightColor.w;
	float alpha = max(0.04, i_Vertex.Roughness * i_Vertex.Roughness);

	vec3 kS = Fresnel_Shlick(baseReflectivity, V, H);
	vec3 kD = vec3(1.0) - kS;

	vec3 diffuse = Diffuse_Lambertian(i_Vertex.Color.rgb);
	vec3 specular = Specular_CookTorence(alpha, N, V, u_LightDirection);
	vec3 brdf = kD * diffuse + specular;

	o_Color = vec4(brdf * incomingLight * max(0.0, dot(u_LightDirection, N)), i_Vertex.Color.a);
}
