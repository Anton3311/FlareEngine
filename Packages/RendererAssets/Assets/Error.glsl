#begin vertex
#version 450

#include "Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 2) in mat4 i_Transform;
layout(location = 6) in int i_EntityIndex;

layout(location = 0) out flat int o_EntityIndex;

void main()
{
	o_EntityIndex = i_EntityIndex;
    gl_Position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);
}
#end

#begin pixel
#version 450

layout(location = 0) in flat int i_EntityIndex;
layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityIndex;

void main()
{
	o_Color = vec4(1.0, 0.0, 1.0, 1.0);
	o_EntityIndex = i_EntityIndex;
}
#end
