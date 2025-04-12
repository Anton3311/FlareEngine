#include "PCH.h"
#include "SpotLightShadowPass.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Passes/GeometryCullingPass.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/SceneSubmition.h"

#include "FlareECS/World.h"

namespace Flare
{
	SpotLightShadowPass::SpotLightShadowPass(RenderGraphTextureId shadowMap,
		Ref<Material> perspectiveDepthOnly,
		const SpotLightShadowsSpecifications& specifications)
		: m_ShadowMap(shadowMap), m_PerspectiveDepthOnly(perspectiveDepthOnly), m_Specifications(specifications)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		GPUBufferSpecifications bufferSpecifications{};
		bufferSpecifications.MemoryType = GPUBufferMemoryType::Dynamic;
		bufferSpecifications.Size = sizeof(RenderView);
		bufferSpecifications.Usage = GPUBufferUsage::UniformBuffer;

		uint32_t maxLightCount = m_Specifications.TileCount.x * m_Specifications.TileCount.y;
		for (uint32_t lightIndex = 0; lightIndex < frameInFlightCount * maxLightCount; lightIndex++)
		{
			PerLightCameraResources& resources = m_PerLightCameras.emplace_back();
			resources.CameraBuffer = GPUBuffer::Create(bufferSpecifications);
			resources.CameraBuffer->SetDebugName(fmt::format("SpotLightShadowPass.CameraBuffer.{}", lightIndex));
			resources.CameraDescriptorSet = Renderer::GetCameraDescriptorSetPool()->AllocateSet();
			resources.CameraDescriptorSet->WriteUniformBuffer(resources.CameraBuffer, 0);
			resources.CameraDescriptorSet->FlushWrites();
		}

		GPUBufferSpecifications transformBufferSpecifications{};
		transformBufferSpecifications.MemoryType = GPUBufferMemoryType::Static;
		transformBufferSpecifications.Size = sizeof(PackedTransform);
		transformBufferSpecifications.Usage = GPUBufferUsage::StorageBuffer;
		for (uint32_t frameIndex = 0; frameIndex < frameInFlightCount; frameIndex++)
		{
			TransformBufferResources& resources = m_TransformBuffers.emplace_back();
			resources.Buffer = GPUBuffer::Create(transformBufferSpecifications);
			resources.Buffer->SetDebugName(fmt::format("SpotLightShadowPass.TransformBuffer.#{}", frameIndex));
			resources.Set = Renderer::GetInstanceDataDescriptorSetPool()->AllocateSet();
			resources.Set->WriteStorageBuffer(resources.Buffer, 0);
			resources.Set->FlushWrites();

		}
	}

	SpotLightShadowPass::~SpotLightShadowPass()
	{
		for (auto& resources : m_PerLightCameras)
		{
			Renderer::GetCameraDescriptorSetPool()->ReleaseSet(resources.CameraDescriptorSet);
		}

		for (auto& resources : m_TransformBuffers)
		{
			Renderer::GetInstanceDataDescriptorSetPool()->ReleaseSet(resources.Set);
		}
	}

	void SpotLightShadowPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		// Clear
		
		m_CulledGeometryTransforms.clear();
		m_CulledBatches.clear();
		m_BatchesPerLight.clear();

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();
		m_HasSpotLight = sceneSubmition.SpotLightShadows.size() > 0;

		uint32_t frameCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);

		if (m_HasSpotLight)
		{
			size_t lightCount = sceneSubmition.SpotLightShadows.size();

			std::vector<SpotLightShadowsEntry> shadowEntries;

			glm::uvec2 shadowMapSize = context.GetRenderGraphResourceManager().GetTexture(m_ShadowMap)->GetSize();
			glm::vec2 tileSize = glm::vec2(1.0f) / static_cast<glm::vec2>(m_Specifications.TileCount);
			for (size_t lightIndex = 0; lightIndex < lightCount; lightIndex++)
			{
				const SpotLightShadowsSubmition& shadowsSubmition = sceneSubmition.SpotLightShadows[lightIndex];
				const SpotLightSubmition& spotLight = sceneSubmition.SpotLights[shadowsSubmition.LightIndex];

				glm::uvec2 tileCoordinate = glm::uvec2(
						lightIndex % m_Specifications.TileCount.x,
						lightIndex / m_Specifications.TileCount.y);

				float radius = glm::sqrt(spotLight.Intensity / 0.01f);

				// TODO: Get rid of acos
				float fov = glm::acos(spotLight.OuterAngleCos) * 2.0f;
				glm::mat4 projection = glm::perspectiveRH_ZO(fov, 1.0f, shadowsSubmition.Near, shadowsSubmition.Far);

				glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
				glm::vec3 forward = spotLight.Direction;
				glm::vec3 right = -glm::cross(up, forward);

				glm::mat4 basis = static_cast<glm::mat4>(glm::mat3(right, up, forward));
				glm::mat4 viewMatrix = glm::inverse(glm::translate(basis, spotLight.Position));

				viewMatrix = glm::lookAt(
						spotLight.Position + spotLight.Direction,
						spotLight.Position,
						glm::vec3(0.0f, 1.0f, 0.0f));

				RenderView view{};
				view.FOV = fov;
				view.Far = shadowsSubmition.Far;
				view.Near = shadowsSubmition.Near;
				view.Position = spotLight.Position;
				view.ViewDirection = spotLight.Direction;
				view.ViewportSize = static_cast<glm::ivec2>(m_Specifications.TileSize);
				view.SetViewAndProjection(projection, viewMatrix);

				size_t cameraResourceIndex = static_cast<uint32_t>(lightIndex) * frameCount + frameIndex;
				m_PerLightCameras[cameraResourceIndex].CameraBuffer->SetData(MemorySpan(&view, 1), 0);

				auto& entry = shadowEntries.emplace_back();
				entry.Projection = view.ViewProjection;
				entry.UVScale = tileSize;
				entry.UVTranslation = tileSize * static_cast<glm::vec2>(tileCoordinate);
				entry.Radius = radius;
				entry.Near = shadowsSubmition.Near;
				entry.Far = shadowsSubmition.Far;
				entry.Bias = shadowsSubmition.Bias;
			}

			globalResources.FrameResources[frameIndex].SpotLightShadowDataBuffer->SetData(MemorySpan::FromVector(shadowEntries), 0);
		}

		CullGeometry(context);

		size_t transformsSize = sizeof(PackedTransform) * m_CulledGeometryTransforms.size();
		if (transformsSize > m_TransformBuffers[frameIndex].Buffer->GetSize())
		{
			m_TransformBuffers[frameIndex].Buffer->Resize(transformsSize);
			m_TransformBuffers[frameIndex].Set->WriteStorageBuffer(m_TransformBuffers[frameIndex].Buffer, 0);
			m_TransformBuffers[frameIndex].Set->FlushWrites();
		}

		m_TransformBuffers[frameIndex].Buffer->SetData(MemorySpan::FromVector(m_CulledGeometryTransforms), 0, commandBuffer);
	}

	void SpotLightShadowPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		uint32_t frameCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();

		commandBuffer->SetGlobalDescriptorSet(m_TransformBuffers[frameIndex].Set, 2);

		for (size_t lightIndex = 0; lightIndex < sceneSubmition.SpotLightShadows.size(); lightIndex++)
		{
			const PerLightCameraResources& cameraResources = m_PerLightCameras[lightIndex * frameCount + frameIndex];
			commandBuffer->SetGlobalDescriptorSet(cameraResources.CameraDescriptorSet, 0);
			commandBuffer->ApplyMaterial(m_PerspectiveDepthOnly);

			glm::uvec2 tileCoordinate = glm::uvec2(
					lightIndex % m_Specifications.TileCount.x,
					lightIndex / m_Specifications.TileCount.y);
			glm::vec2 tileSize = glm::vec2(1.0f) / static_cast<glm::vec2>(m_Specifications.TileCount);

			Math::Rect viewport{};
			viewport.Min = tileSize * static_cast<glm::vec2>(tileCoordinate);
			viewport.Max = viewport.Min + tileSize;

			viewport.Min *= context.RenderAreaSize;
			viewport.Max *= context.RenderAreaSize;

			commandBuffer->SetViewportAndScissors(viewport);

			auto culledBatchesRange = m_BatchesPerLight[lightIndex];
			for (uint32_t i = 0; i < culledBatchesRange.Count; i++)
			{
				const auto& culledBatch = m_CulledBatches[i + culledBatchesRange.Start];

				commandBuffer->DrawDepthOnlyMeshIndexed(culledBatch.GeometryMesh,
					culledBatch.TransformBufferOffset,
					culledBatch.Count);
			}
		}

		commandBuffer->SetGlobalDescriptorSet(globalResources.FrameResources[frameIndex].CameraDescriptorSet, 0);
	}

	static bool AABBIntersectsAABB(const Math::AABB& a, const Math::AABB& b)
	{
		return glm::all(
				glm::greaterThan(
					glm::min(a.Max, b.Max),
					glm::max(a.Min, b.Min)
				)
			);
	}

	size_t SpotLightShadowPass::CullGeometryForLight(const RenderGraphContext& context, size_t lightIndex)
	{
		FLARE_PROFILE_FUNCTION();

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();
		glm::vec3 lightPosition = sceneSubmition.SpotLights[lightIndex].Position;
		float lightRadius = glm::sqrt(sceneSubmition.SpotLights[lightIndex].Intensity / 0.01f);
		Math::AABB lightAABB = Math::AABB(
				lightPosition - glm::vec3(lightRadius),
				lightPosition + glm::vec3(lightRadius));

		size_t culledBatchCount = 0;
		for (const auto& [key, batch] : sceneSubmition.BatchedGeometry.GetBatches())
		{
			Math::AABB meshAABB = batch.GetMesh()->GetBounds();
			SpotLightCulledGeometryBatch* culledBatch = nullptr;

			const auto& transforms = batch.GetTransforms();
			for (size_t i = 0; i < transforms.size(); i++)
			{
				Math::AABB transformedAABB = meshAABB.Transformed(transforms[i].AsMatrix4x4());
				bool intersects = AABBIntersectsAABB(transformedAABB, lightAABB);

				if (!intersects)
					continue;

				if (culledBatch == nullptr)
				{
					culledBatch = &m_CulledBatches.emplace_back();
					culledBatch->TransformBufferOffset = static_cast<uint32_t>(m_CulledGeometryTransforms.size());
					culledBatch->GeometryMesh = batch.GetMesh();

					culledBatchCount += 1;
				}

				m_CulledGeometryTransforms.push_back(transforms[i]);
				culledBatch->Count += 1;
			}
		}

		return culledBatchCount;
	}

	void SpotLightShadowPass::CullGeometry(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();
		m_BatchesPerLight.reserve(sceneSubmition.SpotLightShadows.size());

		size_t rangeStart = 0;
		for (size_t i = 0; i < sceneSubmition.SpotLightShadows.size(); i++)
		{
			size_t culledBatchCount = CullGeometryForLight(context, i);

			m_BatchesPerLight.push_back(SpotLightCulledGeometryRange
			{
				.Start = static_cast<uint32_t>(rangeStart),
				.Count = static_cast<uint32_t>(culledBatchCount)
			});
			rangeStart += culledBatchCount;
		}
	}
}
