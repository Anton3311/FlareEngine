#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/RenderPass.h"

#include "Flare/Renderer/VertexArray.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Mesh.h"

#include <glm/glm.hpp>

namespace Flare
{
	enum class MeshRenderFlags : uint8_t
	{
		None = 0,
		DontCastShadows = 1,
	};

	FLARE_IMPL_ENUM_BITFIELD(MeshRenderFlags);

	struct RendererStatistics
	{
		uint32_t DrawCallsCount;
		uint32_t DrawCallsSavedByInstances;
	};

	struct ShadowSettings
	{
		ShadowSettings()
			: Resolution(0),
			LightSize(1.0f),
			Bias(0.0f) {}

		const uint32_t MaxCascades = 4;

		float LightSize;
		float Bias;
		uint32_t Resolution;
		int32_t Cascades;

		float CascadeSplits[4] = { 0.0f };
	};

	class FLARE_API Renderer
	{
	public:
		static void Initialize();
		static void Shutdown();

		static const RendererStatistics& GetStatistics();
		static void ClearStatistics();

		static void SetMainViewport(Viewport& viewport);

		static void BeginScene(Viewport& viewport);
		static void Flush();
		static void EndScene();

		static void DrawFullscreenQuad(const Ref<Material>& material);
		static void DrawMesh(const Ref<VertexArray>& mesh, const Ref<Material>& material, size_t indicesCount = SIZE_MAX);
		static void DrawMesh(const Ref<Mesh>& mesh,
			const Ref<Material>& material,
			const glm::mat4& transform,
			MeshRenderFlags flags = MeshRenderFlags::None,
			int32_t entityIndex = INT32_MAX);

		static void AddRenderPass(Ref<RenderPass> pass);
		static void ExecuteRenderPasses();

		static Viewport& GetMainViewport();
		static Viewport& GetCurrentViewport();

		static Ref<const VertexArray> GetFullscreenQuad();
		static Ref<Texture> GetWhiteTexture();

		static Ref<FrameBuffer> GetShadowsRenderTarget(size_t index);
		static ShadowSettings& GetShadowSettings();
	private:
		static void DrawQueued(bool shadowPass);
		static void FlushInstances();
	};
}