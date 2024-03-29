#include "OpenGLRendererAPI.h"

#include "FlareCore/Log.h"

#include "Flare/Renderer/Material.h"

#include "Flare/Platform/OpenGL/OpenGLShader.h"
#include "Flare/Platform/OpenGL/OpenGLMesh.h"

#include <glad/glad.h>

namespace Flare
{
	static void OpenGLDebugMessageCallback(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_LOW:
			FLARE_CORE_WARN(message);
			return;
		case GL_DEBUG_SEVERITY_MEDIUM:
			FLARE_CORE_ERROR(message);
			return;
		case GL_DEBUG_SEVERITY_HIGH:
			FLARE_CORE_CRITICAL(message);
			return;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			FLARE_CORE_TRACE(message);
			return;
		}
	}

	void OpenGLRendererAPI::Initialize()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDebugMessageCallback(OpenGLDebugMessageCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);

		glFrontFace(GL_CW);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetClearColor(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::SetDepthTestEnabled(bool enabled)
	{
		if (enabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	void OpenGLRendererAPI::SetCullingMode(CullingMode mode)
	{
		switch (mode)
		{
		case CullingMode::None:
			glDisable(GL_CULL_FACE);
			break;
		case CullingMode::Front:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case CullingMode::Back:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
		}
	}

	void OpenGLRendererAPI::SetDepthComparisonFunction(DepthComparisonFunction function)
	{
		switch (function)
		{
		case DepthComparisonFunction::Less:
			glDepthFunc(GL_LESS);
			break;
		case DepthComparisonFunction::Greater:
			glDepthFunc(GL_GREATER);
			break;
		case DepthComparisonFunction::LessOrEqual:
			glDepthFunc(GL_LEQUAL);
			break;
		case DepthComparisonFunction::GreaterOrEqual:
			glDepthFunc(GL_GEQUAL);
			break;
		case DepthComparisonFunction::Equal:
			glDepthFunc(GL_EQUAL);
			break;
		case DepthComparisonFunction::NotEqual:
			glDepthFunc(GL_NOTEQUAL);
			break;
		case DepthComparisonFunction::Always:
			glDepthFunc(GL_ALWAYS);
			break;
		case DepthComparisonFunction::Never:
			glDepthFunc(GL_LESS);
			break;
		}
	}

	void OpenGLRendererAPI::SetDepthWriteEnabled(bool enabled)
	{
		if (enabled)
			glDepthMask(GL_TRUE);
		else
			glDepthMask(GL_FALSE);
	}

	void OpenGLRendererAPI::SetBlendMode(BlendMode mode)
	{
		switch (mode)
		{
		case BlendMode::Opaque:
			glDisable(GL_BLEND);
			break;
		case BlendMode::Transparent:
			glEnable(GL_BLEND);
			break;
		}
	}

	void OpenGLRendererAPI::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

	inline static void GetIndexCountAndType(const Ref<IndexBuffer>& indexBuffer, int32_t* indicesCount, GLenum* indexType)
	{
		IndexBuffer::IndexFormat indexFormat = indexBuffer->GetIndexFormat();

		if (indicesCount != nullptr)
			*indicesCount = (int32_t)indexBuffer->GetCount();

		switch (indexFormat)
		{
		case IndexBuffer::IndexFormat::UInt16:
			*indexType = GL_UNSIGNED_SHORT;
			break;
		case IndexBuffer::IndexFormat::UInt32:
			*indexType = GL_UNSIGNED_INT;
			break;
		}
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<const VertexArray>& vertexArray)
	{
		vertexArray->Bind();

		int32_t indicesCount = 0;
		GLenum indexType = 0;
		GetIndexCountAndType(vertexArray->GetIndexBuffer(), &indicesCount, &indexType);

		glDrawElements(GL_TRIANGLES, indicesCount, indexType, (const void*)0);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<const VertexArray>& vertexArray, size_t indicesCount)
	{
		vertexArray->Bind();

		GLenum indexType = 0;
		GetIndexCountAndType(vertexArray->GetIndexBuffer(), nullptr, &indexType);

		glDrawElements(GL_TRIANGLES, (int32_t)indicesCount, indexType, (const void*)0);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<const VertexArray>& vertexArray, size_t firstIndex, size_t indicesCount)
	{
		vertexArray->Bind();

		switch (vertexArray->GetIndexBuffer()->GetIndexFormat())
		{
		case IndexBuffer::IndexFormat::UInt16:
			glDrawElements(GL_TRIANGLES, (int32_t)indicesCount, GL_UNSIGNED_SHORT, (const void*)(firstIndex * sizeof(uint16_t)));
			break;
		case IndexBuffer::IndexFormat::UInt32:
			glDrawElements(GL_TRIANGLES, (int32_t)indicesCount, GL_UNSIGNED_INT, (const void*)(firstIndex * sizeof(uint32_t)));
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}
	}

	void OpenGLRendererAPI::DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount)
	{
		mesh->Bind();

		int32_t indicesCount = 0;
		GLenum indexType = 0;
		GetIndexCountAndType(mesh->GetIndexBuffer(), &indicesCount, &indexType);

		glDrawElementsInstanced(GL_TRIANGLES, indicesCount, indexType, (const void*)0, (int32_t)instancesCount);
		mesh->Unbind();
	}

	inline static GLenum ConvertTopologyType(MeshTopology topology)
	{
		switch (topology)
		{
		case MeshTopology::Triangles:
			return GL_TRIANGLES;
		}

		FLARE_CORE_ASSERT(false);
		return 0;
	}

	void OpenGLRendererAPI::DrawInstancesIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t instancesCount, uint32_t baseInstance)
	{
		const SubMesh& subMesh = mesh->GetSubMeshes()[subMeshIndex];
		const VertexArray& vertexArray = As<const OpenGLMesh>(mesh)->GetVertexArray();
		vertexArray.Bind();
		
		size_t indexSize = vertexArray.GetIndexBuffer()->GetIndexFormat() == IndexBuffer::IndexFormat::UInt16 ? sizeof(uint16_t) : sizeof(uint32_t);
		int32_t indicesCount = 0;
		GLenum indexType = 0;
		GetIndexCountAndType(vertexArray.GetIndexBuffer(), &indicesCount, &indexType);

		glDrawElementsInstancedBaseVertexBaseInstance(
			ConvertTopologyType(mesh->GetTopologyType()),
			subMesh.IndicesCount,
			indexType,
			(const void*)(subMesh.BaseIndex * indexSize),
			instancesCount,
			subMesh.BaseVertex,
			baseInstance);
	}

	void OpenGLRendererAPI::DrawInstancesIndexedIndirect(const Ref<const Mesh>& mesh, const Span<DrawIndirectCommandSubMeshData>& subMeshesData, uint32_t baseInstance)
	{
		const VertexArray& vertexArray = As<const OpenGLMesh>(mesh)->GetVertexArray();
		vertexArray.Bind();
		const auto& subMeshes = mesh->GetSubMeshes();
		
		GLenum indexType = 0;
		GetIndexCountAndType(vertexArray.GetIndexBuffer(), nullptr, &indexType);

		m_IndirectCommandDataStorage.clear();

		for (const DrawIndirectCommandSubMeshData& data : subMeshesData)
		{
			auto& commandData = m_IndirectCommandDataStorage.emplace_back();
			const SubMesh& subMesh = subMeshes[data.SubMeshIndex];

			commandData.BaseInstance = baseInstance;
			commandData.BaseVertex = subMesh.BaseVertex;
			commandData.FirstIndex = subMesh.BaseIndex;
			commandData.IndicesCount = subMesh.IndicesCount;
			commandData.InstancesCount = data.InstancesCount;

			baseInstance += data.InstancesCount;
		}

		glMultiDrawElementsIndirect(
			ConvertTopologyType(mesh->GetTopologyType()),
			indexType,
			m_IndirectCommandDataStorage.data(),
			(int32_t)subMeshesData.GetSize(), 0);
	}

	void OpenGLRendererAPI::DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount, size_t baseVertexIndex, size_t startIndex, size_t indicesCount)
	{
		mesh->Bind();

		GLenum indexType = 0;
		GetIndexCountAndType(mesh->GetIndexBuffer(), nullptr, &indexType);

		glDrawElementsInstancedBaseVertex(GL_TRIANGLES, (int32_t)indicesCount, indexType, (const void*)0, (int32_t)instancesCount, (int32_t)baseVertexIndex);
		mesh->Unbind();
	}

	void OpenGLRendererAPI::DrawLines(const Ref<const VertexArray>& vertexArray, size_t verticesCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, (int32_t)verticesCount);
	}

	void OpenGLRendererAPI::ApplyMaterialProperties(const Ref<const Material>& materail)
	{
		FLARE_CORE_ASSERT(materail);
		if (!materail->GetShader())
			return;

		Ref<const OpenGLShader> shader = As<const OpenGLShader>(materail->GetShader());
		const ShaderProperties& properties = shader->GetProperties();

		if (!shader->IsLoaded())
			return;

		uint32_t shaderId = shader->GetId();
		glUseProgram(shaderId);

		if (properties.size() == 0)
			return;
			
		const uint8_t* propertiesBuffer = materail->GetPropertiesBuffer();
		for (uint32_t i = 0; i < (uint32_t)properties.size(); i++)
		{
			const uint8_t* value = propertiesBuffer + properties[i].Offset;
			int32_t location = shader->GetUniformLocations()[i];

			const int32_t* ints = (const int32_t*)value;
			const float* floats = (const float*)value;

			switch (properties[i].Type)
			{
			case ShaderDataType::Int:
				glUniform1i(location, ints[0]);
				break;

			case ShaderDataType::Int2:
				glUniform2i(location, ints[0], ints[1]);
				break;
			case ShaderDataType::Int3:
				glUniform3i(location, ints[0], ints[1], ints[2]);
				break;
			case ShaderDataType::Int4:
				glUniform4i(location, ints[0], ints[1], ints[2], ints[3]);
				break;

			case ShaderDataType::Float:
				glUniform1f(location, floats[0]);
				break;
			case ShaderDataType::Float2:
				glUniform2f(location, floats[0], floats[1]);
				break;
			case ShaderDataType::Float3:
				glUniform3f(location, floats[0], floats[1], floats[2]);
				break;
			case ShaderDataType::Float4:
				glUniform4f(location, floats[0], floats[1], floats[2], floats[3]);
				break;

			case ShaderDataType::Matrix4x4:
				glUniformMatrix4fv(location, 1, GL_FALSE, floats);
				break;

			case ShaderDataType::Sampler:
			{
				const auto& textureValue = materail->ReadPropertyValue<TexturePropertyValue>(i);

				switch (textureValue.ValueType)
				{
				case TexturePropertyValue::Type::FrameBufferAttachment:
					FLARE_CORE_ASSERT(textureValue.FrameBuffer);
					textureValue.FrameBuffer->BindAttachmentTexture(textureValue.FrameBufferAttachmentIndex, properties[i].Location);
					break;
				case TexturePropertyValue::Type::Texture:
					if (textureValue.Texture)
						textureValue.Texture->Bind(properties[i].Location);
					break;
				default:
					FLARE_CORE_ASSERT(false);
				}

				break;
			}
			case ShaderDataType::SamplerArray:
			{
				uint32_t arraySize = (uint32_t)properties[i].Size / ShaderDataTypeSize(ShaderDataType::Sampler);
				glUniform1iv(location, (int32_t)arraySize, (const int32_t*)value);
				break;
			}
			}
		}
	}
}
