Type = Surface
Culling = Back
DepthTest = true
DepthBias = true
DepthClamp = true

#begin vertex
#version 450

layout(std140, set = 2, binding = 0) readonly buffer InstanceTransforms
{
	mat4 u_Transforms[];
};

layout(location = 0) in vec3 i_Position;

void main()
{
	gl_Position = u_Transforms[gl_InstanceIndex] * vec4(i_Position, 1.0);
}
#end

#begin pixel
#version 450

void main() {}

#end
