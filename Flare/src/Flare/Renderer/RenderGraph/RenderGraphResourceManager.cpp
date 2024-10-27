#include "PCH.h"

#include "RenderGraphResourceManager.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Viewport.h"

namespace Flare
{
	RenderGraphResourceManager::RenderGraphResourceManager(const Viewport& viewport)
		: m_Viewport(viewport)
	{
		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		m_FrameInFlightViewportSizes.resize(frameInFlightCount, (glm::uvec2)viewport.GetSize());
	}

	inline static glm::uvec2 CalculateTextureSize(glm::uvec2 viewportSize, float textureScale)
	{
		return (glm::uvec2)glm::ceil((glm::vec2)viewportSize * textureScale);
	}

	RenderGraphTextureId RenderGraphResourceManager::CreateTexture(TextureFormat format, std::string_view debugName, float scale)
	{
		FLARE_PROFILE_FUNCTION();
		RenderGraphTextureId id = RenderGraphTextureId((uint32_t)m_Textures.size());

		RenderGraphTextureResource& resource = m_Textures.emplace_back();
		resource.DebugName = debugName;
		resource.Format = format;
		resource.TextureSizeConstraint = RenderGraphTextureResource::SizeConstraint::ViewportSize;
		resource.TextureHandleIndex = (uint32_t)m_TextureHandles.size();
		resource.Scale = scale;

		glm::uvec2 textureSize = CalculateTextureSize((glm::uvec2)m_Viewport.GetSize(), resource.Scale);

		TextureSpecifications specifications{};
		specifications.Width = textureSize.x;
		specifications.Height = textureSize.y;
		specifications.Format = resource.Format;
		specifications.Usage = TextureUsage::Sampling | TextureUsage::RenderTarget;
		specifications.Wrap = TextureWrap::Clamp;
		specifications.Filtering = TextureFiltering::Closest;

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			Ref<Texture> texture = Texture::Create(specifications);
			texture->SetDebugName(fmt::format("{}.#{}", debugName, i));

			m_TextureHandles.push_back(texture);
		}

		return id;
	}

	RenderGraphTextureId RenderGraphResourceManager::CreateFixedSizeTexture(TextureFormat format, glm::uvec2 size, std::string_view debugName, uint32_t arrayLayers, uint32_t mipCount)
	{
		FLARE_PROFILE_FUNCTION();

		RenderGraphTextureId id = RenderGraphTextureId((uint32_t)m_Textures.size());

		RenderGraphTextureResource& resource = m_Textures.emplace_back();
		resource.DebugName = debugName;
		resource.Format = format;
		resource.TextureSizeConstraint = RenderGraphTextureResource::SizeConstraint::Fixed;
		resource.TextureHandleIndex = (uint32_t)m_TextureHandles.size();
		resource.Scale = 1.0f;

		TextureSpecifications specifications{};
		specifications.Width = size.x;
		specifications.Height = size.y;
		specifications.Format = resource.Format;
		specifications.Usage = TextureUsage::Sampling | TextureUsage::RenderTarget;
		specifications.Wrap = TextureWrap::Clamp;
		specifications.Filtering = TextureFiltering::Closest;
		specifications.ArrayLayerCount = arrayLayers;
		specifications.MipCount = mipCount;

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			Ref<Texture> texture = Texture::Create(specifications);
			texture->SetDebugName(fmt::format("{}.#{}", debugName, i));

			m_TextureHandles.push_back(texture);
		}

		return id;
	}

	RenderGraphTextureId RenderGraphResourceManager::RegisterExistingTexture(Ref<Texture> texture)
	{
		FLARE_CORE_ASSERT(false);
		RenderGraphTextureId id = RenderGraphTextureId((uint32_t)m_Textures.size());

		RenderGraphTextureResource& resource = m_Textures.emplace_back();
		resource.DebugName = texture->GetDebugName();
		resource.Format = texture->GetFormat();
		resource.TextureSizeConstraint = RenderGraphTextureResource::SizeConstraint::Fixed;
		resource.TextureHandleIndex = (uint32_t)m_TextureHandles.size();

		m_TextureHandles.push_back(texture);

		return id;
	}

	void RenderGraphResourceManager::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		m_Textures.clear();
		m_TextureHandles.clear();
	}

	bool RenderGraphResourceManager::ResizeTextures()
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		glm::uvec2 viewportSize = (glm::vec2)m_Viewport.GetSize();

		if (m_FrameInFlightViewportSizes[frameIndex] == viewportSize)
			return false;

		for (const RenderGraphTextureResource& resource : m_Textures)
		{
			if (resource.TextureSizeConstraint == RenderGraphTextureResource::SizeConstraint::Fixed)
				continue;

			Ref<Texture> texture = GetTextureForFrameInFlight(resource, frameIndex);
			glm::uvec2 textureSize = CalculateTextureSize(viewportSize, resource.Scale);

			texture->Resize(textureSize.x, textureSize.y);
		}

		m_FrameInFlightViewportSizes[frameIndex] = viewportSize;
		return true;
	}

	Span<const Ref<Texture>> RenderGraphResourceManager::GetTexturesForEachFrameInFlight(const RenderGraphTextureResource& textureResource)
	{
		return Span<const Ref<Texture>>(
			m_TextureHandles.data() + textureResource.TextureHandleIndex,
			GraphicsContext::GetInstance().GetFrameInFlightCount());
	}
}
