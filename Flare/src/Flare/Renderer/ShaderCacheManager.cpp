#include "ShaderCacheManager.h"

namespace Flare
{
    Scope<ShaderCacheManager> s_Instance;

    void ShaderCacheManager::Uninitialize()
    {
        s_Instance.reset();
    }

    void Flare::ShaderCacheManager::SetInstance(Scope<ShaderCacheManager>&& cacheManager)
    {
        s_Instance = std::move(cacheManager);
    }

    const Scope<ShaderCacheManager>& Flare::ShaderCacheManager::GetInstance()
    {
        return s_Instance;
    }
}
