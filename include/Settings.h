#pragma once

#include "MCP.h"

namespace Settings {

    // ------------------------------
    // Main
    // ------------------------------
    void LoadAll();      // Loads INI and JSON
    void SaveAll();      // Saves INI and JSON
    void ResetAll();     // Resets to defaults and saves

    // ------------------------------
    // General (INI)
    // ------------------------------
    struct GeneralSettings {
        int  BlurType                  = 2;  // 0=None, 1 = Simple, 2 = Advanced
		bool ExtraChecks               = true;
		bool VerboseLogging            = false;
    };

    // ------------------------------
    // Simple  (INI)
    // ------------------------------
    struct SimpleSettings {
        bool  blurSmoothing            = true;

        bool  interiorBlurToggle       = false;
        float interiorBlurStrength     = 0.5f;
		float interiorBlurRange        = 100.0f;

        bool  exteriorBlurToggle       = true;
        float exteriorBlurStrength     = 0.5f;
		float exteriorBlurRange        = 100.0f;
    };

    // Global instances
    inline GeneralSettings general;
    inline SimpleSettings  simple;

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

