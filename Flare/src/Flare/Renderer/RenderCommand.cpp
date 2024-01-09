#include "RenderCommand.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	Scope<RendererAPI> s_API = RendererAPI::Create();

	void RenderCommand::Initialize()
	{
		s_API->Initialize();
	}

	void RenderCommand::Clear()
	{
		s_API->Clear();
	}

	void RenderCommand::SetLineWidth(float width)
	{
		s_API->SetLineWidth(width);
	}

	void RenderCommand::SetClearColor(float r, float g, float b, float a)
	{
		s_API->SetClearColor(r, g, b, a);
	}

	void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		s_API->SetViewport(x, y, width, height);
	}

	void RenderCommand::DrawIndexed(const Ref<const VertexArray>& mesh)
	{
		FLARE_PROFILE_FUNCTION();
		s_API->DrawIndexed(mesh);
	}

	void RenderCommand::DrawIndexed(const Ref<const VertexArray>& mesh, size_t indicesCount)
	{
		FLARE_PROFILE_FUNCTION();
		s_API->DrawIndexed(mesh, indicesCount);
	}

	void RenderCommand::DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount)
	{
		FLARE_PROFILE_FUNCTION();
		s_API->DrawInstanced(mesh, instancesCount);
	}

	void RenderCommand::DrawInstancesIndexed(const Ref<Mesh>& mesh, uint32_t subMeshIndex, size_t instancesCount)
	{
		FLARE_PROFILE_FUNCTION();
		s_API->DrawInstancesIndexed(mesh, subMeshIndex, instancesCount);
	}

	void RenderCommand::DrawInstancesIndexedIndirect(const Ref<Mesh>& mesh, const Span<DrawIndirectCommandSubMeshData>& subMeshesData)
	{
		FLARE_PROFILE_FUNCTION();
		s_API->DrawInstancesIndexedIndirect(mesh, subMeshesData);
	}

	void RenderCommand::DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount, size_t baseVertexIndex, size_t startIndex, size_t indicesCount)
	{
		FLARE_PROFILE_FUNCTION();
		s_API->DrawInstanced(mesh, instancesCount, baseVertexIndex, startIndex, indicesCount);
	}

	void RenderCommand::DrawLines(const Ref<const VertexArray>& lines, size_t verticesCount)
	{	
		FLARE_PROFILE_FUNCTION();
		s_API->DrawLines(lines, verticesCount);
	}

	void RenderCommand::SetDepthTestEnabled(bool enabled)
	{
		s_API->SetDepthTestEnabled(enabled);
	}

	void RenderCommand::SetCullingMode(CullingMode mode)
	{
		s_API->SetCullingMode(mode);
	}

	void RenderCommand::SetDepthComparisonFunction(DepthComparisonFunction function)
	{
		s_API->SetDepthComparisonFunction(function);
	}

	void RenderCommand::SetDepthWriteEnabled(bool enabled)
	{
		s_API->SetDepthWriteEnabled(enabled);
	}

	void RenderCommand::SetBlendMode(BlendMode mode)
	{
		s_API->SetBlendMode(mode);
	}
}