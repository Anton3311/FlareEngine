#begin vertex
#version 450

#include "Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;

layout(location = 0) out vec4 VertexColor;

void main()
{
	VertexColor = i_Color;
	gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
}

#end

#begin pixel
#version 450

layout(location = 0) in vec4 VertexColor;

layout(location = 0) out vec4 o_Color;

void main()
{
	o_Color = VertexColor;
}

#end
