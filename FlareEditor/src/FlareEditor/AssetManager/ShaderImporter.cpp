#include "ShaderImporter.h"

#include "FlareEditor/ShaderCompiler/ShaderCompiler.h"

namespace Flare
{
	Ref<Asset> ShaderImporter::ImportShader(const AssetMetadata& metadata)
	{
		if (!ShaderCompiler::Compile(metadata.Handle))
			return false;

		Ref<Shader> shader = Shader::Create();
		shader->Handle = metadata.Handle;
		shader->Load();

		return shader;
	}
}
