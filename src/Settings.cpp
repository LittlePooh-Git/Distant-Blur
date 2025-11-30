#include "PCH.h"
#include "Settings.h"
#include "Utils.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>


namespace Settings {

    // ============================================================
    // Core Control
    // ============================================================

    void LoadAll() {
        Logger::info("Settings: Loading INI and JSON...");
        INI::Load();
        Json::Load();
		Logger::info("Settings: All settings loaded.");
    }

    void SaveAll() {
        Logger::info("Settings: Saving INI and JSON...");
        INI::Save();
        Json::Save();
		Logger::info("Settings: All settings saved.");
    }

    void ResetAll() {
        Logger::info("Settings: Resetting all settings to defaults...");
        INI::Reset();
        Json::Clear();
        Json::Save();
		Logger::info("Settings: All settings reset to defaults.");
    }

    // ============================================================
    // INI Handling
    // ============================================================

    namespace INI {

        bool Load() {
            CSimpleIniW ini;
            ini.SetUnicode();

            if (!std::filesystem::exists(settingsPath)) {
                Logger::warn("Settings: No INI found, creating new defaults at {}", settingsPath);
                Reset();
                Save();
                return false;
            }

            ini.LoadFile(settingsPath);

            general.BlurType                = static_cast<int>(ini.GetLongValue(L"General", L"BlurType", general.BlurType)); // Change to BlurMode
			general.ExtraChecks             = ini.GetBoolValue(L"General", L"ExtraChecks", general.ExtraChecks);
			general.VerboseLogging          = ini.GetBoolValue(L"General", L"VerboseLogging", general.VerboseLogging);

            simple.blurSmoothing            = ini.GetBoolValue(L"Simple", L"BlurSmoothing", simple.blurSmoothing);
            
            simple.interiorBlurToggle       = ini.GetBoolValue(L"Simple", L"InteriorBlurToggle", simple.interiorBlurToggle);
            simple.interiorBlurStrength     = static_cast<float>(ini.GetDoubleValue(L"Simple", L"InteriorBlurStrength", simple.interiorBlurStrength));
            simple.interiorBlurRange        = static_cast<float>(ini.GetDoubleValue(L"Simple", L"InteriorBlurRange", simple.interiorBlurRange));

            simple.exteriorBlurToggle       = ini.GetBoolValue(L"Simple", L"ExteriorBlurToggle", simple.exteriorBlurToggle);
            simple.exteriorBlurStrength     = static_cast<float>(ini.GetDoubleValue(L"Simple", L"ExteriorBlurStrength", simple.exteriorBlurStrength));
            simple.exteriorBlurRange        = static_cast<float>(ini.GetDoubleValue(L"Simple", L"ExteriorBlurRange", simple.exteriorBlurRange));

            Logger::info("Settings: INI loaded successfully.");
			return true;
        }

        bool Save() {
            CSimpleIniW ini;
            ini.SetUnicode();

            ini.SetLongValue(L"General", L"BlurType", general.BlurType, L"; Blur Type (0 = None, 1 = Simple, 2 = Advanced)");
			ini.SetBoolValue(L"General", L"ExtraChecks", general.ExtraChecks, L"; Enable Extra Safety Checks");
			ini.SetBoolValue(L"General", L"VerboseLogging", general.VerboseLogging, L"; Enable Verbose Logging");

            ini.SetBoolValue(L"Simple", L"BlurSmoothing", simple.blurSmoothing);
            ini.SetBoolValue(L"Simple", L"InteriorBlurToggle", simple.interiorBlurToggle);
            ini.SetDoubleValue(L"Simple", L"InteriorBlurStrength", simple.interiorBlurStrength);
            ini.SetDoubleValue(L"Simple", L"InteriorBlurRange", simple.interiorBlurRange);

            ini.SetBoolValue(L"Simple", L"ExteriorBlurToggle", simple.exteriorBlurToggle);
            ini.SetDoubleValue(L"Simple", L"ExteriorBlurStrength", simple.exteriorBlurStrength);
            ini.SetDoubleValue(L"Simple", L"ExteriorBlurRange", simple.exteriorBlurRange);

            ini.SaveFile(settingsPath);

            Logger::info("Settings: INI saved successfully.");
			return true;
        }

        void Reset() {
            general = GeneralSettings{};
            simple  = SimpleSettings{};
            Save();
            Logger::info("Settings: INI reset to defaults.");
        }
    }

    // ============================================================
    // JSON Weather Handling
    // ============================================================

    namespace Json {
        using namespace rapidjson;

        void Clear() {
            MCP::Advanced::g_advancedWeatherData.settings.clear();
        }

        bool Load() {
            Clear(); // Ensure list is empty before loading
            Logger::info("Settings::Weather: Loading JSON from '{}'", weatherListPath);

            if (!std::filesystem::exists(weatherListPath)) {
                Logger::warn("Settings::Weather: File '{}' does not exist. A new one will be created on save.", weatherListPath);
                return false;
            }

            std::ifstream file(weatherListPath);
            if (!file.is_open()) {
                Logger::error("Settings::Weather: Could not open '{}'", weatherListPath);
                return false;
            }

            std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            Document doc;
            doc.Parse(jsonContent.c_str());

            if (doc.HasParseError() || !doc.HasMember("MCP")) {
                Logger::error("Settings::Weather: Invalid JSON format.");
                return false;
            }

            const auto& mcp           = doc["MCP"];
            if (!mcp.HasMember("Advanced")) return true; // Valid JSON, just no settings yet

            const auto& advancedBlock = mcp["Advanced"];
            if (!advancedBlock.HasMember("WeatherSettings")) return true;

            const auto& settingsArray = advancedBlock["WeatherSettings"];
            if (!settingsArray.IsArray()) return true;

            for (const auto& row : settingsArray.GetArray()) {
                MCP::Advanced::WeatherSettingRow entryRow;
                entryRow.rowToggle       = row.HasMember("rowToggle")    ? row["rowToggle"].GetBool()     : true;
                entryRow.rowWeatherType  = row.HasMember("rowWeather")   ? row["rowWeather"].GetString()  : "None";
                entryRow.rowBlurStrength = row.HasMember("blurStrength") ? row["blurStrength"].GetFloat() : 1.0f;
                entryRow.rowBlurRange    = row.HasMember("blurRange")    ? row["blurRange"].GetFloat()    : 100.0f;
                entryRow.rowStaticToggle = row.HasMember("staticToggle") ? row["staticToggle"].GetBool()  : false;

                MCP::Advanced::g_advancedWeatherData.settings.push_back(entryRow);
            }

            Logger::info("Settings::Weather: Loaded {} weather rows.", MCP::Advanced::g_advancedWeatherData.settings.size());
            return true;
        }

        bool Save() {

            Logger::info("Settings::Weather: Saving to '{}'", weatherListPath);

            Document doc;
            doc.SetObject();
            auto& alloc = doc.GetAllocator();

            Value mcp(kObjectType);
            Value advanced(kObjectType);
            Value settingsArray(kArrayType);

            for (const auto& row : MCP::Advanced::g_advancedWeatherData.settings) {
                Value rowObj(kObjectType);
                rowObj.AddMember("rowToggle", row.rowToggle, alloc);
                rowObj.AddMember("rowWeather", Value(row.rowWeatherType.c_str(), alloc), alloc);
                rowObj.AddMember("blurStrength", row.rowBlurStrength, alloc);
                rowObj.AddMember("blurRange", row.rowBlurRange, alloc);
                rowObj.AddMember("staticToggle", row.rowStaticToggle, alloc);
                settingsArray.PushBack(rowObj, alloc);
            }

            advanced.AddMember("WeatherSettings", settingsArray, alloc);
            mcp.AddMember("Advanced", advanced, alloc);
            doc.AddMember("MCP", mcp, alloc);

            StringBuffer         buffer;
            Writer<StringBuffer> writer(buffer);

            doc.Accept(writer);

            std::ofstream file(weatherListPath);
            if (!file.is_open()) {
                Logger::error("Settings::Weather: Failed to write file.");
                return false;
            }

            file << buffer.GetString();
            file.close();
            Logger::info("Settings::Weather: Saved successfully.");
            return true;
        }
    }
}