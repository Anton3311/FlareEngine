#include "PCH.h"

#include "ShaderImporter.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/ComputeShader.h"

#include "FlareEditor/ShaderCompiler/ShaderCompiler.h"

namespace Flare
{
	Ref<Asset> ShaderImporter::ImportShader(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();
		if (!ShaderCompiler::Compile(metadata.Handle))
			return nullptr;

		Ref<Shader> shader = Shader::Create();
		shader->Handle = metadata.Handle;
		shader->Load();

		return shader;
	}

	Ref<Asset> ShaderImporter::ImportComputeShader(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();
		if (!ShaderCompiler::Compile(metadata.Handle))
			return nullptr;

		Ref<ComputeShader> shader = ComputeShader::Create();
		shader->Handle = metadata.Handle;
		shader->Load();

		return shader;
	}
}
