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
}
