#type vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in mat4 i_Transform;

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

void main()
{
	mat4 normalTransform = transpose(inverse(i_Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;

	vec4 position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);
	o_Vertex.Position = (i_Transform * vec4(i_Position, 1.0)).xyz;

	o_Vertex.LightSpacePosition = u_LightProjection * vec4(o_Vertex.Position, 1.0);

    gl_Position = position;
}

#type fragment
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
};

struct VertexData
{
	vec3 Position;
	vec3 Normal;
	vec4 LightSpacePosition;
};

layout(binding = 2) uniform sampler2D u_ShadowMap;

layout(location = 0) in VertexData i_Vertex;
layout(location = 0) out vec4 o_Color;

float CalculateShadow(vec4 lightSpacePosition)
{
	vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
	projected = projected * 0.5 + vec3(0.5);

	float sampledDepth = texture(u_ShadowMap, projected.xy).r;
	return (projected.z > sampledDepth) ? 1.0 : 0.0;
}

void main()
{
	if (u_InstanceData.Color.a <= 0.0001)
		discard;

	float shadow = 1.0 - CalculateShadow(i_Vertex.LightSpacePosition);

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
}
