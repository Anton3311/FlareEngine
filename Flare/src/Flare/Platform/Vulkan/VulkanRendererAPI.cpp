#include "VulkanRendererAPI.h"

namespace Flare
{
	void VulkanRendererAPI::Initialize()
	{
	}

	void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
	}

	void VulkanRendererAPI::SetClearColor(float r, float g, float b, float a)
	{
	}

	void VulkanRendererAPI::Clear()
	{
	}

	void VulkanRendererAPI::SetDepthTestEnabled(bool enabled)
	{
	}

	void VulkanRendererAPI::SetCullingMode(CullingMode mode)
	{
	}

	void VulkanRendererAPI::SetDepthComparisonFunction(DepthComparisonFunction function)
	{
	}

	void VulkanRendererAPI::SetDepthWriteEnabled(bool enabled)
	{
	}

	void VulkanRendererAPI::SetBlendMode(BlendMode mode)
	{
	}

	void VulkanRendererAPI::SetLineWidth(float width)
	{
	}

	void VulkanRendererAPI::DrawIndexed(const Ref<const VertexArray>& vertexArray)
	{
	}

	void VulkanRendererAPI::DrawIndexed(const Ref<const VertexArray>& vertexArray, size_t indicesCount)
	{
	}

	void VulkanRendererAPI::DrawIndexed(const Ref<const VertexArray>& mesh, size_t firstIndex, size_t indicesCount)
	{
	}

	void VulkanRendererAPI::DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount)
	{
	}

	void VulkanRendererAPI::DrawInstancesIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t instancesCount, uint32_t baseInstance)
	{
	}

	void VulkanRendererAPI::DrawInstancesIndexedIndirect(const Ref<const Mesh>& mesh, const Span<DrawIndirectCommandSubMeshData>& subMeshesData, uint32_t baseInstance)
	{
	}

	void VulkanRendererAPI::DrawLines(const Ref<const VertexArray>& vertexArray, size_t cverticesCountount)
	{
	}

	void VulkanRendererAPI::DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount, size_t baseVertexIndex, size_t startIndex, size_t indicesCount)
	{
	}

	void VulkanRendererAPI::ApplyMaterialProperties(const Ref<const Material>& materail)
	{
	}
}
