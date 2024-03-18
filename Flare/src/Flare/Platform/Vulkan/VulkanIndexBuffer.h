#pragma once

#include "Flare/Renderer/Buffer.h"

namespace Flare
{
	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		void Bind() override;
		void SetData(const MemorySpan& indices, size_t offset) override;
		size_t GetCount() const override;
		IndexFormat GetIndexFormat() const override;
	private:
	};
}
