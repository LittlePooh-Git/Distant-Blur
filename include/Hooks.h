#pragma once

#include "Settings.h"

namespace Hooks {

    class BlurManager {
        public:
            static BlurManager& GetSingleton() {
                static BlurManager instance;
                return instance;
            }
        
            bool Initialize();
        
            void OnPlayerUpdate(float a_delta);
        
            void NotifySettingsChanged() {
                _settingsDirty = true;
                Settings::SaveAll();
            }
        
            void SetSourceIMOD(RE::TESImageSpaceModifier* a_outIMOD) { _sourceIMod = a_outIMOD; }
        
        private:
            BlurManager()                              = default;
            ~BlurManager()                             = default;
            BlurManager(const BlurManager&)            = delete;
            BlurManager(BlurManager&&)                 = delete;
            BlurManager& operator=(const BlurManager&) = delete;
            BlurManager& operator=(BlurManager&&)      = delete;
        
            void CopyIMODData(RE::TESImageSpaceModifier* a_source, RE::TESImageSpaceModifier* a_dest);
        
            RE::TESImageSpaceModifier*          _imod           = nullptr;
            RE::TESImageSpaceModifier*          _sourceIMod     = nullptr;
            RE::ImageSpaceModifierInstanceForm* _imodInstance   = nullptr;
            RE::TESWeather*                     _currentWeather = nullptr;
        
            // Settings Data
            bool  _settingsDirty          = true;
            bool  _lastCellWasInterior    = false; 
        
            // Row Data
            float _currentTargetStrength  = 0.0f;
            float _currentTargetRange     = 0.0f;
            float _currentAppliedStrength = 0.0f;
            float _currentAppliedRange    = 0.0f;
            bool  _useStaticTransition    = false;
            bool  _effectIsActive         = false;
    };


    void InstallHooks();

    struct UpdateHook {
        static void Install();
        static void Update(RE::Actor* a_this, float a_delta);
        static inline REL::Relocation<decltype(Update)> Update_;
    };
}