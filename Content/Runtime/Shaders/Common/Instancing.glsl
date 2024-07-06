struct InstanceData
{
	vec4 PackedTransform0;
	vec4 PackedTransform1;
	vec4 PackedTransform2;
};

layout(std140, set = 0, binding = 3) readonly buffer InstaceData
{
	InstanceData u_InstanceData[];
};

mat4 GetInstanceTransform()
{
	InstanceData data = u_InstanceData[gl_InstanceIndex];
	vec4 translation = vec4(
		data.PackedTransform0.w,
		data.PackedTransform1.w,
		data.PackedTransform2.w,
		1.0f);

	return mat4(
		vec4(data.PackedTransform0.xyz, 0.0f),
		vec4(data.PackedTransform1.xyz, 0.0f),
		vec4(data.PackedTransform2.xyz, 0.0f),
		translation
	);
}
