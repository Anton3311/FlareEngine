#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;

#include "Packages/RendererAssets/Assets/Camera.glsl"
#include "Packages/RendererAssets/Assets/Instancing.glsl"

struct VertexData
{
	vec3 Normal;
};

layout(location = 0) out VertexData o_Vertex;

void main()
{
	mat4 transform = GetInstanceTransform();
	mat4 normalTransform = transpose(inverse(transform));
	o_Vertex.Normal = (normalTransform * vec4(i_Normal, 1.0)).xyz;

    gl_Position = u_Camera.ViewProjection * transform * vec4(i_Position, 1.0);
}

#end

#begin pixel
#version 450

struct VertexData
{
	vec3 Normal;
};

layout(location = 0) in VertexData i_Vertex;
layout(location = 0) out vec4 o_Color;


void main()
{
	o_Color = vec4(i_Vertex.Normal, 1.0);
}

#end
