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

	struct TypeConstructorFunctions
	{
		using DefaultConstructorFunction = void(*)(void* instance);
		using DestructorFunction = void(*)(void* instance);
		using CopyConstructorFunction = void(*)(void* instance, const void* copySource);
		using MoveConstructorFunction = void(*)(void* instance, void* moveSource);
		using CopyAssignmentFunction = void(*)(void* instance, const void* copySource);
		using MoveAssignmentFunction = void(*)(void* instance, void* moveSource);

		DefaultConstructorFunction DefaultConstructor = nullptr;
		DestructorFunction Destructor = nullptr;
		CopyConstructorFunction CopyConstructor = nullptr;
		MoveConstructorFunction MoveConstructor = nullptr;
		CopyAssignmentFunction CopyAssignment = nullptr;
		MoveAssignmentFunction MoveAssignment = nullptr;
	};

	template<typename T>
	TypeConstructorFunctions GetTypeConstructorFunctions()
	{
		TypeConstructorFunctions functions{};
		functions.DefaultConstructor = [](void* instance) { new(instance) T(); };
		functions.Destructor = [](void* instance) { (*(T*)instance).~T(); };
		functions.CopyConstructor = [](void* instance, const void* copySource)
			{
				if constexpr (std::is_copy_constructible_v<T>)
					new(instance) T(*(const T*)copySource);
			};
		functions.MoveConstructor = [](void* instance, void* moveSource)
			{
				if constexpr (std::is_move_constructible_v<T>)
					new (instance) T(std::move(*(T*)moveSource));
			};
		functions.CopyAssignment = [](void* instance, const void* copySource)
			{
				if constexpr (std::is_copy_assignable_v<T>)
					(*(T*)instance).operator=(*(const T*)copySource);
			};
		functions.MoveAssignment = [](void* instance, void* copySource)
			{
				if constexpr (std::is_move_assignable_v<T>)
					(*(T*)instance).operator=(std::move(*(const T*)copySource));
			};

		return functions;
	}

	class FLARECORE_API TypeInitializer
	{
	public:
		using DefaultConstructorFunction = void(*)(void*);
		using DestructorFunction = void(*)(void*);

		using CopyConstructorFunction = void(*)(void* instance, const void* copyFrom);
		using MoveConstructorFunction = void(*)(void* instance, void* moveFrom);

		TypeInitializer(std::string_view typeName, size_t size, size_t alignment,
			const TypeConstructorFunctions& constructorFunctions, TypeFlags flags);
		~TypeInitializer();

		static std::vector<TypeInitializer*>& GetInitializers();
	public:
		const std::string_view TypeName;
		const TypeConstructorFunctions Functions;
		const TypeFlags Flags;
		const size_t Size;
		const size_t Alignment;
	};
}

#define FLARE_TYPE static Flare::TypeInitializer _Type;

#define FLARE_IMPL_TYPE(typeName) Flare::TypeInitializer typeName::_Type =                  \
	Flare::TypeInitializer(typeid(typeName).name(), sizeof(typeName), alignof(typeName),    \
	Flare::GetTypeConstructorFunctions<typeName>(),                                         \
	Flare::GetTypeFlags<typeName>());