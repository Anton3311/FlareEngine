#pragma once

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Mesh.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Math/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <stdint.h>

namespace Flare
{
	class Renderer;
	class FLARE_API RendererSubmitionQueue
	{
	public:
		struct ShadowPassMeshSubmition
		{
			Math::Compact3DTransform Transform;
			float SortKey = 0.0f;
		};

		struct ShadowPassBatch
		{
			Ref<const Mesh> Mesh = nullptr;
			std::vector<ShadowPassMeshSubmition> Submitions;
		};

		struct Item
		{
			Ref<const Mesh> Mesh;
			Ref<const Material> Material;
			uint32_t SubMeshIndex;
			Math::Compact3DTransform Transform;
			MeshRenderFlags Flags;

			float SortKey;
		};

		void Submit(const Ref<const Mesh>& mesh,
			uint32_t subMesh,
			const Ref<const Material>& material,
			const glm::mat4& transform,
			MeshRenderFlags flags,
			int32_t entityIndex)
		{
			Submit(mesh, subMesh, material, Math::Compact3DTransform(transform), flags, entityIndex);
		}

		void Submit(Ref<const Mesh> mesh, Span<AssetHandle> materialHandles, const Math::Compact3DTransform& transform, MeshRenderFlags flags);

		void SubmitForShadowPass(const Ref<const Mesh>& mesh, const Math::Compact3DTransform& transform);

		void Submit(const Ref<const Mesh>& mesh,
			uint32_t subMesh,
			const Ref<const Material>& material,
			const Math::Compact3DTransform& transform,
			MeshRenderFlags flags,
			int32_t entityIndex)
		{
			Item& object = m_Buffer.emplace_back();
			object.Material = material;
			object.Flags = flags;
			object.Mesh = mesh;
			object.SubMeshIndex = subMesh;
			object.Transform = transform;

			glm::vec3 center = mesh->GetSubMeshes()[subMesh].Bounds.GetCenter();
			center = object.Transform.RotationScale * center + object.Transform.Translation;
			object.SortKey = glm::distance2(center, m_CameraPosition);
		}

		inline size_t GetSize() const { return m_Buffer.size(); }
		inline Item& operator[](size_t index) { return m_Buffer[index]; }
		inline const Item& operator[](size_t index) const { return m_Buffer[index]; }

		inline const std::vector<ShadowPassBatch>& GetShadowPassBatches() const { return m_ShadowPassBatches; }

		inline void SetCameraPosition(glm::vec3 cameraPosition) { m_CameraPosition = cameraPosition; }
		void Clear();
	private:
		glm::vec3 m_CameraPosition = glm::vec3(0.0f);
		std::vector<Item> m_Buffer;

		std::vector<ShadowPassBatch> m_ShadowPassBatches;
	};
}
