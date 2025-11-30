#include "PCH.h"
#include "MCP.h"
#include "Hooks.h"
#include "Settings.h"
#include "Utils.h"

namespace MCP {
	void Register() {
		Logger::info("Registering MCP functionality");
		if (!SKSEMenuFramework::IsInstalled()) {
			Logger::error("SKSEMenuFramework is not installed. MCP functionality cannot be registered.");
			return;
		}
		SKSEMenuFramework::SetSection(modName);
		SKSEMenuFramework::AddSectionItem("General Settings", General::Render);
		SKSEMenuFramework::AddSectionItem("Simple Mode", Simple::Render);
		SKSEMenuFramework::AddSectionItem("Advanced Mode", Advanced::Render); // Helper function looks mad tempting here
	}
	

	namespace General {

		enum BlurModes { None, Simple, Advanced, BlurMode_COUNT };

		inline static int  defaultBlurMode                      = Advanced;
		inline const char* blurModeNames[BlurMode_COUNT]        = { "None", "Simple", "Advanced" };
		inline const char* blurModeDescriptions[BlurMode_COUNT] = {
			"No blur effects will be applied.\n\nEfficient, but distant objects may appear sharp and distinct.",
			"The simple implementation.\n\nAllows tweaking of both interior and exterior weathers. Works with all weather mods out of the box.", // Uses the depth buffer and camera distance to calculate blur automatically. (What I want to add)
			"The original implementation.\n\nUses weather IDs to determine blur strength. Good for specific weather setups, but requires manual configuration for new weather mods."
		};

		inline const std::string blurModeIcons[BlurMode_COUNT] = {
			FontAwesome::UnicodeToUtf8(0xf05e), // Disabled
			FontAwesome::UnicodeToUtf8(0xf70e), // Simple
			FontAwesome::UnicodeToUtf8(0xf013)  // Advanced
		}; // I'm probably going to consolidate these icons with the ones in Advanced later.

		void __stdcall Render() {
			auto& general = Settings::general;

			const char* selectedBlurMode = (defaultBlurMode >= 0 && defaultBlurMode < BlurMode_COUNT) ? blurModeNames[defaultBlurMode] : "Unknown";
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::SliderInt("Blur Mode", &defaultBlurMode, 0, BlurMode_COUNT - 1, selectedBlurMode)) {
				general.BlurType = defaultBlurMode;
			}

			ImGui::Spacing();

			if (ImGui::BeginChild("BlurModeInfoBox", ImVec2(0, 175.0f), true, ImGuiWindowFlags_NoScrollbar)) {

				if (ImGui::BeginTable("BlurModeTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {

					ImGui::TableSetupColumn("Icon", columnFlags, 75.0f);
					ImGui::TableSetupColumn("Title", columnFlags, 100.0f);
					ImGui::TableSetupColumn("Desc", lastColumnFlags); // Making a helper function for this is tempting

					ImGui::TableNextRow();

					// -- Column 1: Icon --
					ImGui::TableNextColumn();
					float cellHeight = 100.0f;
					float cursorY    = ImGui::GetCursorPosY();
					ImGui::SetCursorPosY(cursorY + (cellHeight * 0.45f));

					FontAwesome::PushSolid();
					ImGui::SetWindowFontScale(1.8f);
					Utils::CenteredImGuiText(blurModeIcons[defaultBlurMode].c_str());
					ImGui::SetWindowFontScale(1.0f);
					FontAwesome::Pop();

					// -- Column 2: Title --
					ImGui::TableNextColumn();
					ImGui::SetCursorPosY(cursorY + (cellHeight * 0.5f));

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
					Utils::CenteredImGuiText(blurModeNames[defaultBlurMode]);
					ImGui::PopStyleColor();

					// -- Column 3: Description --
					ImGui::TableNextColumn();
					ImGui::SetWindowFontScale(0.8f);
					ImGui::TextDisabled("Mode Description:");
					ImGui::SetWindowFontScale(0.9f);
					ImGui::Separator();
					ImGui::PushTextWrapPos(0.0f);
					ImGui::Text("%s", blurModeDescriptions[defaultBlurMode]);
					ImGui::PopTextWrapPos();
					ImGui::SetWindowFontScale(1.0f);
					ImGui::EndTable();
				}
				ImGui::EndChild();
			}

			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Maintenance Settings##header")) {
				ImGui::Checkbox("Extra Validity Checks", &general.ExtraChecks);
				if (ImGui::IsItemHovered(tooltipFlags)) {
					ImGui::SetTooltip("Perform extra checks on Weather Forms on game startup, this increases stability, but may cause a slight hitch on game load. (Default: Enabled)");
				}

				ImGui::Checkbox("Verbose Logging", &general.VerboseLogging);
				if (ImGui::IsItemHovered(tooltipFlags)) {
					ImGui::SetTooltip("Enable Extra Logging. (Default: Disabled)");
				}
			}

		}
	}

	namespace Simple {
		void __stdcall Render() {
			auto& simple = Settings::simple;

			ImGui::Checkbox("Blur Smoothing Toggle", &simple.blurSmoothing);

			RenderBlurSection("Interior Settings##header",
				simple.interiorBlurToggle,
				simple.interiorBlurStrength,
				simple.interiorBlurRange);

			RenderBlurSection("Exterior Settings##header",
				simple.exteriorBlurToggle,
				simple.exteriorBlurStrength,
				simple.exteriorBlurRange);
		}
	}

	namespace Advanced {

		AdvancedWeatherState g_advancedWeatherData;

		struct ColumnData {
			const char* title;
			float       width;
		};

		const ColumnData COLUMN_SETUPS[] = {
			{ "Handle",   55.0f  },
			{ "Toggle",   60.0f  },
			{ "Weather",  250.0f },
			{ "Forecast", 65.0f  },
			{ "Strength", 175.0f },
			{ "Range",    175.0f },
			{ "Static",   50.0f  },
			{ "Reset",    50.0f  },
			{ "Remove",   0.0f   }  // A width of 0.0f signifies a stretchy column
		};

		struct IconLibrary {
			inline static const std::string DragHandle = FontAwesome::UnicodeToUtf8(0xf0c9) + "##Drag-Handle";
			inline static const std::string GetWeather = FontAwesome::UnicodeToUtf8(0xe09a) + "##Get-Weather";
			inline static const std::string ResetRow   = FontAwesome::UnicodeToUtf8(0xf021) + "##Reset-Row";
			inline static const std::string RemoveRow  = FontAwesome::UnicodeToUtf8(0xf1f8) + "##Remove-Row";
			inline static const std::string AddRow     = FontAwesome::UnicodeToUtf8(0xf0fe) + "##Add-Row";
		};

		void DrawTableRow(int rowIndex, WeatherSettingRow& currentRow, const std::vector<std::string>& weatherNames) {
			ImGui::PushID(rowIndex);
			ImGui::TableNextRow();

			Logger::trace("Drawing row {}: Toggle = {}, Weather = '{}', Strength = {}, Range = {}, Static = {}",
				rowIndex,
				currentRow.rowToggle,
				currentRow.rowWeatherType,
				currentRow.rowBlurStrength,
				currentRow.rowBlurRange,
				currentRow.rowStaticToggle
			);

			// Column 0: Drag Handle
			ImGui::TableNextColumn();
			FontAwesome::PushSolid();
			ImGui::Button(IconLibrary::DragHandle.c_str(), ImVec2(-FLT_MIN, 0.0f));
			FontAwesome::Pop();

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::SetDragDropPayload("MCP_WEATHER_ROW", &rowIndex, sizeof(int));
				ImGui::Text("Moving Row %d", rowIndex);
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MCP_WEATHER_ROW")) {
					int sourceIndex = *(const int*)payload->Data;
					if (sourceIndex != rowIndex) {
						auto& data = g_advancedWeatherData.settings;
						std::swap(data[sourceIndex], data[rowIndex]);
						Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
					}
				}
				ImGui::EndDragDropTarget();
			}
			Logger::trace("Rendered Drag Handle for row {}", rowIndex);

			// Column 1: Toggle
			ImGui::TableNextColumn();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::GetFrameHeight()) * 0.5f);
			if (ImGui::Checkbox("##Toggle", &currentRow.rowToggle)) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			Logger::trace("Rendered Toggle checkbox for row {}", rowIndex);

			// Column 2: Weather ComboBox
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			std::string originalWeather = currentRow.rowWeatherType;
			if (ImGui::BeginCombo("##Weather", originalWeather.c_str())) {
				for (const auto& weatherName : weatherNames) {
					bool isUsedByOther = false;

					// "None" is allowed to be duplicated. Current Row is allowed to select its own weather.
					if (weatherName != "None" && weatherName != currentRow.rowWeatherType) {
						for (const auto& existingRow : g_advancedWeatherData.settings) {
							if (existingRow.rowWeatherType == weatherName) {
								isUsedByOther              =  true;
								break;
							}
						}
					}

					// Skip this iteration if weather is taken
					if (isUsedByOther) continue;
					bool isSelected = (currentRow.rowWeatherType == weatherName);
					if (ImGui::Selectable(weatherName.c_str(), isSelected)) {
						currentRow.rowWeatherType = weatherName;
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
				if (originalWeather != currentRow.rowWeatherType) {
					Logger::trace("Weather changed for row {}: '{}' -> '{}'", rowIndex, originalWeather, currentRow.rowWeatherType);
					Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
				}
			}
			Logger::trace("Rendered Weather ComboBox for row {}", rowIndex);

			// Column 3: Get Current Weather
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			FontAwesome::PushSolid();
			if (ImGui::Button(IconLibrary::GetWeather.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
				Logger::trace("Weather selection button clicked for row {}", rowIndex);
				currentRow.rowWeatherType = Utils::GetCurrentWeather();
			}
			if (originalWeather != currentRow.rowWeatherType) {
				Logger::trace("Weather changed for row {}: '{}' -> '{}'", rowIndex, originalWeather, currentRow.rowWeatherType);
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			FontAwesome::Pop();
			Logger::trace("Rendered Get Current Weather button for row {}", rowIndex);

			// Column 4: Strength
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			if (ImGui::InputFloat("##Strength", &currentRow.rowBlurStrength, 0.01f, 0.1f, "%.2f", inputFlags)) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			Logger::trace("Rendered Strength input for row {}", rowIndex);

			// Column 5: Range
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			if (ImGui::InputFloat("##Range", &currentRow.rowBlurRange, 10.0f, 100.0f, "%.2f", inputFlags)) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			Logger::trace("Rendered Range input for row {}", rowIndex);

			// Column 6: Static
			ImGui::TableNextColumn();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::GetFrameHeight()) * 0.5f);
			if (ImGui::Checkbox("##Static", &currentRow.rowStaticToggle)) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			Logger::trace("Rendered Static checkbox for row {}", rowIndex);

			// Column 7: Reset
			ImGui::TableNextColumn();
			FontAwesome::PushSolid();
			auto originalRow = currentRow;
			if (ImGui::Button(IconLibrary::ResetRow.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
				currentRow = WeatherSettingRow{};
			}
			if (originalRow != currentRow) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			FontAwesome::Pop();
			Logger::trace("Rendered Reset button for row {}", rowIndex);

			// Column 8: Remove
			ImGui::TableNextColumn();
			FontAwesome::PushSolid();
			if (ImGui::Button(IconLibrary::RemoveRow.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
				g_advancedWeatherData.rowToRemove = rowIndex;
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			FontAwesome::Pop();
			Logger::trace("Rendered Remove button for row {}", rowIndex);

			ImGui::PopID();
			Logger::trace("Finished drawing row {}", rowIndex);
		}

		void RenderWeatherTable() {

			Logger::trace("Rendering Weather Table with {} rows", g_advancedWeatherData.settings.size());

			const auto& weatherNames = Utils::g_formCache[std::type_index(typeid(RE::TESWeather))];
			const int   columnCount  = std::size(COLUMN_SETUPS);
			Logger::trace("Weather Table Column Count: {}", columnCount);

			if (ImGui::BeginTable("WeatherTable", columnCount, tableFlags)) {
				for (const auto& setup : COLUMN_SETUPS) {
					if (setup.width > 0.0f) {
						ImGui::TableSetupColumn(setup.title, columnFlags, setup.width);
						Logger::trace("Setting up column '{}' with stretchy width {}", setup.title, setup.width);
					} else {
						ImGui::TableSetupColumn(setup.title, lastColumnFlags);
						Logger::trace("Setting up last column '{}' with adapting width", setup.title);
					}
				}

				ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
				for (int headerColumn = 0; headerColumn < columnCount; headerColumn++) {
					Logger::trace("Drawing header for column index {}", headerColumn);
					ImGui::TableSetColumnIndex(headerColumn);
					Utils::CenteredImGuiText(COLUMN_SETUPS[headerColumn].title);
					Logger::trace("Header drawn: {}", COLUMN_SETUPS[headerColumn].title);
				}

				for (int row = 0; row < g_advancedWeatherData.settings.size(); row++) {
					DrawTableRow(row, g_advancedWeatherData.settings[row], weatherNames);
					Logger::trace("Drawn row {}", row);
				}
				ImGui::EndTable();

				g_advancedWeatherData.RemoveRow();
				Logger::trace("Processed row removals if any");

				// Add Row button
				FontAwesome::PushRegular();
				if (ImGui::Button(IconLibrary::AddRow.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
					Logger::trace("Add New Weather button clicked");
					g_advancedWeatherData.AddRow();
					Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
				}
				FontAwesome::Pop();
			}
		}

		void __stdcall Render() {
			RenderWeatherTable();
		}
	}
}