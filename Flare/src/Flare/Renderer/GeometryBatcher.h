#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

namespace Flare
{
	class Material;
	class Mesh;

	struct GeometryBatchKeyHasher;
	class FLARE_API GeometryBatchKey
	{
	public:
		GeometryBatchKey(const Ref<const Mesh>& mesh, Span<const Ref<Material>> materials)
			: m_Mesh(mesh), m_Materials(materials) {}

		inline bool operator==(const GeometryBatchKey& other) const
		{
			return m_Mesh == other.m_Mesh && m_Materials == other.m_Materials;
		}

		inline bool operator!=(const GeometryBatchKey& other) const
		{
			return m_Mesh != other.m_Mesh && m_Materials != other.m_Materials;
		}
	private:
		Ref<const Mesh> m_Mesh;
		Span<const Ref<Material>> m_Materials;

		friend struct GeometryBatchKeyHasher;
	};

	struct FLARE_API GeometryBatchKeyHasher
	{
		size_t operator()(const GeometryBatchKey& key) const;
	};

	struct PackedTransform
	{
		glm::vec4 Vectors[3];
	};

	class FLARE_API GeometryBatch
	{
	public:
		FLARE_NONCOPYABLE(GeometryBatch);

		GeometryBatch(const Ref<const Mesh>& mesh, Span<const Ref<Material>> materials)
			: m_Mesh(mesh), m_Materials(materials.begin(), materials.end()) {}

		GeometryBatch(GeometryBatch&&) = default;
		GeometryBatch& operator=(GeometryBatch&&) = default;

		inline void Append(const glm::mat4& transform)
		{
			glm::mat3 rotationScale = transform;
			glm::vec3 translation = transform[3];

			PackedTransform& packedTransform = m_Transforms.emplace_back();
			packedTransform.Vectors[0] = glm::vec4(static_cast<glm::vec3>(transform[0]), translation.x);
			packedTransform.Vectors[1] = glm::vec4(static_cast<glm::vec3>(transform[1]), translation.y);
			packedTransform.Vectors[2] = glm::vec4(static_cast<glm::vec3>(transform[2]), translation.z);
		}

		inline GeometryBatchKey GetKey() const { return GeometryBatchKey(m_Mesh, Span<const Ref<Material>>::FromVector(m_Materials)); }

		inline const Ref<const Mesh>& GetMesh() const { return m_Mesh; }
		inline const std::vector<Ref<Material>>& GetMaterials() const { return m_Materials; }
	private:
		Ref<const Mesh> m_Mesh;
		std::vector<Ref<Material>> m_Materials;
		std::vector<PackedTransform> m_Transforms;
	};

	class FLARE_API GeometryBatcher
	{
	public:
		FLARE_NONCOPYABLE(GeometryBatcher);

		GeometryBatcher() = default;

		using BatchMap = std::unordered_map<GeometryBatchKey, GeometryBatch, GeometryBatchKeyHasher>;

		void SubmitGeometry(const Ref<const Mesh>& mesh,
			Span<const Ref<Material>> materials,
			const glm::mat4& transform);

		GeometryBatch& FindOrCreateBatch(const Ref<const Mesh>& mesh, Span<const Ref<Material>> materials);

		inline const BatchMap& GetBatches() const { return m_Batches; }

		void Clear();
	private:
		BatchMap m_Batches;
	};
}
