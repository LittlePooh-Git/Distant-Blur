#include "PCH.h"
#include "Hooks.h"
#include "MCP.h"
#include "Utils.h"

namespace Hooks {

    void InstallHooks() {
		if (BlurManager::GetSingleton().Initialize()) {
			UpdateHook::Install();
			Logger::info("Hooks installed successfully.");
		} else {
			Logger::critical("Failed to initialize BlurManager. Hooks will not be installed.");
		}
	}


	void UpdateHook::Install() {
		REL::Relocation<std::uintptr_t> playerVtbl{ RE::VTABLE_PlayerCharacter[0] };
		Update_ = playerVtbl.write_vfunc(0xAD, Update);
		Logger::info("Player update hook installed.");
	}

	void UpdateHook::Update(RE::Actor* a_this, float a_delta) {
		Update_(a_this, a_delta);
		BlurManager::GetSingleton().OnPlayerUpdate(a_delta);
	}


	bool BlurManager::Initialize() {
		if (!_sourceIMod) {
			Utils::CountAndCacheForms<RE::TESImageSpaceModifier>();
			if (!_sourceIMod) {
				Logger::error("BlurManager: sourceIMod is null. Cannot create runtime IMOD.");
				return false;
			}
		}

		const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESImageSpaceModifier>();
		_imod = factory ? factory->Create() : nullptr;

		if (!Utils::ValidateForm(_imod, RE::FormType::ImageAdapter)) {
			Logger::error("BlurManager: Failed to create TESImageSpaceModifier form.");
			return false;
		}

		CopyIMODData(_sourceIMod, _imod);
		_imod->SetFormEditorID("DistantBlurIMOD");

		auto dataHandler = RE::TESDataHandler::GetSingleton();
		dataHandler->GetFormArray<RE::TESImageSpaceModifier>().push_back(_imod);

        Logger::info("Runtime IMOD '{}' created successfully.", clib_util::editorID::get_editorID(_imod));
		return true;
	}

    void BlurManager::OnPlayerUpdate(float a_delta) {
        if (!_imod) return;

        int currentMode = Settings::general.BlurType;
        // 0 = None, 1 = Simple, 2 = Advanced
        
        switch (currentMode) {
        // ========================================================
        // MODE: NONE (Disable Blur)
        // ========================================================
            case 0: 
            {
                if (_currentTargetStrength != 0.0f) {
                    _currentTargetStrength  = 0.0f;
                    _currentTargetRange     = 0.0f;
                    _useStaticTransition    = false; // Fade out nicely
                    Logger::trace("Mode is None: Removing blur.");
                }
                break;
            }
            
            // ========================================================
            // MODE: SIMPLE - I'm contemplating whether this is necessary or not.
            // ========================================================
            case 1: 
            {
                auto player = RE::PlayerCharacter::GetSingleton();
                auto cell   = player ? player->GetParentCell() : nullptr;
                
                if (!cell) break;
            
                bool isInterior = cell->IsInteriorCell();
            
                if (isInterior != _lastCellWasInterior || _settingsDirty) {
                    auto& simple         = Settings::simple;
                    _lastCellWasInterior = isInterior;
                    _useStaticTransition = !simple.blurSmoothing; 
            
                    if (isInterior) {
                        if (simple.interiorBlurToggle) {
                            _currentTargetStrength = simple.interiorBlurStrength;
                            _currentTargetRange    = simple.interiorBlurRange * 10.0f; // Scale correction
                            Logger::trace("Simple Mode (Interior): Applied Str {}, Rng {}", _currentTargetStrength, _currentTargetRange);
                        } else {
                            _currentTargetStrength = 0.0f;
                            _currentTargetRange    = 0.0f;
                            Logger::trace("Simple Mode (Interior): Blur Disabled via toggle");
                        }
                    } else { // Exterior
                        if (simple.exteriorBlurToggle) {
                            _currentTargetStrength = simple.exteriorBlurStrength;
                            _currentTargetRange    = simple.exteriorBlurRange * 10.0f;
                            Logger::trace("Simple Mode (Exterior): Applied Str {}, Rng {}", _currentTargetStrength, _currentTargetRange);
                        } else {
                            _currentTargetStrength = 0.0f;
                            _currentTargetRange    = 0.0f;
                            Logger::trace("Simple Mode (Exterior): Blur Disabled via toggle");
                        }
                    }
                }
                break;
            }
            
            // ========================================================
            // MODE: ADVANCED (Weather Table)
            // ========================================================
            case 2: 
            {
                auto newWeather       = RE::Sky::GetSingleton()->currentWeather;
                bool isWeatherChanged = (newWeather != _currentWeather || _settingsDirty);
            
                if (isWeatherChanged) {
                    _currentWeather   = newWeather;
                    auto  weatherID   = Utils::GetCurrentWeather(); // I might have to tweak this function later so I can use it for both "newWeather" above and prevWeatherID below.
                    auto& settings    = MCP::Advanced::g_advancedWeatherData.settings;
                    auto  it          = std::find_if(settings.begin(), settings.end(), [&](const auto& setting) {
                        return setting.rowToggle && setting.rowWeatherType == weatherID;
                    });
            
                    if (it != settings.end()) {
                        Logger::trace("Advanced Mode: Weather changed to '{}', applying blur.", weatherID);
                        _currentTargetStrength = it->rowBlurStrength;
                        _currentTargetRange    = it->rowBlurRange * 10;
                        _useStaticTransition   = it->rowStaticToggle;
                    } else {
                        Logger::trace("Advanced Mode: Weather changed to '{}', removing blur.", weatherID);
                        _currentTargetStrength = 0.0f;
                        _currentTargetRange    = 0.0f;
            
                        // Check PREVIOUS weather to decide how we transition OUT
                        auto prevWeatherID = Utils::GetPreviousWeather();
                        auto it_old        = std::find_if(settings.begin(), settings.end(), [&](const auto& setting) {
                            return setting.rowToggle && setting.rowWeatherType == prevWeatherID;
                        });
            
                        if (it_old != settings.end()) {
                            _useStaticTransition = it_old->rowStaticToggle;
                        } else {
                            _useStaticTransition = false; // Default to smooth fade out
                        }
                    }
                }
                break;
            }
        }

        if (_settingsDirty) _settingsDirty = false;

        float nextStrength = _currentAppliedStrength;
        float nextRange    = _currentAppliedRange;

        if (_useStaticTransition) {
            nextStrength   = _currentTargetStrength;
            nextRange      = _currentTargetRange;
        } else {
            if (std::abs(_currentAppliedStrength - _currentTargetStrength) > 0.001f) {
                const float transitionSpeed = 0.5f;
                float       blendFactor     = 1.0f - powf(1.0f - transitionSpeed, a_delta);

                nextStrength = std::lerp(_currentAppliedStrength, _currentTargetStrength, blendFactor);
                nextRange    = std::lerp(_currentAppliedRange, _currentTargetRange, blendFactor);
            } else {
                nextStrength = _currentTargetStrength;
                nextRange    = _currentTargetRange;
            }
        }

        bool strengthChanged = (nextStrength != _currentAppliedStrength);
        bool rangeChanged    = (nextRange    != _currentAppliedRange);

        if (strengthChanged || rangeChanged) {
            _currentAppliedStrength = nextStrength;
            _currentAppliedRange    = nextRange;

            if (_currentAppliedStrength > 0.0f) {
                _imod->dof.strength->floatValue = _currentAppliedStrength;
                _imod->dof.range->floatValue    = _currentAppliedRange;

                Logger::trace("DOF updated: Str {}, Rng {}", _currentAppliedStrength, _currentAppliedRange);

                if (!_effectIsActive) {
                    _imodInstance   = RE::ImageSpaceModifierInstanceForm::Trigger(_imod, 1.0, nullptr);
                    _effectIsActive = true;
                    Logger::debug("Blur effect triggered.");
                }
            }
        }

        if (_currentAppliedStrength <= 0.0f && _effectIsActive) {
            RE::ImageSpaceModifierInstanceForm::Stop(_imod);
            _imodInstance   = nullptr;
            _effectIsActive = false;
            Logger::debug("Blur effect stopped.");
        }
    }

	void BlurManager::CopyIMODData(RE::TESImageSpaceModifier* a_source, RE::TESImageSpaceModifier* a_dest) {
		a_dest->formFlags            = a_source->formFlags;
		a_dest->formType             = a_source->formType;
		a_dest->bloom                = a_source->bloom;
		a_dest->cinematic            = a_source->cinematic;
		a_dest->hdr                  = a_source->hdr;
		a_dest->radialBlur           = a_source->radialBlur;
		a_dest->dof                  = a_source->dof;
		a_dest->doubleVisionStrength = a_source->doubleVisionStrength;
		a_dest->fadeColor            = a_source->fadeColor;
		a_dest->tintColor            = a_source->tintColor;
	}
}