if (game:issingleplayer()) then
	return
end

if (package.loaded["LUI.mp_hud.OptionsMenu"] == nil) then
	return
end

package.loaded["LUI.mp_hud.OptionsMenu"].options_def = function()
	local f14_local0 = GameX.GetGameMode()
	local f14_local1 = Engine.TableLookup( GameTypesTable.File, GameTypesTable.Cols.Ref, f14_local0, GameTypesTable.Cols.ClassChoice ) == "1"

	if not f14_local1 then
		f14_local1 = GameX.UsesFakeLoadout()
	end

	local f14_local2 = checkTeamChoice( f14_local0 )
	local f14_local3 = GameX.IsRankedMatch()
	local f14_local4 = Engine.GetDvarBool( "splitscreen_ingame" )
	local f14_local5 = Game.GetOmnvar( "ui_team_selected" )
	local f14_local6 = Game.GetOmnvar( "ui_loadout_selected" )
	local f14_local7 = chooseClassCheck( f14_local3, f14_local5, f14_local2 )
	local self = LUI.UIVerticalList.new()
	self.id = "pause_selections_Id"

	self:registerAnimationState("default", {
		topAnchor = true,
		leftAnchor = true,
		bottomAnchor = false,
		rightAnchor = false,
		top = GenericMenuDims.menu_top,
		left = GenericMenuDims.menu_left,
		bottom = GenericMenuDims.menu_bottom,
		right = GenericMenuDims.menu_right,
		alignment = LUI.Alignment.Top
	})

	self:animateToState( "default", 0 )
	self:makeFocusable()

	if f14_local0 ~= "aliens" and false == CoD.IsFireTeamMode() and f14_local1 == true and f14_local7 == true and not MLG.IsMLGSpectator() then
		LUI.MenuBuilder.BuildAddChild(self, {
			type = "UIGenericButton",
			id = "btn_MPPause_0",
			properties = {
				childNum = 1,
				button_text = Engine.Localize( "@LUA_MENU_CHOOSE_CLASS_CAPS" ),
				button_action_func = chooseClassButtonAction
			}
		})
	end

	if f14_local0 ~= "aliens" and false == CoD.IsFireTeamMode() and f14_local2 == true and not MLG.IsMLGSpectator() then
		LUI.MenuBuilder.BuildAddChild(self, {
			type = "UIGenericButton",
			id = "btn_MPPause_1",
			properties = {
				childNum = 2,
				button_text = Engine.Localize( "@LUA_MENU_CHANGE_TEAM_CAPS" ),
				button_action_func = changeTeamButtonAction
			}
		})
	end

	LUI.MenuBuilder.BuildAddChild(self, {
		type = "UIGenericButton",
		id = "btn_MPPause_2",
		disabledFunc = optionsLockedUpdate,
		properties = {
			childNum = 3,
			button_text = Engine.Localize( "@LUA_MENU_OPTIONS_CAPS" ),
			button_action_func = optionsButtonAction
		},
		handlers = {
			refresh_options_button = refreshOptionDisable
		}
	})

	if GameX.IsOnlineMatch() and (not Engine.IsAliensMode() or not Game.GetOmnvar( "ui_alien_is_solo" )) and not MLG.IsMLGSpectator() then
		LUI.MenuBuilder.BuildAddChild(self, {
			type = "UIGenericButton",
			id = "btn_MPPause_3",
			properties = {
				childNum = 4,
				button_text = Engine.Localize( "@LUA_MENU_MUTE_PLAYERS_CAPS" ),
				button_action_func = mutePlayersButtonAction
			}
		})
	end

	LUI.MenuBuilder.BuildAddChild(self, {
		type = "UIGenericButton",
		id = "btn_MPPause_5",
		properties = {
			childNum = 6,
			button_text = Engine.Localize( "@LUA_MENU_END_GAME_CAPS" ),
			button_action_func = endGameButtonAction
		}
	})
	return self
end

function refreshOptionDisable( f13_arg0, f13_arg1 )
	if not f13_arg0.disabled then
		f13_arg0:processEvent({
			name = "disable"
		})
	elseif f13_arg0.disabled then
		f13_arg0:processEvent({
			name = "enable"
		})
	end
end

function endGameButtonAction( f8_arg0, f8_arg1 )
	local f8_local0 = Engine.GetDvarBool( "isMatchMakingGame" )
	local f8_local1 = Engine.GetDvarBool( "sv_running" )
	if f8_local0 then
		LUI.FlowManager.RequestPopupMenu( f8_arg0, "popup_leave_game", true, f8_arg1.controller )
	else
		LUI.FlowManager.RequestPopupMenu( f8_arg0, "popup_end_game", true, f8_arg1.controller )
	end
end

function checkTeamChoice( f9_arg0 )
	if Engine.IsSquadVsSquadMode() then
		return false
	else
		return Engine.TableLookup( GameTypesTable.File, GameTypesTable.Cols.Ref, f9_arg0, GameTypesTable.Cols.TeamChoice ) == "1"
	end
end

function chooseClassCheck( f10_arg0, f10_arg1, f10_arg2 )
	local f10_local0 = Game.GetPlayerTeam()
	if GameX.UsesFakeLoadout() then
		return true
	elseif f10_arg0 == true then
		return true
	elseif f10_arg0 == false then
		if f10_arg2 == true and (f10_local0 == Teams.allies or f10_local0 == Teams.axis) then
			return true
		elseif f10_arg2 == false then
			return true
		end
	end
	return false
end

function chooseClassButtonAction( f2_arg0, f2_arg1 )
	LUI.FlowManager.RequestAddMenu( f2_arg0, "class_select_main", true, f2_arg1.controller )
end

function changeTeamButtonAction( f5_arg0, f5_arg1 )
	LUI.FlowManager.RequestAddMenu( f5_arg0, "team_select_main", true, f5_arg1.controller )
end

function mutePlayersButtonAction( f6_arg0, f6_arg1 )
	LUI.FlowManager.RequestAddMenu( f6_arg0, "popup_mute_players", true, f6_arg1.controller )
end

function optionsButtonAction( f7_arg0, f7_arg1 )
	if Engine.IsConsoleGame() then
		LUI.FlowManager.RequestAddMenu( f7_arg0, "mp_in_game_controls_menu", true, f7_arg1.controller )
		if GameX.IsSplitscreen() then
			GameX.SetOptionState( true )
			f7_arg0:dispatchEventToAllOtherRoots({
				name = "refresh_options_button",
				immediate = true
			})
		end
	else
		LUI.FlowManager.RequestAddMenu( f7_arg0, "pc_controls", true, f7_arg1.controller )
	end
end

function optionsLockedUpdate( f12_arg0, f12_arg1 )
	return GameX.IsOptionStateLocked()
end
