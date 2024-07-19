#pragma once

#include "FlareCore/Core.h"

#include <string_view>
#include <string>

namespace Flare
{
	class FLARE_API UniformBuffer
	{
	public:
		virtual ~UniformBuffer() = default;

		virtual void SetData(const void* data, size_t size, size_t offset) = 0;
		virtual size_t GetSize() const = 0;

		virtual void SetDebugName(std::string_view debugName) = 0;
		virtual const std::string& GetDebugName() const = 0;
	public:
		static Ref<UniformBuffer> Create(size_t size);
	};
}