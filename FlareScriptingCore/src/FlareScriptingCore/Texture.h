#pragma once

#include "FlareScriptingCore/Asset.h"

namespace Flare::Scripting
{
	class TextureAsset
	{
	public:
		TextureAsset()
			: m_Handle(0) {}
		TextureAsset(AssetHandle handle)
			: m_Handle(handle) {}
	public:
		AssetHandle GetHandle() const { return m_Handle; }
	private:
		AssetHandle m_Handle;
	};
}