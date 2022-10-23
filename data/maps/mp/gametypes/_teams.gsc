// IW6 GSC SOURCE
// Decompiled by https://github.com/xensik/gsc-tool

init()
{
    initscoreboard();
    level.teambalance = getdvarint( "scr_teambalance" );
    level.maxclients = getdvarint( "sv_maxclients" );
    setplayermodels();
    level.freeplayers = [];

    if ( level.teambased )
    {
        level thread onplayerconnect();
        level thread updateteambalance();
        wait 0.15;

        if ( !maps\mp\_utility::is_aliens() )
            level thread updateplayertimes();
    }
    else
    {
        level thread onfreeplayerconnect();
        wait 0.15;
        level thread updatefreeplayertimes();
    }
}

initscoreboard()
{
    setdvar( "g_TeamName_Allies", getteamshortname( "allies" ) );
    setdvar( "g_TeamIcon_Allies", getteamicon( "allies" ) );
    setdvar( "g_TeamIcon_MyAllies", getteamicon( "allies" ) );
    setdvar( "g_TeamIcon_EnemyAllies", getteamicon( "allies" ) );
    var_0 = getteamcolor( "allies" );
    setdvar( "g_ScoresColor_Allies", var_0[0] + " " + var_0[1] + " " + var_0[2] );
    setdvar( "g_TeamName_Axis", getteamshortname( "axis" ) );
    setdvar( "g_TeamIcon_Axis", getteamicon( "axis" ) );
    setdvar( "g_TeamIcon_MyAxis", getteamicon( "axis" ) );
    setdvar( "g_TeamIcon_EnemyAxis", getteamicon( "axis" ) );
    var_0 = getteamcolor( "axis" );
    setdvar( "g_ScoresColor_Axis", var_0[0] + " " + var_0[1] + " " + var_0[2] );
    setdvar( "g_ScoresColor_Spectator", ".25 .25 .25" );
    setdvar( "g_ScoresColor_Free", ".76 .78 .10" );
    setdvar( "g_teamTitleColor_MyTeam", ".6 .8 .6" );
    setdvar( "g_teamTitleColor_EnemyTeam", "1 .45 .5" );
}

onplayerconnect()
{
    for (;;)
    {
        level waittill( "connected", var_0 );
        var_0 thread onjoinedteam();
        var_0 thread onjoinedspectators();
        var_0 thread onplayerspawned();
        var_0 thread trackplayedtime();
    }
}

onfreeplayerconnect()
{
    for (;;)
    {
        level waittill( "connected", var_0 );
        var_0 thread trackfreeplayedtime();
    }
}

onjoinedteam()
{
    self endon( "disconnect" );

    for (;;)
    {
        self waittill( "joined_team" );
        updateteamtime();
    }
}

onjoinedspectators()
{
    self endon( "disconnect" );

    for (;;)
    {
        self waittill( "joined_spectators" );
        self.pers["teamTime"] = undefined;
    }
}

trackplayedtime()
{
    self endon( "disconnect" );
    self.timeplayed["allies"] = 0;
    self.timeplayed["axis"] = 0;
    self.timeplayed["free"] = 0;
    self.timeplayed["other"] = 0;
    self.timeplayed["total"] = 0;
    maps\mp\_utility::gameflagwait( "prematch_done" );

    for (;;)
    {
        if ( game["state"] == "playing" )
        {
            if ( self.sessionteam == "allies" )
            {
                self.timeplayed["allies"]++;
                self.timeplayed["total"]++;
            }
            else if ( self.sessionteam == "axis" )
            {
                self.timeplayed["axis"]++;
                self.timeplayed["total"]++;
            }
            else if ( self.sessionteam == "spectator" )
                self.timeplayed["other"]++;
        }

        wait 1.0;
    }
}

updateplayertimes()
{
    if ( !level.rankedmatch )
        return;

    level endon( "game_ended" );

    for (;;)
    {
        maps\mp\gametypes\_hostmigration::waittillhostmigrationdone();

        foreach ( var_1 in level.players )
            var_1 updateplayedtime();

        wait 1.0;
    }
}

updateplayedtime()
{
    if ( isai( self ) )
        return;

    if ( !maps\mp\_utility::rankingenabled() )
        return;

    if ( self.timeplayed["allies"] )
    {
        if ( !issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedAllies", self.timeplayed["allies"] );
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedTotal", self.timeplayed["allies"] );
        }

        maps\mp\gametypes\_persistence::stataddchildbuffered( "round", "timePlayed", self.timeplayed["allies"] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 0, self.timeplayed["allies"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 1, self.timeplayed["allies"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][1] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 2, self.timeplayed["allies"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][2] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "challengeXPMultiplierTimePlayed", 0, self.timeplayed["allies"], self.bufferedchildstatsmax["challengeXPMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "weaponXPMultiplierTimePlayed", 0, self.timeplayed["allies"], self.bufferedchildstatsmax["weaponXPMaxMultiplierTimePlayed"][0] );

        if ( issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleXpTimePlayed", self.timeplayed["allies"], self.bufferedstatsmax["prestigeDoubleXpMaxTimePlayed"] );
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleWeaponXpTimePlayed", self.timeplayed["allies"], self.bufferedstatsmax["prestigeDoubleWeaponXpMaxTimePlayed"] );
        }
    }

    if ( self.timeplayed["axis"] )
    {
        if ( !issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedOpfor", self.timeplayed["axis"] );
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedTotal", self.timeplayed["axis"] );
        }

        maps\mp\gametypes\_persistence::stataddchildbuffered( "round", "timePlayed", self.timeplayed["axis"] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 0, self.timeplayed["axis"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 1, self.timeplayed["axis"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][1] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 2, self.timeplayed["axis"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][2] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "challengeXPMultiplierTimePlayed", 0, self.timeplayed["axis"], self.bufferedchildstatsmax["challengeXPMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "weaponXPMultiplierTimePlayed", 0, self.timeplayed["axis"], self.bufferedchildstatsmax["weaponXPMaxMultiplierTimePlayed"][0] );

        if ( issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleXpTimePlayed", self.timeplayed["axis"], self.bufferedstatsmax["prestigeDoubleXpMaxTimePlayed"] );
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleWeaponXpTimePlayed", self.timeplayed["axis"], self.bufferedstatsmax["prestigeDoubleWeaponXpMaxTimePlayed"] );
        }
    }

    if ( self.timeplayed["other"] )
    {
        if ( !issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedOther", self.timeplayed["other"] );
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedTotal", self.timeplayed["other"] );
        }

        maps\mp\gametypes\_persistence::stataddchildbuffered( "round", "timePlayed", self.timeplayed["other"] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 0, self.timeplayed["other"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 1, self.timeplayed["other"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][1] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 2, self.timeplayed["other"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][2] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "challengeXPMultiplierTimePlayed", 0, self.timeplayed["other"], self.bufferedchildstatsmax["challengeXPMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "weaponXPMultiplierTimePlayed", 0, self.timeplayed["other"], self.bufferedchildstatsmax["weaponXPMaxMultiplierTimePlayed"][0] );

        if ( issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleXpTimePlayed", self.timeplayed["other"], self.bufferedstatsmax["prestigeDoubleXpMaxTimePlayed"] );
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleWeaponXpTimePlayed", self.timeplayed["other"], self.bufferedstatsmax["prestigeDoubleWeaponXpMaxTimePlayed"] );
        }
    }

    if ( self.pers["rank"] != level.maxrank )
    {
        if ( self.timeplayed["allies"] )
            maps\mp\gametypes\_persistence::stataddsquadbuffered( "experienceToPrestige", self.timeplayed["allies"] );
        else if ( self.timeplayed["axis"] )
            maps\mp\gametypes\_persistence::stataddsquadbuffered( "experienceToPrestige", self.timeplayed["axis"] );
        else if ( self.timeplayed["other"] )
            maps\mp\gametypes\_persistence::stataddsquadbuffered( "experienceToPrestige", self.timeplayed["other"] );
    }

    if ( game["state"] == "postgame" )
        return;

    self.timeplayed["allies"] = 0;
    self.timeplayed["axis"] = 0;
    self.timeplayed["other"] = 0;
}

updateteamtime()
{
    if ( game["state"] != "playing" )
        return;

    self.pers["teamTime"] = gettime();
}

updateteambalancedvar()
{
    for (;;)
    {
        var_0 = getdvarint( "scr_teambalance" );

        if ( level.teambalance != var_0 )
            level.teambalance = getdvarint( "scr_teambalance" );

        wait 1;
    }
}

updateteambalance()
{
    level.teamlimit = level.maxclients / 2;
    level thread updateteambalancedvar();
    wait 0.15;

    if ( level.teambalance && maps\mp\_utility::isroundbased() )
    {
        if ( isdefined( game["BalanceTeamsNextRound"] ) )
            iprintlnbold( &"MP_AUTOBALANCE_NEXT_ROUND" );

        level waittill( "restarting" );

        if ( isdefined( game["BalanceTeamsNextRound"] ) )
        {
            level balanceteams();
            game["BalanceTeamsNextRound"] = undefined;
        }
        else if ( !getteambalance() )
            game["BalanceTeamsNextRound"] = 1;
    }
    else
    {
        level endon( "game_ended" );

        for (;;)
        {
            if ( level.teambalance )
            {
                if ( !getteambalance() )
                {
                    iprintlnbold( &"MP_AUTOBALANCE_SECONDS", 15 );
                    wait 15.0;

                    if ( !getteambalance() )
                        level balanceteams();
                }

                wait 59.0;
            }

            wait 1.0;
        }
    }
}

getteambalance()
{
    level.team["allies"] = 0;
    level.team["axis"] = 0;
    var_0 = level.players;

    for ( var_1 = 0; var_1 < var_0.size; var_1++ )
    {
        if ( isdefined( var_0[var_1].pers["team"] ) && var_0[var_1].pers["team"] == "allies" )
        {
            level.team["allies"]++;
            continue;
        }

        if ( isdefined( var_0[var_1].pers["team"] ) && var_0[var_1].pers["team"] == "axis" )
            level.team["axis"]++;
    }

    if ( level.team["allies"] > level.team["axis"] + level.teambalance || level.team["axis"] > level.team["allies"] + level.teambalance )
        return 0;
    else
        return 1;
}

balanceteams()
{
    iprintlnbold( game["strings"]["autobalance"] );
    //Create/Clear the team arrays
    AlliedPlayers = [];
    AxisPlayers = [];

    // Populate the team arrays
    players = level.players;
    for (i = 0; i < players.size; i++)
    {
        if ( !isdefined( players[i].pers["teamTime"] ) )
            continue;

        if ( ( isdefined( players[i].pers["team"] ) ) && ( players[i].pers["team"] == "allies" ) )
            AlliedPlayers[AlliedPlayers.size] = players[i];

        else if ( ( isdefined( players[i].pers["team"] ) ) && ( players[i].pers["team"] == "axis" ) )
            AxisPlayers[AxisPlayers.size] = players[i];
    }

    MostRecent = undefined;

    while ( ( AlliedPlayers.size > ( AxisPlayers.size + 1 ) ) || ( AxisPlayers.size > ( AlliedPlayers.size + 1 ) ) )
    {
        if ( AlliedPlayers.size > ( AxisPlayers.size + 1 ) )
        {
            // Move the player that's been on the team the shortest ammount of time (highest teamTime value)
            // Ignore players capturing or carrying objects
            for ( j = 0; j < AlliedPlayers.size; j++ )
            {

                if ( !isdefined( MostRecent ) )
                    MostRecent = AlliedPlayers[j];
                else if ( AlliedPlayers[j].pers["teamTime"] > MostRecent.pers["teamTime"] )
                    MostRecent = AlliedPlayers[j];
            }

            if ( isdefined( MostRecent ) )
                MostRecent changeteam( "axis" );
            else
            {
                // Move the player that's been on the team the shortest ammount of time
                for ( j = 0; j < AlliedPlayers.size; j++ )
                {
                    if ( !isdefined( MostRecent ) )
                        MostRecent = AlliedPlayers[j];
                    else if ( AlliedPlayers[j].pers["teamTime"] > MostRecent.pers["teamTime"] )
                        MostRecent = AlliedPlayers[j];
                }

                MostRecent changeTeam( "axis" );
            }
        }
        else if ( AxisPlayers.size > ( AlliedPlayers.size + 1 ) )
        {
            // Move the player that's been on the team the shortest ammount of time (highest teamTime value)
            // Ignore players capturing or carrying objects
            for ( j = 0; j < AxisPlayers.size; j++ )
            {

                if ( !isdefined( MostRecent ) )
                    MostRecent = AxisPlayers[j];
                else if ( AxisPlayers[j].pers["teamTime"] > MostRecent.pers["teamTime"] )
                    MostRecent = AxisPlayers[j];
            }

            if ( isdefined( MostRecent ) )
                MostRecent changeteam( "allies" );
            else
            {
                // Move the player that's been on the team the shortest ammount of time
                for ( j = 0; j < AxisPlayers.size; j++ )
                {
                    if ( !isdefined( MostRecent ) )
                        MostRecent = AxisPlayers[j];
                    else if ( AxisPlayers[j].pers["teamTime"] > MostRecent.pers["teamTime"] )
                        MostRecent = AxisPlayers[j];
                }

                MostRecent changeTeam( "allies" );
            }
        }

        MostRecent = undefined;
        AlliedPlayers = [];
        AxisPlayers = [];

        players = level.players;

        for ( i = 0; i < players.size; i++ )
        {
            if ( ( isdefined( players[i].pers["team"] ) ) && ( players[i].pers["team"] == "allies" ) )
                AlliedPlayers[AlliedPlayers.size] = players[i];
            else if ( ( isdefined( players[i].pers["team"] ) ) && ( players[i].pers["team"] == "axis" ) )
                AxisPlayers[AxisPlayers.size] = players[i];
        }
    }
}

changeteam( team )
{
    teams[0] = "allies";
    teams[1] = "axis";
    assignment = team;
    
    if ( assignment != self.pers["team"] && ( self.sessionstate == "playing" || self.sessionstate == "dead" ) )
    {
        self.switching_teams = true;
        self.joining_team = assignment;
        self.leaving_team = self.pers["team"];
        self suicide();
    }

    self.pers["team"] = assignment;
    self.team = assignment;
    self.pers["weapon"] = undefined;
    self.pers["spawnweapon"] = undefined;
    self.pers["savedmodel"] = undefined;
    self.pers["teamTime"] = undefined;
    
    self maps\mp\_utility::updateobjectivetext();
    
    if ( level.teambased )
        self.sessionteam = assignment;
    else
    {
        self.sessionteam = "none";
        self.ffateam = assignment;
    }
    
    if ( !isalive( self ) )
        self.statusicon = "hud_status_dead";

    self notify( "joined_team" );
    level notify( "joined_team" );
    self notify( "end_respawn" );
}

setplayermodels()
{
    setdefaultcharacterdata();
    game["allies_model"]["JUGGERNAUT"] = maps\mp\killstreaks\_juggernaut::setjugg;
    game["axis_model"]["JUGGERNAUT"] = maps\mp\killstreaks\_juggernaut::setjugg;
    game["allies_model"]["JUGGERNAUT_MANIAC"] = maps\mp\killstreaks\_juggernaut::setjuggmaniac;
    game["axis_model"]["JUGGERNAUT_MANIAC"] = maps\mp\killstreaks\_juggernaut::setjuggmaniac;
}

playermodelforweapon( var_0, var_1 )
{

}

countplayers()
{
    var_0 = [];

    for ( var_1 = 0; var_1 < level.teamnamelist.size; var_1++ )
        var_0[level.teamnamelist[var_1]] = 0;

    for ( var_1 = 0; var_1 < level.players.size; var_1++ )
    {
        if ( level.players[var_1] == self )
            continue;

        if ( level.players[var_1].pers["team"] == "spectator" )
            continue;

        if ( isdefined( level.players[var_1].pers["team"] ) )
            var_0[level.players[var_1].pers["team"]]++;
    }

    return var_0;
}

setdefaultcharacterdata()
{
    if ( !isdefined( level.defaultheadmodels ) )
    {
        level.defaultheadmodels = [];
        level.defaultheadmodels["allies"] = "head_mp_head_a";
        level.defaultheadmodels["axis"] = "head_mp_head_a";
    }

    if ( !isdefined( level.defaultbodymodels ) )
    {
        level.defaultbodymodels = [];
        level.defaultbodymodels["allies"] = "mp_body_us_rangers_assault_a_urban";
        level.defaultbodymodels["axis"] = "mp_body_us_rangers_assault_a_woodland";
    }

    if ( !isdefined( level.defaultviewarmmodels ) )
    {
        level.defaultviewarmmodels = [];
        level.defaultviewarmmodels["allies"] = "viewhands_us_rangers_urban";
        level.defaultviewarmmodels["axis"] = "viewhands_us_rangers_woodland";
    }

    if ( !isdefined( level.defaultvoices ) )
    {
        level.defaultvoices = [];
        level.defaultvoices["allies"] = "delta";
        level.defaultvoices["axis"] = "delta";
    }
}

setcharactermodels( var_0, var_1, var_2 )
{
    self setmodel( var_0 );
    self setviewmodel( var_2 );
    self attach( var_1, "", 1 );
    self.headmodel = var_1;
}

setmodelfromcustomization()
{
    var_0 = self getcustomizationbody();
    var_1 = self getcustomizationhead();
    var_2 = self getcustomizationviewmodel();
    setcharactermodels( var_0, var_1, var_2 );
}

setdefaultmodel()
{
    var_0 = level.defaultbodymodels[self.team];
    var_1 = level.defaultheadmodels[self.team];
    var_2 = level.defaultviewarmmodels[self.team];
    setcharactermodels( var_0, var_1, var_2 );
}

getplayermodelindex()
{
    if ( level.rankedmatch )
        return self getrankedplayerdata( "squadMembers", self.pers["activeSquadMember"], "body" );
    else
        return self getprivateplayerdata( "privateMatchSquadMembers", self.pers["activeSquadMember"], "body" );
}

getplayerfoleytype( var_0 )
{
    return tablelookup( "mp/cac/bodies.csv", 0, var_0, 5 );
}

getplayermodelname( var_0 )
{
    return tablelookup( "mp/cac/bodies.csv", 0, var_0, 1 );
}

setupplayermodel()
{
    if ( isplayer( self ) )
        setmodelfromcustomization();
    else
        setdefaultmodel();

    if ( !isai( self ) )
    {
        var_0 = getplayermodelindex();
        self.bodyindex = var_0;
        var_1 = getplayerfoleytype( var_0 );
        self setclothtype( var_1 );
    }
    else
        self setclothtype( "vestLight" );

    self.voice = level.defaultvoices[self.team];

    if ( maps\mp\_utility::isanymlgmatch() && !isai( self ) )
    {
        var_2 = getplayermodelname( getplayermodelindex() );

        if ( issubstr( var_2, "fullbody_sniper" ) )
            thread forcedefaultmodel();
    }

    if ( maps\mp\_utility::isjuggernaut() )
    {
        if ( isdefined( self.isjuggernautmaniac ) && self.isjuggernautmaniac )
            thread [[ game[self.team + "_model"]["JUGGERNAUT_MANIAC"] ]]();
        else if ( isdefined( self.isjuggernautlevelcustom ) && self.isjuggernautlevelcustom )
            thread [[ game[self.team + "_model"]["JUGGERNAUT_CUSTOM"] ]]();
        else
            thread [[ game[self.team + "_model"]["JUGGERNAUT"] ]]();
    }
}

forcedefaultmodel()
{
    if ( isdefined( self.headmodel ) )
    {
        self detach( self.headmodel, "" );
        self.headmodel = undefined;
    }

    if ( self.team == "axis" )
    {
        self setmodel( "mp_body_juggernaut_light_black" );
        self setviewmodel( "viewhands_juggernaut_ally" );
    }
    else
    {
        self setmodel( "mp_body_infected_a" );
        self setviewmodel( "viewhands_gs_hostage" );
    }

    if ( isdefined( self.headmodel ) )
    {
        self detach( self.headmodel, "" );
        self.headmodel = undefined;
    }

    self attach( "head_mp_infected", "", 1 );
    self.headmodel = "head_mp_infected";
    self setclothtype( "cloth" );
}

trackfreeplayedtime()
{
    self endon( "disconnect" );
    self.timeplayed["allies"] = 0;
    self.timeplayed["axis"] = 0;
    self.timeplayed["other"] = 0;
    self.timeplayed["total"] = 0;

    for (;;)
    {
        if ( game["state"] == "playing" )
        {
            if ( isdefined( self.pers["team"] ) && self.pers["team"] == "allies" && self.sessionteam != "spectator" )
            {
                self.timeplayed["allies"]++;
                self.timeplayed["total"]++;
            }
            else if ( isdefined( self.pers["team"] ) && self.pers["team"] == "axis" && self.sessionteam != "spectator" )
            {
                self.timeplayed["axis"]++;
                self.timeplayed["total"]++;
            }
            else
                self.timeplayed["other"]++;
        }

        wait 1.0;
    }
}

updatefreeplayertimes()
{
    if ( !level.rankedmatch )
        return;

    var_0 = 0;

    for (;;)
    {
        var_0++;

        if ( var_0 >= level.players.size )
            var_0 = 0;

        if ( isdefined( level.players[var_0] ) )
            level.players[var_0] updatefreeplayedtime();

        wait 1.0;
    }
}

updatefreeplayedtime()
{
    if ( !maps\mp\_utility::rankingenabled() )
        return;

    if ( isai( self ) )
        return;

    if ( self.timeplayed["allies"] )
    {
        if ( !issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedAllies", self.timeplayed["allies"] );
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedTotal", self.timeplayed["allies"] );
        }

        maps\mp\gametypes\_persistence::stataddchildbuffered( "round", "timePlayed", self.timeplayed["allies"] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 0, self.timeplayed["allies"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 1, self.timeplayed["allies"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][1] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 2, self.timeplayed["allies"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][2] );

        if ( issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleXpTimePlayed", self.timeplayed["allies"], self.bufferedstatsmax["prestigeDoubleXpMaxTimePlayed"] );
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleWeaponXpTimePlayed", self.timeplayed["allies"], self.bufferedstatsmax["prestigeDoubleWeaponXpMaxTimePlayed"] );
        }
    }

    if ( self.timeplayed["axis"] )
    {
        if ( !issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedOpfor", self.timeplayed["axis"] );
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedTotal", self.timeplayed["axis"] );
        }

        maps\mp\gametypes\_persistence::stataddchildbuffered( "round", "timePlayed", self.timeplayed["axis"] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 0, self.timeplayed["axis"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 1, self.timeplayed["axis"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][1] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 2, self.timeplayed["axis"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][2] );

        if ( issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleXpTimePlayed", self.timeplayed["axis"], self.bufferedstatsmax["prestigeDoubleXpMaxTimePlayed"] );
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleWeaponXpTimePlayed", self.timeplayed["axis"], self.bufferedstatsmax["prestigeDoubleWeaponXpMaxTimePlayed"] );
        }
    }

    if ( self.timeplayed["other"] )
    {
        if ( !issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedOther", self.timeplayed["other"] );
            maps\mp\gametypes\_persistence::stataddbuffered( "timePlayedTotal", self.timeplayed["other"] );
        }

        maps\mp\gametypes\_persistence::stataddchildbuffered( "round", "timePlayed", self.timeplayed["other"] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 0, self.timeplayed["other"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][0] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 1, self.timeplayed["other"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][1] );
        maps\mp\gametypes\_persistence::stataddchildbufferedwithmax( "xpMultiplierTimePlayed", 2, self.timeplayed["other"], self.bufferedchildstatsmax["xpMaxMultiplierTimePlayed"][2] );

        if ( issquadsmode() )
        {
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleXpTimePlayed", self.timeplayed["other"], self.bufferedstatsmax["prestigeDoubleXpMaxTimePlayed"] );
            maps\mp\gametypes\_persistence::stataddbufferedwithmax( "prestigeDoubleWeaponXpTimePlayed", self.timeplayed["other"], self.bufferedstatsmax["prestigeDoubleWeaponXpMaxTimePlayed"] );
        }
    }

    if ( game["state"] == "postgame" )
        return;

    self.timeplayed["allies"] = 0;
    self.timeplayed["axis"] = 0;
    self.timeplayed["other"] = 0;
}

getjointeampermissions( var_0 )
{
    if ( maps\mp\_utility::is_aliens() )
        return 1;

    var_1 = 0;
    var_2 = 0;
    var_3 = level.players;

    for ( var_4 = 0; var_4 < var_3.size; var_4++ )
    {
        var_5 = var_3[var_4];

        if ( isdefined( var_5.pers["team"] ) && var_5.pers["team"] == var_0 )
        {
            var_1++;

            if ( isbot( var_5 ) )
                var_2++;
        }
    }

    if ( var_1 < level.teamlimit )
        return 1;
    else if ( var_2 > 0 )
        return 1;
    else if ( !maps\mp\_utility::matchmakinggame() )
        return 1;
    else if ( level.gametype == "infect" )
        return 1;
    else
        return 0;
}

onplayerspawned()
{
    level endon( "game_ended" );

    for (;;)
        self waittill( "spawned_player" );
}

mt_getteamname( var_0 )
{
    return tablelookupistring( "mp/MTTable.csv", 0, var_0, 1 );
}

mt_getteamicon( var_0 )
{
    return tablelookup( "mp/MTTable.csv", 0, var_0, 2 );
}

mt_getteamheadicon( var_0 )
{
    return tablelookup( "mp/MTTable.csv", 0, var_0, 3 );
}

getteamname( var_0 )
{
    return tablelookupistring( "mp/factionTable.csv", 0, game[var_0], 1 );
}

getteamshortname( var_0 )
{
    return tablelookupistring( "mp/factionTable.csv", 0, game[var_0], 2 );
}

getteamforfeitedstring( var_0 )
{
    return tablelookupistring( "mp/factionTable.csv", 0, game[var_0], 4 );
}

getteameliminatedstring( var_0 )
{
    return tablelookupistring( "mp/factionTable.csv", 0, game[var_0], 3 );
}

getteamicon( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 5 );
}

getteamhudicon( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 6 );
}

getteamheadicon( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 17 );
}

getteamvoiceprefix( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 7 );
}

getteamspawnmusic( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 8 );
}

getteamwinmusic( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 9 );
}

getteamflagmodel( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 10 );
}

getteamflagcarrymodel( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 11 );
}

getteamflagicon( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 12 );
}

getteamflagfx( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 13 );
}

getteamcolor( var_0 )
{
    return ( maps\mp\_utility::stringtofloat( tablelookup( "mp/factionTable.csv", 0, game[var_0], 14 ) ), maps\mp\_utility::stringtofloat( tablelookup( "mp/factionTable.csv", 0, game[var_0], 15 ) ), maps\mp\_utility::stringtofloat( tablelookup( "mp/factionTable.csv", 0, game[var_0], 16 ) ) );
}

getteamcratemodel( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 18 );
}

getteamdeploymodel( var_0 )
{
    return tablelookup( "mp/factionTable.csv", 0, game[var_0], 19 );
}
