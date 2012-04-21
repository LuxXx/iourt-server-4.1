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
===============
SV_SendConfigstring

Creates and sends the server command necessary to update the CS index for the
given client
===============
*/
static void SV_SendConfigstring(client_t *client, int index)
{
	int maxChunkSize = MAX_STRING_CHARS - 24;
	int len;

	len = strlen(sv.configstrings[index]);

	if( len >= maxChunkSize ) {
		int		sent = 0;
		int		remaining = len;
		char	*cmd;
		char	buf[MAX_STRING_CHARS];

		while (remaining > 0 ) {
			if ( sent == 0 ) {
				cmd = "bcs0";
			}
			else if( remaining < maxChunkSize ) {
				cmd = "bcs2";
			}
			else {
				cmd = "bcs1";
			}
			Q_strncpyz( buf, &sv.configstrings[index][sent],
				maxChunkSize );

			SV_SendServerCommand( client, "%s %i \"%s\"\n", cmd,
				index, buf );

			sent += (maxChunkSize - 1);
			remaining -= (maxChunkSize - 1);
		}
	} else {
		// standard cs, just send it
		SV_SendServerCommand( client, "cs %i \"%s\"\n", index,
			sv.configstrings[index] );
	}
}

/*
===============
SV_UpdateConfigstrings

Called when a client goes from CS_PRIMED to CS_ACTIVE.  Updates all
Configstring indexes that have changed while the client was in CS_PRIMED
===============
*/
void SV_UpdateConfigstrings(client_t *client)
{
	int index;

	for( index = 0; index <= MAX_CONFIGSTRINGS; index++ ) {
		// if the CS hasn't changed since we went to CS_PRIMED, ignore
		if(!client->csUpdated[index])
			continue;

		// do not always send server info to all clients
		if ( index == CS_SERVERINFO && client->gentity &&
			(client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
			continue;
		}
		SV_SendConfigstring(client, index);
		client->csUpdated[index] = qfalse;
	}
}

/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstring (int index, const char *val) {
	int		len, i;
	client_t	*client;

	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_SetConfigstring: bad index %i\n", index);
	}

	if ( !val ) {
		val = "";
	}

	// don't bother broadcasting an update if no change
	if ( !strcmp( val, sv.configstrings[ index ] ) ) {
		return;
	}

	// change the string in sv
	Z_Free( sv.configstrings[index] );
	sv.configstrings[index] = CopyString( val );

	// send it to all the clients if we aren't
	// spawning a new server
	if ( sv.state == SS_GAME || sv.restarting ) {

		// send the data to all relevent clients
		for (i = 0, client = svs.clients; i < sv_maxclients->integer ; i++, client++) {
			if ( client->state < CS_ACTIVE ) {
				if ( client->state == CS_PRIMED )
					client->csUpdated[ index ] = qtrue;
				continue;
			}
			// do not always send server info to all clients
			if ( index == CS_SERVERINFO && client->gentity && (client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
				continue;
			}
		

			len = strlen( val );
			SV_SendConfigstring(client, index);
		}
	}
}

/*
===============
SV_GetConfigstring

===============
*/
void SV_GetConfigstring( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetConfigstring: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_GetConfigstring: bad index %i\n", index);
	}
	if ( !sv.configstrings[index] ) {
		buffer[0] = 0;
		return;
	}

	Q_strncpyz( buffer, sv.configstrings[index], bufferSize );
}


/*
===============
SV_SetUserinfo

===============
*/
void SV_SetUserinfo( int index, const char *val ) {
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_SetUserinfo: bad index %i\n", index);
	}

	if ( !val ) {
		val = "";
	}

	Q_strncpyz( svs.clients[index].userinfo, val, sizeof( svs.clients[ index ].userinfo ) );
	Q_strncpyz( svs.clients[index].name, Info_ValueForKey( val, "name" ), sizeof(svs.clients[index].name) );
}



/*
===============
SV_GetUserinfo

===============
*/
void SV_GetUserinfo( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetUserinfo: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_GetUserinfo: bad index %i\n", index);
	}
	Q_strncpyz( buffer, svs.clients[ index ].userinfo, bufferSize );
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress non-delta messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline( void ) {
	sharedEntity_t *svent;
	int				entnum;	

	for ( entnum = 1; entnum < sv.num_entities ; entnum++ ) {
		svent = SV_GentityNum(entnum);
		if (!svent->r.linked) {
			continue;
		}
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		sv.svEntities[entnum].baseline = svent->s;
	}
}


/*
===============
SV_BoundMaxClients

===============
*/
void SV_BoundMaxClients( int minimum ) {
	// get the current maxclients value
	Cvar_Get( "sv_maxclients", "8", 0 );

	sv_maxclients->modified = qfalse;

	if ( sv_maxclients->integer < minimum ) {
		Cvar_Set( "sv_maxclients", va("%i", minimum) );
	} else if ( sv_maxclients->integer > MAX_CLIENTS ) {
		Cvar_Set( "sv_maxclients", va("%i", MAX_CLIENTS) );
	}
}


/*
===============
SV_Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
void SV_Startup( void ) {
	if ( svs.initialized ) {
		Com_Error( ERR_FATAL, "SV_Startup: svs.initialized" );
	}
	SV_BoundMaxClients( 1 );

	svs.clients = Z_Malloc (sizeof(client_t) * sv_maxclients->integer );
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * 64;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
	}
	svs.initialized = qtrue;

	// Don't respect sv_killserver unless a server is actually running
	if ( sv_killserver->integer ) {
		Cvar_Set( "sv_killserver", "0" );
	}

	Cvar_Set( "sv_running", "1" );
}


/*
==================
SV_ChangeMaxClients
==================
*/
void SV_ChangeMaxClients( void ) {
	int		oldMaxClients;
	int		i;
	client_t	*oldClients;
	int		count;

	// get the highest client number in use
	count = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if (i > count)
				count = i;
		}
	}
	count++;

	oldMaxClients = sv_maxclients->integer;
	// never go below the highest client number in use
	SV_BoundMaxClients( count );
	// if still the same
	if ( sv_maxclients->integer == oldMaxClients ) {
		return;
	}

	oldClients = Hunk_AllocateTempMemory( count * sizeof(client_t) );
	// copy the clients to hunk memory
	for ( i = 0 ; i < count ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			oldClients[i] = svs.clients[i];
		}
		else {
			Com_Memset(&oldClients[i], 0, sizeof(client_t));
		}
	}

	// free old clients arrays
	Z_Free( svs.clients );

	// allocate new clients
	svs.clients = Z_Malloc ( sv_maxclients->integer * sizeof(client_t) );
	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof(client_t) );

	// copy the clients over
	for ( i = 0 ; i < count ; i++ ) {
		if ( oldClients[i].state >= CS_CONNECTED ) {
			svs.clients[i] = oldClients[i];
		}
	}

	// free the old clients on the hunk
	Hunk_FreeTempMemory( oldClients );
	
	// allocate new snapshot entities
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * 64;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
	}
}

/*
================
SV_ClearServer
================
*/
void SV_ClearServer(void) {
	int i;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			Z_Free( sv.configstrings[i] );
		}
	}
	Com_Memset (&sv, 0, sizeof(sv));
}

/*
================
SV_TouchCGame

  touch the cgame.vm so that a pure client can load it if it's in a seperate pk3
================
*/
void SV_TouchCGame(void) {
	fileHandle_t	f;
	char filename[MAX_QPATH];

	Com_sprintf( filename, sizeof(filename), "vm/%s.qvm", "cgame" );
	FS_FOpenFileRead( filename, &f, qfalse );
	if ( f ) {
		FS_FCloseFile( f );
	}
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void SV_SpawnServer( char *server, qboolean killBots ) {
	int			i;
	int			checksum;
	qboolean	isBot;
	char		systemInfo[16384];
	const char	*p;

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();

	Com_Printf ("------ Server Initialization ------\n");
	Com_Printf ("Server: %s\n",server);

	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	CL_MapLoading();

	// make sure all the client stuff is unloaded
	CL_ShutdownAll();

	// clear the whole hunk because we're (re)loading the server
	Hunk_Clear();

#ifndef DEDICATED
	// Restart renderer
	CL_StartHunkUsers( qtrue );
#endif

	// clear collision map data
	CM_ClearMap();

	// init client structures and svs.numSnapshotEntities 
	if ( !Cvar_VariableValue("sv_running") ) {
		SV_Startup();
	} else {
		// check for maxclients change
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

	// clear pak references
	FS_ClearPakReferences(0);

	// allocate the snapshot entities on the hunk
	svs.snapshotEntities = Hunk_Alloc( sizeof(entityState_t)*svs.numSnapshotEntities, h_high );
	svs.nextSnapshotEntities = 0;

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	Cvar_Set( "nextmap", "map_restart 0");
//	Cvar_Set( "nextmap", va("map %s", server) );

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// save when the server started for each client already connected
		if (svs.clients[i].state >= CS_CONNECTED) {
			svs.clients[i].oldServerTime = sv.time;
		}
	}

	// wipe the entire per-level structure
	SV_ClearServer();
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyString("");
	}

	// make sure we are not paused
	Cvar_Set("cl_paused", "0");

	// get a new checksum feed and restart the file system
	srand(Com_Milliseconds());
	sv.checksumFeed = ( ((int) rand() << 16) ^ rand() ) ^ Com_Milliseconds();
	FS_Restart( sv.checksumFeed );

	CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );

	// set serverinfo visible name
	Cvar_Set( "mapname", server );

	Cvar_Set( "sv_mapChecksum", va("%i",checksum) );

	// serverid should be different each time
	sv.serverId = com_frameTime;
	sv.restartedServerId = sv.serverId; // I suppose the init here is just to be safe
	sv.checksumFeedServerId = sv.serverId;
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	// clear physics interaction links
	SV_ClearWorld ();
	
	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	// load and spawn all other entities
	SV_InitGameProgs();

	// don't allow a map_restart if game is modified
	sv_gametype->modified = qfalse;

	// run a few frames to allow everything to settle
	for (i = 0;i < 3; i++)
	{
		VM_Call (gvm, GAME_RUN_FRAME, sv.time);
		SV_BotFrame (sv.time);
		sv.time += 100;
		svs.time += 100;
	}

	// create a baseline for more efficient communications
	SV_CreateBaseline ();
    
    // stop server-side demo (if any)
	Cbuf_ExecuteText(EXEC_NOW, "stopserverdemo all");

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// send the new gamestate to all connected clients
		if (svs.clients[i].state >= CS_CONNECTED) {
			char	*denied;

			if ( svs.clients[i].netchan.remoteAddress.type == NA_BOT ) {
				if ( killBots ) {
					SV_DropClient( &svs.clients[i], "" );
					continue;
				}
				isBot = qtrue;
			}
			else {
				isBot = qfalse;
			}

			// connect the client again
			denied = VM_ExplicitArgPtr( gvm, VM_Call( gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot ) );	// firstTime = qfalse
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SV_DropClient( &svs.clients[i], denied );
			} else {
                svs.clients[i].muted = qfalse;
				svs.clients[i].positionIsSaved = qfalse;
				svs.clients[i].lastLoadPositionTime = 0;
				svs.clients[i].lastGotoTime = 0;
				if( !isBot ) {
					// when we get the next packet from a connected client,
					// the new gamestate will be sent
					svs.clients[i].state = CS_CONNECTED;
				}
				else {
					client_t		*client;
					sharedEntity_t	*ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SV_GentityNum( i );
					ent->s.number = i;
					client->gentity = ent;

					client->deltaMessage = -1;
					client->nextSnapshotTime = svs.time;	// generate a snapshot immediately

					VM_Call( gvm, GAME_CLIENT_BEGIN, i );
				}
			}
		}
	}	

	// run another frame to allow things to look at all the players
	VM_Call (gvm, GAME_RUN_FRAME, sv.time);
	SV_BotFrame (sv.time);
	sv.time += 100;
	svs.time += 100;

	if ( sv_pure->integer ) {
		// the server sends these to the clients so they will only
		// load pk3s also loaded at the server
		p = FS_LoadedPakChecksums();
		Cvar_Set( "sv_paks", p );
		if (strlen(p) == 0) {
			Com_Printf( "WARNING: sv_pure set but no PK3 files loaded\n" );
		}
		p = FS_LoadedPakNames();
		Cvar_Set( "sv_pakNames", p );

		// if a dedicated pure server we need to touch the cgame because it could be in a
		// seperate pk3 file and the client will need to load the latest cgame.qvm
		if ( com_dedicated->integer ) {
			SV_TouchCGame();
		}
	}
	else {
		Cvar_Set( "sv_paks", "" );
		Cvar_Set( "sv_pakNames", "" );
	}
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	p = FS_ReferencedPakChecksums();
	Cvar_Set( "sv_referencedPaks", p );
	p = FS_ReferencedPakNames();
	Cvar_Set( "sv_referencedPakNames", p );

	// save systeminfo and serverinfo strings
	Q_strncpyz( systemInfo, Cvar_InfoString_Big( CVAR_SYSTEMINFO ), sizeof( systemInfo ) );
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SV_SetConfigstring( CS_SYSTEMINFO, systemInfo );

	SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SV_Heartbeat_f();

	Hunk_SetMark();

	Com_Printf ("-----------------------------------\n");
}

/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_BotInitBotLib(void);

void SV_Init (void) {
	SV_AddOperatorCommands ();

	// serverinfo vars
	Cvar_Get ("dmflags", "0", CVAR_SERVERINFO);
	Cvar_Get ("fraglimit", "20", CVAR_SERVERINFO);
	Cvar_Get ("timelimit", "0", CVAR_SERVERINFO);
	sv_gametype = Cvar_Get ("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH );
	Cvar_Get ("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get ("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	sv_mapname = Cvar_Get ("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	sv_privateClients = Cvar_Get ("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get ("sv_hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE );
	sv_maxclients = Cvar_Get ("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);

	sv_minRate = Cvar_Get ("sv_minRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxRate = Cvar_Get ("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_minPing = Cvar_Get ("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxPing = Cvar_Get ("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_floodProtect = Cvar_Get ("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO );

	// systeminfo
	Cvar_Get ("sv_cheats", "1", CVAR_SYSTEMINFO /*| CVAR_ROM*/ ); // hax0r!
	sv_serverid = Cvar_Get ("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_pure = Cvar_Get ("sv_pure", "1", CVAR_SYSTEMINFO );
	Cvar_Get ("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );

	// server vars
	sv_rconPassword = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	sv_privatePassword = Cvar_Get ("sv_privatePassword", "", CVAR_TEMP );
	sv_fps = Cvar_Get ("sv_fps", "20", CVAR_TEMP );
	sv_timeout = Cvar_Get ("sv_timeout", "200", CVAR_TEMP );
	sv_zombietime = Cvar_Get ("sv_zombietime", "2", CVAR_TEMP );
	Cvar_Get ("nextmap", "", CVAR_TEMP );

	sv_allowDownload = Cvar_Get ("sv_allowDownload", "0", CVAR_SERVERINFO);
	Cvar_Get ("sv_dlURL", "", CVAR_SERVERINFO | CVAR_ARCHIVE);
	sv_master[0] = Cvar_Get ("sv_master1", MASTER_SERVER_NAME, 0 );
	sv_master[1] = Cvar_Get ("sv_master2", "", CVAR_ARCHIVE );
	sv_master[2] = Cvar_Get ("sv_master3", "", CVAR_ARCHIVE );
	sv_master[3] = Cvar_Get ("sv_master4", "", CVAR_ARCHIVE );
	sv_master[4] = Cvar_Get ("sv_master5", "", CVAR_ARCHIVE );
	sv_reconnectlimit = Cvar_Get ("sv_reconnectlimit", "3", 0);
	sv_showloss = Cvar_Get ("sv_showloss", "0", 0);
	sv_padPackets = Cvar_Get ("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get ("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get ("sv_mapChecksum", "", CVAR_ROM);
	sv_lanForceRate = Cvar_Get ("sv_lanForceRate", "1", CVAR_ARCHIVE );
	sv_strictAuth = Cvar_Get ("sv_strictAuth", "1", CVAR_ARCHIVE );

	sv_block1337 = Cvar_Get ("sv_block1337", "0", CVAR_ARCHIVE );

	sv_checkUserinfo = Cvar_Get ("sv_checkUserinfo", "0", CVAR_ARCHIVE );

	sv_forceAutojoin = Cvar_Get ("sv_forceAutojoin", "0", CVAR_ARCHIVE );

	sv_ip2locEnable = Cvar_Get ("sv_ip2locEnable", "0", CVAR_ARCHIVE );
	sv_ip2locHost = Cvar_Get ("sv_ip2locHost", "", CVAR_ARCHIVE );
	sv_ip2locPassword = Cvar_Get ("sv_ip2locPassword", "", CVAR_TEMP );

	sv_logRconArgs = Cvar_Get ("sv_logRconArgs", "0", CVAR_ARCHIVE );

	sv_sanitizeNames = Cvar_Get ("sv_sanitizeNames", "0", CVAR_INIT );

	sv_noKevlar = Cvar_Get ("sv_noKevlar", "0", CVAR_INIT );

	sv_requireValidGuid = Cvar_Get ("sv_requireValidGuid", "0", CVAR_ARCHIVE );
	sv_playerDBHost = Cvar_Get ("sv_playerDBHost", "", CVAR_ARCHIVE );
	sv_playerDBPassword = Cvar_Get ("sv_playerDBPassword", "", CVAR_TEMP );
	sv_playerDBUserInfo = Cvar_Get ("sv_playerDBUserInfo", "0", CVAR_ARCHIVE );
	sv_playerDBBanIDs = Cvar_Get ("sv_playerDBBanIDs", "", CVAR_ARCHIVE );
	sv_permaBanBypass = Cvar_Get ("sv_permaBanBypass", "", CVAR_TEMP );

	sv_specChatGlobal = Cvar_Get ("sv_specChatGlobal", "0", CVAR_ARCHIVE );

	sv_tellprefix = Cvar_Get ("sv_tellprefix", "console_tell: ", CVAR_ARCHIVE );
    sv_sayprefix = Cvar_Get ("sv_sayprefix", "console: ", CVAR_ARCHIVE );

    sv_minTeamChangeHealth = Cvar_Get("sv_minTeamChangeHealth", "0", CVAR_ARCHIVE);
    sv_minKillHealth = Cvar_Get("sv_minKillHealth", "0", CVAR_ARCHIVE);
    
	sv_CheckDRDoS = Cvar_Get ("sv_CheckDRDoS", "1", CVAR_ARCHIVE );

    sv_disableWeapDrop = Cvar_Get("sv_disableWeapDrop", "0", CVAR_ARCHIVE);
    sv_disableItemDrop = Cvar_Get("sv_disableItemDrop", "0", CVAR_ARCHIVE);
    sv_disableRadioChat = Cvar_Get("sv_disableRadioChat", "0", CVAR_ARCHIVE);

    sv_disableScope = Cvar_Get("sv_disableScope", "0", CVAR_ARCHIVE);
    
    int i;
    sv_moderatorenable = Cvar_Get("sv_moderatorenable", "0", CVAR_ARCHIVE);
    sv_moderatorremoteenable =
    Cvar_Get("sv_moderatorremoteenable", "0", CVAR_ARCHIVE);
    // Init all the moderator cvars in a loop.
    for (i = 0; i < MAX_MOD_LEVELS; i++)
    {
        sv_moderatorpass[i] =
        Cvar_Get(va("sv_moderatorpass%i", i + 1), "", CVAR_ARCHIVE);
        sv_moderatorcommands[i] =
        Cvar_Get(va("sv_moderatorcommands%i", i + 1), "", CVAR_ARCHIVE);
    }

	//goto, save and load
	sv_allowGoto = Cvar_Get("sv_allowGoto", "0", CVAR_ARCHIVE);
	sv_gotoWaitTime = Cvar_Get("sv_gotoWaitTime", "180", CVAR_ARCHIVE);
	sv_allowLoadPosition = Cvar_Get("sv_allowLoadPosition", "0", CVAR_ARCHIVE);
	sv_loadPositionWaitTime = Cvar_Get("sv_loadPositionWaitTime", "180", CVAR_ARCHIVE);

    // String Replace
    sv_CensoredStrings = Cvar_Get("sv_CensoredStrings", "0", CVAR_ARCHIVE);
    sv_CustomStrings = Cvar_Get("sv_CustomStrings", "0", CVAR_ARCHIVE);
	str_enteredthegame = Cvar_Get("str_enteredthegame", "^7 entered the game", CVAR_ARCHIVE);
	str_joinedtheredteam = Cvar_Get("str_joinedtheredteam", "^7 joined the red team.", CVAR_ARCHIVE);
	str_joinedtheblueteam = Cvar_Get("str_joinedtheblueteam", "^7 joined the blue team.", CVAR_ARCHIVE);
	str_joinedthespectators = Cvar_Get("str_joinedthespectators", "^7 joined the spectators.", CVAR_ARCHIVE);
	str_joinedthebattle = Cvar_Get("str_joinedthebattle", "^7 joined the battle.", CVAR_ARCHIVE);
	str_capturedblueflag = Cvar_Get("str_capturedblueflag", "^7 captured the ^4Blue flag!", CVAR_ARCHIVE);
	str_capturedredflag = Cvar_Get("str_capturedredflag", "^7 captured the ^1Red flag!", CVAR_ARCHIVE);
	str_hastakentheblueflag = Cvar_Get("str_hastakentheblueflag", "^7 has taken the ^4Blue^7 flag!", CVAR_ARCHIVE);
	str_hastakentheredflag = Cvar_Get("str_hastakentheredflag", "^7 has taken the ^1Red^7 flag!", CVAR_ARCHIVE);
	str_droppedtheredflag = Cvar_Get("str_droppedtheredflag", "^7 dropped the ^1Red^7 flag!", CVAR_ARCHIVE);
	str_droppedtheblueflag = Cvar_Get("str_droppedtheblueflag", "^7 dropped the ^4Blue^7 flag!", CVAR_ARCHIVE);
	str_returnedtheredflag = Cvar_Get("str_returnedtheredflag", "^7 returned the RED flag!", CVAR_ARCHIVE);
	str_returnedtheblueflag = Cvar_Get("str_returnedtheblueflag", "^7 returned the BLUE flag!", CVAR_ARCHIVE);
	str_theredflaghasreturned2 = Cvar_Get("str_theredflaghasreturned2", "^7The RED flag has returned", CVAR_ARCHIVE);
	str_theredflaghasreturned = Cvar_Get("str_theredflaghasreturned", "The ^1Red ^7flag has returned", CVAR_ARCHIVE);
	str_theblueflaghasreturned2 = Cvar_Get("str_theblueflaghasreturned2", "^7The BLUE flag has returned", CVAR_ARCHIVE);
	str_theblueflaghasreturned = Cvar_Get("str_theblueflaghasreturned", "The ^4Blue ^7flag has returned", CVAR_ARCHIVE);
	str_wasslappedbytheadmin = Cvar_Get("str_wasslappedbytheadmin", " ^7was ^3SLAPPED!^7 by the Admin", CVAR_ARCHIVE);
	str_youvebeenslapped = Cvar_Get("str_youvebeenslapped", " ^7you've been ^3SLAPPED!", CVAR_ARCHIVE);
	str_blueteamwins = Cvar_Get("str_blueteamwins", "^4Blue^7 team wins", CVAR_ARCHIVE);
	str_redteamwins = Cvar_Get("str_redteamwins", "^1Red^7 team wins", CVAR_ARCHIVE);
    
    sv_mutewords = Cvar_Get("sv_mutewords", "", CVAR_ARCHIVE);
    
    sv_attractplayers = Cvar_Get("sv_attractplayers", "0", CVAR_ARCHIVE);
    
    sv_badRconMessage = Cvar_Get ("sv_badRconMessage", "Bad rconpassword.", CVAR_ARCHIVE );
    
    sv_disableDefaultMaps = Cvar_Get("sv_disableDefaultMaps", "0", CVAR_ARCHIVE);
    
    sv_regainStamina = Cvar_Get("sv_regainStamina", "0", CVAR_ARCHIVE);
    sv_regainHealth = Cvar_Get("sv_regainHealth", "0", CVAR_ARCHIVE);
    
    sv_MedicStation = Cvar_Get ("sv_MedicStation", "0", CVAR_ARCHIVE );
    
    sv_CustomDisconnectCommand = Cvar_Get ("sv_CustomDisconnectCommand", "", CVAR_ARCHIVE );
    sv_CustomDisconnectMessage = Cvar_Get ("sv_CustomDisconnectMessage", "disconnected", CVAR_ARCHIVE );
    
    sv_callvoteRequiredConnectTime = Cvar_Get("sv_callvoteRequiredConnectTime", "0", CVAR_ARCHIVE);
    
    sv_demonotice = Cvar_Get ("sv_demonotice", "Smile! You're on camera!", CVAR_ARCHIVE);
	
	sv_callvoteCyclemapWaitTime = Cvar_Get("sv_callvoteCyclemapWaitTime", "0", CVAR_ARCHIVE);
    
    sv_limitConnectPacketsPerIP = Cvar_Get ("sv_limitConnectPacketsPerIP", "0", CVAR_ARCHIVE );
	sv_maxClientsPerIP = Cvar_Get ("sv_maxClientsPerIP", "0", CVAR_ARCHIVE );
    
    sv_reconnectWaitTime = Cvar_Get("sv_reconnectWaitTime", "0", CVAR_ARCHIVE);
    
	// initialize bot cvars so they are listed and can be set before loading the botlib
	SV_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SV_BotInitBotLib();
}


/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage( char *message ) {
	int			i, j;
	client_t	*cl;
	
	// send it twice, ignoring rate
	for ( j = 0 ; j < 2 ; j++ ) {
		for (i=0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++) {
			if (cl->state >= CS_CONNECTED) {
				// don't send a disconnect to a local client
				if ( cl->netchan.remoteAddress.type != NA_LOOPBACK ) {
					SV_SendServerCommand( cl, "print \"%s\n\"\n", message );
					SV_SendServerCommand( cl, "disconnect" );
				}
				// force a snapshot to be sent
				cl->nextSnapshotTime = -1;
				SV_SendClientSnapshot( cl );
			}
		}
	}
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown( char *finalmsg ) {
	char	playerDBPassSave[MAX_PLAYERDB_PASSWORD_STRING];

	if ( !com_sv_running || !com_sv_running->integer ) {
		return;
	}

	Com_Printf( "----- Server Shutdown (%s) -----\n", finalmsg );

    // stop server-side demos (if any)
	Cbuf_ExecuteText(EXEC_NOW, "stopserverdemo all");

	if ( svs.clients && !com_errorEntered ) {
		SV_FinalMessage( finalmsg );
	}

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	SV_ShutdownGameProgs();

	// free current level
	SV_ClearServer();

	// free server static data
	if ( svs.clients ) {
		Z_Free( svs.clients );
	}

	// SV_Shutdown() is called after 23 days.  We need to save the player database password.
	Com_Memcpy(playerDBPassSave, svs.playerDatabasePassword, MAX_PLAYERDB_PASSWORD_STRING);

	Com_Memset( &svs, 0, sizeof( svs ) );

	// Restore player database password.
	Com_Memcpy(svs.playerDatabasePassword, playerDBPassSave, MAX_PLAYERDB_PASSWORD_STRING);

	Cvar_Set( "sv_running", "0" );
	Cvar_Set("ui_singlePlayerActive", "0");

	Com_Printf( "---------------------------\n" );

	// disconnect any local clients
	if( sv_killserver->integer != 2 )
		CL_Disconnect( qfalse );
}

