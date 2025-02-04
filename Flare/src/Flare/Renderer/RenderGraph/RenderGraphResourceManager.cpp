#include "PCH.h"

#include "RenderGraphResourceManager.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"

namespace Flare
{
	RenderGraphResourceManager::RenderGraphResourceManager(World& renderWorld, Entity viewportEntity)
		: m_ViewportEntity(viewportEntity), m_RenderWorld(renderWorld)
	{
		const Viewport* viewport = renderWorld.TryGetEntityComponent<const Viewport>(viewportEntity);
		FLARE_CORE_ASSERT(viewport);

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		m_FrameInFlightViewportSizes.resize(frameInFlightCount, viewport->Size);
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
		resource.AllocationMode = RenderGraphTextureAllocationMode::OnDemand;
		resource.UniqueAllocationsMask = 1;
		resource.TextureHandleIndex = (uint32_t)m_TextureHandles.size();
		resource.Scale = scale;

		PreallocateTexturesForResource(resource, glm::uvec2(0, 0));

		return id;
	}

	RenderGraphTextureId RenderGraphResourceManager::CreateFixedSizeTexture(TextureFormat format,
		glm::uvec2 size,
		std::string_view debugName,
		uint32_t arrayLayers,
		uint32_t mipCount,
		RenderGraphTextureAllocationMode allocationMode)
	{
		FLARE_PROFILE_FUNCTION();

		RenderGraphTextureId id = RenderGraphTextureId((uint32_t)m_Textures.size());

		RenderGraphTextureResource& resource = m_Textures.emplace_back();
		resource.DebugName = debugName;
		resource.Format = format;
		resource.TextureSizeConstraint = RenderGraphTextureResource::SizeConstraint::Fixed;
		resource.TextureHandleIndex = (uint32_t)m_TextureHandles.size();
		resource.AllocationMode = allocationMode;
		resource.UniqueAllocationsMask = 1;
		resource.Scale = 1.0f;

		PreallocateTexturesForResource(resource, size);

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

	void RenderGraphResourceManager::UpdateOnDemandAllocatedTextures(std::unordered_set<RenderGraphTextureId>& updatedTextures)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		uint32_t frameCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		uint32_t previousFrameIndex = (frameCount + frameIndex - 1) % frameCount;

		uint8_t fullAllocationMask = static_cast<uint8_t>((1 << frameCount) - 1);
		uint8_t currentAllocationMask = static_cast<uint8_t>(1 << frameIndex);

		for (size_t textureResourceIndex = 0; textureResourceIndex < m_Textures.size(); textureResourceIndex++)
		{
			RenderGraphTextureResource& textureResource = m_Textures[textureResourceIndex];
			if (textureResource.AllocationMode == RenderGraphTextureAllocationMode::Preallocated)
				continue;

			RenderGraphTextureId textureId = RenderGraphTextureId(static_cast<uint32_t>(textureResourceIndex));
			Ref<Texture>* textureHandles = m_TextureHandles.data() + textureResource.TextureHandleIndex;

			bool releaseCurrentTexture = textureResource.WritingPassesCount == 0;
			if (!releaseCurrentTexture)
			{
				// There is a render pass that wants to write to this texture,
				// so allocate an actual `Ref<Texture>` if is there isn't one for the current frame in flight

				if ((textureResource.UniqueAllocationsMask & currentAllocationMask) == 0)
				{
					TextureSpecifications specifications{};

					FLARE_CORE_ASSERT(textureResource.TextureSizeConstraint == RenderGraphTextureResource::SizeConstraint::ViewportSize);
					FillTextureSpecifications(specifications, textureResource, glm::uvec2(0, 0));

					textureHandles[frameIndex] = Texture::Create(specifications);
					textureHandles[frameIndex]->SetDebugName(fmt::format("{}.#{}", textureResource.DebugName, frameIndex));

					textureResource.UniqueAllocationsMask |= currentAllocationMask;

					updatedTextures.insert(textureId);
				}
			}
			else
			{
				// There are no passes that write to this texture resource.
				// If there is an allocated texture for this frame in flight, release it.

				if (textureResource.UniqueAllocationsMask & currentAllocationMask)
				{
					textureHandles[frameIndex] = textureHandles[previousFrameIndex];
					textureResource.UniqueAllocationsMask &= ~currentAllocationMask;

					updatedTextures.insert(textureId);
				}
			}
		}
	}

	bool RenderGraphResourceManager::ResizeTextures()
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		const Viewport* viewport = m_RenderWorld.TryGetEntityComponent<const Viewport>(m_ViewportEntity);
		FLARE_CORE_ASSERT(viewport);

		if (m_FrameInFlightViewportSizes[frameIndex] == viewport->Size)
			return false;

		for (const RenderGraphTextureResource& resource : m_Textures)
		{
			if (resource.TextureSizeConstraint == RenderGraphTextureResource::SizeConstraint::Fixed)
				continue;

			Ref<Texture> texture = GetTextureForFrameInFlight(resource, frameIndex);
			glm::uvec2 textureSize = CalculateTextureSize(viewport->Size, resource.Scale);

			texture->Resize(textureSize.x, textureSize.y);
		}

		m_FrameInFlightViewportSizes[frameIndex] = viewport->Size;
		return true;
	}

	Span<const Ref<Texture>> RenderGraphResourceManager::GetTexturesForEachFrameInFlight(const RenderGraphTextureResource& textureResource)
	{
		return Span<const Ref<Texture>>(
			m_TextureHandles.data() + textureResource.TextureHandleIndex,
			GraphicsContext::GetInstance().GetFrameInFlightCount());
	}

	void RenderGraphResourceManager::FillTextureSpecifications(TextureSpecifications& specifications,
		const RenderGraphTextureResource& resource,
		glm::uvec2 fixedSize) const
	{
		const Viewport* viewport = Renderer::GetRenderWorld().TryGetEntityComponent<const Viewport>(m_ViewportEntity);
		FLARE_CORE_ASSERT(viewport);

		glm::uvec2 textureSize = resource.TextureSizeConstraint == RenderGraphTextureResource::SizeConstraint::Fixed
			? fixedSize
			: CalculateTextureSize(viewport->Size, resource.Scale);

		specifications.Width = textureSize.x;
		specifications.Height = textureSize.y;
		specifications.Format = resource.Format;
		specifications.Usage = TextureUsage::Sampling | TextureUsage::RenderTarget;
		specifications.Wrap = TextureWrap::Clamp;
		specifications.Filtering = TextureFiltering::Closest;
	}

	void RenderGraphResourceManager::PreallocateTexturesForResource(const RenderGraphTextureResource& resource, glm::uvec2 fixedSize)
	{
		FLARE_PROFILE_FUNCTION();

		TextureSpecifications specifications{};
		FillTextureSpecifications(specifications, resource, fixedSize);

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		switch (resource.AllocationMode)
		{
		case RenderGraphTextureAllocationMode::OnDemand:
		{
			Ref<Texture> texture = Texture::Create(specifications);
			texture->SetDebugName(fmt::format("{}.#{}", resource.DebugName, 0));

			m_TextureHandles.push_back(texture);
			for (uint32_t i = 1; i < frameInFlightCount; i++)
				m_TextureHandles.push_back(texture);
			break;
		}
		case RenderGraphTextureAllocationMode::Preallocated:
		{
			for (uint32_t i = 0; i < frameInFlightCount; i++)
			{
				Ref<Texture> texture = Texture::Create(specifications);
				texture->SetDebugName(fmt::format("{}.#{}", resource.DebugName, i));
				m_TextureHandles.push_back(texture);
			}
		}
		}
	}
}
