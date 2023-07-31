#pragma once

#include "FlareScriptingCore/Bindings/Asset.h"

namespace Flare::Internal
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