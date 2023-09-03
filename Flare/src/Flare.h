#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Log.h"
#include "FlareCore/Assert.h"

// Rendering

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/VertexArray.h"
#include "Flare/Renderer/FrameBuffer.h"

#include "Flare/Renderer/Sprite.h"

#include <stdint.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
