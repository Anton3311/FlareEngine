#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class FLARE_API UniformBuffer
	{
	public:
		virtual ~UniformBuffer() = default;

		virtual void SetData(const void* data, size_t size, size_t offset) = 0;
		virtual size_t GetSize() const = 0;
	public:
		static Ref<UniformBuffer> Create(size_t size);
	};
}