#pragma once

// External Library Headers
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <wrl/client.h>
#include <CLIBUtil/utils.hpp>
#include "SKSEMCP/SKSEMenuFramework.hpp"

// Standard Library Headers
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <typeindex>
#include <functional>


namespace Plugin {
	inline std::string modName            = std::string(SKSE::PluginDeclaration::GetSingleton()->GetName());
	inline std::string weatherListPath    = "Data/SKSE/Plugins/DBWeatherList.json";
	inline std::string settingsPathString = "Data/SKSE/Plugins/" + modName + ".ini";
	inline const char* settingsPath       = nullptr; // Initialized in Manager.cpp
}

namespace Logger = SKSE::log;

using namespace std::literals;
using namespace Plugin;