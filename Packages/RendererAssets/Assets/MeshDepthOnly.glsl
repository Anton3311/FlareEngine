#begin vertex

#include "Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 3) in mat4 i_Transform;

void main()
{
	gl_Position = u_Camera.ViewProjection * i_Transform * vec4(i_Position, 1.0);
}
#end

#begin pixel

void main() {}

#end
