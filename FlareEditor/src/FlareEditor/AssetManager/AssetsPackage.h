#pragma once

#include "FlareCore/UUID.h"

#include <filesystem>
#include <map>
#include <string>

namespace Flare
{
	struct AssetsPackage
	{
		static const std::filesystem::path InternalPackagesLocation;

		enum class Type
		{
			Internal,
		};

		UUID Id;
		Type PackageType;
		std::string Name;
	};

	class PackageDependenciesSerializer
	{
	public:
		static void Serialize(const std::map<UUID, AssetsPackage>& packages, const std::filesystem::path& path);
		static bool Deserialize(std::map<UUID, AssetsPackage>& packages, const std::filesystem::path& path);

		static bool DeserializePackage(AssetsPackage& package, const std::filesystem::path& path);

		static const std::filesystem::path PackagesFileName;
	};
}