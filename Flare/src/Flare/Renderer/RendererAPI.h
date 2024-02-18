#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/VertexArray.h"
#include "Flare/Renderer/Mesh.h"

#include <stdint.h>

namespace Flare
{
	struct DrawIndirectCommandSubMeshData
	{
		uint32_t SubMeshIndex;
		uint32_t InstancesCount;
	};

	class FLARE_API RendererAPI
	{
	public:
		enum class API
		{
			None,
			OpenGL,
		};
	public:
		virtual void Initialize() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void SetClearColor(float r, float g, float b, float a) = 0;
		virtual void Clear() = 0;

		virtual void SetDepthTestEnabled(bool enabled) = 0;
		virtual void SetCullingMode(CullingMode mode) = 0;
		virtual void SetDepthComparisonFunction(DepthComparisonFunction function) = 0;
		virtual void SetDepthWriteEnabled(bool enabled) = 0;
		virtual void SetBlendMode(BlendMode mode) = 0;

		virtual void SetLineWidth(float width) = 0;

		virtual void DrawIndexed(const Ref<const VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const Ref<const VertexArray>& vertexArray, size_t indicesCount) = 0;
		virtual void DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount) = 0;

		virtual void DrawInstancesIndexed(const Ref<const Mesh>& mesh,
			uint32_t subMeshIndex,
			uint32_t instancesCount,
			uint32_t baseInstance) = 0;

		virtual void DrawInstancesIndexedIndirect(
			const Ref<const Mesh>& mesh,
			const Span<DrawIndirectCommandSubMeshData>& subMeshesData,
			uint32_t baseInstance) = 0;

		virtual void DrawLines(const Ref<const VertexArray>& vertexArray, size_t cverticesCountount) = 0;

		virtual void DrawInstanced(const Ref<const VertexArray>& mesh,
			size_t instancesCount,
			size_t baseVertexIndex,
			size_t startIndex,
			size_t indicesCount) = 0;
	public:
		static Scope<RendererAPI> Create();
		static API GetAPI();
	};
}