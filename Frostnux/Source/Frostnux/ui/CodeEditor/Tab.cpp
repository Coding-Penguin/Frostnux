#include "fxpch.h"
#include "Tab.h"

namespace Frostnux {

	void Tab::invalidateHighlight()
	{
		highlighter.markAllDirty();
		longestDirty = true;
		symbols.rebuild(buffer);
		symbolsDirty = false;
		symbolsDirtyAt = 0.0;
	}

	void Tab::markSymbolsDirty(double now)
	{
		symbolsDirty = true;
		symbolsDirtyAt = now;
	}

}