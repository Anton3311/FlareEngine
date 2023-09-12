#include "Viewport.h"

namespace Flare
{
	Viewport::Viewport() {}

	void Viewport::Resize(const ViewportRect& newRect)
	{
		m_IsDirty = true;
		m_Rect = newRect;
	}
}
