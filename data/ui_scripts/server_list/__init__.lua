local Lobby = luiglobals.Lobby
local SystemLinkJoinMenu = LUI.mp_menus.SystemLinkJoinMenu

if (not SystemLinkJoinMenu) then
	return
end

local CreateRowDef = SystemLinkJoinMenu.CreateRowDef
local menu_systemlink_join = LUI.MenuBuilder.m_definitions["menu_systemlink_join"]

COLUMN_4 = 885

SystemLinkJoinMenu.CreateRowDef = function(a1, a2, a3, a4)
	local row = CreateRowDef(a1, a2, a3, a4)

	-- Reduce width of last column
	row.children[3].states.default.right = -90
	-- Add ping value column
	local ping = Lobby.GetServerData(a1, a2, SystemLinkJoinMenu.GameDataColumn.Ping)
	row.children[8] = SystemLinkJoinMenu.CreateColumnText("ping_text", ping, true, true, COLUMN_4, 0)

	return row
end

LUI.MenuBuilder.m_definitions["menu_systemlink_join"] = function()
	local menu = menu_systemlink_join()

	local rows = menu.children[2].children[3].children
	local header = rows[1]

	-- Reduce width of last column
	header.children[3].states.default.right = -90

	-- Add ping header column
	luiglobals.table.insert(header.children, SystemLinkJoinMenu.CreateColumnText("ping_header", Engine.Localize("@MENU_PING"), true, true, COLUMN_4, 0))

	-- Increase server list width
	menu.children[2].states.default.left = 150
	menu.children[2].states.default.right = -150

	menu.children[3].properties.interval = 10

	return menu
end