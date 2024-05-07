#include "RenderCommand.h"

#include "Flare/Renderer/Material.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	void RenderCommand::Initialize()
	{
		RendererAPI::GetInstance()->Initialize();
	}

	void RenderCommand::Clear()
	{
		RendererAPI::GetInstance()->Clear();
	}

	void RenderCommand::SetLineWidth(float width)
	{
		RendererAPI::GetInstance()->SetLineWidth(width);
	}

	void RenderCommand::SetClearColor(float r, float g, float b, float a)
	{
		RendererAPI::GetInstance()->SetClearColor(r, g, b, a);
	}

	void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		RendererAPI::GetInstance()->SetViewport(x, y, width, height);
	}

	void RenderCommand::DrawIndexed(const Ref<const VertexArray>& mesh)
	{
		FLARE_PROFILE_FUNCTION();
		RendererAPI::GetInstance()->DrawIndexed(mesh);
	}

	void RenderCommand::DrawIndexed(const Ref<const VertexArray>& mesh, size_t indicesCount)
	{
		FLARE_PROFILE_FUNCTION();
		RendererAPI::GetInstance()->DrawIndexed(mesh, indicesCount);
	}

	void RenderCommand::DrawIndexed(const Ref<const VertexArray>& mesh, size_t firstIndex, size_t indicesCount)
	{
		FLARE_PROFILE_FUNCTION();
		RendererAPI::GetInstance()->DrawIndexed(mesh, firstIndex, indicesCount);
	}

	void RenderCommand::DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount)
	{
		FLARE_PROFILE_FUNCTION();
		RendererAPI::GetInstance()->DrawInstanced(mesh, instancesCount);
	}

	void RenderCommand::DrawInstancesIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t instancesCount, uint32_t baseInstance)
	{
		FLARE_PROFILE_FUNCTION();
		RendererAPI::GetInstance()->DrawInstancesIndexed(mesh, subMeshIndex, instancesCount, baseInstance);
	}

	void RenderCommand::DrawInstancesIndexedIndirect(const Ref<const Mesh>& mesh, const Span<DrawIndirectCommandSubMeshData>& subMeshesData, uint32_t baseInstance)
	{
		FLARE_PROFILE_FUNCTION();
		RendererAPI::GetInstance()->DrawInstancesIndexedIndirect(mesh, subMeshesData, baseInstance);
	}

	void RenderCommand::DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount, size_t baseVertexIndex, size_t startIndex, size_t indicesCount)
	{
		FLARE_PROFILE_FUNCTION();
		RendererAPI::GetInstance()->DrawInstanced(mesh, instancesCount, baseVertexIndex, startIndex, indicesCount);
	}

	void RenderCommand::DrawLines(const Ref<const VertexArray>& lines, size_t verticesCount)
	{	
		FLARE_PROFILE_FUNCTION();
		RendererAPI::GetInstance()->DrawLines(lines, verticesCount);
	}

	void RenderCommand::SetDepthTestEnabled(bool enabled)
	{
		RendererAPI::GetInstance()->SetDepthTestEnabled(enabled);
	}

	void RenderCommand::SetCullingMode(CullingMode mode)
	{
		RendererAPI::GetInstance()->SetCullingMode(mode);
	}

	void RenderCommand::SetDepthComparisonFunction(DepthComparisonFunction function)
	{
		RendererAPI::GetInstance()->SetDepthComparisonFunction(function);
	}

	void RenderCommand::SetDepthWriteEnabled(bool enabled)
	{
		RendererAPI::GetInstance()->SetDepthWriteEnabled(enabled);
	}

	void RenderCommand::SetBlendMode(BlendMode mode)
	{
		RendererAPI::GetInstance()->SetBlendMode(mode);
	}

	void RenderCommand::ApplyMaterial(const Ref<const Material>& materail)
	{
		FLARE_CORE_ASSERT(materail);

		RendererAPI::GetInstance()->ApplyMaterialProperties(materail);

		auto shaderFeatures = materail->GetShader()->GetFeatures();
		SetBlendMode(shaderFeatures.Blending);
		SetDepthComparisonFunction(shaderFeatures.DepthFunction);
		SetCullingMode(shaderFeatures.Culling);
		SetDepthTestEnabled(shaderFeatures.DepthTesting);
		SetBlendMode(shaderFeatures.Blending);
		SetDepthWriteEnabled(shaderFeatures.DepthWrite);
	}
}