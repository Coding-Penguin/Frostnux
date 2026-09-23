#include "fxpch.h"
#include "Tab.h"

namespace Frostnux {

	void Tab::invalidateHighlight()
	{
		highlighter.markDirty(0, buffer.lineCount());
		longestDirty = true;
	}

}
