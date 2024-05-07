#pragma once

#include "FlareCore/UUID.h"
#include "FlareCore/Assert.h"

#include "FlareCore/Serialization/TypeInitializer.h"
#include "FlareCore/Serialization/TypeSerializer.h"

#include <filesystem>
#include <string_view>

namespace Flare
{
	struct FLARE_API AssetHandle
	{
	public:
		FLARE_TYPE;

		AssetHandle() = default;
		constexpr AssetHandle(UUID uuid)
			: m_UUID(uuid) {}

		constexpr operator UUID() const { return m_UUID; }
		constexpr operator uint64_t() const { return (uint64_t)m_UUID; }

		constexpr bool operator==(AssetHandle other) const { return m_UUID == other.m_UUID; }
		constexpr bool operator!=(AssetHandle other) const { return m_UUID != other.m_UUID; }
		constexpr AssetHandle& operator=(AssetHandle other) { m_UUID = other.m_UUID; return *this; }
		constexpr AssetHandle& operator=(UUID uuid) { m_UUID = uuid; return *this; }
	private:
		UUID m_UUID;
	};

	constexpr AssetHandle NULL_ASSET_HANDLE = 0;

	enum class AssetType
	{
		None,
		Scene,
		Texture,
		Sprite,
		Prefab,
		Shader,
		ComputeShader,
		Material,
		Font,
		Mesh,
		MaterialsTable,
	};

	enum class AssetSource
	{
		File,
		Memory,
	};

	FLARE_API std::string_view AssetSourceToString(AssetSource soruce);
	FLARE_API AssetSource AssetSourceFromString(std::string_view string);

	FLARE_API std::string_view AssetTypeToString(AssetType type);
	FLARE_API AssetType AssetTypeFromString(std::string_view string);

	struct AssetMetadata
	{
		AssetType Type;
		AssetSource Source;
		AssetHandle Handle;
		AssetHandle Parent;

		std::string Name;
		std::filesystem::path Path;

		std::vector<AssetHandle> SubAssets;
	};

	class FLARE_API AssetDescriptor
	{
	public:
		struct DescriptorsContainer
		{
			std::vector<AssetDescriptor*> Descriptors;
			std::unordered_map<const SerializableObjectDescriptor*, AssetDescriptor*> SerializationDescritproToAsset;
		};

		AssetDescriptor(const SerializableObjectDescriptor& descriptor);

		const SerializableObjectDescriptor& SerializationDescriptor;
	public:
		static DescriptorsContainer& GetDescriptors();
		static const AssetDescriptor* FindBySerializationDescriptor(const SerializableObjectDescriptor& descriptor);
	};

#define FLARE_ASSET                                                      \
	static Flare::AssetDescriptor _Asset;                                \
	virtual const Flare::AssetDescriptor& GetDescriptor() const override;

#define FLARE_IMPL_ASSET(assetType)                                                                      \
	Flare::AssetDescriptor assetType::_Asset(FLARE_SERIALIZATION_DESCRIPTOR_OF(assetType));              \
	const Flare::AssetDescriptor& assetType::GetDescriptor() const { return assetType::_Asset; }

	class FLARE_API Asset
	{
	public:
		Asset(AssetType type)
			: m_Type(type) {}
	public:
		virtual const AssetDescriptor& GetDescriptor() const = 0;
		inline AssetType GetType() const { return m_Type; }
	private:
		AssetType m_Type;
	public:
		AssetHandle Handle = NULL_ASSET_HANDLE;
	};
}

template<>
struct std::hash<Flare::AssetHandle>
{
	size_t operator()(Flare::AssetHandle handle) const
	{
		return std::hash<Flare::UUID>()((Flare::UUID)handle);
	}
};