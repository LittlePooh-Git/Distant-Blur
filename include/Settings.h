#pragma once

#include "MCP.h"

namespace Settings {

    // ------------------------------
    // Main
    // ------------------------------
    void LoadAll();      // Loads INI and JSON
    void SaveAll();      // Saves INI and JSON
    void ResetAll();     // Resets to defaults and saves

    inline std::string weatherListPath = "Data/SKSE/Plugins/DBWeatherList.json";

    // ------------------------------
    // General (INI)
    // ------------------------------
    struct GeneralSettings {
        int  BlurType       = 1;  // 0=None, 1=Advanced
		bool ExtraChecks    = true;
		bool VerboseLogging = false;
    };

    // Global instances
    inline GeneralSettings general;

    // ------------------------------
    // JSON
    // ------------------------------
    namespace Json {
        bool Load();
        bool Save();
        void Clear();
    }

    // ------------------------------
    // INI handlers
    // ------------------------------
    namespace INI {
        bool Load();
        bool Save();
        void Reset();
    }
}
