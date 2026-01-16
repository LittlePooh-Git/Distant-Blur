#pragma once

#include "PCH.h"

namespace MCP {
	using namespace ImGuiMCP;

	void Register();
	
	static ImGuiTableFlags       tableFlags      = ImGuiTableFlags_Resizable          | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_SizingFixedFit;
	static ImGuiTableColumnFlags columnFlags     = ImGuiTableColumnFlags_WidthFixed   | ImGuiTableColumnFlags_NoReorder;
	static ImGuiTableColumnFlags lastColumnFlags = ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoReorder;
	static ImGuiInputTextFlags   inputFlags      = ImGuiInputTextFlags_CharsDecimal   | ImGuiInputTextFlags_AutoSelectAll;
	static ImGuiHoveredFlags     tooltipFlags    = ImGuiHoveredFlags_DelayNormal      | ImGuiHoveredFlags_NoSharedDelay;

	namespace General {
		void Render();
	}

	namespace Advanced {
		struct WeatherSettingRow {
			bool        rowToggle       = true;
			std::string rowWeatherType  = "None";
			float       rowBlurStrength = 1.0f;
			float       rowBlurRange    = 100.0f;
			bool        rowStaticToggle = false;

			bool operator==(const WeatherSettingRow& other) const {
				return rowToggle       == other.rowToggle &&
					   rowWeatherType  == other.rowWeatherType &&
					   rowBlurStrength == other.rowBlurStrength &&
					   rowBlurRange    == other.rowBlurRange &&
					   rowStaticToggle == other.rowStaticToggle;
			}
		};

		struct AdvancedWeatherState {
			std::vector<WeatherSettingRow> settings;
			int                            rowToRemove = -1;

			void AddRow() { settings.emplace_back(); }
			void RemoveRow() {
				if (rowToRemove != -1) {
					settings.erase(settings.begin() + rowToRemove);
					rowToRemove = -1;
				}
			}
		};

		extern AdvancedWeatherState g_advancedWeatherData;

		void Render();
	}

}