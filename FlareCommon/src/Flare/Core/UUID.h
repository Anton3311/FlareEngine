#pragma once

#include <xhash>

namespace Flare
{
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t value);

		inline operator uint64_t() const { return m_Value; }
	private:
		uint64_t m_Value;
		
		friend struct std::hash<UUID>;
	};
}

template<>
struct std::hash<Flare::UUID>
{
	size_t operator()(Flare::UUID uuid) const
	{
		return std::hash<uint64_t>()(uuid.m_Value);
	}
};