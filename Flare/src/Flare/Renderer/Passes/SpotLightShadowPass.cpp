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

		for (uint32_t lightIndex = 0; lightIndex < frameInFlightCount * m_Specifications.MaxLightCount; lightIndex++)
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
		m_Tiles.clear();

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();
		m_HasSpotLight = sceneSubmition.SpotLightShadows.size() > 0;

		uint32_t frameCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);

		if (m_HasSpotLight)
		{
			size_t lightCount = sceneSubmition.SpotLightShadows.size();

			glm::uvec2 shadowMapSize = context.GetRenderGraphResourceManager().GetTexture(m_ShadowMap)->GetSize();
			FLARE_CORE_ASSERT(shadowMapSize == glm::uvec2(1 << m_Specifications.SizePowerOfTwo));

			SpotLightShadowsEntry* shadowEntries = new SpotLightShadowsEntry[lightCount];

			for (size_t lightIndex = 0; lightIndex < lightCount; lightIndex++)
			{
				const SpotLightShadowsSubmition& shadowsSubmition = sceneSubmition.SpotLightShadows[lightIndex];
				const SpotLightSubmition& spotLight = sceneSubmition.SpotLights[shadowsSubmition.LightIndex];

				if (shadowsSubmition.SizePowerOfTwo > m_Specifications.SizePowerOfTwo)
				{
					FLARE_CORE_ERROR("Spotlight at index {} is larger that the shadow atlas", lightIndex);
				}

				float radius = glm::sqrt(spotLight.Intensity / 0.01f);

				// TODO: Get rid of acos
				float fov = glm::acos(spotLight.OuterAngleCos) * 2.0f;
				glm::mat4 projection = glm::perspectiveRH_ZO(fov, 1.0f, shadowsSubmition.Near, shadowsSubmition.Far);

				glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
				glm::vec3 forward = spotLight.Direction;
				glm::vec3 right = -glm::cross(up, forward);

				glm::mat4 viewMatrix = glm::lookAt(
						spotLight.Position + spotLight.Direction,
						spotLight.Position,
						glm::vec3(0.0f, 1.0f, 0.0f));

				glm::vec3 snappedSpotLightPosition = spotLight.Position;

				// Snap to texel
				{
					glm::mat4 viewProjection = projection * viewMatrix;
					glm::mat4 inverseViewProjection = glm::inverse(viewProjection);

					glm::vec3 forwardOffset = spotLight.Direction * shadowsSubmition.Near;

					glm::vec3 samplePoint = spotLight.Position + forwardOffset;
					glm::vec4 projectedSamplePoint = viewProjection * glm::vec4(samplePoint, 1.0f);
					projectedSamplePoint /= projectedSamplePoint.w;
					projectedSamplePoint.x = glm::round(projectedSamplePoint.x);
					projectedSamplePoint.y = glm::round(projectedSamplePoint.y);

					glm::vec4 snappedSamplePoint = inverseViewProjection * projectedSamplePoint;
					snappedSamplePoint /= snappedSamplePoint.w;

					snappedSpotLightPosition = snappedSamplePoint;
				}

				viewMatrix = glm::lookAt(
						snappedSpotLightPosition + spotLight.Direction,
						snappedSpotLightPosition,
						glm::vec3(0.0f, 1.0f, 0.0f));

				RenderView view{};
				view.FOV = fov;
				view.Far = shadowsSubmition.Far;
				view.Near = shadowsSubmition.Near;
				view.Position = spotLight.Position;
				view.ViewDirection = spotLight.Direction;
				view.ViewportSize = glm::ivec2(1 << shadowsSubmition.SizePowerOfTwo);
				view.SetViewAndProjection(projection, viewMatrix);

				auto& entry = shadowEntries[lightIndex];
				entry.Projection = view.ViewProjection;
				entry.Radius = radius;
				entry.Near = shadowsSubmition.Near;
				entry.Far = shadowsSubmition.Far;
				entry.Bias = shadowsSubmition.Bias;

				size_t cameraResourceIndex = static_cast<uint32_t>(lightIndex) * frameCount + frameIndex;
				m_PerLightCameras[cameraResourceIndex].CameraBuffer->SetData(MemorySpan(&view, 1), 0);
			}

			// Fill tiles

			m_Tiles.resize(lightCount);
			for (size_t i = 0; i < lightCount; i++)
			{
				m_Tiles[i] = SpotLightTile
				{
					.Position = glm::ivec2(0, 0),
					.SizePowerOfTwo = sceneSubmition.SpotLightShadows[i].SizePowerOfTwo,
					.CulledBatches = SpotLightCulledGeometryRange {}
				};
			}

			// Sort shadow casting lights by their size
			uint32_t* lightIndices = new uint32_t[lightCount];

			for (size_t i = 0; i < lightCount; i++)
				lightIndices[i] = static_cast<uint32_t>(i);

			std::sort(lightIndices,
					lightIndices + sceneSubmition.SpotLightShadows.size(),
					[&sceneSubmition](uint32_t lightAIndex, uint32_t lightBIndex) -> bool
					{
						uint32_t aSize = sceneSubmition.SpotLightShadows[lightAIndex].SizePowerOfTwo;
						uint32_t bSize = sceneSubmition.SpotLightShadows[lightBIndex].SizePowerOfTwo;
						return aSize > bSize;
					});

			// Pack tiles into the atlas
			glm::uvec2 tileOffset = glm::ivec2(0, 0);
			uint32_t rowHeight = 0;
			for (size_t i = 0; i < lightCount; i++)
			{
				uint32_t lightIndex = lightIndices[i];

				// Calculate tile position
				uint32_t tileSize = 1 << sceneSubmition.SpotLightShadows[lightIndex].SizePowerOfTwo;
				if (tileOffset.x + tileSize > shadowMapSize.x)
				{
					tileOffset.x = 0;
					tileOffset.y += rowHeight;
					rowHeight = 0;
				}

				// TODO: Handle the case when it is not possible to pack all the lights into the atlas.

				glm::uvec2 tilePosition = tileOffset;
				tileOffset.x += tileSize;
				rowHeight = glm::max(rowHeight, tileSize);

				auto& entry = shadowEntries[lightIndex];
				entry.UVScale = glm::vec2(static_cast<float>(tileSize)) / static_cast<glm::vec2>(shadowMapSize);
				entry.UVTranslation = static_cast<glm::vec2>(tilePosition) / static_cast<glm::vec2>(shadowMapSize);

				m_Tiles[lightIndex].Position = tilePosition;
			}

			globalResources.FrameResources[frameIndex].SpotLightShadowDataBuffer->SetData(
					MemorySpan(shadowEntries, lightCount), 0);

			delete[] shadowEntries;
			delete[] lightIndices;
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
		glm::uvec2 shadowMapSize = context.GetRenderGraphResourceManager().GetTexture(m_ShadowMap)->GetSize();

		for (size_t lightIndex = 0; lightIndex < sceneSubmition.SpotLightShadows.size(); lightIndex++)
		{
			const PerLightCameraResources& cameraResources = m_PerLightCameras[lightIndex * frameCount + frameIndex];
			commandBuffer->SetGlobalDescriptorSet(cameraResources.CameraDescriptorSet, 0);
			commandBuffer->ApplyMaterial(m_PerspectiveDepthOnly);

			Math::Rect viewport{};
			viewport.Min = m_Tiles[lightIndex].Position;
			viewport.Max = viewport.Min + glm::vec2(static_cast<float>(1 << m_Tiles[lightIndex].SizePowerOfTwo));

			commandBuffer->SetViewportAndScissors(viewport);

			auto culledBatchesRange = m_Tiles[lightIndex].CulledBatches;
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
		m_Tiles.reserve(sceneSubmition.SpotLightShadows.size());

		size_t rangeStart = 0;
		for (size_t i = 0; i < sceneSubmition.SpotLightShadows.size(); i++)
		{
			size_t culledBatchCount = CullGeometryForLight(context, i);

			m_Tiles[i].CulledBatches = SpotLightCulledGeometryRange
			{
				.Start = static_cast<uint32_t>(rangeStart),
				.Count = static_cast<uint32_t>(culledBatchCount)
			};

			rangeStart += culledBatchCount;
		}
	}
}
