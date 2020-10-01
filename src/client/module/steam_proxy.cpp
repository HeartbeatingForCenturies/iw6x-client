#include <std_include.hpp>
#include "steam_proxy.hpp"
#include "scheduler.hpp"
#include "utils/string.hpp"
#include "utils/binary_resource.hpp"
#include "loader/loader.hpp"
#include "game/game.hpp"

namespace
{
	utils::binary_resource runner_file(RUNNER, "runner.exe");
}

void steam_proxy::post_load()
{
	if (game::environment::is_dedi())
	{
		return;
	}

	this->load_client();
	this->clean_up_on_error();

	try
	{
		this->start_mod("\xF0\x9F\x90\x8D" " IW6x: "s + (game::environment::is_sp() ? "Singleplayer" : "Multiplayer"),
		                game::environment::is_sp() ? 209160 : 209170);
	}
	catch (std::exception& e)
	{
		printf("Steam: %s\n", e.what());
	}
}

void steam_proxy::pre_destroy()
{
	if (this->steam_client_module_)
	{
		if (this->steam_pipe_)
		{
			if (this->global_user_)
			{
				this->steam_client_module_.invoke<void>("Steam_ReleaseUser", this->steam_pipe_, this->global_user_);
			}

			this->steam_client_module_.invoke<bool>("Steam_BReleaseSteamPipe", this->steam_pipe_);
		}
	}
}

void* steam_proxy::load_client_engine() const
{
	if (!this->steam_client_module_) return nullptr;

	for (auto i = 1; i > 0; ++i)
	{
		std::string name = utils::string::va("CLIENTENGINE_INTERFACE_VERSION%03i", i);
		auto* const client_engine = this->steam_client_module_
		                                .invoke<void*>("CreateInterface", name.data(), nullptr);
		if (client_engine) return client_engine;
	}

	return nullptr;
}

void steam_proxy::load_client()
{
	const auto steam_path = get_steam_install_directory();
	if (steam_path.empty()) return;

	utils::nt::module::load(steam_path / "tier0_s64.dll");
	utils::nt::module::load(steam_path / "vstdlib_s64.dll");
	this->steam_overlay_module_ = utils::nt::module::load(steam_path / "gameoverlayrenderer64.dll");
	this->steam_client_module_ = utils::nt::module::load(steam_path / "steamclient64.dll");
	if (!this->steam_client_module_) return;

	this->client_engine_ = load_client_engine();
	if (!this->client_engine_) return;

	this->steam_pipe_ = this->steam_client_module_.invoke<void*>("Steam_CreateSteamPipe");
	this->global_user_ = this->steam_client_module_.invoke<void*>("Steam_ConnectToGlobalUser", this->steam_pipe_);
	this->client_user_ = this->client_engine_.invoke<void*>(8, this->steam_pipe_, this->global_user_); // GetIClientUser
	this->client_utils_ = this->client_engine_.invoke<void*>(13, this->steam_pipe_); // GetIClientUtils
}

void steam_proxy::start_mod(const std::string& title, size_t app_id)
{
	if (!this->client_utils_ || !this->client_user_) return;

	if (!this->client_user_.invoke<bool>("BIsSubscribedApp", app_id))
	{
		app_id = 480; // Spacewar
	}

	this->client_utils_.invoke<void>("SetAppIDForCurrentPipe", app_id, false);

	char our_directory[MAX_PATH] = {0};
	GetCurrentDirectoryA(sizeof(our_directory), our_directory);

	const auto path = runner_file.get_extracted_file();
	const std::string cmdline = utils::string::va("\"%s\" -proc %d", path.data(), GetCurrentProcessId());

	steam::game_id game_id;
	game_id.raw.type = 1; // k_EGameIDTypeGameMod
	game_id.raw.app_id = app_id & 0xFFFFFF;

	const auto* mod_id = "IW6x";
	game_id.raw.mod_id = *reinterpret_cast<const unsigned int*>(mod_id) | 0x80000000;

	this->client_user_.invoke<bool>("SpawnProcess", path.data(), cmdline.data(), our_directory,
	                                &game_id.bits, title.data(), 0, 0, 0);
}

void steam_proxy::clean_up_on_error()
{
	scheduler::schedule([this]()
	{
		if (this->steam_client_module_
			&& this->steam_pipe_
			&& this->global_user_
			&& this->steam_client_module_.invoke<bool>("Steam_BConnected", this->global_user_, this->steam_pipe_)
			&& this->steam_client_module_.invoke<bool>("Steam_BLoggedOn", this->global_user_, this->steam_pipe_))
		{
			return scheduler::cond_continue;
		}

		this->client_engine_ = nullptr;
		this->client_user_ = nullptr;
		this->client_utils_ = nullptr;

		this->steam_pipe_ = nullptr;
		this->global_user_ = nullptr;

		this->steam_client_module_ = utils::nt::module{nullptr};

		return scheduler::cond_end;
	});
}

std::filesystem::path steam_proxy::get_steam_install_directory()
{
	HKEY reg_key;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\WOW6432Node\\Valve\\Steam", 0, KEY_QUERY_VALUE, &reg_key) !=
		ERROR_SUCCESS)
	{
		return {};
	}

	char path[MAX_PATH] = {0};
	DWORD length = sizeof(path);
	RegQueryValueExA(reg_key, "InstallPath", nullptr, nullptr, reinterpret_cast<BYTE*>(path),
	                 &length);
	RegCloseKey(reg_key);

	return path;
}

#ifndef DEV_BUILD
REGISTER_MODULE(steam_proxy)
#endif
