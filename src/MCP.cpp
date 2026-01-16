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
		SKSEMenuFramework::AddSectionItem("Advanced Mode", Advanced::Render);
	}
	
	namespace General {

		enum BlurModes { None, Advanced, BlurMode_COUNT };

		inline static int  defaultBlurMode                      = Advanced;
		inline const char* blurModeNames[BlurMode_COUNT]        = { "None", "Advanced" };
		inline const char* blurModeDescriptions[BlurMode_COUNT] = {
			"No blur effects will be applied.\n\nEfficient, but distant objects may appear sharp and distinct.",
			"The original implementation.\n\nUses weather IDs to determine blur strength. Good for specific weather setups, but requires manual configuration for new weather mods."
		};

		inline const std::string blurModeIcons[BlurMode_COUNT] = {
			FontAwesome::UnicodeToUtf8(0xf05e), // Disabled
			FontAwesome::UnicodeToUtf8(0xf013)  // Advanced
		};

		void __stdcall Render() {
			auto& general = Settings::general;

			const char* selectedBlurMode = (defaultBlurMode >= 0 && defaultBlurMode < BlurMode_COUNT) ? blurModeNames[defaultBlurMode] : "Unknown";
			ImGuiMCP::SetNextItemWidth(-1.0f);
			if (ImGuiMCP::SliderInt("Blur Mode", &defaultBlurMode, 0, BlurMode_COUNT - 1, selectedBlurMode)) {
				general.BlurType = defaultBlurMode;
			}

			ImGuiMCP::Spacing();

			if (ImGuiMCP::BeginChild("BlurModeInfoBox", ImVec2(0, 175.0f), true, ImGuiWindowFlags_NoScrollbar)) {

				if (ImGuiMCP::BeginTable("BlurModeTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {

					ImGuiMCP::TableSetupColumn("Icon", columnFlags, 75.0f);
					ImGuiMCP::TableSetupColumn("Title", columnFlags, 100.0f);
					ImGuiMCP::TableSetupColumn("Desc", lastColumnFlags); // Making a helper function for this is tempting

					ImGuiMCP::TableNextRow();

					// -- Column 1: Icon --
					ImGuiMCP::TableNextColumn();
					float cellHeight = 100.0f;
					float cursorY    = ImGuiMCP::GetCursorPosY();
					ImGuiMCP::SetCursorPosY(cursorY + (cellHeight * 0.45f));

					FontAwesome::PushSolid();
					ImGuiMCP::SetWindowFontScale(1.8f);
					Utils::CenteredImGuiText(blurModeIcons[defaultBlurMode].c_str());
					ImGuiMCP::SetWindowFontScale(1.0f);
					FontAwesome::Pop();

					// -- Column 2: Title --
					ImGuiMCP::TableNextColumn();
					ImGuiMCP::SetCursorPosY(cursorY + (cellHeight * 0.5f));

					ImGuiMCP::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
					Utils::CenteredImGuiText(blurModeNames[defaultBlurMode]);
					ImGuiMCP::PopStyleColor();

					// -- Column 3: Description --
					ImGuiMCP::TableNextColumn();
					ImGuiMCP::SetWindowFontScale(0.8f);
					ImGuiMCP::TextDisabled("Mode Description:");
					ImGuiMCP::SetWindowFontScale(0.9f);
					ImGuiMCP::Separator();
					ImGuiMCP::PushTextWrapPos(0.0f);
					ImGuiMCP::Text("%s", blurModeDescriptions[defaultBlurMode]);
					ImGuiMCP::PopTextWrapPos();
					ImGuiMCP::SetWindowFontScale(1.0f);
					ImGuiMCP::EndTable();
				}
				ImGuiMCP::EndChild();
			}

			ImGuiMCP::Spacing();

			if (ImGuiMCP::CollapsingHeader("Maintenance Settings##header")) {
				ImGuiMCP::Checkbox("Extra Validity Checks", &general.ExtraChecks);
				if (ImGuiMCP::IsItemHovered(tooltipFlags)) {
					ImGuiMCP::SetTooltip("Perform extra checks on Weather Forms on game startup, this increases stability, but may cause a slight hitch on game load. (Default: Enabled)");
				}

				ImGuiMCP::Checkbox("Verbose Logging", &general.VerboseLogging);
				if (ImGuiMCP::IsItemHovered(tooltipFlags)) {
					ImGuiMCP::SetTooltip("Enable Extra Logging. (Default: Disabled)");
				}
			}

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
			ImGuiMCP::PushID(rowIndex);
			ImGuiMCP::TableNextRow();

			Logger::trace("Drawing row {}: Toggle = {}, Weather = '{}', Strength = {}, Range = {}, Static = {}",
				rowIndex,
				currentRow.rowToggle,
				currentRow.rowWeatherType,
				currentRow.rowBlurStrength,
				currentRow.rowBlurRange,
				currentRow.rowStaticToggle
			);

			// Column 0: Drag Handle
			ImGuiMCP::TableNextColumn();
			FontAwesome::PushSolid();
			ImGuiMCP::Button(IconLibrary::DragHandle.c_str(), ImVec2(-FLT_MIN, 0.0f));
			FontAwesome::Pop();

			if (ImGuiMCP::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGuiMCP::SetDragDropPayload("MCP_WEATHER_ROW", &rowIndex, sizeof(int));
				ImGuiMCP::Text("Moving Row %d", rowIndex);
				ImGuiMCP::EndDragDropSource();
			}

			if (ImGuiMCP::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGuiMCP::AcceptDragDropPayload("MCP_WEATHER_ROW")) {
					int sourceIndex = *(const int*)payload->Data;
					if (sourceIndex != rowIndex) {
						auto& data = g_advancedWeatherData.settings;
						std::swap(data[sourceIndex], data[rowIndex]);
						Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
					}
				}
				ImGuiMCP::EndDragDropTarget();
			}
			Logger::trace("Rendered Drag Handle for row {}", rowIndex);

			// Column 1: Toggle
			ImGuiMCP::TableNextColumn();
			ImGuiMCP::SetCursorPosX(ImGuiMCP::GetCursorPosX() + (ImGuiMCP::GetColumnWidth() - ImGuiMCP::GetFrameHeight()) * 0.5f);
			if (ImGuiMCP::Checkbox("##Toggle", &currentRow.rowToggle)) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			Logger::trace("Rendered Toggle checkbox for row {}", rowIndex);

			// Column 2: Weather ComboBox
			ImGuiMCP::TableNextColumn();
			ImGuiMCP::PushItemWidth(-FLT_MIN);
			std::string originalWeather = currentRow.rowWeatherType;
			if (ImGuiMCP::BeginCombo("##Weather", originalWeather.c_str())) {
				for (const auto& weatherName : weatherNames) {
					bool isUsedByOther = false;

					// "None" is allowed to be duplicated. Current Row is allowed to select its own weather.
					if (weatherName != "None" && weatherName != currentRow.rowWeatherType) {
						for (const auto& existingRow : g_advancedWeatherData.settings) {
							if (existingRow.rowWeatherType == weatherName) {
								isUsedByOther = true;
								break;
							}
						}
					}

					// Skip this iteration if weather is taken
					if (isUsedByOther) continue;
					bool isSelected = (currentRow.rowWeatherType == weatherName);
					if (ImGuiMCP::Selectable(weatherName.c_str(), isSelected)) {
						currentRow.rowWeatherType = weatherName;
					}
					if (isSelected) ImGuiMCP::SetItemDefaultFocus();
				}
				ImGuiMCP::EndCombo();
				if (originalWeather != currentRow.rowWeatherType) {
					Logger::trace("Weather changed for row {}: '{}' -> '{}'", rowIndex, originalWeather, currentRow.rowWeatherType);
					Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
				}
			}
			Logger::trace("Rendered Weather ComboBox for row {}", rowIndex);

			// Column 3: Get Current Weather
			ImGuiMCP::TableNextColumn();
			ImGuiMCP::PushItemWidth(-FLT_MIN);
			FontAwesome::PushSolid();
			if (ImGuiMCP::Button(IconLibrary::GetWeather.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
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
			ImGuiMCP::TableNextColumn();
			ImGuiMCP::PushItemWidth(-FLT_MIN);
			if (ImGuiMCP::InputFloat("##Strength", &currentRow.rowBlurStrength, 0.01f, 0.1f, "%.2f", inputFlags)) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			Logger::trace("Rendered Strength input for row {}", rowIndex);

			// Column 5: Range
			ImGuiMCP::TableNextColumn();
			ImGuiMCP::PushItemWidth(-FLT_MIN);
			if (ImGuiMCP::InputFloat("##Range", &currentRow.rowBlurRange, 10.0f, 100.0f, "%.2f", inputFlags)) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			Logger::trace("Rendered Range input for row {}", rowIndex);

			// Column 6: Static
			ImGuiMCP::TableNextColumn();
			ImGuiMCP::SetCursorPosX(ImGuiMCP::GetCursorPosX() + (ImGuiMCP::GetColumnWidth() - ImGuiMCP::GetFrameHeight()) * 0.5f);
			if (ImGuiMCP::Checkbox("##Static", &currentRow.rowStaticToggle)) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			Logger::trace("Rendered Static checkbox for row {}", rowIndex);

			// Column 7: Reset
			ImGuiMCP::TableNextColumn();
			FontAwesome::PushSolid();
			auto originalRow = currentRow;
			if (ImGuiMCP::Button(IconLibrary::ResetRow.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
				currentRow = WeatherSettingRow{};
			}
			if (originalRow != currentRow) {
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			FontAwesome::Pop();
			Logger::trace("Rendered Reset button for row {}", rowIndex);

			// Column 8: Remove
			ImGuiMCP::TableNextColumn();
			FontAwesome::PushSolid();
			if (ImGuiMCP::Button(IconLibrary::RemoveRow.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
				g_advancedWeatherData.rowToRemove = rowIndex;
				Hooks::BlurManager::GetSingleton().NotifySettingsChanged();
			}
			FontAwesome::Pop();
			Logger::trace("Rendered Remove button for row {}", rowIndex);

			ImGuiMCP::PopID();
			Logger::trace("Finished drawing row {}", rowIndex);
		}

		void RenderWeatherTable() {

			Logger::trace("Rendering Weather Table with {} rows", g_advancedWeatherData.settings.size());

			const auto& weatherNames = Utils::g_formCache[std::type_index(typeid(RE::TESWeather))];
			const int   columnCount  = std::size(COLUMN_SETUPS);
			Logger::trace("Weather Table Column Count: {}", columnCount);

			if (ImGuiMCP::BeginTable("WeatherTable", columnCount, tableFlags)) {
				for (const auto& setup : COLUMN_SETUPS) {
					if (setup.width > 0.0f) {
						ImGuiMCP::TableSetupColumn(setup.title, columnFlags, setup.width);
						Logger::trace("Setting up column '{}' with stretchy width {}", setup.title, setup.width);
					} else {
						ImGuiMCP::TableSetupColumn(setup.title, lastColumnFlags);
						Logger::trace("Setting up last column '{}' with adapting width", setup.title);
					}
				}

				ImGuiMCP::TableNextRow(ImGuiTableRowFlags_Headers);
				for (int headerColumn = 0; headerColumn < columnCount; headerColumn++) {
					Logger::trace("Drawing header for column index {}", headerColumn);
					ImGuiMCP::TableSetColumnIndex(headerColumn);
					Utils::CenteredImGuiText(COLUMN_SETUPS[headerColumn].title);
					Logger::trace("Header drawn: {}", COLUMN_SETUPS[headerColumn].title);
				}

				for (int row = 0; row < g_advancedWeatherData.settings.size(); row++) {
					DrawTableRow(row, g_advancedWeatherData.settings[row], weatherNames);
					Logger::trace("Drawn row {}", row);
				}
				ImGuiMCP::EndTable();

				g_advancedWeatherData.RemoveRow();
				Logger::trace("Processed row removals if any");
				
				// Add Row button
				FontAwesome::PushRegular();
				if (ImGuiMCP::Button(IconLibrary::AddRow.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
					Logger::trace("Add row button clicked");
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