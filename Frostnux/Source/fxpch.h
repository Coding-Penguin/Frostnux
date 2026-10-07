#pragma once

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <utility>
#include <sstream>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <codecvt>
#include <cwctype>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <format>
#include <locale>
#include <array>
#include <stack>
#include <queue>
#include <mutex>
#include <tuple>
#include <list>
#include <map>
#include <set>

#include "Frostnux/Core/SettingsManager.h"
#include "Frostnux/Core/Language.h"
#include "Frostnux/Core/Channel.h"
#include "Frostnux/Core/Theme.h"
#include "Frostnux/Core/Core.h"
#include "Frostnux/Core/Log.h"

#include "Frostnux/Core/KeyCodes.h"
#include "Frostnux/Core/MouseButtonCodes.h"

#include "Global.h"

#ifdef FX_PLATFORM_WINDOWS
#include <Windows.h>
#endif
