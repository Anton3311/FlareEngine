#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include <glm/glm.hpp>

#include <vector>
#include <string_view>

namespace Flare
{
	enum class TypeFlags
	{
		None = 0,

		DefaultConstructable = 1,

		TriviallyConstructable = 2,
		TriviallyCopyConstructable,
		TriviallyMoveConstructable,

		TriviallyCopyAssignable,
		TriviallyMoveAssignable,
	};

	FLARE_IMPL_ENUM_BITFIELD(TypeFlags);

	template<typename T>
	TypeFlags GetTypeFlags()
	{
		TypeFlags flags = TypeFlags::None;

		if constexpr (std::is_default_constructible_v<T>)
			flags |= TypeFlags::DefaultConstructable;
		if constexpr (std::is_trivially_constructible_v<T>)
			flags |= TypeFlags::TriviallyConstructable;

		if constexpr (std::is_trivially_copy_constructible_v<T>)
			flags |= TypeFlags::TriviallyCopyConstructable;
		if constexpr (std::is_trivially_move_constructible_v<T>)
			flags |= TypeFlags::TriviallyMoveConstructable;

		if constexpr (std::is_trivially_copy_assignable_v<T>)
			flags |= TypeFlags::TriviallyCopyAssignable;
		if constexpr (std::is_trivially_move_assignable_v<T>)
			flags |= TypeFlags::TriviallyMoveAssignable;

		return flags;
	}

	class FLARECORE_API TypeInitializer
	{
	public:
		using DefaultConstructorFunction = void(*)(void*);
		using DestructorFunction = void(*)(void*);

		using CopyConstructorFunction = void(*)(void* instance, const void* copyFrom);
		using MoveConstructorFunction = void(*)(void* instance, void* moveFrom);

		TypeInitializer(std::string_view typeName, size_t size, size_t alignment,
			DestructorFunction destructor, 
			DefaultConstructorFunction constructor,
			MoveConstructorFunction moveConstructor,
			CopyConstructorFunction copyConstructor,
			TypeFlags flags);
		~TypeInitializer();

		static std::vector<TypeInitializer*>& GetInitializers();
	public:
		const std::string_view TypeName;
		const DestructorFunction Destructor;
		const DefaultConstructorFunction DefaultConstructor;
		const CopyConstructorFunction CopyConstructor;
		const MoveConstructorFunction MoveConstructor;
		const TypeFlags Flags;
		const size_t Size;
		const size_t Alignment;
	};
}

#define FLARE_TYPE static Flare::TypeInitializer _Type;

#define FLARE_IMPL_TYPE(typeName) Flare::TypeInitializer typeName::_Type =                            \
	Flare::TypeInitializer(typeid(typeName).name(), sizeof(typeName), alignof(typeName),              \
	[](void* instance) { ((typeName*)instance)->~typeName(); },                                       \
	[](void* instance) { new(instance) typeName;},                                                    \
	[](void* instance, void* moveFrom) { (*(typeName*)instance) = std::move(*(typeName*)moveFrom); }, \
	[](void* instance, const void* copyFrom) { (*(typeName*)instance) = *(typeName*)copyFrom; },      \
	Flare::GetTypeFlags<typeName>());