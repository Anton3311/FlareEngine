#include "RendererSubmitionQueue.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

namespace Flare
{
	void RendererSubmitionQueue::Submit(Ref<const Mesh> mesh, Span<AssetHandle> materialHandles, const Math::Compact3DTransform& transform, MeshRenderFlags flags)
	{
		FLARE_PROFILE_FUNCTION();
		
		const auto& subMeshes = mesh->GetSubMeshes();

		if (!HAS_BIT(flags, MeshRenderFlags::DontCastShadows))
			SubmitForShadowPass(mesh, transform);

		for (size_t subMeshIndex = 0; subMeshIndex < subMeshes.size(); subMeshIndex++)
		{
			Submit(mesh, (uint32_t)subMeshIndex, AssetManager::GetAsset<Material>(materialHandles[subMeshIndex]), transform, flags, 0);
		}
	}

	void RendererSubmitionQueue::SubmitForShadowPass(const Ref<const Mesh>& mesh, const Math::Compact3DTransform& transform)
	{
		FLARE_PROFILE_FUNCTION();

		auto batchIterator = std::find_if(
			m_ShadowPassBatches.begin(),
			m_ShadowPassBatches.end(),
			[&mesh](const ShadowPassBatch& batch)
			{
				return mesh.get() == batch.Mesh.get();
			});

		ShadowPassBatch* batch = nullptr;
		if (batchIterator == m_ShadowPassBatches.end())
		{
			ShadowPassBatch& newBatch = m_ShadowPassBatches.emplace_back();
			newBatch.Mesh = mesh;

			batch = &newBatch;
		}
		else

		{
			batch = &(*batchIterator);
		}

		glm::vec3 center = transform.RotationScale * mesh->GetBounds().GetCenter() + transform.Translation;

		ShadowPassMeshSubmition& submition = batch->Submitions.emplace_back();
		submition.Transform = transform;
		submition.SortKey = glm::distance2(center, m_CameraPosition);
	}

	void RendererSubmitionQueue::Clear()
	{
		m_ShadowPassBatches.clear();
		m_Buffer.clear();
	}
}
