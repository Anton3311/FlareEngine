#type vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in mat4 i_Transform;

#include "Camera.glsl"

layout(std140, push_constant) uniform InstanceData
{
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
	mat4 normalTransform = transpose(inverse(i_Transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;
	o_Vertex.Color = u_InstanceData.Color;
	o_Vertex.Roughness = u_InstanceData.Roughness;

	vec4 position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);
	o_Vertex.Position = (i_Transform * vec4(i_Position, 1.0)).xyz;

    gl_Position = position;
}

#type fragment
#version 450

#include "Camera.glsl"
#include "BRDF.glsl"

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

void main()
{
	if (i_Vertex.Color.a <= 0.0001)
		discard;

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
