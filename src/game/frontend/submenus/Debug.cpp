#include "Debug.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/rdr/ScriptGlobal.hpp"
#include "core/filemgr/FileMgr.hpp"
#include "game/features/features.hpp"

namespace YimMenu::Submenus
{
    enum GlobalAppendageType : int
	{
		GlobalAppendageType_At,
		GlobalAppendageType_ReadGlobal,
		GlobalAppendageType_PlayerId,
	};

	struct global_debug_inner
	{
		GlobalAppendageType type{};
		std::ptrdiff_t index{};
		std::size_t size{};
		std::string global_name{};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(global_debug_inner, type, index, size, global_name)
	};

	struct global_debug
	{
		std::size_t global_index{};
		std::vector<global_debug_inner> global_appendages{};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(global_debug, global_index, global_appendages)
	};

	nlohmann::json get_globals_json()
	{
		nlohmann::json globals{};

		auto file = FileMgr::GetProjectFile("./globals.json");
		if (file.Exists())
		{
			std::ifstream iffstream_file(file.Path());
			iffstream_file >> globals;
		}

		return globals;
	}

	void load_global_menu(const std::string& selected_global, global_debug& global_obj)
	{
		if (!selected_global.empty())
		{
			auto globals = get_globals_json();
			if (globals[selected_global].is_null())
				return;
			global_obj = globals[selected_global].get<global_debug>();
		}
	}

	int64_t* get_global_ptr(global_debug& global_test)
	{
		auto global_to_read = ScriptGlobal(global_test.global_index);
		for (auto item : global_test.global_appendages)
		{
			if (item.type == GlobalAppendageType_At)
			{
				if (item.size != 0)
					global_to_read = global_to_read.At(item.index, item.size);
				else
					global_to_read = global_to_read.At(item.index);
			}
			else if (item.type == GlobalAppendageType_ReadGlobal)
			{
				global_debug global_read;
				load_global_menu(item.global_name, global_read);
				if (auto ptr = get_global_ptr(global_read))
					if (item.size != 0)
						global_to_read = global_to_read.At(*ptr, item.size);
					else
						global_to_read = global_to_read.At(*ptr);
				else
					LOG(WARNING) << "Failed to read " << item.global_name << "for get_global_ptr";
			}
			else if (item.type == GlobalAppendageType_PlayerId)
			{
				if (item.size != 0)
					global_to_read = global_to_read.At(YimMenu::Self::Id, item.size);
				else
					global_to_read = global_to_read.At(YimMenu::Self::Id);
			}
		}
		auto retn_val = global_to_read.As<int64_t*>();
		if ((size_t)retn_val < UINT32_MAX)
			return nullptr;
		return retn_val;
	}

	std::map<std::string, global_debug> list_globals()
	{
		auto json = get_globals_json();
		std::map<std::string, global_debug> return_value;
		for (auto& item : json.items())
			return_value[item.key()] = item.value();
		return return_value;
	}

	void save_global(char* global_name, global_debug& global_obj)
	{
		std::string teleport_name_string = global_name;
		if (!teleport_name_string.empty())
		{
			auto json                  = get_globals_json();
			json[teleport_name_string] = global_obj;

			auto file_path = FileMgr::GetProjectFile("./globals.json").Path();
			std::ofstream file(file_path, std::ios::out | std::ios::trunc);
			file << json.dump(4);
			file.close();
			ZeroMemory(global_name, sizeof(global_name));
		}
	}

	void delete_global(std::string name)
	{
		auto locations = get_globals_json();
		if (locations[name].is_null())
			return;
		locations.erase(name);
		auto file_path = FileMgr::GetProjectFile("./globals.json").Path();
		std::ofstream file(file_path, std::ios::out | std::ios::trunc);
		file << locations.dump(4);
		file.close();
	}

	void RenderGlobalsEditor()
	{
			static global_debug global_test{};
			static ScriptGlobal glo_bal_sunday = ScriptGlobal(global_test.global_index);
			ImGui::SetNextItemWidth(200.f);
			if (ImGui::InputScalar("Global", ImGuiDataType_U64, &global_test.global_index))
				glo_bal_sunday = ScriptGlobal(global_test.global_index);

			for (int i = 0; i < global_test.global_appendages.size(); i++)
			{
				auto item = global_test.global_appendages[i];
				switch (item.type)
				{
				case GlobalAppendageType_At:
					ImGui::SetNextItemWidth(200.f);
					ImGui::InputScalar(std::format("At##{}{}", i, (int)item.type).c_str(),
					    ImGuiDataType_S64,
					    &global_test.global_appendages[i].index);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(200.f);
					ImGui::InputScalar(std::format("Size##{}{}", i, (int)item.type).c_str(),
					    ImGuiDataType_S64,
					    &global_test.global_appendages[i].size);
					break;
				case GlobalAppendageType_ReadGlobal:
					ImGui::Text(std::format("Read Global {}", item.global_name).c_str());
					ImGui::SameLine();
					ImGui::SetNextItemWidth(200.f);
					ImGui::InputScalar(std::format("Size##{}{}", i, (int)item.type).c_str(),
					    ImGuiDataType_S64,
					    &global_test.global_appendages[i].size);
					break;
				case GlobalAppendageType_PlayerId:
					ImGui::SetNextItemWidth(200.f);
					ImGui::InputScalar(std::format("Read Player ID Size##{}{}", i, (int)item.type).c_str(),
					    ImGuiDataType_S64,
					    &global_test.global_appendages[i].size);
					break;
				}
			}

			if (ImGui::Button("Add Offset"))
				global_test.global_appendages.push_back({GlobalAppendageType_At, 0LL, 0ULL});
			ImGui::SameLine();
			if (ImGui::Button("Add Read Player Id"))
				global_test.global_appendages.push_back({GlobalAppendageType_PlayerId, 0LL, 0ULL});

			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				global_test.global_index = 0;
				global_test.global_appendages.clear();
			}

			if (global_test.global_appendages.size() > 0 && ImGui::Button("Remove Offset"))
				global_test.global_appendages.pop_back();

			if (auto ptr = get_global_ptr(global_test))
			{
				ImGui::SetNextItemWidth(200.f);
				ImGui::InputScalar("Value", ImGuiDataType_S32, ptr);
			}
			else
				ImGui::Text("INVALID_GLOBAL_READ");

			auto globals = list_globals();
			static std::string selected_global;
			ImGui::Text("Saved Globals");
			if (ImGui::BeginListBox("##savedglobals", ImVec2(200, 200)))
			{
				for (auto pair : globals)
				{
					if (ImGui::Selectable(pair.first.c_str(), selected_global == pair.first))
						selected_global = std::string(pair.first);
				}
				ImGui::EndListBox();
			}
			ImGui::SameLine();
			if (ImGui::BeginListBox("##globalvalues", ImVec2(200, 200)))
			{
				for (auto pair : globals)
				{
					if (auto ptr = get_global_ptr(pair.second))
						ImGui::Selectable(std::format("{}", (std::int32_t)*ptr).c_str(), false, ImGuiSelectableFlags_Disabled);
					else
						ImGui::Selectable("INVALID_GLOBAL_READ", false, ImGuiSelectableFlags_Disabled);
				}
				ImGui::EndListBox();
			}

			ImGui::BeginGroup();
			static char global_name[50]{};
			ImGui::SetNextItemWidth(200.f);
			ImGui::InputText("##GlobalName", global_name, IM_ARRAYSIZE(global_name));
			if (ImGui::IsItemActive())
				;//TODO block control
			if (ImGui::Button("Save Global"))
			{
				save_global(global_name, global_test);
			}
			ImGui::SameLine();
			if (ImGui::Button("Load Global"))
			{
				load_global_menu(selected_global, global_test);
			}

			if (ImGui::Button("Delete Global"))
			{
				if (!selected_global.empty())
				{
					delete_global(selected_global);
					selected_global.clear();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Add Read Global"))
			{
				global_test.global_appendages.push_back({GlobalAppendageType_ReadGlobal, 0LL, 0ULL, selected_global});
			}
			ImGui::EndGroup();
			ImGui::EndTabItem();
		
	}

	Debug::Debug() :
	    Submenu::Submenu("Debug")
	{
		auto globals = std::make_shared<Category>("Globals");

        globals->AddItem(std::make_shared<ImGuiItem>([] {
            	RenderGlobalsEditor();
			}));

		AddCategory(std::move(globals));

		auto debug = std::make_shared<Category>("Logging");
		debug->AddItem(std::make_shared<BoolCommandItem>("logclones"_J));
		debug->AddItem(std::make_shared<BoolCommandItem>("logevents"_J));
		debug->AddItem(std::make_shared<BoolCommandItem>("logtses"_J));
		AddCategory(std::move(debug));
	}
}