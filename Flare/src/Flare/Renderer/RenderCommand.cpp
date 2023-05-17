#include "RenderCommand.h"

namespace Flare
{
	Scope<RendererAPI> RenderCommand::s_API = RendererAPI::Create();
}