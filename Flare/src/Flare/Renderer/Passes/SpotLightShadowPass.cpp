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
		m_AllocatedTileCount = 0;

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		uint32_t frameCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		const SpotLightShadowsEntry* shadowEntries = PrepareShadowEntries(context);

		if (shadowEntries != nullptr)
		{
			CullGeometry(context, shadowEntries);

			size_t transformsSize = sizeof(PackedTransform) * m_CulledGeometryTransforms.size();
			if (transformsSize > m_TransformBuffers[frameIndex].Buffer->GetSize())
			{
				m_TransformBuffers[frameIndex].Buffer->Resize(transformsSize);
				m_TransformBuffers[frameIndex].Set->WriteStorageBuffer(m_TransformBuffers[frameIndex].Buffer, 0);
				m_TransformBuffers[frameIndex].Set->FlushWrites();
			}

			m_TransformBuffers[frameIndex].Buffer->SetData(MemorySpan::FromVector(m_CulledGeometryTransforms), 0, commandBuffer);

			delete[] shadowEntries;
		}
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

		for (size_t lightIndex = 0; lightIndex < m_AllocatedTileCount; lightIndex++)
		{
			const PerLightCameraResources& cameraResources = m_PerLightCameras[lightIndex * frameCount + frameIndex];
			commandBuffer->SetGlobalDescriptorSet(cameraResources.CameraDescriptorSet, 0);
			commandBuffer->ApplyMaterial(m_PerspectiveDepthOnly);

			Math::Rect viewport{};
			viewport.Min = m_Tiles[lightIndex].Position;
			viewport.Max = viewport.Min + glm::vec2(static_cast<float>(1 << m_Tiles[lightIndex].SizeLog2));

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

	const SpotLightShadowsEntry* SpotLightShadowPass::PrepareShadowEntries(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();
		if (sceneSubmition.SpotLightShadows.size() == 0)
			return nullptr;

		uint32_t frameCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		size_t lightCount = sceneSubmition.SpotLightShadows.size();

		glm::uvec2 shadowMapSize = context.GetRenderGraphResourceManager().GetTexture(m_ShadowMap)->GetSize();
		FLARE_CORE_ASSERT(shadowMapSize == glm::uvec2(1 << m_Specifications.SizeLog2));

		SpotLightShadowsEntry* shadowEntries = new SpotLightShadowsEntry[lightCount];

		for (size_t lightIndex = 0; lightIndex < lightCount; lightIndex++)
		{
			const SpotLightShadowsSubmition& shadowsSubmition = sceneSubmition.SpotLightShadows[lightIndex];
			const SpotLightSubmition& spotLight = sceneSubmition.SpotLights[shadowsSubmition.LightIndex];

			if (shadowsSubmition.SizeLog2 > m_Specifications.SizeLog2)
			{
				FLARE_CORE_ERROR("Spotlight at index {} is larger that the shadow atlas", lightIndex);
				break;
			}

			float radius = glm::sqrt(spotLight.Intensity / 0.01f);

			// TODO: Get rid of acos
			float fov = glm::acos(spotLight.OuterAngleCos) * 2.0f;
			glm::mat4 projection = glm::perspectiveRH_ZO(fov, 1.0f, shadowsSubmition.Near, shadowsSubmition.Far);

			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::vec3 forward = spotLight.Direction;
			glm::vec3 right = -glm::cross(up, forward);

			glm::mat4 viewMatrix;

			// Snap to texel
			{ 
				viewMatrix = glm::lookAt(
					spotLight.Position,
					spotLight.Position + spotLight.Direction,
					glm::vec3(0.0f, 1.0f, 0.0f));

				glm::vec3 snappedSpotLightPosition = spotLight.Position;

				glm::mat4 viewProjection = projection * viewMatrix;
				glm::mat4 inverseViewProjection = glm::inverse(viewProjection);

				glm::vec3 forwardOffset = spotLight.Direction * shadowsSubmition.Near;

				glm::vec3 samplePoint = spotLight.Position + forwardOffset;
				glm::vec4 projectedSamplePoint = viewProjection * glm::vec4(samplePoint, 1.0f);
				float shadowMapRegionSize = static_cast<float>(1 << shadowsSubmition.SizeLog2);
				projectedSamplePoint /= projectedSamplePoint.w;
				projectedSamplePoint.x = glm::round(projectedSamplePoint.x * shadowMapRegionSize) / shadowMapRegionSize;
				projectedSamplePoint.y = glm::round(projectedSamplePoint.y * shadowMapRegionSize) / shadowMapRegionSize;

				glm::vec4 snappedSamplePoint = inverseViewProjection * projectedSamplePoint;
				snappedSamplePoint /= snappedSamplePoint.w;

				snappedSpotLightPosition = snappedSamplePoint;

				viewMatrix = glm::lookAt(
						snappedSpotLightPosition,
						snappedSpotLightPosition + spotLight.Direction,
						glm::vec3(0.0f, 1.0f, 0.0f));
			}

			RenderView view{};
			view.FOV = fov;
			view.Far = shadowsSubmition.Far;
			view.Near = shadowsSubmition.Near;
			view.Position = spotLight.Position;
			view.ViewDirection = spotLight.Direction;
			view.ViewportSize = glm::ivec2(1 << shadowsSubmition.SizeLog2);
			view.SetViewAndProjection(projection, viewMatrix);

			auto& entry = shadowEntries[lightIndex];
			entry.ViewProjection = view.ViewProjection;
			entry.Bias = shadowsSubmition.Bias;
			entry.FilterRadius = shadowsSubmition.FilterRadius;
			entry.NormalBias = shadowsSubmition.NormalBias;

			size_t cameraResourceIndex = static_cast<uint32_t>(lightIndex) * frameCount + frameIndex;
			m_PerLightCameras[cameraResourceIndex].CameraBuffer->SetData(MemorySpan(&view, 1), 0);
		}

		// Fill tiles

		m_Tiles.resize(lightCount);
		for (size_t i = 0; i < lightCount; i++)
		{
			m_Tiles[i] = SpotLightTile
			{
				// Set the position to a coordinate outside the shadow map to indicate an invalid state.
				.Position = glm::ivec2(shadowMapSize),
				.SizeLog2 = sceneSubmition.SpotLightShadows[i].SizeLog2,
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
					uint32_t aSize = sceneSubmition.SpotLightShadows[lightAIndex].SizeLog2;
					uint32_t bSize = sceneSubmition.SpotLightShadows[lightBIndex].SizeLog2;
					return aSize > bSize;
				});

		// Pack tiles into the atlas
		glm::uvec2 tileOffset = glm::ivec2(0, 0);
		uint32_t rowHeight = 0;

		for (size_t i = 0; i < lightCount; i++)
		{
			uint32_t lightIndex = lightIndices[i];

			// Calculate tile position
			uint32_t tileSizeLog2 = sceneSubmition.SpotLightShadows[lightIndex].SizeLog2;
			uint32_t tileSize = 1 << tileSizeLog2;
			if (tileOffset.x + tileSize > shadowMapSize.x || tileOffset.y + tileSize > shadowMapSize.y)
			{
				// TODO: Should update the number of shadow casting spotlights to omit the ones that were failed to be allocated
				FLARE_CORE_ERROR("Can't fit all the spotlights ({}) into a single shadow atlas ({}x{})", lightCount, shadowMapSize.x, shadowMapSize.y);
				continue;
			}

			glm::uvec2 tilePosition = tileOffset;
			tileOffset.x += tileSize;
			rowHeight = glm::max(rowHeight, tileSize);

			if (tileOffset.x + tileSize > shadowMapSize.x)
			{
				tileOffset.x = 0;
				tileOffset.y += rowHeight;
				rowHeight = 0;
			}

			// UV Transform format:
			// 4 bits - log2 size
			// 14 bits - x position
			// 14 bits - y position
			constexpr uint32_t TILE_SIZE_MASK = 0b1111;
			constexpr uint32_t TILE_POSITION_MASK = 0x3fff;
			constexpr uint32_t TILE_POSITION_OFFSET = 14;

			FLARE_CORE_ASSERT((tileSizeLog2 >> TILE_SIZE_MASK) == 0);
			FLARE_CORE_ASSERT((tilePosition.x >> TILE_POSITION_OFFSET) == 0);
			FLARE_CORE_ASSERT((tilePosition.y >> TILE_POSITION_OFFSET) == 0);

			auto& entry = shadowEntries[lightIndex];
			entry.UVTransform = (tileSizeLog2) << 28 | (tilePosition.x << 14) | tilePosition.y;

			m_Tiles[lightIndex].Position = tilePosition;

			m_AllocatedTileCount++;
		}

		globalResources.FrameResources[frameIndex].SpotLightShadowDataBuffer->SetData(
				MemorySpan(shadowEntries, lightCount), 0);

		delete[] lightIndices;

		return shadowEntries;
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

	struct Sphere
	{
		glm::vec3 Center;
		float Radius;
	};

	static Sphere CreateConeBoundingSphere(glm::vec3 conePosition, glm::vec3 coneDirection, float radius, float halfAngle)
	{
		Sphere sphere{};
		if (halfAngle >= glm::pi<float>() / 4.0f)
		{
			sphere.Center = conePosition + coneDirection * glm::cos(halfAngle) * radius;
			sphere.Radius = glm::sin(halfAngle) * radius;
		}
		else
		{
			sphere.Radius = radius / (2.0f * glm::cos(halfAngle));
			sphere.Center = conePosition + coneDirection * sphere.Radius;
		}

		return sphere;
	}

	size_t SpotLightShadowPass::CullGeometryForLight(const RenderGraphContext& context, size_t lightIndex, const SpotLightShadowsEntry& shadows)
	{
		FLARE_PROFILE_FUNCTION();

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();
		const SpotLightShadowsSubmition& spotLightShadows = sceneSubmition.SpotLightShadows[lightIndex];
		const SpotLightSubmition& spotLight = sceneSubmition.SpotLights[spotLightShadows.LightIndex];

		FrustumPlanes frustumPlanes(glm::inverse(shadows.ViewProjection), spotLight.Direction);

		size_t culledBatchCount = 0;
		for (const auto& [key, batch] : sceneSubmition.BatchedGeometry.GetBatches())
		{
			Math::AABB meshAABB = batch.GetMesh()->GetBounds();
			SpotLightCulledGeometryBatch* culledBatch = nullptr;

			const auto& transforms = batch.GetTransforms();
			for (size_t i = 0; i < transforms.size(); i++)
			{
				Math::AABB transformedAABB = meshAABB.Transformed(transforms[i].AsMatrix4x4());

				bool intersects = true;
				for (size_t planeIndex = 0; planeIndex < FrustumPlanes::PlanesCount; planeIndex++)
				{
					if (!transformedAABB.IntersectsOrInFrontOfPlane(frustumPlanes.Planes[planeIndex]))
					{
						intersects = false;
						break;
					}
				}

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

	void SpotLightShadowPass::CullGeometry(const RenderGraphContext& context, const SpotLightShadowsEntry* shadowEntries)
	{
		FLARE_PROFILE_FUNCTION();

		const SceneSubmition& sceneSubmition = context.GetSceneSubmition();
		m_Tiles.reserve(sceneSubmition.SpotLightShadows.size());

		size_t rangeStart = 0;
		for (size_t i = 0; i < sceneSubmition.SpotLightShadows.size(); i++)
		{
			size_t culledBatchCount = CullGeometryForLight(context, i, shadowEntries[i]);

			m_Tiles[i].CulledBatches = SpotLightCulledGeometryRange
			{
				.Start = static_cast<uint32_t>(rangeStart),
				.Count = static_cast<uint32_t>(culledBatchCount)
			};

			rangeStart += culledBatchCount;
		}
	}
}
