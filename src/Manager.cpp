#include "PCH.h"
#include "Manager.h"
#include "Hooks.h"
#include "Utils.h"

namespace Manager {
	/**
	* @brief The main initialization function for the mod logic.
	* Called from the SKSE plugin entry point after game data is loaded.
	*/
	void Initialize() {
		settingsPath = settingsPathString.c_str(); // Fuck dangling pointers
		Logger::trace("Manager: Mod name set to {} | Settings path set to {}", modName, settingsPath);

		InitializeFormCaches();

		Settings::LoadAll();

		Hooks::InstallHooks();

		Logger::info("Manager: Exiting, {} Initialization finished.", modName);
	}

	void InitializeFormCaches() {
		Logger::info("Initializing form caches...");

		Utils::CountAndCacheForms<RE::TESWeather>();

		auto iMADProcessingLogic = [&](RE::TESImageSpaceModifier* imageAdapter) {
			if (imageAdapter->GetFormID() == 0x2FBB2) {
				Logger::trace("Found Vanilla Image Adapter, FormID: {:x}, EditorID: {}, using this record as a source form.",
					imageAdapter->GetFormID(), clib_util::editorID::get_editorID(imageAdapter));

				Hooks::BlurManager::GetSingleton().SetSourceIMOD(imageAdapter);
				RE::TESFile* targetFile = imageAdapter->GetFile(0);

				Logger::trace("Setting targetFile to plugin: {}", targetFile->GetFilename());
			}
		};

		Utils::CountAndCacheForms<RE::TESImageSpaceModifier>(iMADProcessingLogic);

		const auto& weatherNames = Utils::g_formCache[std::type_index(typeid(RE::TESWeather))];
		const auto& iMADNames    = Utils::g_formCache[std::type_index(typeid(RE::TESImageSpaceModifier))];

		Logger::info("Found {} weathers and {} IMADs in the cache.", weatherNames.size(), iMADNames.size());
	}
}