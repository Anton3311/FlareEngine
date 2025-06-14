Type = Surface
DepthFunction = LessOrEqual
Properties = 
{
	u_Material.Color = { Type = Color }
	u_Material.Roughness = {}
	u_Material.Metallic = {}
	u_Material.Emission = { Type = HDR }
	u_Texture = { Default = White }
	u_NormalMap = { Default = DefaultNormals }
	u_RoughnessMap = { Default = White }
	u_EmissionMap = { Default = White }
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

#include "BasePBRSurface.glsl"

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
layout(set = 3, binding = 3) uniform sampler2D u_EmissionMap;

layout(location = 0) in VertexData i_Vertex;

layout(location = 0) out vec4 o_Color;

void main()
{
	vec2 uv = vec2(i_Vertex.UVx, i_Vertex.UVy);
	vec4 color = u_Material.Color * texture(u_Texture, uv);
	if (color.a == 0.0f)
		discard;

	vec3 normal = normalize(i_Vertex.Normal);

	mat3 tangentSpace = ComputeTangentSpace(normal, normalize(i_Vertex.Tangent));
	vec3 sampledNormal = UnpackNormal(texture(u_NormalMap, uv).xyz);
	normal = normalize(tangentSpace * sampledNormal);

	PBRMaterialProperties material;
	material.SurfacePosition = i_Vertex.Position;
	material.SurfaceNormal = normal;
	material.SurfaceColor = color;
	material.SurfaceEmission = texture(u_EmissionMap, uv).rgb * u_Material.Emission;
	material.Metallic = u_Material.Metallic;
	material.Roughness = u_Material.Roughness * texture(u_RoughnessMap, uv).r;

	vec3 surfaceColor = ShadePBRSurface(material);
	o_Color = vec4(surfaceColor, color.a);
}

#end
