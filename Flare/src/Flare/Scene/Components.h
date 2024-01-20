#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "Flare.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Renderer/Mesh.h"
#include "Flare/Renderer/Material.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/ComponentInitializer.h"

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
    struct FLARE_API NameComponent
    {
        FLARE_COMPONENT;

        NameComponent();
        NameComponent(std::string_view name);
        ~NameComponent();

        std::string Value;
    };

    template<>
    struct TypeSerializer<NameComponent>
    {
        void OnSerialize(NameComponent& name, SerializationStream& stream)
        {
            stream.Serialize("Value", SerializationValue(name.Value));
        }
    };



    struct FLARE_API TransformComponent
    {
        FLARE_COMPONENT;

        TransformComponent();
        TransformComponent(const glm::vec3& position);
        TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
        
        glm::mat4 GetTransformationMatrix() const;
        glm::vec3 TransformDirection(const glm::vec3& direction) const;

        glm::vec3 Position;
        glm::vec3 Rotation;
        glm::vec3 Scale;
    };

    template<>
    struct TypeSerializer<TransformComponent>
    {
        void OnSerialize(TransformComponent& transform, SerializationStream& stream)
        {
            stream.Serialize("Position", SerializationValue(transform.Position));
            stream.Serialize("Rotation", SerializationValue(transform.Rotation));
            stream.Serialize("Scale", SerializationValue(transform.Scale));
        }
    };



    struct FLARE_API CameraComponent
    {
        FLARE_COMPONENT;

        enum class ProjectionType : uint8_t
        {
            Orthographic,
            Perspective,
        };

        CameraComponent();
        CameraComponent(ProjectionType projection);

        glm::mat4 GetProjection() const;
        glm::vec3 ScreenToWorld(glm::vec2 point) const;
        glm::vec3 ViewportToWorld(glm::vec2 point) const;

        ProjectionType Projection;

        float Size;
        float FOV;
        float Near;
        float Far;
    };

    template<>
    struct TypeSerializer<CameraComponent>
    {
        void OnSerialize(CameraComponent& camera, SerializationStream& stream)
        {
            stream.Serialize("Size", SerializationValue(camera.Size));
            stream.Serialize("FOV", SerializationValue(camera.FOV));
            stream.Serialize("Near", SerializationValue(camera.Near));
            stream.Serialize("Far", SerializationValue(camera.Far));
        }
    };



    struct FLARE_API SpriteComponent
    {
        FLARE_COMPONENT;

        SpriteComponent();
        SpriteComponent(AssetHandle texture);

        glm::vec4 Color;
        glm::vec2 TextureTiling;
        AssetHandle Texture;
        SpriteRenderFlags Flags;
    };

    template<>
    struct TypeSerializer<SpriteComponent>
    {
        void OnSerialize(SpriteComponent& sprite, SerializationStream& stream)
        {
            stream.Serialize("Color", SerializationValue(sprite.Color, SerializationValueFlags::Color));
            stream.Serialize("TextureTiling", SerializationValue(sprite.TextureTiling));
            stream.Serialize("Texture", SerializationValue(sprite.Texture));
            
            using FlagsUnderlyingType = std::underlying_type_t<decltype(sprite.Flags)>;
            stream.Serialize("Flags", SerializationValue(reinterpret_cast<FlagsUnderlyingType&>(sprite.Flags)));
        }
    };



    struct FLARE_API SpriteLayer
    {
        FLARE_COMPONENT;

        SpriteLayer();
        SpriteLayer(int32_t layer);

        int32_t Layer;
    };

    template<>
    struct TypeSerializer<SpriteLayer>
    {
        void OnSerialize(SpriteLayer& value, SerializationStream& stream)
        {
            stream.Serialize("Layer", SerializationValue(value.Layer));
        }
    };



    struct FLARE_API MaterialComponent
    {
        FLARE_COMPONENT;

        MaterialComponent();
        MaterialComponent(AssetHandle handle);

        AssetHandle Material;
    };

    template<>
    struct TypeSerializer<MaterialComponent>
    {
        void OnSerialize(MaterialComponent& material, SerializationStream& stream)
        {
            stream.Serialize("Material", SerializationValue(material.Material));
        }
    };



    struct FLARE_API TextComponent
    {
        FLARE_COMPONENT;

        TextComponent();
        TextComponent(std::string_view text, const glm::vec4& color = glm::vec4(1.0f), const Ref<Font>& font = nullptr);

        std::string Text;
        glm::vec4 Color;
        Ref<Font> Font;
    };

    template<>
    struct TypeSerializer<TextComponent>
    {
        void OnSerialize(TextComponent& text, SerializationStream& stream)
        {
            stream.Serialize("Text", SerializationValue(text.Text));
            stream.Serialize("Color", SerializationValue(text.Color, SerializationValueFlags::Color));
            stream.Serialize("Font", SerializationValue(text.Font));
        }
    };



    struct FLARE_API MeshComponent
    {
        FLARE_COMPONENT;

        MeshComponent(MeshRenderFlags flags = MeshRenderFlags::None);
        MeshComponent(AssetHandle mesh, AssetHandle material, MeshRenderFlags flags = MeshRenderFlags::None);

        AssetHandle Mesh;
        AssetHandle Material;
        MeshRenderFlags Flags;
    };

    template<>
    struct TypeSerializer<MeshComponent>
    {
        void OnSerialize(MeshComponent& mesh, SerializationStream& stream)
        {
            stream.Serialize("Mesh", SerializationValue(mesh.Mesh));
            stream.Serialize("Material", SerializationValue(mesh.Material));

            using FlagsUnderlyingType = std::underlying_type_t<decltype(mesh.Flags)>;
            stream.Serialize("Flags", SerializationValue(reinterpret_cast<FlagsUnderlyingType&>(mesh.Flags)));
        }
    };



    struct FLARE_API DirectionalLight
    {
        FLARE_COMPONENT;

        DirectionalLight();
        DirectionalLight(const glm::vec3& color, float intensity);

        glm::vec3 Color;
        float Intensity;
    };

    template<>
    struct TypeSerializer<DirectionalLight>
    {
        void OnSerialize(DirectionalLight& light, SerializationStream& stream)
        {
            stream.Serialize("Color", SerializationValue(light.Color, SerializationValueFlags::Color));
            stream.Serialize("Intensity", SerializationValue(light.Intensity));
        }
    };



    struct FLARE_API PointLight
    {
        FLARE_COMPONENT;

        PointLight()
            : Color(glm::vec3(1.0f)), Intensity(1.0f) {}
        PointLight(const glm::vec3& color, float intensity)
            : Color(color), Intensity(intensity) {}

        glm::vec3 Color;
        float Intensity;
    };

    template<>
    struct TypeSerializer<PointLight>
    {
        void OnSerialize(PointLight& light, SerializationStream& stream)
        {
            stream.Serialize("Color", SerializationValue(light.Color, SerializationValueFlags::Color));
            stream.Serialize("Intensity", SerializationValue(light.Intensity));
        }
    };



    struct FLARE_API SpotLight
    {
        FLARE_COMPONENT;

        SpotLight()
            : Color(glm::vec3(1.0f)), Intensity(1.0f), InnerAngle(30.0f), OuterAngle(45.0f) {}
        SpotLight(const glm::vec3& color, float intensity, float innerAngle, float outerAngle)
            : Color(color), Intensity(intensity), InnerAngle(innerAngle), OuterAngle(outerAngle) {}

        glm::vec3 Color;
        float Intensity;

        float InnerAngle;
        float OuterAngle;
    };

    template<>
    struct TypeSerializer<SpotLight>
    {
        void OnSerialize(SpotLight& light, SerializationStream& stream)
        {
            stream.Serialize("Color", SerializationValue(light.Color, SerializationValueFlags::Color));
            stream.Serialize("Intensity", SerializationValue(light.Intensity));
            stream.Serialize("InnerAngle", SerializationValue(light.InnerAngle));
            stream.Serialize("OuterAngle", SerializationValue(light.OuterAngle));
        }
    };



    struct FLARE_API Environment
    {
        FLARE_COMPONENT;

        Environment();
        Environment(glm::vec3 color, float intensity);

        glm::vec3 EnvironmentColor;
        float EnvironmentColorIntensity;

        ShadowSettings ShadowSettings;
    };

    template<>
    struct TypeSerializer<Environment>
    {
        void OnSerialize(Environment& environment, SerializationStream& stream)
        {
            stream.Serialize("EnvironmentColor", SerializationValue(environment.EnvironmentColor, SerializationValueFlags::Color));
            stream.Serialize("EnvironmentColorIntensity", SerializationValue(environment.EnvironmentColorIntensity));

            stream.Serialize("ShadowSettings", SerializationValue(environment.ShadowSettings));
        }
    };
}