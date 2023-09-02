#include "TypeInitializer.h"

#include "Flare/Core/Assert.h"

namespace Flare
{
    TypeInitializer::TypeInitializer(std::string_view typeName, size_t size, 
        DestructorFunction destructor,
        DefaultConstructorFunction constructor,
        const std::initializer_list<FieldData>& fields)
        : TypeName(typeName), Size(size), 
          Destructor(destructor),
          DefaultConstructor(constructor),
          SerializedFields(fields)
    {
        GetInitializers().push_back(this);
    }

    TypeInitializer::~TypeInitializer()
    {
        auto& initializers = GetInitializers();
        for (size_t i = 0; i < initializers.size(); i++)
        {
            if (initializers[i] == this)
            {
                initializers.erase(initializers.begin() + i);
                break;
            }
        }
    }

    std::vector<TypeInitializer*>& TypeInitializer::GetInitializers()
    {
        static std::vector<TypeInitializer*> s_Initializers;
        return s_Initializers;
    }

    size_t GetFieldTypeSize(SerializableFieldType fieldType)
    {
        switch (fieldType)
        {
        case SerializableFieldType::None:
            return 0;
        case SerializableFieldType::Bool:
            return sizeof(bool);
        case SerializableFieldType::Int8:
            return sizeof(int8_t);
        case SerializableFieldType::UInt8:
            return sizeof(uint8_t);
        case SerializableFieldType::Int16:
            return sizeof(int16_t);
        case SerializableFieldType::UInt16:
            return sizeof(uint16_t);
        case SerializableFieldType::Int32:
            return sizeof(int32_t);
        case SerializableFieldType::UInt32:
            return sizeof(uint32_t);
        case SerializableFieldType::Int64:
            return sizeof(int64_t);
        case SerializableFieldType::UInt64:
            return sizeof(uint64_t);
        case SerializableFieldType::Float32:
            return sizeof(float);
        case SerializableFieldType::Float64:
            return sizeof(double);
        case SerializableFieldType::Float2:
            return sizeof(glm::vec2);
        case SerializableFieldType::Float3:
            return sizeof(glm::vec3);
        case SerializableFieldType::Float4:
            return sizeof(glm::vec4);
        case SerializableFieldType::Int2:
            return sizeof(glm::ivec2);
        case SerializableFieldType::Int3:
            return sizeof(glm::ivec3);
        case SerializableFieldType::Int4:
            return sizeof(glm::ivec4);
        case SerializableFieldType::UInt2:
            return sizeof(glm::uvec2);
        case SerializableFieldType::UInt3:
            return sizeof(glm::uvec3);
        case SerializableFieldType::UInt4:
            return sizeof(glm::uvec4);
        default:
            FLARE_CORE_ASSERT(false, "Unhandled field type");
        }
        return 0;
    }

    size_t FieldTypeInfo::GetSize() const
    {
        FLARE_CORE_ASSERT(FieldType != SerializableFieldType::Array);
        switch (FieldType)
        {
        case SerializableFieldType::Custom:
            return CustomType->Size;
        default:
            return GetFieldTypeSize(FieldType);
        }
        return 0;
    }
}