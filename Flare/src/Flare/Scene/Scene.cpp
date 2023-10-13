#include "Scene.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Scene/Components.h"
#include "Flare/Input/InputManager.h"

#include "Flare/Scripting/ScriptingEngine.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Flare
{

	static Ref<VertexArray> m_TestMesh;
	static Ref<Shader> m_MeshShader;

	static void ProcessMeshNode(aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<glm::vec3> vertices;
			std::vector<uint32_t> indices;

			vertices.resize(mesh->mNumVertices);

			for (size_t i = 0; i < vertices.size(); i++)
			{
				vertices[i].x = mesh->mVertices[i].x;
				vertices[i].y = mesh->mVertices[i].y;
				vertices[i].z = mesh->mVertices[i].z;
			}

			for (int32_t face = 0; face < mesh->mNumFaces; face++)
			{
				aiFace& f = mesh->mFaces[face];
				for (int32_t i = 0; i < f.mNumIndices; i++)
					indices.push_back(f.mIndices[i]);
			}

			Ref<VertexBuffer> buffer = VertexBuffer::Create(sizeof(glm::vec3) * vertices.size(), vertices.data());
			buffer->SetLayout({
				{ "i_Position", ShaderDataType::Float3 }
				});

			m_TestMesh->AddVertexBuffer(buffer);
			m_TestMesh->SetIndexBuffer(IndexBuffer::Create(indices.size(), indices.data()));
			return;
		}

		for (int i = 0; i < node->mNumChildren; i++)
			ProcessMeshNode(node->mChildren[i], scene);
	}

	Ref<Scene> s_Active = nullptr;

	Scene::Scene(ECSContext& context)
		: Asset(AssetType::Scene), m_World(context)
	{
		m_World.MakeCurrent();
		Initialize();


		std::filesystem::path modelPath = "assets/Cube.fbx";

		m_MeshShader = Shader::Create("assets/Shaders/Mesh.glsl");

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(modelPath.string(), aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			FLARE_CORE_ERROR("Failed to load model");
		else
		{
			m_TestMesh = VertexArray::Create();
			ProcessMeshNode(scene->mRootNode, scene);
			m_TestMesh->Unbind();
		}
	}

	Scene::~Scene()
	{
	}

	void Scene::Initialize()
	{
		ScriptingEngine::SetCurrentECSWorld(m_World);
		m_World.Components.RegisterComponents();

		SystemsManager& systemsManager = m_World.GetSystemsManager();
		systemsManager.CreateGroup("Debug Rendering");

		m_2DRenderingGroup = systemsManager.CreateGroup("2D Rendering");
		m_ScriptingUpdateGroup = systemsManager.CreateGroup("Scripting Update");
		m_OnRuntimeStartGroup = systemsManager.CreateGroup("On Runtime Start");
		m_OnRuntimeEndGroup = systemsManager.CreateGroup("On Runtime End");

		m_CameraDataUpdateQuery = m_World.CreateQuery<With<TransformComponent>, With<CameraComponent>>();

		systemsManager.RegisterSystem("Sprites Renderer", m_2DRenderingGroup, new SpritesRendererSystem());
	}

	void Scene::InitializeRuntime()
	{
		ScriptingEngine::RegisterSystems();
		m_World.GetSystemsManager().RebuildExecutionGraphs();
	}

	void Scene::OnRuntimeStart()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_OnRuntimeStartGroup);
	}

	void Scene::OnRuntimeEnd()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_OnRuntimeEndGroup);
	}

	void Scene::OnBeforeRender(Viewport& viewport)
	{
		if (!viewport.FrameData.IsEditorCamera)
		{
			float aspectRation = viewport.GetAspectRatio();

			for (EntityView entityView : m_CameraDataUpdateQuery)
			{
				ComponentView<CameraComponent> cameras = entityView.View<CameraComponent>();
				ComponentView<TransformComponent> transforms = entityView.View<TransformComponent>();

				for (EntityViewElement entity : entityView)
				{
					CameraComponent& camera = cameras[entity];

					float halfSize = camera.Size / 2;

					viewport.FrameData.Camera.View = glm::inverse(transforms[entity].GetTransformationMatrix());

					if (camera.Projection == CameraComponent::ProjectionType::Orthographic)
						viewport.FrameData.Camera.Projection = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, camera.Near, camera.Far);
					else
						viewport.FrameData.Camera.Projection = glm::perspective<float>(glm::radians(camera.FOV), aspectRation, camera.Near, camera.Far);

					viewport.FrameData.Camera.Position = transforms[entity].Position;
					viewport.FrameData.Camera.CalculateViewProjection();
				}
			}
		}
	}

	void Scene::OnRender(const Viewport& viewport)
	{
		RenderCommand::SetDepthTestEnabled(false);
		Renderer2D::Begin();

		m_World.GetSystemsManager().ExecuteGroup(m_2DRenderingGroup);

		Renderer::ExecuteRenderPasses();

		Renderer2D::End();
		RenderCommand::SetDepthTestEnabled(true);

		m_MeshShader->Bind();
		RenderCommand::DrawIndexed(m_TestMesh);

	}

	void Scene::OnUpdateRuntime()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_ScriptingUpdateGroup);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
	}

	World& Scene::GetECSWorld()
	{
		return m_World;
	}

	Ref<Scene> Scene::GetActive()
	{
		return s_Active;
	}

	void Scene::SetActive(const Ref<Scene>& scene)
	{
		s_Active = scene;
	}
}