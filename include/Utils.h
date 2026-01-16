#pragma once

#include "Settings.h"

namespace Utils {
	extern std::map<std::type_index, std::vector<std::string>> g_formCache;
	extern std::map<std::type_index, bool>                     g_cachePopulated;

	/**
	 * @brief Validates a TESForm pointer against expected criteria.
	 *
	 * This function checks if the provided form is non-null, has a valid
	 * FormType matching the expected type, and possesses a valid FormID.
	 *
	 * @param form The TESForm pointer to validate.
	 * @param expectedType The expected FormType of the form.
	 * @return true if the form is valid and matches the expected type; false otherwise.
	 */
	bool ValidateForm(RE::TESForm* form, RE::FormType expectedType);

	/**
	 * @brief Counts, logs, and caches all forms of a specified type.
	 *
	 * This function is a template, allowing it to work with any RE::TESForm-derived class.
	 * It iterates through the form array for the given type, validates each form,
	 * logs its information, and stores its editorID in a global cache for later use.
	 *
	 * @tparam T The form type to count (e.g., RE::TESWeather, RE::TESImageSpaceModifier).
	 *           This type MUST have a static member `formType` of type RE::FormType.
	 * @param specialProcessing An optional lambda function to run on each valid form found.
	 *                          This is useful for handling type-specific logic without
	 *                          cluttering the main function.
	 * @return The total number of valid forms found for the given type.
	 */
	template <typename T>
	int CountAndCacheForms(std::function<void(T*)> specialProcessing = nullptr) {
		// Get a unique, safe key to use in map.
		auto typeKey  = std::type_index(typeid(T));
		auto typeName = typeid(T).name(); // Cache for logging

		if (g_cachePopulated[typeKey]) {
			Logger::info("Cache for {} is already populated with {} entries. Skipping.", typeName, g_formCache[typeKey].size());
			return static_cast<int>(g_formCache[typeKey].size());
		}

		Logger::info("Populating form cache for type: {}.", typeName);

		auto& formList = g_formCache[typeKey];
		formList.clear();
		formList.push_back("None"); // Add a default "None" option

		int count = 0;

		for (T* form : RE::TESDataHandler::GetSingleton()->GetFormArray<T>()) {
			if ((ValidateForm(form, T::FORMTYPE) && Settings::general.ExtraChecks) || form) {
				auto editorID = clib_util::editorID::get_editorID(form);
				Logger::trace("Found {}: {} | FormID: {:x}", typeName, editorID, form->GetFormID());

				formList.push_back(editorID);
				count++;

				if (specialProcessing) {
					specialProcessing(form);
				}
			} else {
				Logger::warn("Skipping a null pointer in the {} form array.", typeName);
			}
		}
		if (formList.size() > 1) {
			std::sort(formList.begin() + 1, formList.end());
		}
		g_cachePopulated[typeKey] = true;
		Logger::info("Finished populating cache for {}. Found {} valid entries.", typeName, count);

		// The final size will be count + 1 because of the "None" entry.
		return static_cast<int>(formList.size());
	}

	std::string GetCurrentWeather();
	std::string GetPreviousWeather();

	inline void CenteredImGuiText(const char* text) {
		ImGuiMCP::ImVec2 a_vec;
		ImGuiMCP::CalcTextSize(&a_vec, text, nullptr, false, 0);

		// Calculate the horizontal position to center the text
		float textX = ImGuiMCP::GetCursorPosX() + (ImGuiMCP::GetColumnWidth() - a_vec.x) * 0.5f;

		ImGuiMCP::SetCursorPosX(textX);
		ImGuiMCP::Text("%s", text);
		return;
	}
}