#include "fxpch.h"
#include "Tab.h"

namespace Frostnux {

    void Tab::invalidateHighlight()
    {
        highlighter.markAllDirty();
        longestDirty = true;
    }

}
