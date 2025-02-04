#pragma once

#include "FlareECS/Entity/Entity.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/GraphicsContext.h"

#include <string>
#include <stdint.h>

namespace Flare
{
	struct RenderGraphTextureId
	{
	public:
		RenderGraphTextureId() = default;

		explicit RenderGraphTextureId(uint32_t value)
			: m_Value(value) {}


		constexpr bool operator==(RenderGraphTextureId other) const { return m_Value == other.m_Value; }
		constexpr bool operator!=(RenderGraphTextureId other) const { return m_Value != other.m_Value; }

		constexpr uint32_t GetValue() const { return m_Value; }
	private:
		uint32_t m_Value = UINT32_MAX;
	};
}

template<>
struct std::hash<Flare::RenderGraphTextureId>
{
	size_t operator()(Flare::RenderGraphTextureId id) const
	{
		return std::hash<uint32_t>()(id.GetValue());
	}
};

namespace Flare
{
	enum class RenderGraphTextureAllocationMode : uint8_t
	{
		OnDemand,
		Preallocated,
	};

	struct RenderGraphTextureResource
	{
		enum class SizeConstraint
		{
			// The texture has a fixed size
			Fixed,

			// The size of a texture always matches the viewport size
			ViewportSize,
		};

		std::string DebugName;
		TextureFormat Format = TextureFormat::RGBA8;

		float Scale = 1.0f;

		// Textures are stored sequentially for each frame in flight
		// 
		// Texture for frame 0 is at index: TextureHandleIndex + 0
		// Texture for frame 1 is at index: TextureHandleIndex + 1
		// Texture for frame 2 is at index: TextureHandleIndex + 2
		uint32_t TextureHandleIndex = UINT32_MAX;
		uint32_t WritingPassesCount = 0;
		SizeConstraint TextureSizeConstraint = SizeConstraint::Fixed;

		uint8_t UniqueAllocationsMask = 0;
		RenderGraphTextureAllocationMode AllocationMode = RenderGraphTextureAllocationMode::Preallocated;
	};

	class World;
	class FLARE_API RenderGraphResourceManager
	{
	public:
		RenderGraphResourceManager(World& renderWorld, Entity viewportEntity);

		RenderGraphTextureId CreateTexture(TextureFormat format,
			std::string_view debugName,
			float scale = 1.0f,
			RenderGraphTextureAllocationMode allocationMode = RenderGraphTextureAllocationMode::OnDemand);

		RenderGraphTextureId CreateFixedSizeTexture(TextureFormat format,
			glm::uvec2 size,
			std::string_view debugName,
			uint32_t arrayLayers,
			uint32_t mipCount,
			RenderGraphTextureAllocationMode allocationMode = RenderGraphTextureAllocationMode::Preallocated);

		inline RenderGraphTextureId CreateFixedSizeTexture(TextureFormat format,
			glm::uvec2 size,
			std::string_view debugName,
			RenderGraphTextureAllocationMode allocationMode = RenderGraphTextureAllocationMode::Preallocated)
		{
			return CreateFixedSizeTexture(format, size, debugName, 1, 1, allocationMode);
		}

		RenderGraphTextureId RegisterExistingTexture(Ref<Texture> texture);

		void Clear();
		void UpdateOnDemandAllocatedTextures(std::unordered_set<RenderGraphTextureId>& updatedTextures);

		// Resizes the texture with SizeConstraint::ViewportSize.
		// Returns whether any textures were resized.
		bool ResizeTextures();

		inline bool IsTextureIdValid(RenderGraphTextureId textureId) const { return textureId.GetValue() < (uint32_t)m_Textures.size(); }
		inline Ref<Texture> GetTexture(RenderGraphTextureId textureId) const
		{
			FLARE_CORE_ASSERT(IsTextureIdValid(textureId));

			uint32_t textureHandleIndex = m_Textures[textureId.GetValue()].TextureHandleIndex;
			return m_TextureHandles[textureHandleIndex + GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
		}

		inline RenderGraphTextureResource& GetTextureResource(RenderGraphTextureId textureId) { return m_Textures[textureId.GetValue()]; }

		inline const RenderGraphTextureResource& GetTextureResource(RenderGraphTextureId textureId) const
		{
			FLARE_CORE_ASSERT(IsTextureIdValid(textureId));
			return m_Textures[textureId.GetValue()];
		}

		inline Ref<Texture> GetTextureForFrameInFlight(RenderGraphTextureId textureId, uint32_t frameInFlightIndex) const
		{
			FLARE_CORE_ASSERT(frameInFlightIndex < GraphicsContext::GetInstance().GetFrameInFlightCount());
			uint32_t textureHandleIndex = m_Textures[textureId.GetValue()].TextureHandleIndex;
			return m_TextureHandles[textureHandleIndex + frameInFlightIndex];
		}

		inline Ref<Texture> GetTextureForFrameInFlight(const RenderGraphTextureResource& texture, uint32_t frameInFlightIndex) const
		{
			FLARE_CORE_ASSERT(frameInFlightIndex < GraphicsContext::GetInstance().GetFrameInFlightCount());
			uint32_t textureHandleIndex = texture.TextureHandleIndex;
			return m_TextureHandles[textureHandleIndex + frameInFlightIndex];
		}

		inline TextureFormat GetTextureFormat(RenderGraphTextureId textureId) const
		{
			FLARE_CORE_ASSERT(IsTextureIdValid(textureId));
			return m_Textures[textureId.GetValue()].Format;
		}

		Span<const Ref<Texture>> GetTexturesForEachFrameInFlight(const RenderGraphTextureResource& textureResource);

		inline size_t GetTextureResourceCount() const { return m_Textures.size(); }
	private:
		void FillTextureSpecifications(TextureSpecifications& specifications, const RenderGraphTextureResource& resource, glm::uvec2 fixedSize) const;
		void PreallocateTexturesForResource(const RenderGraphTextureResource& resource, glm::uvec2 fixedSize);
	private:
		Entity m_ViewportEntity;
		World& m_RenderWorld;

		std::vector<RenderGraphTextureResource> m_Textures;
		std::vector<Ref<Texture>> m_TextureHandles;

		// The size of each texture with SizeConstraint::ViewportSize
		// that is used in specific frame in flight.
		std::vector<glm::uvec2> m_FrameInFlightViewportSizes;
	};
}
