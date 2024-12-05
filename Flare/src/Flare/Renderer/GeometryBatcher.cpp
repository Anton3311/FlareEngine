#include "PCH.h"
#include "GeometryBatcher.h"

namespace Flare
{
	size_t GeometryBatchKeyHasher::operator()(const GeometryBatchKey& key) const
	{
		FLARE_PROFILE_FUNCTION();

		size_t hash = 0;
		CombineHashes(hash, key.m_Mesh.GetRawPointer());

		for (const auto& material : key.m_Materials)
		{
			CombineHashes(hash, material.GetRawPointer());
		}

		return hash;
	}

	//
	// GeometryBatcher
	//

	void GeometryBatcher::SubmitGeometry(const Ref<const Mesh>& mesh,
		Span<const Ref<Material>> materials,
		const glm::mat4& transform)
	{
		FLARE_PROFILE_FUNCTION();
		GeometryBatch& batch = FindOrCreateBatch(mesh, materials);
		batch.Append(transform);
	}

	GeometryBatch& GeometryBatcher::FindOrCreateBatch(const Ref<const Mesh>& mesh, Span<const Ref<Material>> materials)
	{
		GeometryBatchKey key(mesh, materials);

		auto it = m_Batches.find(key);
		if (it != m_Batches.end())
			return it->second;

		GeometryBatch batch(mesh, materials);
		return m_Batches.try_emplace(batch.GetKey(), std::move(batch)).first->second;
	}

	void GeometryBatcher::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		m_Batches.clear();
	}
}
