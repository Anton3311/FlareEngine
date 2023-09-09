#type vertex
#version 410

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;

uniform mat4 u_ViewProjection;

out vec4 VertexColor;

void main()
{
	VertexColor = i_Color;
	gl_Position = u_ViewProjection * vec4(i_Position, 1.0);
}

#type fragment
#version 410

in vec4 VertexColor;

out vec4 o_Color;

void main()
{
	o_Color = VertexColor;
}
