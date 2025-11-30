#include "PCH.h"
#include "Utils.h"
#include "MCP.h"

namespace Utils {
	std::map<std::type_index, std::vector<std::string>> g_formCache;
	std::map<std::type_index, bool>                     g_cachePopulated;

    bool ValidateForm(RE::TESForm* form, RE::FormType expectedType) {
        if (!form) {
            Logger::error("ValidateForm: form is null or does not exist, cannot proceed.");
            return false;
        }

        const auto formType = form->GetFormType();
        const auto formID   = form->GetFormID();

        if (form->IsDeleted()) {
            Logger::error("ValidateForm: form is deleted.");
            return false;
        }

        if (form->IsIgnored()) {
            Logger::error("ValidateForm: form is marked as Ignored, cannot proceed.");
            return false;
        }

        if (!form->IsInitialized()) {
            Logger::error("ValidateForm: form is not initialized.");
            return false;
        }

        if (form->GetFormFlags() & RE::TESForm::RecordFlags::kDisabled) {
            Logger::error("ValidateForm: form is marked as Disabled, cannot proceed.");
            return false;
        }

        if (formType == RE::FormType::None || formType != expectedType) {
            Logger::error("ValidateForm: form has incorrect FormType: {}, cannot proceed.", formType);
            return false;
        }

        if (!formID) {
            Logger::error("ValidateForm: form has invalid FormID, cannot proceed.");
            return false;
        }

        const char* editorID = (clib_util::editorID::get_editorID(form)).c_str();
        if (!editorID || editorID[0] == '\0') {
            Logger::warn("ValidateForm: form has empty EditorID.");
        }

        if (Settings::general.VerboseLogging) {
            Logger::info("ValidateForm: form is not null, proceeding.");

            auto* file = form->GetFile();
            if (!file) {
                Logger::warn("ValidateForm: form has no source file.");
            }
            else {
                Logger::info("ValidateForm: form has source file: {}", file->fileName);
            }

            auto* descFile = form->GetDescriptionOwnerFile();
            if (descFile) {
                Logger::info("ValidateForm: form has a description owner file: {}", descFile->fileName);
            }

            Logger::info("ValidateForm: form has correct FormType: {}", formType);
            Logger::info("ValidateForm: form has valid FormID: {:x}", formID);
            Logger::info("ValidateForm: form has EditorID: {}", editorID);

            const char* name = form->GetName();
            if (!name || name[0] == '\0') {
                Logger::warn("ValidateForm: form has no name.");
            } else {
                Logger::info("ValidateForm: form has name: {}", name);
            }

            if (form->IsParentForm())                Logger::info("ValidateForm: form is a Parent Form.");
            if (form->IsParentFormTree())            Logger::info("ValidateForm: form is a Parent Form Tree.");
            if (form->IsFormTypeChild(expectedType)) Logger::info("ValidateForm: form is a Child Form.");
            if (form->IsDynamicForm())               Logger::info("ValidateForm: form was made by Form Factory.");

            auto refCount = form->GetRefCount();
            if (refCount > 0) {
                Logger::info("ValidateForm: form has {} references in the current game session.", refCount);
            }
        }

        return true;
    }

    inline std::string GetWeatherName(RE::TESWeather* weather) {
        if (weather) {
            return clib_util::editorID::get_editorID(weather);
        }
        return "None";
    }

    std::string GetCurrentWeather() {
        const auto sky = RE::Sky::GetSingleton();

        if (!sky) {
            return "None";
        }

        return GetWeatherName(sky->currentWeather);
    }

    std::string GetPreviousWeather() {
        const auto sky = RE::Sky::GetSingleton();

        if (!sky) {
            return "None";
        }

        return GetWeatherName(sky->lastWeather);
    }
}