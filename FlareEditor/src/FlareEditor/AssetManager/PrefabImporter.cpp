#include "PrefabImporter.h"

#include "Flare/Scene/Prefab.h"

#include "FlareECS/World.h"

#include "FlareEditor/Serialization/Serialization.h"
#include "FlareEditor/Serialization/SceneSerializer.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <exception>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
    void PrefabImporter::SerializePrefab(AssetHandle prefab, World& world, Entity entity)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(prefab));

        if (!world.IsEntityAlive(entity))
            return;

        const AssetMetadata* metadata = AssetManager::GetAssetMetadata(prefab);
        
        YAML::Emitter emitter;
        SceneSerializer::SerializeEntity(emitter, world, entity);

        std::ofstream output(metadata->Path);
        output << emitter.c_str();
        output.close();

        Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
        assetManager->ReloadAsset(prefab);
    }

    Ref<Asset> PrefabImporter::ImportPrefab(const AssetMetadata& metadata)
    {
        try
        {
            YAML::Node node = YAML::LoadFile(metadata.Path.generic_string());
            
            World& world = World::GetCurrent();

            size_t prefabDataSize = 0;
            std::vector<ComponentId> ids;

            if (YAML::Node componentsNode = node["Components"])
            {
                for (YAML::Node componentNode : componentsNode)
                {
                    if (YAML::Node nameNode = componentNode["Name"])
                    {
                        std::string name = nameNode.as<std::string>();
                        auto componentId = world.Components.FindComponnet(name);
                        if (componentId.has_value())
                        {
                            prefabDataSize += world.Components.GetComponentInfo(componentId.value()).Size;
                            ids.push_back(componentId.value());
                        }
                    }
                }
            }

            std::sort(ids.begin(), ids.end());

            uint8_t* prefabData = nullptr;
            if (prefabDataSize != 0)
            {
                prefabData = new uint8_t[prefabDataSize];
                std::memset(prefabData, 0, prefabDataSize);

                size_t writeOffset = 0;

                if (YAML::Node componentsNode = node["Components"])
                {
                    for (YAML::Node componentNode : componentsNode)
                    {
                        if (YAML::Node nameNode = componentNode["Name"])
                        {
                            std::string name = nameNode.as<std::string>();
                            auto componentId = world.Components.FindComponnet(name);
                            
                            if (componentId)
                            {
                                const ComponentInfo& info = world.Components.GetComponentInfo(componentId.value());

                                if (info.Initializer)
                                    DeserializeType(componentNode, info.Initializer->Type, prefabData + writeOffset);

                                writeOffset += info.Size;
                            }
                        }
                    }
                }
            }

            const Archetypes& archetypes = world.GetArchetypes();
            auto it = archetypes.ComponentSetToArchetype.find(ComponentSet(ids.data(), ids.size()));
            if (it != archetypes.ComponentSetToArchetype.end())
                return CreateRef<Prefab>(it->second, prefabData);

            return CreateRef<Prefab>(std::move(ids), prefabData);
        }
        catch (std::exception& e)
        {
            FLARE_CORE_ERROR("Failed to import prefab '{0}': {1}", metadata.Path.generic_string(), e.what());
        }

        return nullptr;
    }
}
