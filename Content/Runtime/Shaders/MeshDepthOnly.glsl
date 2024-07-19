Type = Surface
Culling = Back
DepthTest = true
DepthBias = true
DepthClamp = true

#begin vertex
#version 450

#include "Common/Camera.glsl"
#include "Common/Instancing.glsl"

layout(location = 0) in vec3 i_Position;

void main()
{
	gl_Position = u_Camera.ViewProjection * GetInstanceTransform() * vec4(i_Position, 1.0);
}
#end

#begin pixel
#version 450

void main() {}

#end
