#pragma once

#include <atomic>
#include <intrin0.inl.h>
#include <mmintrin.h>
#include <intrin0.h>
#include <memory>

namespace Flare
{
	using ReferenceCounterT = uint64_t;

	template<typename T>
	class Ref;

	class RefCountedTag {};

	template<typename T>
	class RefCounted : public RefCountedTag
	{
	public:
		virtual ~RefCounted() = default;
	private:
		std::atomic_uint32_t m_ReferenceCount = 0;
		friend class Ref<T>;
	};

	template<typename T>
	constexpr bool IsRefCounted = std::is_base_of_v<RefCounted<T>, T>;

	template<typename T>
	class Ref
	{
	public:
		using RefCountedT = RefCounted<T>;
		using ElementT = T;

		constexpr Ref() = default;

		constexpr Ref(std::nullptr_t)
			: m_Instance(nullptr) {}

		explicit Ref(T* instance)
		{
			m_Instance = instance;

			if (m_Instance != nullptr)
				IncrementRefCount(m_Instance);
		}

		template<typename U, std::enable_if_t<std::_SP_pointer_compatible<U, T>::value, int> = 0>
		explicit Ref(U* instance)
		{
			m_Instance = (T*)instance;

			if (m_Instance != nullptr)
				IncrementRefCount(m_Instance);
		}

		inline Ref(const Ref<T>& other)
		{
			m_Instance = other.m_Instance;

			if (m_Instance != nullptr)
				IncrementRefCount(m_Instance);
		}

		template<typename U, std::enable_if_t<std::_SP_pointer_compatible<U, T>::value, int> = 0>
		inline Ref(const Ref<U>& other)
		{
			m_Instance = (T*)other.m_Instance;

			if (m_Instance != nullptr)
				IncrementRefCount(m_Instance);
		}

		inline Ref(Ref<T>&& other)
		{
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
		}

		template<typename U, std::enable_if_t<std::_SP_pointer_compatible<U, T>::value, int> = 0>
		constexpr Ref(Ref<U>&& other) noexcept
		{
			m_Instance = (T*)other.m_Instance;
			other.m_Instance = nullptr;
		}

		~Ref()
		{
			ReleaseCurrent();
		}

		inline Ref<T>& operator=(const Ref<T>& other)
		{
			ReleaseCurrent();

			m_Instance = other.m_Instance;

			if (m_Instance)
				IncrementRefCount(m_Instance);

			return *this;
		}

		template<typename U>
		inline Ref<T>& operator=(const Ref<U>& other)
		{
			static_assert(std::is_convertible_v<U*, T*>);
			ReleaseCurrent();

			m_Instance = other.m_Instance;
			
			if (m_Instance)
				IncrementRefCount(m_Instance);

			return *this;
		}

		constexpr Ref<T>& operator=(Ref<T>&& other) noexcept
		{
			ReleaseCurrent();

			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;

			return *this;
		}

		template<typename U>
		constexpr Ref<T>& operator=(Ref<U>&& other) noexcept
		{
			static_assert(std::is_convertible_v<U*, T*>);
			ReleaseCurrent();

			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;

			return *this;
		}

		inline operator Ref<const T>() const { return Ref<const T>((const T*)m_Instance); }

		template<typename U>
		inline Ref<U> As() const
		{
			static_assert(std::is_base_of_v<RefCountedTag, U>, "U is not a RefCounted");
			return Ref<U>((U*)m_Instance);
		}

		template<typename U>
		U& DerefAs() const
		{
			static_assert(std::is_base_of_v<RefCountedTag, U>, "U is not a RefCounted");
			return *(U*)m_Instance;
		}

		constexpr T* GetRawPointer() const { return (T*)m_Instance; }
		constexpr T& operator*() const { return *(T*)m_Instance; }
		constexpr T* operator->() const { return (T*)m_Instance; }

		constexpr bool operator==(std::nullptr_t) const { return m_Instance == nullptr; }
		constexpr bool operator!=(std::nullptr_t) const { return m_Instance != nullptr; }

		constexpr bool operator==(const Ref<T>& other) const { return m_Instance == other.m_Instance; }
		constexpr bool operator!=(const Ref<T>& other) const { return m_Instance != other.m_Instance; }

		constexpr operator bool() const { return m_Instance != nullptr; }
		constexpr void Swap(Ref<T>& other) { std::swap(other.m_Instance, m_Instance); }

		template<typename... Args>
		inline static Ref<T> New(Args&&... args)
		{
			static_assert(std::is_base_of_v<RefCountedTag, T>, "T is not a RefCounted");
			return Ref<T>(new T(std::forward<Args>(args)...));
		}
	private:
		inline void ReleaseCurrent()
		{
			if (m_Instance == nullptr)
				return;

			if (DecrementRefCount(m_Instance) == 0)
			{
				delete (RefCounted<T>*)m_Instance;
				m_Instance = nullptr;
			}
		}

		inline static ReferenceCounterT IncrementRefCount(T* instance)
		{
			return ((RefCounted<T>*)instance)->m_ReferenceCount.fetch_add(1, std::memory_order_relaxed) + 1;
		}

		inline static ReferenceCounterT DecrementRefCount(T* instance)
		{
			return ((RefCounted<T>*)instance)->m_ReferenceCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
		}
	private:
		T* m_Instance = nullptr;

		template<typename U>
		friend class Ref;
	};
}
