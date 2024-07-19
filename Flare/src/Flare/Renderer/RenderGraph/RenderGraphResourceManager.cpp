#include "RenderGraphResourceManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Viewport.h"

namespace Flare
{
	RenderGraphResourceManager::RenderGraphResourceManager(const Viewport& viewport)
		: m_Viewport(viewport)
	{
	}

	RenderGraphTextureId RenderGraphResourceManager::CreateTexture(TextureFormat format, std::string_view debugName)
	{
		FLARE_PROFILE_FUNCTION();
		RenderGraphTextureId id = RenderGraphTextureId((uint32_t)m_Textures.size());

		RenderGraphTextureResource& resource = m_Textures.emplace_back();
		resource.DebugName = debugName;
		resource.Format = format;

		TextureSpecifications specifications{};
		specifications.Width = m_Viewport.GetSize().x;
		specifications.Height = m_Viewport.GetSize().y;
		specifications.Format = resource.Format;
		specifications.Usage = TextureUsage::Sampling | TextureUsage::RenderTarget;
		specifications.GenerateMipMaps = false;
		specifications.Wrap = TextureWrap::Clamp;
		specifications.Filtering = TextureFiltering::Closest;

		resource.Texture = Texture::Create(specifications);

		return id;
	}

	RenderGraphTextureId RenderGraphResourceManager::RegisterExistingTexture(Ref<Texture> texture)
	{
		RenderGraphTextureId id = RenderGraphTextureId((uint32_t)m_Textures.size());

		RenderGraphTextureResource& resource = m_Textures.emplace_back();
		resource.Texture = texture;
		resource.DebugName = texture->GetDebugName();
		resource.Format = texture->GetFormat();

		return id;
	}

	void RenderGraphResourceManager::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		m_Textures.clear();
	}
}
