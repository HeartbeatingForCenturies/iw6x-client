#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include <utils/io.hpp>
#include <utils/hook.hpp>

#include "component/filesystem.hpp"
#include "component/console.hpp"
#include "component/scripting.hpp"
#include "component/fastfiles.hpp"

#include <xsk/gsc/types.hpp>
#include <xsk/gsc/interfaces/compiler.hpp>
#include <xsk/gsc/interfaces/decompiler.hpp>
#include <xsk/gsc/interfaces/assembler.hpp>
#include <xsk/gsc/interfaces/disassembler.hpp>
#include <xsk/utils/compression.hpp>
#include <xsk/resolver.hpp>
#include <interface.hpp>

namespace gsc
{
	namespace
	{
		auto compiler = ::gsc::compiler();
		auto decompiler = ::gsc::decompiler();
		auto assembler = ::gsc::assembler();
		auto disassembler = ::gsc::disassembler();

		std::unordered_map<std::string, unsigned int> main_handles;
		std::unordered_map<std::string, unsigned int> init_handles;

		std::unordered_map<std::string, game::ScriptFile*> loaded_scripts;

		void clear()
		{
			main_handles.clear();
			init_handles.clear();
			loaded_scripts.clear();
		}

		bool read_script_file(const std::string& name, std::string* data)
		{
			if (filesystem::read_file(name, data))
			{
				return true;
			}

			const auto* name_str = name.data();
			if (game::DB_XAssetExists(game::ASSET_TYPE_RAWFILE, name_str) &&
				!game::DB_IsXAssetDefault(game::ASSET_TYPE_RAWFILE, name_str))
			{
				const auto asset = game::DB_FindXAssetHeader(game::ASSET_TYPE_RAWFILE, name.data(), false);
				const auto len = game::DB_GetRawFileLen(asset.rawfile);
				data->resize(len);
				game::DB_GetRawBuffer(asset.rawfile, data->data(), len);
				if (len > 0)
				{
					data->pop_back();
				}

				return true;
			}

			return false;
		}

		game::ScriptFile* load_custom_script(const char* file_name, const std::string& real_name)
		{
			if (const auto got = loaded_scripts.find(real_name); got != loaded_scripts.end())
			{
				return got->second;
			}

			std::string source_buffer{};
			if (!read_script_file(real_name + ".gsc", &source_buffer))
			{
				return nullptr;
			}

			std::vector<std::uint8_t> data;
			data.assign(source_buffer.begin(), source_buffer.end());

			try
			{
				compiler->compile(real_name, data);
			}
			catch (const std::exception& ex)
			{
				console::error("*********** script compile error *************\n");
				console::error("failed to compile '%s':\n%s", real_name.data(), ex.what());
				console::error("**********************************************\n");
				return nullptr;
			}

			auto assembly = compiler->output();

			try
			{
				assembler->assemble(real_name, assembly);
			}
			catch (const std::exception& ex)
			{
				console::error("*********** script compile error *************\n");
				console::error("failed to assemble '%s':\n%s", real_name.data(), ex.what());
				console::error("**********************************************\n");
				return nullptr;
			}

			const auto script_file_ptr = static_cast<game::ScriptFile*>(game::PMem_AllocFromSource_NoDebug(sizeof(game::ScriptFile), 4, 1, game::PMEM_SOURCE_SCRIPT));
			script_file_ptr->name = file_name;

			const auto stack = assembler->output_stack();
			script_file_ptr->len = static_cast<int>(stack.size());

			const auto script = assembler->output_script();
			script_file_ptr->bytecodeLen = static_cast<int>(script.size());

			const auto script_size = script.size();
			// Use PMem for both stack and byte code
			const auto buffer_size = script_size + stack.size() + 2;

			const auto buffer = static_cast<std::uint8_t*>(game::PMem_AllocFromSource_NoDebug(static_cast<std::uint32_t>(buffer_size), 4, 1, game::PMEM_SOURCE_SCRIPT));
			std::memcpy(buffer, script.data(), script_size);
			std::memcpy(&buffer[script_size], stack.data(), stack.size());

			script_file_ptr->bytecode = &buffer[0];
			script_file_ptr->buffer = reinterpret_cast<char*>(&buffer[script.size()]);
			script_file_ptr->compressedLen = 0;

			loaded_scripts[real_name] = script_file_ptr;

			return script_file_ptr;
		}

		std::string get_script_file_name(const std::string& name)
		{
			const auto id = xsk::gsc::iw6::resolver::token_id(name);
			if (!id)
			{
				return name;
			}

			return std::to_string(id);
		}

		std::vector<std::uint8_t> decompile_script_file(const std::string& name, const std::string& real_name)
		{
			const auto* script_file = game::DB_FindXAssetHeader(game::ASSET_TYPE_SCRIPTFILE, name.data(), false).scriptfile;
			if (!script_file)
			{
				throw std::runtime_error(std::format("Could not load scriptfile '{}'", real_name));
			}

			console::info("Decompiling scriptfile '%s'\n", real_name.data());

			std::vector<std::uint8_t> stack{script_file->buffer, script_file->buffer + script_file->len};
			std::vector<std::uint8_t> bytecode{script_file->bytecode, script_file->bytecode + script_file->bytecodeLen};

			auto decompressed_stack = xsk::utils::zlib::decompress(stack, static_cast<std::uint32_t>(stack.size()));

			disassembler->disassemble(name, bytecode, decompressed_stack);
			auto output = disassembler->output();

			decompiler->decompile(name, output);

			return decompiler->output();
		}

		void load_script(const std::string& name)
		{
			if (!game::Scr_LoadScript(name.data()))
			{
				return;
			}

			const auto main_handle = game::Scr_GetFunctionHandle(name.data(), xsk::gsc::iw6::resolver::token_id("main"));
			const auto init_handle = game::Scr_GetFunctionHandle(name.data(), xsk::gsc::iw6::resolver::token_id("init"));

			if (main_handle)
			{
				console::info("Loaded '%s::main'\n", name.data());
				main_handles[name] = main_handle;
			}

			if (init_handle)
			{
				console::info("Loaded '%s::init'\n", name.data());
				init_handles[name] = init_handle;
			}
		}

		void load_scripts(const std::filesystem::path& root_dir)
		{
			const std::filesystem::path script_dir = root_dir / "scripts";
			if (!utils::io::directory_exists(script_dir.generic_string()))
			{
				return;
			}

			const auto scripts = utils::io::list_files(script_dir.generic_string());
			for (const auto& script : scripts)
			{
				if (!script.ends_with(".gsc"))
				{
					continue;
				}

				std::filesystem::path path(script);
				const auto relative = path.lexically_relative(root_dir).generic_string();
				const auto base_name = relative.substr(0, relative.size() - 4);

				load_script(base_name);
			}
		}

		game::ScriptFile* find_script(game::XAssetType /*type*/, const char* name, int /*allow_create_default*/)
		{
			std::string real_name = name;
			const auto id = static_cast<std::uint16_t>(std::atoi(name));
			if (id)
			{
				real_name = xsk::gsc::iw6::resolver::token_name(id);
			}

			auto* script = load_custom_script(name, real_name);
			if (script)
			{
				return script;
			}

			return game::DB_FindXAssetHeader(game::ASSET_TYPE_SCRIPTFILE, name, 1).scriptfile;
		}

		int db_is_x_asset_default(game::XAssetType type, const char* name)
		{
			if (loaded_scripts.contains(name))
			{
				return 0;
			}

			return game::DB_IsXAssetDefault(type, name);
		}

		void gscr_load_game_type_script_stub()
		{
			utils::hook::invoke<void>(0x1403CCB10);

			clear();

			fastfiles::enum_assets(game::ASSET_TYPE_RAWFILE, [](const game::XAssetHeader header)
			{
				const std::string name = header.rawfile->name;

				if (name.ends_with(".gsc") && name.starts_with("scripts/"))
				{
					// Remove .gsc from the file name as it will be re-added by the game later
					const auto base_name = name.substr(0, name.size() - 4);
					load_script(base_name);
				}
			}, true);

			for (const auto& path : filesystem::get_search_paths())
			{
				load_scripts(path);
			}
		}

		void db_get_raw_buffer_stub(const game::RawFile* rawfile, char* buf, const int size)
		{
			if (rawfile->len > 0 && rawfile->compressedLen == 0)
			{
				std::memset(buf, 0, size);
				std::memcpy(buf, rawfile->buffer, std::min(rawfile->len, size));
				return;
			}

			game::DB_GetRawBuffer(rawfile, buf, size);
		}

		void g_load_structs_stub()
		{
			for (auto& function_handle : main_handles)
			{
				console::info("Executing '%s::main'\n", function_handle.first.data());
				const auto thread = game::Scr_ExecThread(static_cast<int>(function_handle.second), 0);
				game::RemoveRefToObject(thread);
			}

			utils::hook::invoke<void>(0x1403D2CA0);
		}

		void scr_load_level_stub()
		{
			utils::hook::invoke<void>(0x1403CDC60);

			for (auto& function_handle : init_handles)
			{
				console::info("Executing '%s::init'\n", function_handle.first.data());
				const auto thread = game::Scr_ExecThread(static_cast<int>(function_handle.second), 0);
				game::RemoveRefToObject(thread);
			}
		}
	}

	class loading final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp())
			{
				return;
			}

			// Allow custom scripts to include other custom scripts
			xsk::gsc::iw6::resolver::init([](const auto& include_name) -> std::vector<std::uint8_t>
			{
				const auto real_name = include_name + ".gsc";

				std::string file_buffer;
				if (!read_script_file(real_name, &file_buffer) || file_buffer.empty())
				{
					const auto name = get_script_file_name(include_name);
					if (game::DB_XAssetExists(game::ASSET_TYPE_SCRIPTFILE, name.data()))
					{
						return decompile_script_file(name, real_name);
					}
					else
					{
						throw std::runtime_error(std::format("Could not load gsc file '{}'", real_name));
					}
				}

				std::vector<std::uint8_t> result;
				result.assign(file_buffer.begin(), file_buffer.end());

				return result;
			});

			// ProcessScript
			utils::hook::call(0x1404378D7, find_script);
			utils::hook::call(0x1404378E7, db_is_x_asset_default);

			// GScr_LoadScripts
			utils::hook::call(0x1403CD009, gscr_load_game_type_script_stub);

			// Load our scripts with an uncompressed stack
			utils::hook::call(0x140437940, db_get_raw_buffer_stub);

			// Exec script handles
			utils::hook::call(0x14039F64E, g_load_structs_stub);
			utils::hook::call(0x14039F653, scr_load_level_stub);

			scripting::on_shutdown([](int free_scripts)
			{
				if (free_scripts)
				{
					xsk::gsc::iw6::resolver::cleanup();
					clear();
				}
			});
		}
	};
}

REGISTER_COMPONENT(gsc::loading)
