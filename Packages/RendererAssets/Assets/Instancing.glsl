struct InstanceData
{
	mat4 Transform;
	int EntityIndex;
};

layout(std140, binding = 3) buffer InstacesData
{
	InstanceData Data[];
} u_InstancesData;

mat4 GetInstanceTransform()
{
	return u_InstancesData.Data[gl_InstanceIndex].Transform;
}
