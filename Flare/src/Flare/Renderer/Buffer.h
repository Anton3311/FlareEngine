#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include <stdint.h>
#include <string>
#include <string_view>

namespace Flare
{
	class CommandBuffer;

	enum class GPUBufferMemoryType
	{
		Static = 0,
		Dynamic = 1,
	};

	enum class GPUBufferUsage
	{
		None = 0,
		VertexBuffer = 1,
		IndexBuffer = 2,
		StorageBuffer = 4,
		UniformBuffer = 8,
		IndirectDrawBuffer = 16,
		Readback = 32,
	};

	FLARE_IMPL_ENUM_BITFIELD(GPUBufferUsage);

	struct GPUBufferSpecifications
	{
		size_t Size = 0;
		GPUBufferMemoryType MemoryType = GPUBufferMemoryType::Static;
		GPUBufferUsage Usage = GPUBufferUsage::None;
	};

	enum class IndexFormat
	{
		UInt32,
		UInt16,
	};

	inline size_t GetIndexFormatSize(IndexFormat format)
	{
		switch (format)
		{
		case IndexFormat::UInt16:
			return sizeof(uint16_t);
		case IndexFormat::UInt32:
			return sizeof(uint32_t);
		}

		FLARE_CORE_VERIFY_UNREACHABLE();
		return 0;
	}

	class FLARE_API GPUBuffer : public RefCounted<GPUBuffer>
	{
	public:
		virtual ~GPUBuffer() = default;

		virtual void SetData(MemorySpan data, size_t offset) = 0;
		virtual void SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer) = 0;

		virtual void ReadData(size_t readOffset, void* outBuffer) = 0;

		virtual void Resize(size_t newSize) = 0;

		virtual const GPUBufferSpecifications& GetSpecifications() const = 0;

		virtual void SetDebugName(std::string_view debugName) = 0;
		virtual const std::string& GetDebugName() const = 0;

		inline size_t GetSize() const { return GetSpecifications().Size; }
	public:
		static Ref<GPUBuffer> Create(const GPUBufferSpecifications& specifications);

		inline static Ref<GPUBuffer> CreateVertexBuffer(size_t size, GPUBufferMemoryType memoryType)
		{
			GPUBufferSpecifications specifications{};
			specifications.MemoryType = memoryType;
			specifications.Size = size;
			specifications.Usage = GPUBufferUsage::VertexBuffer;
			return Create(specifications);
		}

		static Ref<GPUBuffer> CreateIndexBuffer(size_t indexCount, IndexFormat format, GPUBufferMemoryType memoryType)
		{
			GPUBufferSpecifications specifications{};
			specifications.MemoryType = memoryType;
			specifications.Size = indexCount * GetIndexFormatSize(format);
			specifications.Usage = GPUBufferUsage::IndexBuffer;
			return Create(specifications);
		}

		static Ref<GPUBuffer> CreateUniformBuffer(size_t size)
		{
			GPUBufferSpecifications specifications{};
			specifications.MemoryType = GPUBufferMemoryType::Dynamic;
			specifications.Size = size;
			specifications.Usage = GPUBufferUsage::UniformBuffer;
			return Create(specifications);
		}

		static Ref<GPUBuffer> CreateStorageBuffer(size_t size, GPUBufferMemoryType memoryType)
		{
			GPUBufferSpecifications specifications{};
			specifications.MemoryType = memoryType;
			specifications.Size = size;
			specifications.Usage = GPUBufferUsage::StorageBuffer;
			return Create(specifications);
		}
	};
}
