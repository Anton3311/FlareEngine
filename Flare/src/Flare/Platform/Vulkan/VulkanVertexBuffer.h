#pragma once

#include "Flare/Renderer/Buffer.h"

namespace Flare
{
	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		const BufferLayout& GetLayout() const override;
		void SetLayout(const BufferLayout& layout) override;
		void Bind() override;
		void SetData(const void* data, size_t size, size_t offset) override;
	private:
		BufferLayout m_Layout;
	};
}
