#include <std_include.hpp>
#include "context.hpp"
#include "error.hpp"
#include "value_conversion.hpp"

#include "game/scripting/execution.hpp"

#include "component/command.hpp"
#include "component/notifies.hpp"
#include "component/scripting.hpp"

#include <xsk/gsc/types.hpp>
#include <xsk/resolver.hpp>

#include <utils/string.hpp>

namespace scripting::lua
{
	namespace
	{
		vector normalize_vector(const vector& vec)
		{
			const auto length = std::sqrt((vec.get_x() * vec.get_x()) + (vec.get_y() * vec.get_y()) + (vec.get_z() * vec.get_z()));

			return vector(
				vec.get_x() / length,
				vec.get_y() / length,
				vec.get_z() / length
			);
		}

		std::vector<std::string> load_game_constants()
		{
			std::vector<std::string> result{};

			const auto constants = game::GScr_LoadConsts.get();

			ud_t ud;
			ud_init(&ud);
			ud_set_mode(&ud, 64);
			ud_set_pc(&ud, uint64_t(constants));
			ud_set_input_buffer(&ud, reinterpret_cast<const uint8_t*>(constants), INT32_MAX);

			while (true)
			{
				ud_disassemble(&ud);

				if (ud_insn_mnemonic(&ud) == UD_Iret)
				{
					break;
				}

				if (ud_insn_mnemonic(&ud) == UD_Ilea)
				{
					const auto* operand = ud_insn_opr(&ud, 0);
					if (!operand || operand->type != UD_OP_REG || operand->base != UD_R_RCX)
					{
						continue;
					}

					operand = ud_insn_opr(&ud, 1);
					if (operand && operand->type == UD_OP_MEM && operand->base == UD_R_RIP)
					{
						auto* operand_ptr = reinterpret_cast<char*>(ud_insn_len(&ud) + ud_insn_off(&ud) + operand->lval.
							sdword);
						if (!utils::memory::is_bad_read_ptr(operand_ptr) && utils::memory::is_rdata_ptr(operand_ptr) &&
							strlen(operand_ptr) > 0)
						{
							result.emplace_back(operand_ptr);
						}
					}
				}

				if (*reinterpret_cast<unsigned char*>(ud.pc) == 0xCC) break; // int 3
			}

			return result;
		}

		const std::vector<std::string>& get_game_constants()
		{
			static auto constants = load_game_constants();
			return constants;
		}

		void setup_vector_type(sol::state& state)
		{
			state["level"] = entity{ *::game::levelEntityId };
			state["player"] = call("getentbynum", {0}).as<entity>();

			auto vector_type = state.new_usertype<vector>("vector", sol::constructors<vector(float, float, float)>());
			vector_type["x"] = sol::property(&vector::get_x, &vector::set_x);
			vector_type["y"] = sol::property(&vector::get_y, &vector::set_y);
			vector_type["z"] = sol::property(&vector::get_z, &vector::set_z);

			vector_type["r"] = sol::property(&vector::get_x, &vector::set_x);
			vector_type["g"] = sol::property(&vector::get_y, &vector::set_y);
			vector_type["b"] = sol::property(&vector::get_z, &vector::set_z);

			vector_type[sol::meta_function::addition] = sol::overload([](const vector& a, const vector& b)
			{
				return vector(
					a.get_x() + b.get_x(),
					a.get_y() + b.get_y(),
					a.get_z() + b.get_z()
				);
			},
			[](const vector& a, const int value)
			{
				return vector(
					a.get_x() + value,
					a.get_y() + value,
					a.get_z() + value
				);
			});

			vector_type[sol::meta_function::subtraction] = sol::overload([](const vector& a, const vector& b)
			{
				return vector(
					a.get_x() - b.get_x(),
					a.get_y() - b.get_y(),
					a.get_z() - b.get_z()
				);
			},
			[](const vector& a, const int value)
			{
				return vector(
					a.get_x() - value,
					a.get_y() - value,
					a.get_z() - value
				);
			});

			vector_type[sol::meta_function::multiplication] = sol::overload([](const vector& a, const vector& b)
			{
				return vector(
					a.get_x() * b.get_x(),
					a.get_y() * b.get_y(),
					a.get_z() * b.get_z()
				);
			},
			[](const vector& a, const int value)
			{
				return vector(
					a.get_x() * value,
					a.get_y() * value,
					a.get_z() * value
				);
			});

			vector_type[sol::meta_function::division] = sol::overload([](const vector& a, const vector& b)
			{
				return vector(
					a.get_x() / b.get_x(),
					a.get_y() / b.get_y(),
					a.get_z() / b.get_z()
				);
			},
			[](const vector& a, const int value)
			{
				return vector(
					a.get_x() / value,
					a.get_y() / value,
					a.get_z() / value
				);
			});

			vector_type[sol::meta_function::equal_to] = [](const vector& a, const vector& b)
			{
				return a.get_x() == b.get_x() &&
					a.get_y() == b.get_y() &&
					a.get_z() == b.get_z();
			};

			vector_type[sol::meta_function::length] = [](const vector& a)
			{
				return sqrt((a.get_x() * a.get_x()) + (a.get_y() * a.get_y()) + (a.get_z() * a.get_z()));
			};

			vector_type[sol::meta_function::to_string] = [](const vector& a)
			{
				return utils::string::va("(%g, %g, %g)", a.get_x(), a.get_y(), a.get_z());
			};

			vector_type["normalize"] = [](const vector& a)
			{
				return normalize_vector(a);
			};

			vector_type["toangles"] = [](const vector& a)
			{
				return call("vectortoangles", {a}).as<vector>();
			};

			vector_type["toyaw"] = [](const vector& a)
			{
				return call("vectortoyaw", {a}).as<vector>();
			};

			vector_type["tolerp"] = [](const vector& a)
			{
				return call("vectortolerp", {a}).as<vector>();
			};

			vector_type["toup"] = [](const vector& a)
			{
				return call("anglestoup", {a}).as<vector>();
			};

			vector_type["toright"] = [](const vector& a)
			{
				return call("anglestoright", {a}).as<vector>();
			};

			vector_type["toforward"] = [](const vector& a)
			{
				return call("anglestoforward", {a}).as<vector>();
			};
		}

		void setup_entity_type(sol::state& state, event_handler& handler, scheduler& scheduler)
		{
			state["level"] = entity{*game::levelEntityId};

			auto entity_type = state.new_usertype<entity>("entity");

			for (const auto& func : xsk::gsc::iw6::resolver::get_methods())
			{
				const auto name = std::string(func.first);
				entity_type[name] = [name](const entity& entity, const sol::this_state s, sol::variadic_args va)
				{
					std::vector<script_value> arguments{};

					for (auto arg : va)
					{
						arguments.push_back(convert({s, arg}));
					}

					return convert(s, entity.call(name, arguments));
				};
			}

			for (const auto& constant : get_game_constants())
			{
				entity_type[constant] = sol::property(
					[constant](const entity& entity, const sol::this_state s)
					{
						return convert(s, entity.get(constant));
					},
					[constant](const entity& entity, const sol::this_state s, const sol::lua_value& value)
					{
						entity.set(constant, convert({s, value}));
					});
			}

			entity_type["set"] = [](const entity& entity, const std::string& field,
			                        const sol::lua_value& value)
			{
				entity.set(field, convert(value));
			};

			entity_type["get"] = [](const entity& entity, const sol::this_state s, const std::string& field)
			{
				return convert(s, entity.get(field));
			};

			entity_type["notify"] = [](const entity& entity, const sol::this_state s, const std::string& event, 
									   sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				notify(entity, event, arguments);
			};

			entity_type["onnotify"] = [&handler](const entity& entity, const std::string& event,
			                                     const event_callback& callback)
			{
				event_listener listener{};
				listener.callback = callback;
				listener.entity = entity;
				listener.event = event;
				listener.is_volatile = false;

				return handler.add_event_listener(std::move(listener));
			};

			entity_type["onnotifyonce"] = [&handler](const entity& entity, const std::string& event,
			                                         const event_callback& callback)
			{
				event_listener listener{};
				listener.callback = callback;
				listener.entity = entity;
				listener.event = event;
				listener.is_volatile = true;

				return handler.add_event_listener(std::move(listener));
			};

			entity_type["call"] = [](const entity& entity, const sol::this_state s, const std::string& function,
			                         sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				return convert(s, entity.call(function, arguments));
			};

			entity_type[sol::meta_function::new_index] = [](const entity& entity, const std::string& field,
															const sol::lua_value& value)
			{
				entity.set(field, convert(value));
			};

			entity_type[sol::meta_function::index] = [](const entity& entity, const sol::this_state s, const std::string& field)
			{
				return convert(s, entity.get(field));
			};

			entity_type["struct"] = sol::property([](const entity& entity, const sol::this_state s)
			{
				const auto id = entity.get_entity_id();
				return entity_to_struct(s, id);
			});

			entity_type["getstruct"] = [](const entity& entity, const sol::this_state s)
			{
				const auto id = entity.get_entity_id();
				return entity_to_struct(s, id);
			};

			entity_type["scriptcall"] = [](const entity& entity, const sol::this_state s, const std::string& filename,
				const std::string function, sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				notifies::hook_enabled = true;
				const auto result = convert(s, call_script_function(entity, filename, function, arguments));
				notifies::hook_enabled = true;
				return result;
			};

			entity_type["getentref"] = [](const entity& entity)
			{
				const auto entref = entity.get_entity_reference();
				std::vector<unsigned int> returns = {entref.entnum, entref.classnum};
				return sol::as_returns(returns);
			};
		}
	}

	void setup_game_type(sol::state& state, event_handler& handler, scheduler& scheduler)
	{
		struct game
		{
		};
		auto game_type = state.new_usertype<game>("game_");
		state["game"] = game();

		for (const auto& func : xsk::gsc::iw6::resolver::get_functions())
		{
			const auto name = std::string(func.first);
			game_type[name] = [name](const game&, const sol::this_state s, sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				return convert(s, call(name, arguments));
			};
		}

		game_type["call"] = [](const game&, const sol::this_state s, const std::string& function, sol::variadic_args va)
		{
			std::vector<script_value> arguments{};

			for (auto arg : va)
			{
				arguments.push_back(convert({s, arg}));
			}

			return convert(s, call(function, arguments));
		};

		game_type["ontimeout"] = [&scheduler](const game&, const sol::protected_function& callback,
			const long long milliseconds)
		{
			return scheduler.add(callback, milliseconds, true);
		};

		game_type["oninterval"] = [&scheduler](const game&, const sol::protected_function& callback,
			const long long milliseconds)
		{
			return scheduler.add(callback, milliseconds, false);
		};

		game_type["executecommand"] = [](const game&, const std::string& command)
		{
			command::execute(command, false);
		};

		game_type["onplayerdamage"] = [](const game&, const sol::protected_function& callback)
		{
			notifies::add_player_damage_callback(callback);
		};

		game_type["onplayerkilled"] = [](const game&, const sol::protected_function& callback)
		{
			notifies::add_player_killed_callback(callback);
		};

		game_type["getgamevar"] = [](const sol::this_state s)
		{
			const auto id = *reinterpret_cast<unsigned int*>(0x144A43020);
			const auto value = ::game::scr_VarGlob->childVariableValue[id];

			::game::VariableValue variable{};
			variable.type = value.type;
			variable.u.uintValue = value.u.u.uintValue;

			return convert(s, variable);
		};

		game_type["getfunctions"] = [](const game&, const sol::this_state s, const std::string& filename)
		{
			if (!script_function_table.contains(filename))
			{
				throw std::runtime_error("File '" + filename + "' not found");
			}

			auto functions = sol::table::create(s.lua_state());

			for (const auto& function : scripting::script_function_table[filename])
			{
				functions[function.first] = sol::overload([filename, function](const entity& entity, const sol::this_state s, sol::variadic_args va)
				{
					std::vector<script_value> arguments{};

					for (auto arg : va)
					{
						arguments.push_back(convert({s, arg}));
					}

					const auto _0 = gsl::finally(&notifies::enable_vm_execute_hook);
					notifies::disable_vm_execute_hook();

					return convert(s, call_script_function(entity, filename, function.first, arguments));
				},
				[filename, function](const sol::this_state s, sol::variadic_args va)
				{
					std::vector<script_value> arguments{};

					for (auto arg : va)
					{
						arguments.push_back(convert({s, arg}));
					}

					const auto _0 = gsl::finally(&notifies::enable_vm_execute_hook);
					notifies::disable_vm_execute_hook();

					return convert(s, call_script_function(*::game::levelEntityId, filename, function.first, arguments));
				});
			}

			return functions;
		};

		game_type["scriptcall"] = [](const game&, const sol::this_state s, const std::string& filename, const std::string function, sol::variadic_args va)
		{
			std::vector<script_value> arguments{};

			for (auto arg : va)
			{
				arguments.push_back(convert({s, arg}));
			}

			const auto _0 = gsl::finally(&notifies::enable_vm_execute_hook);
			notifies::disable_vm_execute_hook();

			return convert(s, call_script_function(*::game::levelEntityId, filename, function, arguments));
		};

		game_type["detour"] = [](const game&, const sol::this_state s, const std::string& filename, const std::string function_name, const sol::protected_function& function)
		{
			const auto pos = get_function_pos(filename, function_name);
			notifies::set_lua_hook(pos, function);

			auto detour = sol::table::create(function.lua_state());

			detour["disable"] = [&]
			{
				notifies::clear_hook(pos);
			};

			detour["enable"] = [&]
			{
				notifies::set_lua_hook(pos, function);
			};

			detour["invoke"] = sol::overload([filename, function_name](const entity& entity, const sol::this_state s, sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				const auto _0 = gsl::finally(&notifies::enable_vm_execute_hook);
				notifies::disable_vm_execute_hook();

				return convert(s, call_script_function(entity, filename, function_name, arguments));
			},
			[filename, function_name](const sol::this_state s, sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				const auto _0 = gsl::finally(&notifies::enable_vm_execute_hook);
				notifies::disable_vm_execute_hook();

				return convert(s, call_script_function(*::game::levelEntityId, filename, function_name, arguments));
			});

			return detour;
		};
	}

	context::context(std::string folder)
		: folder_(std::move(folder))
		  , scheduler_(state_)
		  , event_handler_(state_)

	{
		this->state_.open_libraries(sol::lib::base,
		                            sol::lib::package,
		                            sol::lib::io,
		                            sol::lib::string,
		                            sol::lib::os,
		                            sol::lib::math,
		                            sol::lib::table);

		this->state_["include"] = [this](const std::string& file)
		{
			this->load_script(file);
		};

		sol::function old_require = this->state_["require"];
		auto base_path = utils::string::replace(this->folder_, "/", ".") + ".";
		this->state_["require"] = [base_path, old_require](const std::string& path)
		{
			return old_require(base_path + path);
		};

		this->state_["scriptdir"] = [this]()
		{
			return this->folder_;
		};

		setup_vector_type(this->state_);
		setup_entity_type(this->state_, this->event_handler_, this->scheduler_);
		setup_game_type(this->state_, this->event_handler_, this->scheduler_);

		printf("Loading script '%s'\n", this->folder_.data());
		this->load_script("__init__");
	}

	context::~context()
	{
		this->collect_garbage();
		this->scheduler_.clear();
		this->event_handler_.clear();
		this->state_ = {};
	}

	void context::run_frame()
	{
		this->scheduler_.run_frame();
		this->collect_garbage();
	}

	void context::notify(const event& e)
	{
		this->scheduler_.dispatch(e);
		this->event_handler_.dispatch(e);
	}

	void context::handle_endon_conditions(const event& e)
	{
		this->scheduler_.dispatch(e);
		this->event_handler_.handle_endon_conditions(e);
	}

	void context::collect_garbage()
	{
		this->state_.collect_garbage();
	}

	void context::load_script(const std::string& script)
	{
		if (!this->loaded_scripts_.emplace(script).second)
		{
			return;
		}

		const auto file = (std::filesystem::path(this->folder_) / (script + ".lua")).generic_string();
		handle_error(this->state_.safe_script_file(file, &sol::script_pass_on_error));
	}
}
