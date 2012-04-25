/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "server.h"

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
Reusable version of SV_GetPlayerByHandle() that doesn't
print any silly messages.
*/
client_t *SV_BetterGetPlayerByHandle(const char *handle)
{
	client_t	*cl;
	int		i;
	char		cleanName[64];
    
	// make sure server is running
	if ( !com_sv_running->integer ) {
		return NULL;
	}
    
	// Check whether this is a numeric player handle
	for(i = 0; handle[i] >= '0' && handle[i] <= '9'; i++);
	
	if(!handle[i])
	{
		int plid = atoi(handle);
        
		// Check for numeric playerid match
		if(plid >= 0 && plid < sv_maxclients->integer)
		{
			cl = &svs.clients[plid];
			
			if(cl->state)
				return cl;
		}
	}
    
	// check for a name match
	for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
		if ( !cl->state ) {
			continue;
		}
		if ( !Q_stricmp( cl->name, handle ) ) {
			return cl;
		}
        
		Q_strncpyz( cleanName, cl->name, sizeof(cleanName) );
		Q_CleanStr( cleanName );
		if ( !Q_stricmp( cleanName, handle ) ) {
			return cl;
		}
	}
    
	return NULL;
}

/*
==================
SV_GetPlayerByHandle

Returns the player with player id or name from Cmd_Argv(1)
==================
*/
static client_t *SV_GetPlayerByHandle( void ) {
	client_t	*cl;
	int			i;
	char		*s;
	char		cleanName[64];

	// make sure server is running
	if ( !com_sv_running->integer ) {
		return NULL;
	}

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "No player specified.\n" );
		return NULL;
	}

	s = Cmd_Argv(1);

	// Check whether this is a numeric player handle
	for(i = 0; s[i] >= '0' && s[i] <= '9'; i++);
	
	if(!s[i])
	{
		int plid = atoi(s);

		// Check for numeric playerid match
		if(plid >= 0 && plid < sv_maxclients->integer)
		{
			cl = &svs.clients[plid];
			
			if(cl->state)
				return cl;
		}
	}

	// check for a name match
	for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
		if ( !cl->state ) {
			continue;
		}
		if ( !Q_stricmp( cl->name, s ) ) {
			return cl;
		}

		Q_strncpyz( cleanName, cl->name, sizeof(cleanName) );
		Q_CleanStr( cleanName );
		if ( !Q_stricmp( cleanName, s ) ) {
			return cl;
		}
	}

    if (Q_stricmp( s, "all" )) {
	Com_Printf( "Player %s is not on the server\n", s );
    }

	return NULL;
}

/*
==================
SV_GetPlayerByNum

Returns the player with idnum from Cmd_Argv(1)
==================
*/
static client_t *SV_GetPlayerByNum( void ) {
	client_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		return NULL;
	}

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "No player specified.\n" );
		return NULL;
	}

	s = Cmd_Argv(1);

	for (i = 0; s[i]; i++) {
		if (s[i] < '0' || s[i] > '9') {
			Com_Printf( "Bad slot number: %s\n", s);
			return NULL;
		}
	}
	idnum = atoi( s );
	if ( idnum < 0 || idnum >= sv_maxclients->integer ) {
		Com_Printf( "Bad client slot: %i\n", idnum );
		return NULL;
	}

	cl = &svs.clients[idnum];
	if ( !cl->state ) {
		Com_Printf( "Client %i is not active\n", idnum );
		return NULL;
	}
	return cl;
}

//=========================================================


/*
==================
SV_Map_f

Restart the server on a different map
==================
*/
static void SV_Map_f( void ) {
	char		*cmd;
	char		*map;
	qboolean	killBots, cheat;
	char		expanded[MAX_QPATH];
	char		mapname[MAX_QPATH];

	map = Cmd_Argv(1);
	if ( !map ) {
		return;
	}

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	Com_sprintf (expanded, sizeof(expanded), "maps/%s.bsp", map);
	if ( FS_ReadFile (expanded, NULL) == -1 ) {
		Com_Printf ("Can't find map %s\n", expanded);
		return;
	}

	// force latched values to get set
	Cvar_Get ("g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH );

	cmd = Cmd_Argv(0);
	if( Q_stricmpn( cmd, "sp", 2 ) == 0 ) {
		Cvar_SetValue( "g_gametype", GT_SINGLE_PLAYER );
		Cvar_SetValue( "g_doWarmup", 0 );
		// may not set sv_maxclients directly, always set latched
		Cvar_SetLatched( "sv_maxclients", "8" );
		cmd += 2;
		cheat = qfalse;
		killBots = qtrue;
	}
	else {
		if ( !Q_stricmp( cmd, "devmap" ) || !Q_stricmp( cmd, "spdevmap" ) ) {
			cheat = qtrue;
			killBots = qtrue;
		} else {
			cheat = qfalse;
			killBots = qfalse;
		}
		if( sv_gametype->integer == GT_SINGLE_PLAYER ) {
			Cvar_SetValue( "g_gametype", GT_FFA );
		}
	}

	// save the map name here cause on a map restart we reload the q3config.cfg
	// and thus nuke the arguments of the map command
	Q_strncpyz(mapname, map, sizeof(mapname));

	// start up the map
	SV_SpawnServer( mapname, killBots );

	// set the cheat value
	// if the level was started with "map <levelname>", then
	// cheats will not be allowed.  If started with "devmap <levelname>"
	// then cheats will be allowed
	if ( cheat ) {
		Cvar_Set( "sv_cheats", "1" );
	} else {
		Cvar_Set( "sv_cheats", "0" );
	}
}

/*
================
SV_MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
static void SV_MapRestart_f( void ) {
	int			i;
	client_t	*client;
	char		*denied;
	qboolean	isBot;
	int			delay;

	// make sure we aren't restarting twice in the same frame
	if ( com_frameTime == sv.serverId ) {
		return;
	}

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( sv.restartTime ) {
		return;
	}

	if (Cmd_Argc() > 1 ) {
		delay = atoi( Cmd_Argv(1) );
	}
	else {
		delay = 5;
	}
	if( delay && !Cvar_VariableValue("g_doWarmup") ) {
		sv.restartTime = sv.time + delay * 1000;
		SV_SetConfigstring( CS_WARMUP, va("%i", sv.restartTime) );
		return;
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if ( sv_maxclients->modified || sv_gametype->modified ) {
		char	mapname[MAX_QPATH];

		Com_Printf( "variable change -- restarting.\n" );
		// restart the map the slow way
		Q_strncpyz( mapname, Cvar_VariableString( "mapname" ), sizeof( mapname ) );

		SV_SpawnServer( mapname, qfalse );
		return;
	}

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// generate a new serverid	
	// TTimo - don't update restartedserverId there, otherwise we won't deal correctly with multiple map_restart
	sv.serverId = com_frameTime;
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	// reset all the vm data in place without changing memory allocation
	// note that we do NOT set sv.state = SS_LOADING, so configstrings that
	// had been changed from their default values will generate broadcast updates
	sv.state = SS_LOADING;
	sv.restarting = qtrue;

	SV_RestartGameProgs();

	// run a few frames to allow everything to settle
	for (i = 0; i < 3; i++)
	{
		VM_Call (gvm, GAME_RUN_FRAME, sv.time);
		sv.time += 100;
		svs.time += 100;
	}

	sv.state = SS_GAME;
	sv.restarting = qfalse;

	// connect and begin all the clients
	for (i=0 ; i<sv_maxclients->integer ; i++) {
		client = &svs.clients[i];

		// send the new gamestate to all connected clients
		if ( client->state < CS_CONNECTED) {
			continue;
		}

		if ( client->netchan.remoteAddress.type == NA_BOT ) {
			isBot = qtrue;
		} else {
			isBot = qfalse;
		}

		// add the map_restart command
		SV_AddServerCommand( client, "map_restart\n" );

		// connect the client again, without the firstTime flag
		denied = VM_ExplicitArgPtr( gvm, VM_Call( gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot ) );
		if ( denied ) {
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SV_DropClient( client, denied );
			Com_Printf( "SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i );
			continue;
		}

		client->state = CS_ACTIVE;

		SV_ClientEnterWorld( client, &client->lastUsercmd );
	}	

	// run another frame to allow things to look at all the players
	VM_Call (gvm, GAME_RUN_FRAME, sv.time);
	sv.time += 100;
	svs.time += 100;
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server  FIXME: move to game
==================
*/
/*
static void SV_Kick_f( void ) {
	client_t	*cl;
	int			i;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: kick <player name>\nkick all = kick everyone\nkick allbots = kick all bots\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	if ( !cl ) {
		if ( !Q_stricmp(Cmd_Argv(1), "all") ) {
			for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
				if ( !cl->state ) {
					continue;
				}
				if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
					continue;
				}
				SV_DropClient( cl, "was kicked" );
				cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			}
		}
		else if ( !Q_stricmp(Cmd_Argv(1), "allbots") ) {
			for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
				if ( !cl->state ) {
					continue;
				}
				if( cl->netchan.remoteAddress.type != NA_BOT ) {
					continue;
				}
				SV_DropClient( cl, "was kicked" );
				cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			}
		}
		return;
	}
	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}

	SV_DropClient( cl, "was kicked" );
	cl->lastPacketTime = svs.time;	// in case there is a funny zombie
}
*/

/*
==================
SV_Ban_f

Ban a user from being able to play on this server through the auth
server
==================
*/
static void SV_Ban_f( void ) {
	client_t	*cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: banUser <player name>\n");
		return;
	}

	cl = SV_GetPlayerByHandle();

	if (!cl) {
		return;
	}

	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}

	// look up the authorize server's IP
	if ( !svs.authorizeAddress.ip[0] && svs.authorizeAddress.type != NA_BAD ) {
		Com_Printf( "Resolving %s\n", AUTHORIZE_SERVER_NAME );
		if ( !NET_StringToAdr( AUTHORIZE_SERVER_NAME, &svs.authorizeAddress ) ) {
			Com_Printf( "Couldn't resolve address\n" );
			return;
		}
		svs.authorizeAddress.port = BigShort( PORT_AUTHORIZE );
		Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", AUTHORIZE_SERVER_NAME,
			svs.authorizeAddress.ip[0], svs.authorizeAddress.ip[1],
			svs.authorizeAddress.ip[2], svs.authorizeAddress.ip[3],
			BigShort( svs.authorizeAddress.port ) );
	}

	// otherwise send their ip to the authorize server
	if ( svs.authorizeAddress.type != NA_BAD ) {
		NET_OutOfBandPrint( NS_SERVER, svs.authorizeAddress,
			"banUser %i.%i.%i.%i", cl->netchan.remoteAddress.ip[0], cl->netchan.remoteAddress.ip[1], 
								   cl->netchan.remoteAddress.ip[2], cl->netchan.remoteAddress.ip[3] );
		Com_Printf("%s was banned from coming back\n", cl->name);
	}
}

/*
==================
SV_BanNum_f

Ban a user from being able to play on this server through the auth
server
==================
*/
static void SV_BanNum_f( void ) {
	client_t	*cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: banClient <client number>\n");
		return;
	}

	cl = SV_GetPlayerByNum();
	if ( !cl ) {
		return;
	}
	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}

	// look up the authorize server's IP
	if ( !svs.authorizeAddress.ip[0] && svs.authorizeAddress.type != NA_BAD ) {
		Com_Printf( "Resolving %s\n", AUTHORIZE_SERVER_NAME );
		if ( !NET_StringToAdr( AUTHORIZE_SERVER_NAME, &svs.authorizeAddress ) ) {
			Com_Printf( "Couldn't resolve address\n" );
			return;
		}
		svs.authorizeAddress.port = BigShort( PORT_AUTHORIZE );
		Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", AUTHORIZE_SERVER_NAME,
			svs.authorizeAddress.ip[0], svs.authorizeAddress.ip[1],
			svs.authorizeAddress.ip[2], svs.authorizeAddress.ip[3],
			BigShort( svs.authorizeAddress.port ) );
	}

	// otherwise send their ip to the authorize server
	if ( svs.authorizeAddress.type != NA_BAD ) {
		NET_OutOfBandPrint( NS_SERVER, svs.authorizeAddress,
			"banUser %i.%i.%i.%i", cl->netchan.remoteAddress.ip[0], cl->netchan.remoteAddress.ip[1], 
								   cl->netchan.remoteAddress.ip[2], cl->netchan.remoteAddress.ip[3] );
		Com_Printf("%s was banned from coming back\n", cl->name);
	}
}

/*
==================
SV_KickNum_f

Kick a user off of the server  FIXME: move to game
==================
*/
static void SV_KickNum_f( void ) {
	client_t	*cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: kicknum <client number>\n");
		return;
	}

	cl = SV_GetPlayerByNum();
	if ( !cl ) {
		return;
	}
	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}

	SV_DropClient( cl, "was kicked" );
	cl->lastPacketTime = svs.time;	// in case there is a funny zombie
}

/*
================
SV_Status_f
================
*/
static void SV_Status_f( void ) {
	int			i, j, l;
	client_t	*cl;
	playerState_t	*ps;
	const char		*s;
	int			ping;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	Com_Printf ("map: %s\n", sv_mapname->string );

	Com_Printf ("num score ping name            lastmsg address               qport rate\n");
	Com_Printf ("--- ----- ---- --------------- ------- --------------------- ----- -----\n");
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++)
	{
		if (!cl->state)
			continue;
		Com_Printf ("%3i ", i);
		ps = SV_GameClientNum( i );
		Com_Printf ("%5i ", ps->persistant[PERS_SCORE]);

		if (cl->state == CS_CONNECTED)
			Com_Printf ("CNCT ");
		else if (cl->state == CS_ZOMBIE)
			Com_Printf ("ZMBI ");
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf ("%4i ", ping);
		}

		Com_Printf ("%s", cl->name);
    // TTimo adding a ^7 to reset the color
    // NOTE: colored names in status breaks the padding (WONTFIX)
    Com_Printf ("^7");
		l = 16 - strlen(cl->name);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");

		Com_Printf ("%7i ", svs.time - cl->lastPacketTime );

		s = NET_AdrToString( cl->netchan.remoteAddress );
		Com_Printf ("%s", s);
		l = 22 - strlen(s);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");
		
		Com_Printf ("%5i", cl->netchan.qport);

		Com_Printf (" %5i", cl->rate);

		Com_Printf ("\n");
	}
	Com_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f(void) {
	char	*p;
	char	text[1024];

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc () < 2 ) {
		return;
	}

	strcpy (text, sv_sayprefix->string);
	p = Cmd_Args();

	if ( *p == '"' ) {
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	SV_SendServerCommand(NULL, "chat \"%s\n\"", text);
}


/*
==================
SV_ConTell_f
==================
*/
static void SV_ConTell_f(void) {
	char	*p;
	char	text[1024];
	client_t        *cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() < 3 ) {
		Com_Printf ("Usage: tell <client number> <text>\n");
		return;
	}
	
	cl = SV_GetPlayerByNum();
	if ( !cl ) {
		return;
	}

	strcpy (text, sv_tellprefix->string);
	p = Cmd_ArgsFrom(2);

	if ( *p == '"' ) {
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	SV_SendServerCommand(cl, "chat \"%s\n\"", text);
}


/*
==================
SV_Heartbeat_f

Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
==================
*/
void SV_Heartbeat_f( void ) {
	svs.nextHeartbeatTime = -9999999;
}


/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f( void ) {
	Com_Printf ("Server info settings:\n");
	Info_Print ( Cvar_InfoString( CVAR_SERVERINFO ) );
}


/*
===========
SV_Systeminfo_f

Examine or change the serverinfo string
===========
*/
static void SV_Systeminfo_f( void ) {
	Com_Printf ("System info settings:\n");
	Info_Print ( Cvar_InfoString( CVAR_SYSTEMINFO ) );
}


/*
===========
SV_DumpUser_f

Examine all a users info strings FIXME: move to game
===========
*/
static void SV_DumpUser_f( void ) {
	client_t	*cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: info <userid>\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	if ( !cl ) {
		return;
	}

	Com_Printf( "userinfo\n" );
	Com_Printf( "--------\n" );
	Info_Print( cl->userinfo );
}


/*
=================
SV_KillServer
=================
*/
static void SV_KillServer_f( void ) {
	SV_Shutdown( "killserver" );
}

/*
==================
SV_ForceCvar_f_helper

Called internally by SV_ForceCvar_f.
==================
*/
static void SV_ForceCvar_f_helper( client_t *cl ) {
	int		oldInfoLen;
	int		newInfoLen;
	qboolean	touchedUserinfo = qfalse;

	// Who knows what would happen if we called the VM with a GAME_CLIENT_USERINFO_CHANGED
	// when this client wasn't connected.
	if (cl->state < CS_CONNECTED) {
		return;
	}

	// First remove all keys; there might exist more than one in the userinfo.
	oldInfoLen = strlen(cl->userinfo);
	while (qtrue) {
		Info_RemoveKey(cl->userinfo, Cmd_Argv(2));
		newInfoLen = strlen(cl->userinfo);
		if (oldInfoLen == newInfoLen) { break; } // userinfo wasn't modified.
		oldInfoLen = newInfoLen;
		touchedUserinfo = qtrue;
	}

	if (strlen(Cmd_Argv(3)) > 0) {
		if (strlen(Cmd_Argv(2)) + strlen(Cmd_Argv(3)) + 2 + newInfoLen >= MAX_INFO_STRING) {
			SV_DropClient(cl, "userinfo string length exceeded");
			return;
		}
		Info_SetValueForKey(cl->userinfo, Cmd_Argv(2), Cmd_Argv(3));
		touchedUserinfo = qtrue;
	}

	if (touchedUserinfo) {
		SV_UserinfoChanged(cl);
		VM_Call(gvm, GAME_CLIENT_USERINFO_CHANGED, cl - svs.clients);
	}
}

/*
==================
SV_ForceCvar_f

Set a cvar for a user.
==================
*/
static void SV_ForceCvar_f(void) {
	client_t	*cl;
	int		i;

	// Make sure server is running.
	if (!com_sv_running->integer) {
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 4 || strlen(Cmd_Argv(2)) == 0) {
		Com_Printf("Usage: forcecvar <player name> <cvar name> <cvar value>\nPlayer may be 'allbots'\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	if (!cl) {
		if (!Q_stricmp(Cmd_Argv(1), "allbots")) {
			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
				if (!cl->state) {
					continue;
				}
				if(cl->netchan.remoteAddress.type != NA_BOT) {
					continue;
				}
				SV_ForceCvar_f_helper(cl);
			}
		}
		return;
	}

	SV_ForceCvar_f_helper(cl);
}

////////////////////////////////////////////////////
// separator for forcecvar.patch and incognito.patch
////////////////////////////////////////////////////

/*
==================
SV_SendClientCommand_f

Send a reliable command to a specific client.
==================
*/
static void SV_SendClientCommand_f(void) {
	client_t	*cl;
	char		*cmd;

	// Make sure server is running.
	if (!com_sv_running->integer) {
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() < 3 || strlen(Cmd_Argv(2)) == 0) {
		Com_Printf("Usage: sendclientcommand <player name> <command>\nPlayer may be 'all' or 'allbots'\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	cmd = Cmd_ArgsFromRaw(2);

	if (!cl) {
		if (!Q_stricmp(Cmd_Argv(1), "all")) {
			SV_SendServerCommand(NULL, "%s", cmd);
		}
		return;
	}

	SV_SendServerCommand(cl, "%s", cmd);
}


/*
==================
SV_Incognito_f

Pretend that you disconnect, but really go to spec.
==================
*/
static void SV_Incognito_f(void) {
	client_t	*cl;
	int		i;
	char		cmd[64];

	// Make sure server is running.
	if (!com_sv_running->integer) {
		Com_Printf("Server is not running.\n");
		return;
	}

	if (!in_redirect) {
		Com_Printf("The incognito command can only be run through rcon\n");
		return;
	}

	if (Cmd_Argc() != 1) {
		Com_Printf("No arguments expected for incognito command\n");
		return;
	}

	// Find the person connected to server who issued the incognito command.
	for (i = 0, cl = svs.clients;; i++, cl++) {
		if (i == sv_maxclients->integer) {
			cl = NULL;
			break;
		}
		if (cl->state >= CS_ACTIVE && NET_CompareAdr(cl->netchan.remoteAddress, svs.redirectAddress)) {
			break; // found
		}
	}

	if (cl != NULL) {
		sv.incognitoJoinSpec = qtrue;
		Com_sprintf(cmd, sizeof(cmd), "forceteam %i spectator\n", i);
		Cmd_ExecuteString(cmd);
		sv.incognitoJoinSpec = qfalse;
		SV_SendServerCommand(NULL, "print \"%s" S_COLOR_WHITE " disconnected\n\"", cl->name); // color OK
		Com_sprintf(cmd, sizeof(cmd), "sendclientcommand all cs %i \"\"\n", 548 + i);
		Cmd_ExecuteString(cmd);
	}
	else {
		Com_Printf("Must be connected to server for incognito to work\n");
	}

}

/*
==================
SV_Teleport_f
- used mickaels code
- added messages
==================
*/
static void SV_Teleport_f(void)
{
    client_t *cl;
    playerState_t *ps;
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.\n");
        return;
    }
	
    if (Cmd_Argc() != 2 && Cmd_Argc() != 3 && Cmd_Argc() != 5)
    {
        Com_Printf
		("Usage: teleport <player name> [<x> <y> <z> | <player name (src)>]\n");
        return;
    }
	
    if (!(cl = SV_GetPlayerByHandle()))
        return;
	
    ps = SV_GameClientNum(cl - svs.clients);
	
    if (Cmd_Argc() == 2)        // print a player's position
    {
        //Com_Printf("(%f %f %f)\n", ps->origin[0], ps->origin[1], ps->origin[2]);
		Com_Printf("Position: (%f %f %f)\n", ps->origin[0], ps->origin[1], ps->origin[2]);
        return;
    }
	
    else if (Cmd_Argc() == 3)   // teleport a player to another player's location
    {
        client_t *cl_src;
        playerState_t *ps_src;
		
        Cmd_TokenizeString(Cmd_Args()); // ugly hack for SV_GetPlayerByHandle()
		
        if (!(cl_src = SV_GetPlayerByHandle()))
            return;
		
		if (cl_src == cl) {
			Com_Printf("You cant't teleport a player to himself\n");
			return;
		}
		
        ps_src = SV_GameClientNum(cl_src - svs.clients);
        VectorCopy(ps_src->origin, ps->origin);
		SV_SendServerCommand(cl, "print \"You (%s^7) wer teleported to %s^7\n\"",cl->name, cl_src->name);
		SV_SendServerCommand(cl_src, "print \"%s^7 was teleported to You (%s^7)\n\"",cl->name, cl_src->name);
    }
    else                        // teleport a player to the specified x, y, z coordinates
    {
        int i;
        for (i = 0; i < 3; ++i) {
            ps->origin[i] = atof(Cmd_Argv(i + 2));
		}
		SV_SendServerCommand(cl, "print \"You (%s^7) were teleported to x: %f y: %f z: %f^7\n\"",cl->name, ps->origin[0], ps->origin[1], ps->origin[2]);
    }
	
    VectorClear(ps->velocity);
}

/*
==================
SV_Kick_f
Kick a user off of the server  FIXME: move to game
arg2 is reason
==================
*/
static void SV_Kick_f( void ) {
	client_t	*cl;
	int			i;
	char		*reason = "was kicked";
	
	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}
	
	if ( Cmd_Argc() < 2 || Cmd_Argc() > 3 ) {
		Com_Printf ("Usage: kick <player> <reason>\nkick all = kick everyone\nkick allbots = kick all bots\n");
		return;
	}
	
	
	if ( Cmd_Argc() == 3 ) {
		reason = Cmd_Argv(2);
	}
	
	cl = SV_GetPlayerByHandle();
	if ( !cl ) {
		if ( !Q_stricmp(Cmd_Argv(1), "all") ) {
			for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
				if ( !cl->state ) {
					continue;
				}
				if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
					continue;
				}
				SV_DropClient( cl, reason );
				cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			}
		}
		else if ( !Q_stricmp(Cmd_Argv(1), "allbots") ) {
			for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
				if ( !cl->state ) {
					continue;
				}
				if( cl->netchan.remoteAddress.type != NA_BOT ) {
					continue;
				}
				SV_DropClient( cl, reason );
				cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			}
		}
		return;
	}
	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
		return;
	}
    
	SV_DropClient( cl, reason );
	cl->lastPacketTime = svs.time;	// in case there is a funny zombie
}

/*
==========
SV_Spoof_f
Send a server command as a specific client
==========
*/

static void SV_Spoof_f(void)
{
    client_t *cl;
    char *cmd;
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.\n");
        return;
    }
	
    if (Cmd_Argc() < 3 || strlen(Cmd_Argv(2)) == 0)
    {
        Com_Printf("Usage: spoof <player name> <command>\n");
        return;
    }
	
    if (!(cl = SV_GetPlayerByHandle()))
        return;
	
	cmd = Cmd_ArgsFromRaw(2);
	Cmd_TokenizeString(cmd);
    
    VM_Call(gvm, GAME_CLIENT_COMMAND, cl - svs.clients);
}

/*
==================
SV_CrashPlayer_f
==================
*/
static void SV_CrashPlayer_f(void) {
	client_t	*cl;
	
	// Make sure server is running.
	if (!com_sv_running->integer) {
		Com_Printf("Server is not running.\n");
		return;
	}
	
	if (Cmd_Argc() < 2 || strlen(Cmd_Argv(1)) == 0) {
		Com_Printf("Usage: crash <player name>\nPlayer may be 'all'\n");
		return;
	}
    
	cl = SV_GetPlayerByHandle();
	
	if (!cl) {
		if (!Q_stricmp(Cmd_Argv(1), "all")) {
			SV_SendServerCommand(cl, "cs 512"); // use 512 to crash?
		}
		return;
	}
    
	SV_SendServerCommand(cl, "cs 512"); // use 512 to crash?
}

/*
==========
SV_KillPlayer_f
Kill a client
==========
*/
static void SV_KillPlayer_f(void)
{
    client_t *cl;
    char *cmd;
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.\n");
        return;
    }
	
    if (Cmd_Argc() < 2 || strlen(Cmd_Argv(1)) == 0)
    {
        Com_Printf("Usage: kill <player name or id>\n");
        return;
    }
	
    if (!(cl = SV_GetPlayerByHandle()))
        return;
	
	cmd = "kill";
	Cmd_TokenizeString(cmd);
	
    
    VM_Call(gvm, GAME_CLIENT_COMMAND, cl - svs.clients);
}

/*
==========
SV_FatText_f
made by LuxXx!
Send a fat text to screen
Same functions like "bigtext"
Difference: - Message stays at screen until you send other string or send the "clear" command
- bigger than bigtext lol.
- send to specified client
==========
*/

static void SV_FatText_f(void)
{
    client_t *cl;
	char *cmd;
	
	// Make sure server is running.
	if (!com_sv_running->integer) {
		Com_Printf("Server is not running.\n");
		return;
	}
	
	if ((Cmd_Argc() < 3 || strlen(Cmd_Argv(2)) == 0) && Q_stricmp(Cmd_Argv(1), "clear")) // If arg1 ignore the syntax error message
    {
        Com_Printf("Usage: fattext <player name> <string>\nPlayer may be 'all'\nUse 'fattext clear' to clear the text\n");
        return;
    }
	
	cl = SV_GetPlayerByHandle();
	cmd = Cmd_ArgsFromRaw(2);
	
	if (!cl) {
		if (!Q_stricmp(Cmd_Argv(1), "all")) { // If arg1 is "all" well send the SV_SendServerCommand to a NULL Client (all)
			SV_SendServerCommand(NULL, "cs 28 %s", cmd); 
		}
		if (!Q_stricmp(Cmd_Argv(1), "clear")) { // We need a command to clear the text use "/rcon fattext clear" to do this
			SV_SendServerCommand(NULL, "cs 28 "); // send an empty string to clear the fattext
		}
		return;
	}
	
	SV_SendServerCommand(cl, "cs 28 %s", cmd);	// Send the fattext via ConfigString 28. It's also used by qvm. e.g. "^1Red ^7team wins"
}

/*
==================
SV_FastRestart_f
==================
*/
static void SV_FastRestart_f(void) {
	
	// Make sure server is running.
	if (!com_sv_running->integer) {
		Com_Printf("Server is not running.\n");
		return;
	}
	
	if (Cmd_Argc() < 1 || strlen(Cmd_Argv(0)) == 0) {
		Com_Printf("Usage: fastrestart\n");
		return;
	}
	Cbuf_AddText( "map_restart 0\n" );
	
	
}

/*
==================
SV_ReasonSlap_f
by LuxXx
slaps a player with reasona s bigtext message

Usage: rslap <player id> [player message] [crowd message]
Example /rcon rslap 0 "^7, do not kill your team" "^7 teamkilled"

Just works if sv_UseCustomStrings is 1
==================
*/
static void SV_ReasonSlap_f(void) {
	char		cmd[128];
    char wasslappedbytheadmin[128];
    char youvebeenslapped[128];
    
	// Make sure server is running.
	if (!com_sv_running->integer) {
		Com_Printf("Server is not running.\n");
		return;
	}
	
	if (Cmd_Argc() < 2 || strlen(Cmd_Argv(1)) == 0) {
		Com_Printf("Usage: rslap <player id> [player message] [crowd message]\nExample: /rcon rslap 3 \"^7, do not kill your team\" \"^7 teamkilled\"\n");
		return;
	}
    strcpy(wasslappedbytheadmin,str_wasslappedbytheadmin->string); // store org crowd cvar
    strcpy(youvebeenslapped,str_youvebeenslapped->string); // store org player cvar
    if (strlen(Cmd_Argv(2)) != 0) {
        Cvar_Set( "str_youvebeenslapped", Cmd_Argv(2) ); // set reason for player
    }
    if (strlen(Cmd_Argv(3)) != 0) {
        Cvar_Set( "str_wasslappedbytheadmin", Cmd_Argv(3) ); // set reason for all
    }
    Com_sprintf(cmd, sizeof(cmd), "slap %i\n", atoi(Cmd_Argv(1)));
    Cmd_ExecuteString(cmd);
    
    
    Cvar_Set( "str_youvebeenslapped", youvebeenslapped ); // restore org player cvar
    Cvar_Set( "str_wasslappedbytheadmin", wasslappedbytheadmin ); // restore org crowd cvar
}

/*
==========
SV_InvisiblePlayer_f
==========
*/
static void SV_InvisiblePlayer_f(void)
{
    client_t *cl;
    char cmd[1024];
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.\n");
        return;
    }
	
    if (Cmd_Argc() < 2 || strlen(Cmd_Argv(1)) == 0)
    {
        Com_Printf("Usage: invisibleplayer <player id>\n");
        return;
    }
	
    if (!(cl = SV_GetPlayerByHandle()))
        return;
    
	Q_snprintf(cmd, sizeof(cmd), "sendclientcommand all cs %i \"\"\n", cl - svs.clients + 548);
	Cmd_ExecuteString(cmd);
}

/*
==========
SV_RadioSpoof_f
==========
*/
static void SV_RadioSpoof_f(void)
{
    char cmd[1024];
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.\n");
        return;
    }
	
    if (Cmd_Argc() < 2 || strlen(Cmd_Argv(1)) == 0)
    {
        Com_Printf("Usage: radiospoof <radio1> <radio2> <location> <message>\n");
        return;
    }
	
    
    
	Q_snprintf(cmd, sizeof(cmd), "sendclientcommand all rsay 48 %i %i \"%s\" \"%s\"\n", atoi(Cmd_Argv(1)), atoi(Cmd_Argv(2)), Cmd_Argv(3), Cmd_Argv(4));
	Cmd_ExecuteString(cmd);
}

/*
==========
SV_DisplaySpoof_f
spoofs a team and name
player stay in his team
and won't change his name it just seems so.
==========
*/
static void SV_DisplaySpoof_f(void)
{
    client_t *cl;
    char cmd[1024];
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.\n");
        return;
    }
	
    if (Cmd_Argc() < 2 || strlen(Cmd_Argv(1)) == 0)
    {
        Com_Printf("Usage: displayspoof <player> <spoofname> <spoofteam>\n");
        return;
    }
	
    if (!(cl = SV_GetPlayerByHandle()))
        return;
    //     "n\^2LuxXx^3\t\1\r\3\tl\0\f0\\f1\\f2\\a0\100\a1\0\a2\11"
    //     "n\name\t\team\r\3\tl\0\f0\\f1\\f2\\a0\100\a1\0\a2\11"
    // TODO: add cg_rgb, model support?
    
	Q_snprintf(cmd, sizeof(cmd), "sendclientcommand all cs %i \"n\\%s\\t\\%i\\\n", cl - svs.clients + 548,Cmd_Argv(2), atoi(Cmd_Argv(3)));
	Cmd_ExecuteString(cmd);
}

/*
Start a server-side demo.

This does it all, create the file and adjust the demo-related
stuff in client_t.

This is mostly ripped from sv_client.c/SV_SendClientGameState
and cl_main.c/CL_Record_f.
*/
static void SVD_StartDemoFile(client_t *client, const char *path)
{
	int		i, len;
	entityState_t	*base, nullstate;
	msg_t		msg;
	byte		buffer[MAX_MSGLEN];
	fileHandle_t	file;
    
	Com_DPrintf("SVD_StartDemoFile\n");
	assert(!client->demo_recording);
    
	// create the demo file and write the necessary header
	file = FS_FOpenFileWrite(path);
	assert(file != 0);
    
	MSG_Init(&msg, buffer, sizeof(buffer));
	MSG_Bitstream(&msg); // XXX server code doesn't do this, client code does
    
	MSG_WriteLong(&msg, client->lastClientCommand); // TODO: or is it client->reliableSequence?
    
	MSG_WriteByte(&msg, svc_gamestate);
	MSG_WriteLong(&msg, client->reliableSequence);
    
	for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
		if (sv.configstrings[i][0]) {
			MSG_WriteByte(&msg, svc_configstring);
			MSG_WriteShort(&msg, i);
			MSG_WriteBigString(&msg, sv.configstrings[i]);
		}
	}
    
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0 ; i < MAX_GENTITIES; i++) {
		base = &sv.svEntities[i].baseline;
		if (!base->number) {
			continue;
		}
		MSG_WriteByte(&msg, svc_baseline);
		MSG_WriteDeltaEntity(&msg, &nullstate, base, qtrue);
	}
    
	MSG_WriteByte(&msg, svc_EOF);
    
	MSG_WriteLong(&msg, client - svs.clients);
	MSG_WriteLong(&msg, sv.checksumFeed);
    
	MSG_WriteByte(&msg, svc_EOF); // XXX server code doesn't do this, SV_Netchan_Transmit adds it!
    
	len = LittleLong(client->netchan.outgoingSequence-1);
	FS_Write(&len, 4, file);
    
	len = LittleLong (msg.cursize);
	FS_Write(&len, 4, file);
	FS_Write(msg.data, msg.cursize, file);
    
	FS_Flush(file);
    
	// adjust client_t to reflect demo started
	client->demo_recording = qtrue;
	client->demo_file = file;
	client->demo_waiting = qtrue;
	client->demo_backoff = 1;
	client->demo_deltas = 0;
}

/*
Write a message to a server-side demo file.
*/
void SVD_WriteDemoFile(const client_t *client, const msg_t *msg)
{
	int len;
	msg_t cmsg;
	byte cbuf[MAX_MSGLEN];
	fileHandle_t file = client->demo_file;
    
	if (*(int *)msg->data == -1) { // TODO: do we need this?
		Com_DPrintf("Ignored connectionless packet, not written to demo!\n");
		return;
	}
    
	// TODO: we only copy because we want to add svc_EOF; can we add it and then
	// "back off" from it, thus avoiding the copy?
	MSG_Copy(&cmsg, cbuf, sizeof(cbuf), (msg_t*) msg);
	MSG_WriteByte(&cmsg, svc_EOF); // XXX server code doesn't do this, SV_Netchan_Transmit adds it!
    
	// TODO: the headerbytes stuff done in the client seems unnecessary
	// here because we get the packet *before* the netchan has it's way
	// with it; just not sure that's really true :-/
    
	len = LittleLong(client->netchan.outgoingSequence);
	FS_Write(&len, 4, file);
    
	len = LittleLong(cmsg.cursize);
	FS_Write(&len, 4, file);
    
	FS_Write(cmsg.data, cmsg.cursize, file); // XXX don't use len!
	FS_Flush(file);
}

/*
Stop a server-side demo.

This finishes out the file and clears the demo-related stuff
in client_t again.
*/
static void SVD_StopDemoFile(client_t *client)
{
	int marker = -1;
	fileHandle_t file = client->demo_file;
    
	Com_DPrintf("SVD_StopDemoFile\n");
	assert(client->demo_recording);
    
	// write the necessary trailer and close the demo file
	FS_Write(&marker, 4, file);
	FS_Write(&marker, 4, file);
	FS_Flush(file);
	FS_FCloseFile(file);
    
	// adjust client_t to reflect demo stopped
	client->demo_recording = qfalse;
	client->demo_file = -1;
	client->demo_waiting = qfalse;
	client->demo_backoff = 1;
	client->demo_deltas = 0;
}

/*
Clean up player name to be suitable as path name.
Similar to Q_CleanStr() but tweaked.
*/
static void SVD_CleanPlayerName(char *name)
{
	char *src = name, *dst = name, c;
	while ((c = *src)) {
		// note Q_IsColorString(src++) won't work since it's a macro
		if (Q_IsColorString(src)) {
			src++;
		}
		else if (c == ':' || c == '\\' || c == '/' || c == '*' || c == '?') {
			*dst++ = '%';
		}
		else if (c > ' ' && c < 0x7f) {
			*dst++ = c;
		}
		src++;
	}
	*dst = '\0';
    
	if (strlen(name) == 0) {
		strcpy(name, "UnnamedPlayer");
	}
}

/*
Generate unique name for a new server demo file.
(We pretend there are no race conditions.)
*/
static void SV_NameServerDemo(char *filename, int length, const client_t *client)
{
	qtime_t time;
	char playername[64];
    
	Com_DPrintf("SV_NameServerDemo\n");
    
	Com_RealTime(&time);
	Q_strncpyz(playername, client->name, sizeof(playername));
	SVD_CleanPlayerName(playername);
    
	do {
		// TODO: really this should contain something identifying
		// the server instance it came from; but we could be on
		// (multiple) IPv4 and IPv6 interfaces; in the end, some
		// kind of server guid may be more appropriate; mission?
		// TODO: when the string gets too long (what exactly is
		// the limit?) it get's cut off at the end ruining the
		// file extension
		Com_sprintf(
                    filename, length-1, "serverdemos/%.4d-%.2d-%.2d_%.2d-%.2d-%.2d_%s_%d.dm_%d",
                    time.tm_year+1900, time.tm_mon, time.tm_mday,
                    time.tm_hour, time.tm_min, time.tm_sec,
                    playername,
                    Sys_Milliseconds(),
                    PROTOCOL_VERSION
                    );
		filename[length-1] = '\0';
	} while (FS_FileExists(filename));
}

static void SV_StartRecordOne(client_t *client)
{
	char path[MAX_OSPATH];
    
	Com_DPrintf("SV_StartRecordOne\n");
    
	if (client->demo_recording) {
		Com_Printf("startserverdemo: %s is already being recorded\n", client->name);
		return;
	}
	if (client->state != CS_ACTIVE) {
		Com_Printf("startserverdemo: %s is not active\n", client->name);
		return;
	}
	if (client->netchan.remoteAddress.type == NA_BOT) {
		Com_Printf("startserverdemo: %s is a bot\n", client->name);
		return;
	}
    
	SV_NameServerDemo(path, sizeof(path), client);
	SVD_StartDemoFile(client, path);
    
	SV_SendServerCommand(client, "print \"[!] %s\"\n", sv_demonotice->string);
    
	Com_Printf("startserverdemo: recording %s to %s\n", client->name, path);
}

static void SV_StartRecordAll(void)
{
	int slot;
	client_t *client;
    
	Com_DPrintf("SV_StartRecordAll\n");
    
	for (slot=0, client=svs.clients; slot < sv_maxclients->integer; slot++, client++) {
		// filter here to avoid lots of bogus messages from SV_StartRecordOne()
		if (client->netchan.remoteAddress.type == NA_BOT
		    || client->state != CS_ACTIVE
		    || client->demo_recording) {
			continue;
		}
		SV_StartRecordOne(client);
	}
}

static void SV_StopRecordOne(client_t *client)
{
	Com_DPrintf("SV_StopRecordOne\n");
    
	if (!client->demo_recording) {
		Com_Printf("stopserverdemo: %s is not being recorded\n", client->name);
		return;
	}
	if (client->state != CS_ACTIVE) { // disconnects are handled elsewhere
		Com_Printf("stopserverdemo: %s is not active\n", client->name);
		return;
	}
	if (client->netchan.remoteAddress.type == NA_BOT) {
		Com_Printf("stopserverdemo: %s is a bot\n", client->name);
		return;
	}
    
	SVD_StopDemoFile(client);
    
	Com_Printf("stopserverdemo: stopped recording %s\n", client->name);
}

static void SV_StopRecordAll(void)
{
	int slot;
	client_t *client;
    
	Com_DPrintf("SV_StopRecordAll\n");
    
	for (slot=0, client=svs.clients; slot < sv_maxclients->integer; slot++, client++) {
		// filter here to avoid lots of bogus messages from SV_StopRecordOne()
		if (client->netchan.remoteAddress.type == NA_BOT
		    || client->state != CS_ACTIVE // disconnects are handled elsewhere
		    || !client->demo_recording) {
			continue;
		}
		SV_StopRecordOne(client);
	}
}

/*
==================
SV_StartServerDemo_f

Record a server-side demo for given player/slot. The demo
will be called "YYYY-MM-DD_hh-mm-ss_playername_id.dm_proto",
in the "demos" directory under your game directory. Note
that "startserverdemo all" will start demos for all players
currently in the server. Players who join later require a
new "startserverdemo" command. If you are already recording
demos for some players, another "startserverdemo all" will
start new demos only for players not already recording. Note
that bots will never be recorded, not even if "all" is given.
The server-side demos will stop when "stopserverdemo" is issued
or when the server restarts for any reason (such as a new map
loading).
==================
*/
static void SV_StartServerDemo_f(void)
{
	client_t *client;
    
	Com_DPrintf("SV_StartServerDemo_f\n");
    
	if (!com_sv_running->integer) {
		Com_Printf("startserverdemo: Server not running\n");
		return;
	}
    
	if (Cmd_Argc() != 2) {
		Com_Printf("Usage: startserverdemo <player-or-all>\n");
		return;
	}
    
	client = SV_BetterGetPlayerByHandle(Cmd_Argv(1));
	if (!Q_stricmp(Cmd_Argv(1), "all")) {
		if (client) {
			Com_Printf("startserverdemo: Player 'all' ignored, starting all demos instead\n");
		}
		SV_StartRecordAll();
	}
	else if (client) {
		SV_StartRecordOne(client);
	}
	else {
		Com_Printf("startserverdemo: No player with that handle/in that slot\n");
	}
}

/*
==================
SV_StopServerDemo_f

Stop a server-side demo for given player/slot. Note that
"stopserverdemo all" will stop demos for all players in
the server.
==================
*/
static void SV_StopServerDemo_f(void)
{
	client_t *client;
    
	Com_DPrintf("SV_StopServerDemo_f\n");
    
	if (!com_sv_running->integer) {
		Com_Printf("stopserverdemo: Server not running\n");
		return;
	}
    
	if (Cmd_Argc() != 2) {
		Com_Printf("Usage: stopserverdemo <player-or-all>\n");
		return;
	}
    
	client = SV_BetterGetPlayerByHandle(Cmd_Argv(1));
	if (!Q_stricmp(Cmd_Argv(1), "all")) {
		if (client) {
			Com_Printf("stopserverdemo: Player 'all' ignored, stopping all demos instead\n");
		}
		SV_StopRecordAll();
	}
	else if (client) {
		SV_StopRecordOne(client);
	}
	else {
		Com_Printf("stopserverdemo: No player with that handle/in that slot\n");
	}
}

/*
================
SV_utxinfo_f
================
*/
static void SV_utxinfo_f( void ) {
    if ((!Q_stricmp("4.1",Cvar_VariableString("g_modversion")))) {
        Com_Printf("You do not use the modded utx/tsu qvm by Panda\n");
        return;
    }
    Com_Printf("Version is %s but server spoofs %s\n",Cvar_VariableString("g_modversion"),Cvar_VariableString("sv_modversion"));
    Com_Printf("\n");
    if (strlen(Cvar_VariableString("utx_entFile")) != 0) {
        Com_Printf("utx_entFile: %s\n",Cvar_VariableString("utx_entFile"));
        Com_Printf("    suspected ent file: %s_%s.ent\n", sv_mapname->string, Cvar_VariableString("utx_entFile"));
    }
    else {
        Com_Printf("utx_entFile: none loaded\n");
    }
    
    Com_Printf("utx_fancynames: %s\n",Cvar_VariableString("utx_fancynames"));
    if (strlen(Cvar_VariableString("utx_config")) != 0) {
        Com_Printf("utx_config: %s\n",Cvar_VariableString("utx_config")); 
    }
    else {
        Com_Printf("utx_config: none loaded\n");
    }
    if (strlen(Cvar_VariableString("utx_config")) != 0) {
        Com_Printf("utx_rotation: %s\n",Cvar_VariableString("utx_rotation")); 
    }
    else {
        Com_Printf("utx_rotation: none loaded\n");
    }
    if (strlen(Cvar_VariableString("utx_config")) != 0) {
        Com_Printf("utx_submaps: %s\n",Cvar_VariableString("utx_submaps")); 
    }
    else {
        Com_Printf("utx_submaps: none loaded\n");
    }
}

/*
==========
SV_BigText_f
==========
*/
static void SV_BigText_f(void)
{
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.\n");
        return;
    }
	
    if (Cmd_Argc() < 2 || strlen(Cmd_Argv(1)) == 0)
    {
        Com_Printf("Usage: bigtext <string>\n");
        return;
    }
    SV_SendServerCommand(NULL, "cp \"%s\"",Cmd_Args());
    return; // do not run the qvm bigtext
}

/*
==========
SV_CenterPrint_f
==========
*/
static void SV_CenterPrint_f(void)
{
    client_t *cl;
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.\n");
        return;
    }
	
    if (Cmd_Argc() < 3 || strlen(Cmd_Argv(2)) == 0)
    {
        Com_Printf("Usage: centerprint <player id> <message>\n");
        return;
    }
	
    if (!(cl = SV_GetPlayerByHandle()))
        return;
    
    SV_SendServerCommand(cl, "cp \"%s\"",Cmd_Argv(2));
}

/*
================
SV_Guids_f
================
*/
static void SV_Guids_f(void)
{
    int i, j, l;
    client_t *cl;
    playerState_t *ps;
	
    // make sure server is running
    if (!com_sv_running->integer)
    {
        Com_Printf("Server is not running.n");
        return;
    }
	
    Com_Printf
	("num name               guid                             password\n");
    Com_Printf
	("--- ------------------ -------------------------------- -----------------\n");
	
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
    {
        if (!cl->state)
            continue;
		
        Com_Printf("%3i ", i);
        ps = SV_GameClientNum(i);
        Com_Printf("%s", cl->name);
		
        // TTimo adding a ^7 to reset the color
        // NOTE: colored names in status breaks the padding (WONTFIX)
        Com_Printf("^7");
        l = 16 - strlen(cl->name);
        for (j = 0; j < l; j++)
            Com_Printf(" ");
		
        Com_Printf(" %s", Info_ValueForKey(cl->userinfo, "cl_guid"));
		
        Com_Printf(" %s", Info_ValueForKey(cl->userinfo, "password"));
		
        Com_Printf("\n");
    }
    Com_Printf("\n");
}

//===========================================================

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands( void ) {
	static qboolean	initialized;

	if ( initialized ) {
		return;
	}
	initialized = qtrue;

	Cmd_AddCommand ("teleport", SV_Teleport_f);
	Cmd_AddCommand ("tp", SV_Teleport_f);
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("kick", SV_Kick_f);
    Cmd_AddCommand ("rkick", SV_Kick_f);
    Cmd_AddCommand ("reasonkick", SV_Kick_f);
    Cmd_AddCommand ("spoof", SV_Spoof_f);
	Cmd_AddCommand ("spf", SV_Spoof_f);
    Cmd_AddCommand ("crashplayer", SV_CrashPlayer_f);
	Cmd_AddCommand ("crash", SV_CrashPlayer_f);
    Cmd_AddCommand ("killplayer", SV_KillPlayer_f);
    Cmd_AddCommand ("kill", SV_KillPlayer_f);
    Cmd_AddCommand ("fattext", SV_FatText_f);
    Cmd_AddCommand ("ft", SV_FatText_f);
    Cmd_AddCommand ("frestart", SV_FastRestart_f);
    Cmd_AddCommand ("fastrestart", SV_FastRestart_f);
    Cmd_AddCommand ("fr", SV_FastRestart_f);
    Cmd_AddCommand ("rslap", SV_ReasonSlap_f);
    Cmd_AddCommand ("reasonslap", SV_ReasonSlap_f);
    Cmd_AddCommand ("inv", SV_InvisiblePlayer_f);
    Cmd_AddCommand ("invisible", SV_InvisiblePlayer_f);
    Cmd_AddCommand ("invisibleplayer", SV_InvisiblePlayer_f);
    Cmd_AddCommand ("rs", SV_RadioSpoof_f);
    Cmd_AddCommand ("rspoof", SV_RadioSpoof_f);
    Cmd_AddCommand ("radiospoof", SV_RadioSpoof_f);
    Cmd_AddCommand ("displayspoof", SV_DisplaySpoof_f);
    Cmd_AddCommand ("dspoof", SV_DisplaySpoof_f);
    Cmd_AddCommand ("ds", SV_DisplaySpoof_f);
    Cmd_AddCommand ("startserverdemo", SV_StartServerDemo_f);
	Cmd_AddCommand ("stopserverdemo", SV_StopServerDemo_f);
    Cmd_AddCommand ("utxinfo", SV_utxinfo_f);
    Cmd_AddCommand ("utx", SV_utxinfo_f);
    Cmd_AddCommand ("bigtext", SV_BigText_f);
    Cmd_AddCommand ("centerprint", SV_CenterPrint_f);
    Cmd_AddCommand ("cprint", SV_CenterPrint_f);
    Cmd_AddCommand ("cp", SV_CenterPrint_f);
    Cmd_AddCommand ("privatebigtext", SV_CenterPrint_f);
    Cmd_AddCommand ("pbigtext", SV_CenterPrint_f);
    Cmd_AddCommand ("guids", SV_Guids_f);
    
	/*
	Cmd_AddCommand ("banUser", SV_Ban_f);
	Cmd_AddCommand ("banClient", SV_BanNum_f);
	*/
	Cmd_AddCommand ("clientkick", SV_KickNum_f);
	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("systeminfo", SV_Systeminfo_f);
	Cmd_AddCommand ("dumpuser", SV_DumpUser_f);
	Cmd_AddCommand ("map_restart", SV_MapRestart_f);
	Cmd_AddCommand ("sectorlist", SV_SectorList_f);
	Cmd_AddCommand ("map", SV_Map_f);
#ifndef PRE_RELEASE_DEMO
	Cmd_AddCommand ("devmap", SV_Map_f);
	Cmd_AddCommand ("spmap", SV_Map_f);
	Cmd_AddCommand ("spdevmap", SV_Map_f);
#endif
	Cmd_AddCommand ("killserver", SV_KillServer_f);
	if( com_dedicated->integer ) {
		Cmd_AddCommand ("say", SV_ConSay_f);
		Cmd_AddCommand ("tell", SV_ConTell_f);
	}

	Cmd_AddCommand ("forcecvar", SV_ForceCvar_f);
	////////////////////////////////////////////////////
	// separator for forcecvar.patch and incognito.patch
	////////////////////////////////////////////////////
	Cmd_AddCommand ("sendclientcommand", SV_SendClientCommand_f);
    Cmd_AddCommand ("scc", SV_SendClientCommand_f);
	Cmd_AddCommand ("incognito", SV_Incognito_f);
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands( void ) {
#if 0
	// removing these won't let the server start again
	Cmd_RemoveCommand ("heartbeat");
	Cmd_RemoveCommand ("kick");
	Cmd_RemoveCommand ("banUser");
	Cmd_RemoveCommand ("banClient");
	Cmd_RemoveCommand ("status");
	Cmd_RemoveCommand ("serverinfo");
	Cmd_RemoveCommand ("systeminfo");
	Cmd_RemoveCommand ("dumpuser");
	Cmd_RemoveCommand ("map_restart");
	Cmd_RemoveCommand ("sectorlist");
	Cmd_RemoveCommand ("say");
	Cmd_RemoveCommand ("tell");
	Cmd_RemoveCommand ("startserverdemo");
	Cmd_RemoveCommand ("stopserverdemo");
#endif
}

