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
	class RendererSubmitionQueue
	{
	public:
		struct Item
		{
			Ref<const Mesh> Mesh;
			Ref<const Material> Material;
			uint32_t SubMeshIndex;
			Math::Compact3DTransform Transform;
			MeshRenderFlags Flags;
			int32_t EntityIndex;

			float SortKey;
		};

		inline void Submit(const Ref<const Mesh>& mesh,
			uint32_t subMesh,
			const Ref<const Material>& material,
			const glm::mat4& transform,
			MeshRenderFlags flags,
			int32_t entityIndex)
		{
			FLARE_PROFILE_FUNCTION();
			if (m_ErrorMaterial == nullptr)
				return;

			Item& object = m_Buffer.emplace_back();

			if (material)
				object.Material = material->GetShader() == nullptr ? m_ErrorMaterial : material;
			else
				object.Material = m_ErrorMaterial;

			glm::vec3 center = mesh->GetSubMeshes()[subMesh].Bounds.GetCenter();
			center = transform * glm::vec4(center, 1.0f);

			object.Flags = flags;
			object.Mesh = mesh;
			object.SubMeshIndex = subMesh;
			object.Transform = transform;
			object.EntityIndex = entityIndex;
			object.SortKey = glm::distance2(center, m_CameraPosition);
		}

		inline size_t GetSize() const { return m_Buffer.size(); }
		inline Item& operator[](size_t index) { return m_Buffer[index]; }
		inline const Item& operator[](size_t index) const { return m_Buffer[index]; }
	private:
		glm::vec3 m_CameraPosition;
		Ref<const Material> m_ErrorMaterial;
		std::vector<Item> m_Buffer;

		friend class Renderer;
	};
}
