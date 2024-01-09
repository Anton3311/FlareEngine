#begin vertex
#version 450

#include "Camera.glsl"

struct InstanceData
{
	mat4 Transform;
	int EntityIndex;
};

layout(std140, binding = 3) buffer InstacesData
{
	InstanceData Data[];
} u_InstancesData;

layout(location = 0) in vec3 i_Position;

void main()
{
	gl_Position = u_Camera.ViewProjection * u_InstancesData.Data[gl_InstanceIndex].Transform * vec4(i_Position, 1.0);
}
#end

#begin pixel
#version 450

void main() {}

#end
