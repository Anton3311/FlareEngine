#pragma once

#include "FlareCore/Collections/Span.h"

#include <stdint.h>

namespace Flare
{
    class FLARE_API ShaderStorageBuffer
    {
    public:
        virtual ~ShaderStorageBuffer() {}

        virtual size_t GetSize() const = 0;
        virtual void SetData(const MemorySpan& data) = 0;
    public:
        static Ref<ShaderStorageBuffer> Create(uint32_t binding);
    };
}
