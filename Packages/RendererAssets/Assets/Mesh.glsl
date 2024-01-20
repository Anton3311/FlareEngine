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

#include "Common/Camera.glsl"
#include "Common/Instancing.glsl"

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

#include "Common/Camera.glsl"
#include "Common/BRDF.glsl"
#include "Common/ShadowMapping.glsl"
#include "Common/Light.glsl"

layout(std140, push_constant) uniform InstanceData
{
	vec4 Color;
	float Roughness;
} u_InstanceData;


struct VertexData
{
	vec4 Position;
	vec3 Normal;
	vec2 UV;
	vec3 ViewSpacePosition;
	vec2 ScreenSpacePosition;
};

layout(binding = 7) uniform sampler2D u_Texture;

layout(location = 0) in VertexData i_Vertex;
layout(location = 5) in flat int i_EntityIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_Normal;
layout(location = 2) out int o_EntityIndex;

#define DEBUG_CASCADES 0

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

	int cascadeIndex = CalculateCascadeIndex(i_Vertex.ViewSpacePosition);

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

	float shadow = CalculateShadow(N, i_Vertex.Position, cascadeIndex);
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
