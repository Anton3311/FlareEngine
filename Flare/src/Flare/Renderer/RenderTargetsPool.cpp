#include "RenderTargetsPool.h"

namespace Flare
{
    Ref<FrameBuffer> RenderTargetsPool::Get()
    {
        if (m_Pool.size() == 0)
            return FrameBuffer::Create(m_Specifications);

        Ref<FrameBuffer> target = m_Pool.back();
        m_Pool.pop_back();

        const FrameBufferSpecifications& specs = target->GetSpecifications();
        bool needsResizing = specs.Width != m_Specifications.Width || specs.Height != m_Specifications.Height;
        
        if (needsResizing)
            target->Resize(m_Specifications.Width, m_Specifications.Height);
        return target;
    }

    Ref<FrameBuffer> RenderTargetsPool::GetFullscreen(const Span<TextureFormat>& formats)
    {
        auto it = m_FullscreenTargets.find(formats);
        if (it == m_FullscreenTargets.end())
        {
            FullscreenRenderTargetEntry entry{};
            entry.Formats = std::vector<TextureFormat>(formats.begin(), formats.end());

            FrameBufferSpecifications& specifications = entry.Specifications;
            specifications.Width = m_ViewportSize.x;
            specifications.Height = m_ViewportSize.y;
            specifications.Attachments.resize(formats.GetSize());

            for (size_t i = 0; i < formats.GetSize(); i++)
            {
                specifications.Attachments[i].Filtering = TextureFiltering::Closest;
                specifications.Attachments[i].Format = formats[i];
                specifications.Attachments[i].Wrap = TextureWrap::Clamp;
            }

            Ref<FrameBuffer> renderTarget = FrameBuffer::Create(specifications);

            entry.Pool.push_back(renderTarget);

            m_FullscreenTargets.emplace(Span<TextureFormat>::FromVector(entry.Formats), std::move(entry));
            return renderTarget;
        }
        else
        {
            auto& entry = it->second;
            Ref<FrameBuffer> renderTarget = nullptr;

            if (entry.Specifications.Width != m_ViewportSize.x || entry.Specifications.Height != m_ViewportSize.y)
            {
                entry.Specifications.Width = m_ViewportSize.x;
                entry.Specifications.Height = m_ViewportSize.y;
            }

            if (entry.Pool.size() > 0)
            {
                renderTarget = entry.Pool.back();
                entry.Pool.pop_back();

				const auto& specs = renderTarget->GetSpecifications();
				if (specs.Width != m_ViewportSize.x || specs.Height != m_ViewportSize.y)
				{
					renderTarget->Resize(m_ViewportSize.x, m_ViewportSize.y);
				}
            }
            else
            {
                renderTarget = FrameBuffer::Create(entry.Specifications);
            }

            return renderTarget;
        }
    }

    void RenderTargetsPool::ReturnFullscreen(Span<TextureFormat> formats, Ref<FrameBuffer> renderTarget)
    {
        auto it = m_FullscreenTargets.find(formats);
        FLARE_CORE_ASSERT(it != m_FullscreenTargets.end());

        it->second.Pool.push_back(renderTarget);
    }

    void RenderTargetsPool::Release(const Ref<FrameBuffer>& renderTarget)
    {
        m_Pool.push_back(renderTarget);
    }

    void RenderTargetsPool::SetRenderTargetsSize(glm::ivec2 size)
    {
        m_Specifications.Width = (uint32_t)size.x;
        m_Specifications.Height = (uint32_t)size.y;
    }

    void RenderTargetsPool::SetSpecifications(const FrameBufferSpecifications& specs)
    {
        m_Specifications = specs;
    }

    void RenderTargetsPool::OnViewportResize(glm::uvec2 newSize)
    {
        m_ViewportSize = newSize;
    }

    void RenderTargetsPool::Clear()
    {
        m_FullscreenTargets.clear();
        m_Pool.clear();
    }
}
