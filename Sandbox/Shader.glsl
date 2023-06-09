#type vertex
#version 330

layout(location = 0) in vec3 i_Position;

out vec4 VertexColor;

void main()
{
	gl_Position = vec4(i_Position, 1.0);
	VertexColor = vec4(i_Position, 1.0);
}

#type fragment
#version 330

in vec4 VertexColor;
out vec4 o_Color;

void main()
{
	o_Color = abs(VertexColor);
}
