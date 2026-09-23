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

#include "Frostnux/SettingsManager.h"
#include "Frostnux/Language.h"
#include "Frostnux/Channel.h"
#include "Frostnux/Theme.h"
#include "Frostnux/Core.h"
#include "Frostnux/Log.h"

#include "Frostnux/KeyCodes.h"
#include "Frostnux/MouseButtonCodes.h"

#include "Global.h"

#ifdef FX_PLATFORM_WINDOWS
#include <Windows.h>
#endif
