#include "ShaderImporter.h"

namespace Flare
{
	Ref<Asset> ShaderImporter::ImportShader(const AssetMetadata& metadata)
	{
		Ref<Shader> shader = Shader::Create();
		shader->Handle = metadata.Handle;
		shader->Load();

		return shader;
	}
}
