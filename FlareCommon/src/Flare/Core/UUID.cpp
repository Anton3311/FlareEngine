#include "UUID.h"

#include <random>

namespace Flare
{
	static std::random_device s_Device;
	static std::mt19937_64 s_Engine(s_Device());
	static std::uniform_int_distribution<uint64_t> s_UniformDistricution;

	UUID::UUID()
		: m_Value(s_UniformDistricution(s_Engine)) {}
}