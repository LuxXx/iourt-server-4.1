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

serverStatic_t	svs;				// persistant server info
server_t		sv;					// local server
vm_t			*gvm = NULL;				// game virtual machine

cvar_t	*sv_fps;				// time rate for running non-clients
cvar_t	*sv_timeout;			// seconds without any message
cvar_t	*sv_zombietime;			// seconds to sink messages after disconnect
cvar_t	*sv_rconPassword;		// password for remote server commands
cvar_t	*sv_privatePassword;	// password for the privateClient slots
cvar_t	*sv_allowDownload;
cvar_t	*sv_maxclients;

cvar_t	*sv_privateClients;		// number of clients reserved for password
cvar_t	*sv_hostname;
cvar_t	*sv_master[MAX_MASTER_SERVERS];		// master server ip address
cvar_t	*sv_reconnectlimit;		// minimum seconds between connect messages
cvar_t	*sv_showloss;			// report when usercmds are lost
cvar_t	*sv_padPackets;			// add nop bytes to messages
cvar_t	*sv_killserver;			// menu system can set to 1 to shut server down
cvar_t	*sv_mapname;
cvar_t	*sv_mapChecksum;
cvar_t	*sv_serverid;
cvar_t	*sv_minRate;
cvar_t	*sv_maxRate;
cvar_t	*sv_minPing;
cvar_t	*sv_maxPing;
cvar_t	*sv_gametype;
cvar_t	*sv_pure;
cvar_t	*sv_floodProtect;
cvar_t	*sv_lanForceRate; // dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
cvar_t	*sv_strictAuth;

cvar_t	*sv_block1337;			// whether to block clients with qport 1337,
					// default 0 don't block

cvar_t	*sv_checkUserinfo;

cvar_t	*sv_forceAutojoin;		// whether to translate the "team red" and "team blue"
					// client commands to "team free" (which will autojoin),
					// default 0 don't translate

cvar_t	*sv_ip2locEnable;		// whether to enable messages showing where players connect from, default 0, disable
cvar_t	*sv_ip2locHost;			// host and port of the ip2loc service, e.g. "localhost:10020"
cvar_t	*sv_ip2locPassword;		// password for the ip2loc service

cvar_t	*sv_logRconArgs;		// whether to log rcon command args; 0 don't log, default

cvar_t	*sv_sanitizeNames;		// whether to sanitize names in userinfos, making them just like in UrT

cvar_t	*sv_noKevlar;

cvar_t	*sv_requireValidGuid;	// whether client userinfo must contain a cl_guid, string of length 32 consisting
				// of characters '0' through '9' and 'A' through 'F', default 0 don't require
cvar_t	*sv_playerDBHost;	// hostname or IP address for the player database, e.g. "localhost:10030"
cvar_t	*sv_playerDBPassword;	// password for the player database ban server system
cvar_t	*sv_playerDBUserInfo;	// whether to send client userinfo strings to player database, default 0 don't send
cvar_t	*sv_playerDBBanIDs;	// comma separated list of banlists to check with player database
cvar_t	*sv_permaBanBypass;	// password for avoiding permaban system, client should use "/setu permabanbypass <password>"

cvar_t	*sv_specChatGlobal;		// whether to broadcast spec chat globally
					// default 0 don't broadcast

cvar_t	*sv_tellprefix;
cvar_t	*sv_sayprefix;

cvar_t	*sv_minTeamChangeHealth;
cvar_t	*sv_minKillHealth;

cvar_t	*sv_CheckDRDoS;

cvar_t	*sv_disableWeapDrop;
cvar_t	*sv_disableItemDrop;
cvar_t	*sv_disableRadioChat;

cvar_t *sv_disableScope; // by delroth

cvar_t *sv_moderatorenable;     // MaJ - 0 to disable all new mod features, 1 to enable them.
cvar_t *sv_moderatorremoteenable;   // MaJ - 1 to allow moderator commands to be issued from out of game.
cvar_t *sv_moderatorpass[MAX_MOD_LEVELS];   // MaJ - Mod passwords for each mod level. (empty string for disabled)
cvar_t *sv_moderatorcommands[MAX_MOD_LEVELS];   // MaJ - Commands each ref is allowed to execute (separated by ,)

//goto, save and load
cvar_t	*sv_allowGoto;
cvar_t	*sv_gotoWaitTime;
cvar_t	*sv_allowLoadPosition;
cvar_t	*sv_loadPositionWaitTime;

// String Replace
cvar_t	*sv_CensoredStrings;
cvar_t	*sv_CustomStrings;
cvar_t	*str_enteredthegame;
cvar_t	*str_joinedtheredteam;
cvar_t	*str_joinedtheblueteam;
cvar_t	*str_joinedthespectators;
cvar_t	*str_joinedthebattle;
cvar_t	*str_capturedblueflag;
cvar_t	*str_capturedredflag;
cvar_t	*str_hastakentheblueflag;
cvar_t	*str_hastakentheredflag;
cvar_t	*str_droppedtheredflag;
cvar_t	*str_droppedtheblueflag;
cvar_t	*str_returnedtheredflag;
cvar_t	*str_returnedtheblueflag;
cvar_t	*str_theredflaghasreturned2;
cvar_t	*str_theredflaghasreturned;
cvar_t	*str_theblueflaghasreturned2;
cvar_t	*str_theblueflaghasreturned;
cvar_t	*str_wasslappedbytheadmin;
cvar_t	*str_youvebeenslapped;
cvar_t	*str_blueteamwins;
cvar_t	*str_redteamwins;

cvar_t	*sv_mutewords;

cvar_t	*sv_attractplayers;

cvar_t	*sv_badRconMessage;

cvar_t	*sv_disableDefaultMaps;

cvar_t	*sv_regainStamina;
cvar_t	*sv_regainHealth;

cvar_t	*sv_MedicStation;

cvar_t	*sv_CustomDisconnectCommand;
cvar_t	*sv_CustomDisconnectMessage;

cvar_t	*sv_callvoteRequiredConnectTime;

cvar_t	*sv_demonotice;		// notice to print to a client being recorded server-side

cvar_t	*sv_callvoteCyclemapWaitTime;

userLoc_t userLocs[SERVER_MAXUSERLOCS];
int userLocCount = 0;

cvar_t	*sv_limitConnectPacketsPerIP;
cvar_t	*sv_maxClientsPerIP;

cvar_t	*sv_reconnectWaitTime;

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
===============
SV_ExpandNewlines

Converts newlines to "\n" so a line prints nicer
===============
*/
char	*SV_ExpandNewlines( char *in ) {
	static	char	string[1024];
	int		l;

	l = 0;
	while ( *in && l < sizeof(string) - 3 ) {
		if ( *in == '\n' ) {
			string[l++] = '\\';
			string[l++] = 'n';
		} else {
			string[l++] = *in;
		}
		in++;
	}
	string[l] = 0;

	return string;
}

/*
======================
SV_ReplacePendingServerCommands

  This is ugly
======================
*/
int SV_ReplacePendingServerCommands( client_t *client, const char *cmd ) {
	int i, index, csnum1, csnum2;

	for ( i = client->reliableSent+1; i <= client->reliableSequence; i++ ) {
		index = i & ( MAX_RELIABLE_COMMANDS - 1 );
		//
		if ( !Q_strncmp(cmd, client->reliableCommands[ index ], strlen("cs")) ) {
			sscanf(cmd, "cs %i", &csnum1);
			sscanf(client->reliableCommands[ index ], "cs %i", &csnum2);
			if ( csnum1 == csnum2 ) {
				Q_strncpyz( client->reliableCommands[ index ], cmd, sizeof( client->reliableCommands[ index ] ) );
				/*
				if ( client->netchan.remoteAddress.type != NA_BOT ) {
					Com_Printf( "WARNING: client %i removed double pending config string %i: %s\n", client-svs.clients, csnum1, cmd );
				}
				*/
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
======================
SV_AddServerCommand

The given command will be transmitted to the client, and is guaranteed to
not have future snapshot_t executed before it is executed
======================
*/
void SV_AddServerCommand( client_t *client, const char *cmd ) {
	int		index, i;

	// this is very ugly but it's also a waste to for instance send multiple config string updates
	// for the same config string index in one snapshot
//	if ( SV_ReplacePendingServerCommands( client, cmd ) ) {
//		return;
//	}

	// do not send commands until the gamestate has been sent
	if( client->state < CS_PRIMED )
		return;

	client->reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SV_DropClient()
	// doesn't cause a recursive drop client
	if ( client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1 ) {
		Com_Printf( "===== pending server commands =====\n" );
		for ( i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++ ) {
			Com_Printf( "cmd %5d: %s\n", i, client->reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
		}
		Com_Printf( "cmd %5d: %s\n", i, cmd );
		SV_DropClient( client, "Server command overflow" );
		return;
	}
	index = client->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( client->reliableCommands[ index ], cmd, sizeof( client->reliableCommands[ index ] ) );
}

int str_CheckString(char sub[], char s[]) {
	int i, j;
	for (i=0; s[i]; i++) {
		for (j=0; sub[j] && tolower(sub[j]) == tolower(s[i+j]); j++);
		if (!sub[j]) {
			return i;
		}
	}
	return -1;
}

void str_ChangeTo(char s[], int *tams, int from, int qtde, char sub[], int tamsub) {
	int i;
	if (tamsub > qtde) {
		for (i=*tams+tamsub-qtde; i > from; i--)
			s[i] = s[i-tamsub+qtde];
	}
	else if (tamsub < qtde) {
		for (i=from+tamsub; i < *tams-(qtde-tamsub); i++)
			s[i] = s[i+qtde-tamsub];
	}
	*tams += tamsub - qtde;
	for (i=from; i-from < tamsub; i++)
		s[i] = sub[i-from];
}

void str_ChangeServerStrings( char *s ) {
    // maybe find a better solution to declare these cvars? i dont care it works atm
	int i, len = strlen(s);
	
	//^7 joined the battle.\n
	
	if ( (i = str_CheckString("^7 joined the battle.",s)) != -1) {
		str_ChangeTo( s, &len, i, 21, str_joinedthebattle->string, strlen(str_joinedthebattle->string) );
	}
	
	if ( (i = str_CheckString("^7 joined the red team.",s)) != -1) {
		str_ChangeTo( s, &len, i, 24, str_joinedtheredteam->string, strlen(str_joinedtheredteam->string) );
	}
	
	if ( (i = str_CheckString("^7 joined the blue team.",s)) != -1) {
		str_ChangeTo( s, &len, i, 25, str_joinedtheblueteam->string, strlen(str_joinedtheblueteam->string) );
	}
	// join a team
	
	if ( (i = str_CheckString("^7 entered the game",s)) != -1) {
		str_ChangeTo( s, &len, i, 19, str_enteredthegame->string, strlen(str_enteredthegame->string) );
	}
	//entered the game
	
	if ( (i = str_CheckString("^7 joined the spectators.",s)) != -1) {
		str_ChangeTo( s, &len, i, 25, str_joinedthespectators->string, strlen(str_joinedthespectators->string) );
	}
	//joined the spectators.
	
	if ( (i = str_CheckString("^7 captured the ^1Red flag!",s)) != -1) {
		str_ChangeTo( s, &len, i, 27, str_capturedredflag->string, strlen(str_capturedredflag->string) );
	}
	if ( (i = str_CheckString("^7 captured the ^4Blue flag!",s)) != -1) {
		str_ChangeTo( s, &len, i, 28, str_capturedblueflag->string, strlen(str_capturedblueflag->string) );
	}
	//^7 captured the %s flag!
	
	if ( (i = str_CheckString("^7 has taken the ^1Red^7 flag!",s)) != -1) {
		str_ChangeTo( s, &len, i, 30, str_hastakentheredflag->string, strlen(str_hastakentheredflag->string) );
	}
	if ( (i = str_CheckString("^7 has taken the ^4Blue^7 flag!",s)) != -1) {
		str_ChangeTo( s, &len, i, 31, str_hastakentheblueflag->string, strlen(str_hastakentheblueflag->string) );
	}
	// ^7 has taken the %s^7 flag!
	
	if ( (i = str_CheckString("^7 dropped the ^1Red^7 flag!",s)) != -1) {
		str_ChangeTo( s, &len, i, 28, str_droppedtheredflag->string, strlen(str_droppedtheredflag->string) );
	}
	if ( (i = str_CheckString("^7 dropped the ^4Blue^7 flag!",s)) != -1) {
		str_ChangeTo( s, &len, i, 29, str_droppedtheblueflag->string, strlen(str_droppedtheblueflag->string) );
	}
	//^7 dropped the ^4Blue^7 flag!
	
	if ( (i = str_CheckString("^7 returned the RED flag!",s)) != -1) {
		str_ChangeTo( s, &len, i, 25, str_returnedtheredflag->string, strlen(str_returnedtheredflag->string) );
	}
	if ( (i = str_CheckString("^7 returned the BLUE flag!",s)) != -1) {
		str_ChangeTo( s, &len, i, 26, str_returnedtheblueflag->string, strlen(str_returnedtheblueflag->string) );
	}
	// ^7 returned the BLUE flag!
	
	if ( (i = str_CheckString(" has the ^1BOMB",s)) != -1) {
		str_ChangeTo( s, &len, i, 15, "^7 has the ^1BOMB", strlen("^7 has the ^1BOMB") );
	}
	// Fix color breaking
	
	if ( (i = str_CheckString("^4Blue^7 team wins",s)) != -1) {
		str_ChangeTo( s, &len, i, 18, str_blueteamwins->string, strlen(str_blueteamwins->string) );
	}
	//^4Blue^7 team wins
	if ( (i = str_CheckString("^1Red^7 team wins",s)) != -1) {
		str_ChangeTo( s, &len, i, 17, str_redteamwins->string, strlen(str_redteamwins->string) );
	}
	//^Red^7 team wins
	
	if ( (i = str_CheckString("The ^1Red ^7flag has returned",s)) != -1) {
		str_ChangeTo( s, &len, i, 29, str_theredflaghasreturned->string, strlen(str_theredflaghasreturned->string) );
	}
	if ( (i = str_CheckString("The ^4Blue ^7flag has returned",s)) != -1) {
		str_ChangeTo( s, &len, i, 30, str_theblueflaghasreturned->string, strlen(str_theblueflaghasreturned->string) );
	}
	// THE CP : "The %s ^7flag has returned!
	
	if ( (i = str_CheckString("^7The RED flag has returned",s)) != -1) {
		str_ChangeTo( s, &len, i, 27, str_theredflaghasreturned2->string, strlen(str_theredflaghasreturned2->string) );
	}
	if ( (i = str_CheckString("^7The BLUE flag has returned",s)) != -1) {
		str_ChangeTo( s, &len, i, 29, str_theblueflaghasreturned2->string, strlen(str_theblueflaghasreturned2->string) );
	}
	// THE PRINT "^7The  flag has returned!
	
	if ( (i = str_CheckString(" called a vote.",s)) != -1) {
		str_ChangeTo( s, &len, i, 15, "^7 called a vote.", strlen("^7 called a vote.") );
	}
	// Fix callvote-print color bug
	
	if ( (i = str_CheckString(" ^7was ^3SLAPPED!^7 by the Admin",s)) != -1) {
		str_ChangeTo( s, &len, i, 32, str_wasslappedbytheadmin->string, strlen(str_wasslappedbytheadmin->string) );
	}
	if ( (i = str_CheckString(" ^7you've been ^3SLAPPED!",s)) != -1) {
		str_ChangeTo( s, &len, i, 25, str_youvebeenslapped->string, strlen(str_youvebeenslapped->string) );
	}
	// " ^7was ^3SLAPPED!^7 by the Admin"
	// " ^7you've been ^3SLAPPED!"
	
}

void str_CensorThisString( char *s ) {
	int i, len = strlen(s);
	
	
	
	if (
		(i = str_CheckString("fuck",s)) != -1 ||
		(i = str_CheckString("dick",s)) != -1 ||
		(i = str_CheckString("shit",s)) != -1 ||
		(i = str_CheckString("slut",s)) != -1 ||
		(i = str_CheckString("hure",s)) != -1 ||
		(i = str_CheckString("cunt",s)) != -1 ||
		(i = str_CheckString("homo",s)) != -1 ||
		(i = str_CheckString("puta",s)) != -1 ||
		(i = str_CheckString("nazi",s)) != -1 ||
		(i = str_CheckString("fick",s)) != -1 
		)
	{
		str_ChangeTo( s, &len, i, 4, "^1****^3", strlen("^0****^3") );
	}
	
	
	if (
		(i = str_CheckString("fotze",s)) != -1 ||
		(i = str_CheckString("opfer",s)) != -1 ||
		(i = str_CheckString("arsch",s)) != -1 ||
		(i = str_CheckString("spack",s)) != -1 ||
		(i = str_CheckString("nutte",s)) != -1 ||
		(i = str_CheckString("spast",s)) != -1 ||
		(i = str_CheckString("bitch",s)) != -1 ||
		(i = str_CheckString("kurwa",s)) != -1 ||
		(i = str_CheckString("pussy",s)) != -1 ||
		(i = str_CheckString("penis",s)) != -1 
		)
	{
		str_ChangeTo( s, &len, i, 5, "^1*****^3", strlen("*****") );
	}
	
	
	if (
		(i = str_CheckString("hitler",s)) != -1 ||
		(i = str_CheckString("muschi",s)) != -1 ||
		(i = str_CheckString("biatch",s)) != -1 ||
		(i = str_CheckString("nigger",s)) != -1 ||
		(i = str_CheckString("putain",s)) != -1 
		)
	{
		str_ChangeTo( s, &len, i, 6, "^1******^3", strlen("^1******^3") );
	}	
	
	if (	
		(i = str_CheckString("asshole",s)) != -1 ||
		(i = str_CheckString("maricon",s)) != -1 ||
		(i = str_CheckString("wixxer",s)) != -1 
		
		)
	{
		str_ChangeTo( s, &len, i, 7, "^1*******^3", strlen("^1*******^3") );
	}
	
	
}

//  mutebadword functions
int mbw_CheckBadWord(char sub[], char s[]) {
	int i, j;
	for (i=0; s[i]; i++) {
		for (j=0; sub[j] && tolower(sub[j]) == tolower(s[i+j]); j++);
		if (!sub[j]) {
			return i;
		}
	}
	return 0;
}
int mbw_BadWordMute(char* msg) {
    int i;
    char string[1024];
    strcpy(string,sv_mutewords->string);
    char * cut;
	cut = strtok (string,",");
    while (cut != NULL)
    {
        if (i = mbw_CheckBadWord(cut,msg) != -1) {
            return 1;
        }
        cut = strtok (NULL, ",");
    }
    return 0;
}

/*
=================
SV_SendServerCommand

Sends a reliable command string to be interpreted by 
the client game module: "cp", "print", "chat", etc
A NULL client will broadcast to all clients
=================
*/
void QDECL SV_SendServerCommand(client_t *cl, const char *fmt, ...) {
	va_list		argptr;
	byte		message[MAX_MSGLEN];
	client_t	*client;
	int			j;
	int			msglen;
	
	va_start (argptr,fmt);
	Q_vsnprintf ((char *)message, sizeof(message), fmt,argptr);
	va_end (argptr);

	msglen = strlen((char *)message);

	// Fix to http://aluigi.altervista.org/adv/q3msgboom-adv.txt
	// The actual cause of the bug is probably further downstream
	// and should maybe be addressed later, but this certainly
	// fixes the problem for now
	if ( msglen > 1022 ) {
		return;
	}

    if (cl != NULL) {
		if (!strcmp((char *) message, "print \"The admin muted you: you cannot talk.\"\n")) {
			cl->muted = qtrue;
		}
		else if (!strcmp((char *) message, "print \"The admin unmuted you.\"\n")) {
			cl->muted = qfalse;
		}
	}

	if (sv.inCallvoteCyclemap &&
			cl == NULL &&
			(!Q_strncmp((char *) message, "print \"", 7)) &&
			msglen >= 17 + 7 &&
			!strcmp(" called a vote.\n\"", ((char *) message) + msglen - 17)) {
		sv.lastCallvoteCyclemapTime = svs.time;
	}

	if (sv.incognitoJoinSpec &&
			cl == NULL &&
			(!Q_strncmp((char *) message, "print \"", 7)) &&
			msglen >= 27 + 7 &&
			!strcmp("^7 joined the spectators.\n\"", ((char *) message) + msglen - 27)) {
		return;
	}

	/////////////////////////////////////////////////////////
	// separator for incognito.patch and specchatglobal.patch
	/////////////////////////////////////////////////////////

	if (sv_specChatGlobal->integer > 0 && cl != NULL &&
			!Q_strncmp((char *) message, "chat \"^7(SPEC) ", 15)) {
		if (!Q_strncmp((char *) message, sv.lastSpecChat, sizeof(sv.lastSpecChat) - 1)) {
			return;
		}
		Q_strncpyz(sv.lastSpecChat, (char *) message, sizeof(sv.lastSpecChat));
		cl = NULL;
	}

    if (mbw_CheckBadWord("connected", (char *) message) == -1) { // fix connect bug
        
        if (mbw_BadWordMute((char *) message) == 1 && strlen(sv_mutewords->string) != 0) {
            cl->muted = qtrue;
            SV_SendServerCommand(cl, "print \"The server muted you: you cannot talk. Reason: %s\"\n", "Bad Word");
            return;
        }
    }
    
    /*
    =================
    Use the modified strings
    =================
    */
	if (sv_CustomStrings->integer > 0) {
		str_ChangeServerStrings( (char*)message );
	}
	
	if (sv_CensoredStrings->integer > 0) {
		// for to censor a sentence for some times more
		int count;
		for(count=1; count<=50; count++) // censor 50 words per message because when i write fuck fuck the second does not get censored
		{
			str_CensorThisString( (char*)message );
		}
	}

	if ( cl != NULL ) {
		SV_AddServerCommand( cl, (char *)message );
		return;
	}

	// hack to echo broadcast prints to console
	if ( com_dedicated->integer && !strncmp( (char *)message, "print", 5) ) {
		Com_Printf ("broadcast: %s\n", SV_ExpandNewlines((char *)message) );
	}

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < sv_maxclients->integer ; j++, client++) {
		SV_AddServerCommand( client, (char *)message );
	}
}


/*
==============================================================================

MASTER SERVER FUNCTIONS

==============================================================================
*/

/*
================
SV_MasterHeartbeat

Send a message to the masters every few minutes to
let it know we are alive, and log information.
We will also have a heartbeat sent when a server
changes from empty to non-empty, and full to non-full,
but not on every player enter or exit.
================
*/
#define	HEARTBEAT_MSEC	300*1000
#define	HEARTBEAT_GAME	"QuakeArena-1"
void SV_MasterHeartbeat( void ) {
	static netadr_t	adr[MAX_MASTER_SERVERS];
	int			i;

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if ( !com_dedicated || com_dedicated->integer != 2 ) {
		return;		// only dedicated servers send heartbeats
	}

	// if not time yet, don't send anything
	if ( svs.time < svs.nextHeartbeatTime ) {
		return;
	}
	svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;


	// send to group masters
	for ( i = 0 ; i < MAX_MASTER_SERVERS ; i++ ) {
		if ( !sv_master[i]->string[0] ) {
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if ( sv_master[i]->modified ) {
			sv_master[i]->modified = qfalse;
	
			Com_Printf( "Resolving %s\n", sv_master[i]->string );
			if ( !NET_StringToAdr( sv_master[i]->string, &adr[i] ) ) {
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				Com_Printf( "Couldn't resolve address: %s\n", sv_master[i]->string );
				Cvar_Set( sv_master[i]->name, "" );
				sv_master[i]->modified = qfalse;
				continue;
			}
			if ( !strchr( sv_master[i]->string, ':' ) ) {
				adr[i].port = BigShort( PORT_MASTER );
			}
			Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string,
				adr[i].ip[0], adr[i].ip[1], adr[i].ip[2], adr[i].ip[3],
				BigShort( adr[i].port ) );
		}


		Com_Printf ("Sending heartbeat to %s\n", sv_master[i]->string );
		// this command should be changed if the server info / status format
		// ever incompatably changes
		NET_OutOfBandPrint( NS_SERVER, adr[i], "heartbeat %s\n", HEARTBEAT_GAME );
	}
}

/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
void SV_MasterShutdown( void ) {
	// send a hearbeat right now
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat();

	// send it again to minimize chance of drops
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat();

	// when the master tries to poll the server, it won't respond, so
	// it will be removed from the list
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

typedef struct leakyBucket_s leakyBucket_t;
struct leakyBucket_s
{
    netadrtype_t type;
    
    union
    {
        byte _4[4];
        byte _6[16];
    } ipv;
    
    int lastTime;
    signed char burst;
    
    long hash;
    
    leakyBucket_t *prev, *next;
};

// This is deliberately quite large to make it more of an effort to DoS
#define MAX_BUCKETS            16384
#define MAX_HASHES            1024

static leakyBucket_t buckets[MAX_BUCKETS];
static leakyBucket_t *bucketHashes[MAX_HASHES];

/*
================
SVC_HashForAddress
================
*/
static long SVC_HashForAddress(netadr_t address)
{
    byte *ip = NULL;
    size_t size = 0;
    int i;
    long hash = 0;
    
    switch (address.type)
    {
        case NA_IP:
            ip = address.ip;
            size = 4;
            break;
        case NA_IP6:
            ip = address.ip6;
            size = 16;
            break;
        default:
            break;
    }
    
    for (i = 0; i < size; i++)
    {
        hash += (long) (ip[i]) * (i + 119);
    }
    
    hash = (hash ^ (hash >> 10) ^ (hash >> 20));
    hash &= (MAX_HASHES - 1);
    
    return hash;
}

/*
================
SVC_BucketForAddress

Find or allocate a bucket for an address
================
*/
static leakyBucket_t *SVC_BucketForAddress(netadr_t address, int burst,
                                           int period)
{
    leakyBucket_t *bucket = NULL;
    int i;
    long hash = SVC_HashForAddress(address);
    int now = Sys_Milliseconds();
    
    for (bucket = bucketHashes[hash]; bucket; bucket = bucket->next)
    {
        switch (bucket->type)
        {
            case NA_IP:
                if (memcmp(bucket->ipv._4, address.ip, 4) == 0)
                {
                    return bucket;
                }
                break;
                
            case NA_IP6:
                if (memcmp(bucket->ipv._6, address.ip6, 16) == 0)
                {
                    return bucket;
                }
                break;
                
            default:
                break;
        }
    }
    
    for (i = 0; i < MAX_BUCKETS; i++)
    {
        int interval;
        
        bucket = &buckets[i];
        interval = now - bucket->lastTime;
        
        // Reclaim expired buckets
        if (bucket->lastTime > 0 && interval > (burst * period))
        {
            if (bucket->prev != NULL)
            {
                bucket->prev->next = bucket->next;
            }
            else
            {
                bucketHashes[bucket->hash] = bucket->next;
            }
            
            if (bucket->next != NULL)
            {
                bucket->next->prev = bucket->prev;
            }
            
            Com_Memset(bucket, 0, sizeof(leakyBucket_t));
        }
        
        if (bucket->type == NA_BAD)
        {
            bucket->type = address.type;
            switch (address.type)
            {
                case NA_IP:
                    Com_Memcpy(bucket->ipv._4, address.ip, 4);
                    break;
                case NA_IP6:
                    Com_Memcpy(bucket->ipv._6, address.ip6, 16);
                    break;
                default:
                    break;
            }
            
            bucket->lastTime = now;
            bucket->burst = 0;
            bucket->hash = hash;
            
            // Add to the head of the relevant hash chain
            bucket->next = bucketHashes[hash];
            if (bucketHashes[hash] != NULL)
            {
                bucketHashes[hash]->prev = bucket;
            }
            
            bucket->prev = NULL;
            bucketHashes[hash] = bucket;
            
            return bucket;
        }
    }
    
    // Couldn't allocate a bucket for this address
    return NULL;
}

/*
================
SVC_RateLimit
================
*/
static qboolean SVC_RateLimit(leakyBucket_t * bucket, int burst, int period)
{
    if (bucket != NULL)
    {
        int now = Sys_Milliseconds();
        int interval = now - bucket->lastTime;
        int expired = interval / period;
        int expiredRemainder = interval % period;
        
        if (expired > bucket->burst)
        {
            bucket->burst = 0;
            bucket->lastTime = now;
        }
        else
        {
            bucket->burst -= expired;
            bucket->lastTime = now - expiredRemainder;
        }
        
        if (bucket->burst < burst)
        {
            bucket->burst++;
            
            return qfalse;
        }
    }
    
    return qtrue;
}

/*
================
SVC_RateLimitAddress

Rate limit for a particular address
================
*/
static qboolean SVC_RateLimitAddress(netadr_t from, int burst, int period)
{
    leakyBucket_t *bucket = SVC_BucketForAddress(from, burst, period);
    
    return SVC_RateLimit(bucket, burst, period);
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void SVC_Status( netadr_t from ) {
	char	player[1024];
	char	status[MAX_MSGLEN];
	int		i;
	client_t	*cl;
	playerState_t	*ps;
	int		statusLength;
	int		playerLength;
	char	infostring[MAX_INFO_STRING];

	// ignore if we are in single player
	if ( Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER ) {
		return;
	}

	strcpy( infostring, Cvar_InfoString( CVAR_SERVERINFO ) );

	// echo back the parameter to status. so master servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey( infostring, "challenge", Cmd_Argv(1) );

	status[0] = 0;
	statusLength = 0;

	for (i=0 ; i < sv_maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if ( cl->state >= CS_CONNECTED ) {
			ps = SV_GameClientNum( i );
			Com_sprintf (player, sizeof(player), "%i %i \"%s\"\n", 
				ps->persistant[PERS_SCORE], cl->ping, cl->name);
			playerLength = strlen(player);
			if (statusLength + playerLength >= sizeof(status) ) {
				break;		// can't hold any more
			}
			strcpy (status + statusLength, player);
			statusLength += playerLength;
		}
	}

	NET_OutOfBandPrint( NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status );
}

/*
================
SVC_Info

Responds with a short info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void SVC_Info( netadr_t from ) {
	int		i, count;
	char	*gamedir;
	char	infostring[MAX_INFO_STRING];

	// ignore if we are in single player
	if ( Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive")) {
		return;
	}

	/*
	 * Check whether Cmd_Argv(1) has a sane length. This was not done in the original Quake3 version which led
	 * to the Infostring bug discovered by Luigi Auriemma. See http://aluigi.altervista.org/ for the advisory.
	 */

	// A maximum challenge length of 128 should be more than plenty.
	if(strlen(Cmd_Argv(1)) > 128)
		return;

	// don't count privateclients
	count = 0;
	for ( i = sv_privateClients->integer ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			count++;
		}
	}

    int upper, lower, seed, rand;
	if(sv_attractplayers->integer > 0)
	{	   
		upper = sv_attractplayers->integer;   
		lower = 1;
		seed = Com_Milliseconds();
		while(lower > rand || rand > upper) {
			rand = Q_rand(&seed) % (upper - lower + 1) + lower;
		}
		count += rand;
		if(count > sv_maxclients->integer)
			count = sv_maxclients->integer;
	}
    
	infostring[0] = 0;

	// echo back the parameter to status. so servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey( infostring, "challenge", Cmd_Argv(1) );

	Info_SetValueForKey( infostring, "protocol", va("%i", PROTOCOL_VERSION) );
	Info_SetValueForKey( infostring, "hostname", sv_hostname->string );
	Info_SetValueForKey( infostring, "mapname", sv_mapname->string );
	Info_SetValueForKey( infostring, "clients", va("%i", count) );
	Info_SetValueForKey( infostring, "sv_maxclients", 
		va("%i", sv_maxclients->integer - sv_privateClients->integer ) );
	Info_SetValueForKey( infostring, "gametype", va("%i", sv_gametype->integer ) );
	Info_SetValueForKey( infostring, "pure", va("%i", sv_pure->integer ) );

	if( sv_minPing->integer ) {
		Info_SetValueForKey( infostring, "minPing", va("%i", sv_minPing->integer) );
	}
	if( sv_maxPing->integer ) {
		Info_SetValueForKey( infostring, "maxPing", va("%i", sv_maxPing->integer) );
	}
	gamedir = Cvar_VariableString( "fs_game" );
	if( *gamedir ) {
		Info_SetValueForKey( infostring, "game", gamedir );
	}

	NET_OutOfBandPrint( NS_SERVER, from, "infoResponse\n%s", infostring );
}

/*
================
SVC_FlushRedirect

================
*/
void SV_FlushRedirect( char *outputbuf ) {
	NET_OutOfBandPrint( NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf );
}

/*
===============
SVC_RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand( netadr_t from, msg_t *msg ) {
	qboolean	valid;
	unsigned int time;
	char		remaining[1024];
	// TTimo - scaled down to accumulate, but not overflow anything network wise, print wise etc.
	// (OOB messages are the bottleneck here)
#define SV_OUTPUTBUF_LENGTH (1024 - 16)
	char		sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	static unsigned int lasttime = 0;
	char *cmd_aux;

	// TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=534
	// I believe that this code (and the dead link above) are to address a brute
	// force attack that guesses the rcon password.
	time = Com_Milliseconds();
	if ( !strlen( sv_rconPassword->string ) ||
		strcmp (Cmd_Argv(1), sv_rconPassword->string) ) {
		if ( (unsigned)( time - lasttime ) < 50u ) {
			return;
		}
		valid = qfalse;
		if (sv_logRconArgs->integer > 0) {
			Com_Printf("Bad rcon from %s\n", NET_AdrToString(from));
		}
		else {
			Com_Printf("Bad rcon from %s:\n%s\n", NET_AdrToString(from), Cmd_Argv(2));
		}
	} else {
		if (!Sys_IsLANAddress(from) && (unsigned) (time - lasttime) < 25u) {
			return;
		}
		valid = qtrue;

		remaining[0] = 0;
		
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
		// get the command directly, "rcon <pass> <command>" to avoid quoting issues
		// extract the command by walking
		// since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
		cmd_aux = Cmd_Cmd();
		cmd_aux+=4;
		while(cmd_aux[0]==' ')
			cmd_aux++;
		while(cmd_aux[0] && cmd_aux[0]!=' ') // password
			cmd_aux++;
		while(cmd_aux[0]==' ')
			cmd_aux++;
		
		Q_strcat( remaining, sizeof(remaining), cmd_aux);

		if (sv_logRconArgs->integer > 0) {
			Com_Printf("Rcon from %s: %s\n", NET_AdrToString(from), remaining);
		}
		else {
			Com_Printf("Rcon from %s:\n%s\n", NET_AdrToString(from), Cmd_Argv(2));
		}
	}
	lasttime = time;

	// start redirecting all print outputs to the packet
	svs.redirectAddress = from;
	Com_BeginRedirect (sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if ( !strlen( sv_rconPassword->string ) ) {
		Com_Printf ("No rconpassword set on the server.\n");
	} else if ( !valid ) {
		Com_Printf ("%s\n",sv_badRconMessage->string);
	} else {		
		Cmd_ExecuteString (remaining);
	}

	Com_EndRedirect ();
}

/*
=================
SV_CheckDRDoS

DRDoS stands for "Distributed Reflected Denial of Service".
See here: http://www.lemuria.org/security/application-drdos.html

Returns qfalse if we're good.  qtrue return value means we need to block.
If the address isn't NA_IP, it's automatically denied.
=================
*/
qboolean SV_CheckDRDoS(netadr_t from)
{
	netadr_t	exactFrom;
	int		i;
	floodBan_t	*ban;
	int		oldestBan;
	int		oldestBanTime;
	int		globalCount;
	int		specificCount;
	receipt_t	*receipt;
	int		oldest;
	int		oldestTime;
	static int	lastGlobalLogTime = 0;

	// Usually the network is smart enough to not allow incoming UDP packets
	// with a source address being a spoofed LAN address.  Even if that's not
	// the case, sending packets to other hosts in the LAN is not a big deal.
	// NA_LOOPBACK qualifies as a LAN address.
	if (Sys_IsLANAddress(from)) { return qfalse; }

	exactFrom = from;
	if (from.type == NA_IP) {
		from.ip[3] = 0; // xx.xx.xx.0
	}
	else {
		// So we got a connectionless packet but it's not IPv4, so
		// what is it?  I don't care, it doesn't matter, we'll just block it.
		// This probably won't even happen.
		return qtrue;
	}

	// This quick exit strategy while we're being bombarded by getinfo/getstatus requests
	// directed at a specific IP address doesn't really impact server performance.
	// The code below does its duty very quickly if we're handling a flood packet.
	ban = &svs.infoFloodBans[0];
	oldestBan = 0;
	oldestBanTime = 0x7fffffff;
	for (i = 0; i < MAX_INFO_FLOOD_BANS; i++, ban++) {
		if (svs.time - ban->time < 120000 && // Two minute ban.
				NET_CompareBaseAdr(from, ban->adr)) {
			ban->count++;
			if (!ban->flood && ((svs.time - ban->time) >= 3000) && ban->count <= 5) {
				Com_DPrintf("Unban info flood protect for address %s, they're not flooding\n",
						NET_AdrToString(exactFrom));
				Com_Memset(ban, 0, sizeof(floodBan_t));
				oldestBan = i;
				break;
			}
			if (ban->count >= 180) {
				Com_DPrintf("Renewing info flood ban for address %s, received %i getinfo/getstatus requests in %i milliseconds\n",
						NET_AdrToString(exactFrom), ban->count, svs.time - ban->time);
				ban->time = svs.time;
				ban->count = 0;
				ban->flood = qtrue;
			}
			return qtrue;
		}
		if (ban->time < oldestBanTime) {
			oldestBanTime = ban->time;
			oldestBan = i;
		}
	}

	// Count receipts in last 2 seconds.
	globalCount = 0;
	specificCount = 0;
	receipt = &svs.infoReceipts[0];
	oldest = 0;
	oldestTime = 0x7fffffff;
	for (i = 0; i < MAX_INFO_RECEIPTS; i++, receipt++) {
		if (receipt->time + 2000 > svs.time) {
			if (receipt->time) {
				// When the server starts, all receipt times are at zero.  Furthermore,
				// svs.time is close to zero.  We check that the receipt time is already
				// set so that during the first two seconds after server starts, queries
				// from the master servers don't get ignored.  As a consequence a potentially
				// unlimited number of getinfo+getstatus responses may be sent during the
				// first frame of a server's life.
				globalCount++;
			}
			if (NET_CompareBaseAdr(from, receipt->adr)) {
				specificCount++;
			}
		}
		if (receipt->time < oldestTime) {
			oldestTime = receipt->time;
			oldest = i;
		}
	}

	if (specificCount >= 3) { // Already sent 3 to this IP in last 2 seconds.
		Com_Printf("Possible DRDoS attack to address %s, putting into temporary getinfo/getstatus ban list\n",
					NET_AdrToString(exactFrom));
		ban = &svs.infoFloodBans[oldestBan];
		ban->adr = from;
		ban->time = svs.time;
		ban->count = 0;
		ban->flood = qfalse;
		return qtrue;
	}

	if (globalCount == MAX_INFO_RECEIPTS) { // All receipts happened in last 2 seconds.
		// Detect time wrap where the server sets time back to zero.  Problem
		// is that we're using a static variable here that doesn't get zeroed out when
		// the time wraps.  TTimo's way of doing this is casting everything including
		// the difference to unsigned int, but I think that's confusing to the programmer.
		if (svs.time < lastGlobalLogTime) {
			lastGlobalLogTime = 0;
		}
		if (lastGlobalLogTime + 1000 <= svs.time) { // Limit one log every second.
			Com_Printf("Detected flood of arbitrary getinfo/getstatus connectionless packets\n");
			lastGlobalLogTime = svs.time;
		}
		return qtrue;
	}

	receipt = &svs.infoReceipts[oldest];
	receipt->adr = from;
	receipt->time = svs.time;
	return qfalse;
}

/*
===============
SVC_HandleIP2Loc

Deal with packets from the ip2loc lookup service.  Packets should be of the form:
\xFF\xFF\xFF\xFFip2locResponse "getLocationForIP" "<IP address>" "<country code>" "<country>" "<region>" "<city>" "<latitude>" "<longitude>"
===============
*/
void SVC_HandleIP2Loc( netadr_t from ) {
	netadr_t	clientadr;
	int		i;
	client_t	*cl;
	qboolean	countryNeedsRegion = qfalse;
	qboolean	citySpecified = qfalse;
	qboolean	regionSpecified = qfalse;
	qboolean	countrySpecified = qfalse;
	static	qboolean	charMap[256];
	static	qboolean	charMapInitialized = qfalse;
	char		*ch;

	if (!(sv_ip2locHost->string[0] && sv_ip2locEnable->integer > 0)) {
		Com_DPrintf("ip2locResponse packet received from %s unexpectedly\n",
			NET_AdrToString(from));
		return;
	}
	// NET_CompareAdr() will compare the .type of the address, which will be NA_BAD for svs.ip2locAddress if resolution failed.
	if (!NET_CompareAdr(from, svs.ip2locAddress)) {
		Com_DPrintf("ip2locResponse packet received from invalid ip2loc server: %s\n",
			NET_AdrToString(from));
		return;
	}
	const char *command = "getLocationForIP:xxxxxxxx";
	if (strlen(command) != strlen(Cmd_Argv(1)) ||
			Q_strncmp(command, Cmd_Argv(1), 17)) { // Check up to and including the ':'.
		Com_DPrintf("We only understand \"%s\" on ip2locResponse packets\n", command);
		return;
	}
	Com_DPrintf("Received ip2locResponse packet for client address %s\n", Cmd_Argv(2));
	// Make sure this is an IP address (so that DNS lookups won't happen below).
	if (!NET_IsIP(Cmd_Argv(2))) { // Unfortunately does not currently handle IPv6.
		Com_DPrintf("Invalid IP address in ip2locResponse packet: %s\n", Cmd_Argv(2));
		return;
	}
	if (!NET_StringToAdr(Cmd_Argv(2), &clientadr)) { // Should never ever happen.
		Com_DPrintf("Invalid IP address in ip2locResponse packet: %s\n", Cmd_Argv(2));
		return;
	}

	// We won't check that the address is a non-LAN address.  That's going too far in error checking.

	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
		if (cl->state < CS_CONNECTED || cl->netchan.remoteAddress.type == NA_BOT || cl->location[0] ||
				// IMPORTANT!  Make sure we check length of Cmd_Argv(1) above!!!
				// Use strncmp instead of strcmp in case cl->ip2locChallengeStr didn't
				// get initialized to zeroes when the client was created (programming error).
				Q_strncmp(cl->ip2locChallengeStr, Cmd_Argv(1) + 17, sizeof(cl->ip2locChallengeStr) - 1)) {
			continue;
		}
		if (NET_CompareBaseAdr(clientadr, cl->netchan.remoteAddress)) {
			if (Cmd_Argv(6)[0]) {
				Q_strcat(cl->location, sizeof(cl->location), Cmd_Argv(6));
				citySpecified = qtrue;
			}
			if ((!Q_stricmp(Cmd_Argv(3), "US")) || (!Q_stricmp(Cmd_Argv(3), "CA"))) {
				countryNeedsRegion = qtrue;
			}
			if (countryNeedsRegion && Cmd_Argv(5)[0]) {
				if (citySpecified) {
					Q_strcat(cl->location, sizeof(cl->location), ", ");
				}
				Q_strcat(cl->location, sizeof(cl->location), Cmd_Argv(5));
				regionSpecified = qtrue;
			}
			if (Cmd_Argv(4)[0]) {
				if (regionSpecified && citySpecified) {
					Q_strcat(cl->location, sizeof(cl->location), " (");
				}
				else if (regionSpecified || citySpecified) {
					Q_strcat(cl->location, sizeof(cl->location), ", ");
				}
				Q_strcat(cl->location, sizeof(cl->location), Cmd_Argv(4));
				if (regionSpecified && citySpecified) {
					Q_strcat(cl->location, sizeof(cl->location), ")");
				}
				countrySpecified = qtrue;
			}

			if (citySpecified || regionSpecified || countrySpecified) {
				// Check the location string.  If the ip2loc server sends malicious strings,
				// it may force the client to be dropped when we change their userinfo.
				// Allow the same characters that the checkuserinfo.patch allows.
				if (!charMapInitialized) {
					// These are characters that are allowed/disallowed to be in cvar keys and values.
					for (i = 0;   i <= 31;  i++) { charMap[i] = qfalse; }
					for (i = 32;  i <= 33;  i++) { charMap[i] = qtrue; }
					charMap[34] = qfalse; // double quote
					for (i = 35;  i <= 58;  i++) { charMap[i] = qtrue; }
					charMap[59] = qfalse; // semicolon
					for (i = 60;  i <= 91;  i++) { charMap[i] = qtrue; }
					charMap[92] = qfalse; // backslash
					for (i = 93;  i <= 126; i++) { charMap[i] = qtrue; }
					for (i = 127; i <= 255; i++) { charMap[i] = qfalse; }
					charMapInitialized = qtrue;
				}
				ch = cl->location;
				while (*ch) {
					if (!charMap[0xff & *ch]) {
						Com_DPrintf("The ip2loc server is misbehaving; the location for IP %s ended "
								"up being \"%s\", which contains illegal characters\n",
								NET_AdrToString(clientadr), cl->location);
						Q_strncpyz(cl->location, "ILLEGAL CHARACTERS IN LOCATION", sizeof(cl->location));
						break;
					}
					ch++;
				}
				if ('\0' == *ch) {
					// Not illegal characters.
					SV_SendServerCommand(NULL, "print \"    from %s\n\"", cl->location);
				}
			}
			else {
				Q_strcat(cl->location, sizeof(cl->location), "UNKNOWN LOCATION");
			}
			SV_UserinfoChanged(cl);
			VM_Call( gvm, GAME_CLIENT_USERINFO_CHANGED, cl - svs.clients );
			break;
		}
	}
}

/*
================
SVC_StatuswLoc

Same as SVC_Status() but with [quoted] locations after player names.
================
*/
void SVC_StatuswLoc(netadr_t from) {
	char		player[1024];
	char		status[MAX_MSGLEN];
	int		i;
	client_t	*cl;
	playerState_t	*ps;
	int		statusLength;
	char		infostring[MAX_INFO_STRING];

	// Ignore if we are in single player.
	if (Cvar_VariableValue("g_gametype") == GT_SINGLE_PLAYER) {
		return;
	}

	Q_strncpyz(infostring, Cvar_InfoString(CVAR_SERVERINFO), sizeof(infostring));

	// Prevent spoofed reply packets.
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1));

	status[0] = '\0';
	statusLength = 0;

	for (i = 0; i < sv_maxclients->integer; i++) {
		if (statusLength >= sizeof(status) - 1) { break; }
		cl = &svs.clients[i];
		if (cl->state >= CS_CONNECTED) {
			ps = SV_GameClientNum(i);
			Q_snprintf(player, sizeof(player), "%i %i \"%s\" \"%s\"\n",
					ps->persistant[PERS_SCORE], cl->ping, cl->name, cl->location);
			Q_strncpyz(status + statusLength, player, sizeof(status) - statusLength);
			statusLength += strlen(player);
		}
	}

	NET_OutOfBandPrint(NS_SERVER, from, "statuswlocResponse\n%s\n%s", infostring, status);
}

/*
===============
SVC_AuthorizePlayer

Deal with packets from the player database ban server.
===============
*/
void SVC_AuthorizePlayer(netadr_t from)
{
	char		*arg1;
	char		*arg2;
	char		*arg3;
	int		i;
	char		ch;
	netadr_t	clientAddr;
	int		challenge;
	qboolean	denied;
	client_t	*cl;
	const	char	*command = "authorizePlayer:xxxxxxxx";

	// This is probably a redundant check with the compare of addresses below.
	if (!sv_playerDBHost->string[0]) {
		Com_DPrintf("playerDBResponse packet received from %s but no sv_playerDBHost set\n",
			NET_AdrToString(from));
		return;
	}

	// See if this packet is from the player database server.  Drop it if not.
	// Note that it's very easy to spoof the source address of a UDP packet, so
	// we use the challenge concept for further protection.
	// Note that NET_CompareAdr() does compare the .type of the addresses.
	if (!NET_CompareAdr(from, svs.playerDatabaseAddress)) {
		Com_DPrintf("playerDBResponse packet received from invalid playerdb server: %s\n",
			NET_AdrToString(from));
		return;
	}

	arg1 = Cmd_Argv(1);
	if (strlen(command) != strlen(arg1) ||
			Q_strncmp(command, arg1, 16)) { // Check up to and including the ':'.
		Com_DPrintf("First argument of playerDBResponse packet was \"%s\", not of the form \"%s\"\n",
				arg1, command);
		return;
	}
	challenge = 0;
	for (i = 0; i < 8; i++) {
		ch = arg1[16 + i];
		if ('0' <= ch && ch <= '9') {
			challenge |= (ch - '0') << ((7 - i) << 2);
		}
		else if ('a' <= ch && ch <= 'f') {
			challenge |= (ch - 'a' + 10) << ((7 - i) << 2);
		}
		else {
			Com_DPrintf("Invalid challenge \"%s\" in playerDBResponse authorizePlayer packet\n", arg1 + 16);
			return;
		}
	}
	Com_DPrintf("SVC_AuthorizePlayer: parsed hex challenge %s to be %i\n", arg1 + 16, challenge);

	arg2 = Cmd_Argv(2);
	// Make sure this is an IP address (so that DNS lookups won't happen below).
	if (!NET_IsIP(arg2)) { // Does not currently handle IPv6 addresses.
		Com_DPrintf("Invalid IP address in playerDBResponse packet: %s\n", arg2);
		return;	
	}

	if (!NET_StringToAdr(arg2, &clientAddr)) {
		// This condition should never happen because of the check above.
		Com_DPrintf("Invalid IP address in playerDBResponse packet: %s\n", arg2);
		return;
	}

	// This is a freak condition that will only happen if someone starts sending random crap
	// from the auth server IP/port.  We never send auth packets for LAN addresses.
	if (Sys_IsLANAddress(clientAddr)) {
		Com_DPrintf("playerDBResponse packet received for a LAN address, ignoring\n");
		return;
	}

	arg3 = Cmd_Argv(3);
	if (!Q_stricmp(arg3, "denied")) {
		denied = qtrue;
	}
	else if (!Q_stricmp(arg3, "allowed")) {
		denied = qfalse;
	}
	else {
		Com_DPrintf("Invalid third argument in playerDBResponse packet: %s\n", arg3);
		return;
	}

	if (challenge & 0x80000000) { // High bit set.  This is a regular periodic auth (not getchallenge).
		// Find the connected player that has this challenge and IP address.
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
			if (cl->state >= CS_CONNECTED && ((challenge ^ cl->challenge) & 0x7fffffff) == 0 &&
					NET_CompareBaseAdr(clientAddr, cl->netchan.remoteAddress)) {
				if (!denied) {
					// This player is allowed to play, so we don't need any further action.
					return;
				}
				Com_DPrintf("Dropping player %s (address %s) at request of the banserver\n",
						cl->name, NET_AdrToString(cl->netchan.remoteAddress));
				if (sv_permaBanBypass->string[0] &&
						!strcmp(Info_ValueForKey(cl->userinfo, "permabanbypass"),
							sv_permaBanBypass->string)) {
					Com_DPrintf("Ban avoided by having correct permabanbypass\n");
				}
				else {
					SV_DropClient(cl, "denied access");
					// A brand new challenge will be issued when the client tries to reconnect.
					// The auth server will be contacted all over again.
				}
				return;
			}
		}
		Com_DPrintf("SVC_AuthorizePlayer: player to periodic auth not connected (addr %s)\n",
				NET_AdrToString(clientAddr));
		return;
	}

	// Otherwise this is a getchallenge auth (high bit not set on challenge).
	for (i = 0; i < MAX_CHALLENGES; i++) {
		// Checking the connected state of the challenge is actually a bit superfluous because
		// the chances of another challenge matching in address and challenge number is quite
		// slim.  I'm still adding the connected check for clarity.
		if (((challenge ^ svs.challenges[i].challenge) & 0x7fffffff) == 0 && (!svs.challenges[i].connected) &&
				NET_CompareBaseAdr(clientAddr, svs.challenges[i].adr)) {
			break;
		}
	}
	if (i == MAX_CHALLENGES) {
		Com_DPrintf("SVC_AuthorizePlayer: challenge not found\n");
		return;
	}
	if (denied) {
		Com_DPrintf("Marking challenge for client address %s as banned\n",
				NET_AdrToString(svs.challenges[i].adr));
		// There is a quirk (or "feature") in SV_DirectConnect().  When this banned player
		// connects via SV_DirectConnect() and their challenge is marked as permabanned,
		// they will be dropped, but their challenge.connected will never be
		// reset to false; it will remain as true.  Therefore, when the player
		// tries to reconnect from scratch (via getchallenge packet), they
		// will pick up a brand new challenge, which will cause a packet to
		// be sent to the playerdb auth server all over again.  This is good
		// because changes to who's banned on the auth server will be picked
		// up very quickly.  However this is also bad because more packets will
		// be sent to the auth server.
		svs.challenges[i].permabanned = qtrue;
	}
	// Let the client connect even if they are banned.  We do this for two reasons.
	// First, we want their userinfo string for the sake of sending it to the playerdb
	// so that we can track offenders better.  Second, we need a way of letting
	// innocent players affected by someone else's ban into the server, and the only way
	// this is possible is by means of the userinfo string, which first appears in
	// SV_DirectConnect().  We use "permabanbypass" in the userinfo for this.
	svs.challenges[i].pingTime = svs.time;
	Com_DPrintf("Sending challengeResponse to %s from SVC_AuthorizePlayer\n",
			NET_AdrToString(svs.challenges[i].adr));
	NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "challengeResponse %i", svs.challenges[i].challenge);
}

/*
================
SVC_RemoteMod
================
*/
void SVC_RemoteMod(netadr_t from, msg_t * msg)
{
    int slot;
    char *cmd_ptr;
    char cmdstr[1024];
    int i;
    
#define SV_REMOTEMOD_OUTPUTBUF_LENGTH (1024 - 16)
    // A place to hold data before we dump it to the client's console.
    char sv_remotemod_outputbuf[SV_REMOTEMOD_OUTPUTBUF_LENGTH];
    
    // Prevent using mod as an amplifier and make dictionary attacks impractical
    if (SVC_RateLimitAddress(from, 10, 1000))
    {
        Com_DPrintf
        ("SVC_RemoteMod: rate limit from %s exceeded, dropping request\n",
         NET_AdrToString(from));
        return;
    }
    
    if (sv_moderatorremoteenable->integer < 1)
        return;
    
    svs.redirectAddress = from; // This will need to be set for later when we tell the client stuff.
    
    slot = SVC_GetModSlotByPassword(Cmd_Argv(1));
    
    if (!slot)
    {
        Com_Printf("Invalid remote mod password from %s. Password: %s\n",
                   NET_AdrToString(from), Cmd_Argv(1));
        
        // Tell the client what happened.
        Com_BeginRedirect(sv_remotemod_outputbuf, SV_REMOTEMOD_OUTPUTBUF_LENGTH,
                          SV_FlushRedirect);
        Com_Printf("Invalid password.\n");
        Com_EndRedirect();
        return;
    }
    
    if (!SV_ModCommandAllowed(sv_moderatorcommands[slot - 1]->string,
                              Cmd_Argv(2)))
    {
        Com_Printf("Invalid permissions for command issued from: %s.\n",
                   NET_AdrToString(from));
        Com_Printf("Mod slot %i is not able to execute command: %s\n", slot,
                   Cmd_Argv(2));
        
        // Tell the client what happened.
        Com_BeginRedirect(sv_remotemod_outputbuf, SV_REMOTEMOD_OUTPUTBUF_LENGTH,
                          SV_FlushRedirect);
        Com_Printf("You may only execute commands: %s\n",
                   sv_moderatorcommands[slot - 1]->string);
        Com_EndRedirect();
        return;
    }
    
    // To get this far:
    //   remote mod must be enabled
    //   a valid password was supplied
    //   the mod slot has permissions for the command
    
    // To deal with quoting, we should use the full comand, skipping until we find the full command line to exec.
    cmd_ptr = Cmd_Cmd();
    
    while (cmd_ptr[0] == ' ')   // Trim off leading spaces.
        cmd_ptr++;
    while (cmd_ptr[0] != '\0' && cmd_ptr[0] != ' ') // Trim off packet command.
        cmd_ptr++;
    while (cmd_ptr[0] == ' ')   // Trim off more leading spaces.
        cmd_ptr++;
    while (cmd_ptr[0] != '\0' && cmd_ptr[0] != ' ') // Trim off password.
        cmd_ptr++;
    while (cmd_ptr[0] == ' ')   // Trim off spaces after the password.
        cmd_ptr++;
    
    // Copy the full command to a new string for trimming and execution.
    cmdstr[0] = '\0';
    Q_strcat(cmdstr, sizeof(cmdstr), cmd_ptr);
    
    // Make sure nobody tried to sneak in a second command into the args with a command separator
    for (i = 0; cmdstr[i]; i++)
    {
        if (cmdstr[i] == ',' || cmdstr[i] == '\n' || cmdstr[i] == '\r' || (cmdstr[i] == '\\' && cmdstr[i + 1] == '$'))  // no cvar substitution (eg \$rconpassword\)
        {
            Com_Printf
            ("Remote mod command rejected from %s. Reason: Command contains separators.\n",
             NET_AdrToString(from));
            // Tell the client we rejected the command.
            Com_BeginRedirect(sv_remotemod_outputbuf,
                              SV_REMOTEMOD_OUTPUTBUF_LENGTH, SV_FlushRedirect);
            Com_Printf
            ("Remote mod command rejected. Reason: Contains an invalid character.\n");
            Com_EndRedirect();
            return;
        }
    }
    
    // Execute our trimmed string.
    Com_Printf
    ("Executing remote command from %s under mod slot %i. Command: %s\n",
     NET_AdrToString(from), slot, cmdstr);
    Com_BeginRedirect(sv_remotemod_outputbuf, SV_REMOTEMOD_OUTPUTBUF_LENGTH,
                      SV_FlushRedirect);
    Cmd_ExecuteString(cmdstr);
    Com_EndRedirect();
    
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket( netadr_t from, msg_t *msg ) {
	char	*s;
	char	*c;

	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );		// skip the -1 marker

	if (!Q_strncmp("connect", (char *) &msg->data[4], 7)) {
		Huff_Decompress(msg, 12);
	}

	s = MSG_ReadStringLine( msg );
	Cmd_TokenizeString( s );

	c = Cmd_Argv(0);

	if (!Q_stricmp(c, "getstatus")) {
		if (sv_CheckDRDoS->integer > 0) {
            if (SV_CheckDRDoS(from)) { return; }
        }
		SVC_Status( from  );
  } else if (!Q_stricmp(c, "getinfo")) {
        if (sv_CheckDRDoS->integer > 0) {
            if (SV_CheckDRDoS(from)) { return; }
        }
		SVC_Info( from );
	} else if (!Q_stricmp(c, "getchallenge")) {
		SV_GetChallenge( from );
	} else if (!Q_stricmp(c, "connect")) {
		SV_DirectConnect( from );
	/*
	} else if (!Q_stricmp(c, "ipAuthorize")) {
		SV_AuthorizeIpPacket( from );
	*/
	} else if (!Q_stricmp(c, "rcon")) {
		SVC_RemoteCommand( from, msg );
	} else if (!Q_stricmp(c, "ip2locResponse")) {
		SVC_HandleIP2Loc( from );
	} else if (!Q_stricmp(c, "getstatuswloc")) {
		if (SV_CheckDRDoS(from)) { return; }
		SVC_StatuswLoc( from );
	////////////////////////////////////////////////
	// separator for ip2loc.patch and playerdb.patch
	////////////////////////////////////////////////
	} else if (!Q_stricmp(c, "playerDBResponse")) {
		SVC_AuthorizePlayer( from );
	} else if (!Q_stricmp(c, "mod")) {
        SVC_RemoteMod(from, msg);
    } else if (!Q_stricmp(c, "disconnect")) {
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	} else {
		Com_DPrintf ("bad connectionless packet from %s:\n%s\n"
		, NET_AdrToString (from), s);
	}

	// We moved this from the top of this function to the bottom.
	// During a DRDoS attack we get thousands of lines of packets
	// that just garble the screen.  Since we return from this
	// function early if a DRDoS is detected, we don't print a line
	// for each attacking packet anymore.
	Com_DPrintf ("SV packet %s : %s\n", NET_AdrToString(from), c);
}

//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_PacketEvent( netadr_t from, msg_t *msg ) {
	int			i;
	client_t	*cl;
	int			qport;

	// check for connectionless packet (0xffffffff) first
	if ( msg->cursize >= 4 && *(int *)msg->data == -1) {
		SV_ConnectionlessPacket( from, msg );
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );				// sequence number
	qport = MSG_ReadShort( msg ) & 0xffff;

	// find which client the message is from
	for (i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if (cl->state == CS_FREE) {
			continue;
		}
		if ( !NET_CompareBaseAdr( from, cl->netchan.remoteAddress ) ) {
			continue;
		}
		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if (cl->netchan.qport != qport) {
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if (cl->netchan.remoteAddress.port != from.port) {
			Com_Printf( "SV_PacketEvent: fixing up a translated port\n" );
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if (SV_Netchan_Process(cl, msg)) {
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if (cl->state != CS_ZOMBIE) {
				cl->lastPacketTime = svs.time;	// don't timeout
				SV_ExecuteClientMessage( cl, msg );
			}
		}
		return;
	}
	
	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	NET_OutOfBandPrint( NS_SERVER, from, "disconnect" );
}


/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings( void ) {
	int			i, j;
	client_t	*cl;
	int			total, count;
	int			delta;
	playerState_t	*ps;

	for (i=0 ; i < sv_maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if ( cl->state != CS_ACTIVE ) {
			cl->ping = 999;
			continue;
		}
		if ( !cl->gentity ) {
			cl->ping = 999;
			continue;
		}
		if ( cl->gentity->r.svFlags & SVF_BOT ) {
			cl->ping = 0;
			continue;
		}

		total = 0;
		count = 0;
		for ( j = 0 ; j < PACKET_BACKUP ; j++ ) {
			if ( cl->frames[j].messageAcked <= 0 ) {
				continue;
			}
			delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
			count++;
			total += delta;
		}
		if (!count) {
			cl->ping = 999;
		} else {
			cl->ping = total/count;
			if ( cl->ping > 999 ) {
				cl->ping = 999;
			}
		}

		// let the game dll know about the ping
		ps = SV_GameClientNum( i );
		ps->ping = cl->ping;
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->integer 
seconds, drop the conneciton.  Server time is used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts( void ) {
	int		i;
	client_t	*cl;
	int			droppoint;
	int			zombiepoint;

	droppoint = svs.time - 1000 * sv_timeout->integer;
	zombiepoint = svs.time - 1000 * sv_zombietime->integer;

	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		// message times may be wrong across a changelevel
		if (cl->lastPacketTime > svs.time) {
			cl->lastPacketTime = svs.time;
		}

		if (cl->state == CS_ZOMBIE
		&& cl->lastPacketTime < zombiepoint) {
			// using the client id cause the cl->name is empty at this point
			Com_DPrintf( "Going from CS_ZOMBIE to CS_FREE for client %d\n", i );
			cl->state = CS_FREE;	// can now be reused
			continue;
		}
		if ( cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint) {
			// wait several frames so a debugger session doesn't
			// cause a timeout
			if ( ++cl->timeoutCount > 5 ) {
				SV_DropClient (cl, "timed out"); 
				cl->state = CS_FREE;	// don't bother with zombie state
			}
		} else {
			cl->timeoutCount = 0;
		}
	}
}

/*
==================
SV_CheckPaused
==================
*/
qboolean SV_CheckPaused( void ) {
	int		count;
	client_t	*cl;
	int		i;

	if ( !cl_paused->integer ) {
		return qfalse;
	}

	// only pause if there is just a single client connected
	count = 0;
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if ( cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT ) {
			count++;
		}
	}

	if ( count > 1 ) {
		// don't pause
		if (sv_paused->integer)
			Cvar_Set("sv_paused", "0");
		return qfalse;
	}

	if (!sv_paused->integer)
		Cvar_Set("sv_paused", "1");
	return qtrue;
}

/*
==================
SV_CheckLocation
Returns 1 or -1
1 = yes player i is in area of xy with the r
-1 = no player i isnt in area of xy with the r
==================
*/

int SV_CheckLocation( float x, float y , float r, int i ) {
	playerState_t *ps;
    ps = SV_GameClientNum(i);
    if (ps->origin[0] > x-r &&
        ps->origin[0] < x+r &&
        ps->origin[1] > y-r &&
        ps->origin[1] < y+r
        ) {
        return 1;
    }
	return -1;
}

/*
==================
SV_GivePlayerHealth
i = client id
h = amount of health
==================
*/
void SV_GivePlayerHealth(int clId, int h) {
    char    cmd[64];
    
    Com_sprintf(cmd, sizeof(cmd), "gh %i \"+%i\"\n", clId, h); // needs qvm mod
    Cmd_ExecuteString(cmd);
}

/*
==================
SV_GivePlayerStamina
i = client id
h = amount of stamina
==================
*/
void SV_GivePlayerStamina(int clId, int s) {
    char    cmd[64];
    
    Com_sprintf(cmd, sizeof(cmd), "gs %i \"+%i\"\n", clId, s); // needs qvm mod
    Cmd_ExecuteString(cmd);
}

/*
==================
SV_MedicStation
==================
*/
void SV_MedicStation( char* map, float x, float y, float r, float h ) {
	client_t	*cl;
	int i;
	char    cmd[64];
    
	if (Q_stricmp(sv_mapname->string, map)) { // This MedicStation is not on this map
		return;
	}
    
	for (i=0 ; i < sv_maxclients->integer ; i++) {
		cl = &svs.clients[i];
        if (cl->state == CS_ACTIVE) {
            if (SV_CheckLocation(x, y, r, i) == 1) { // is player in MedicZone?
                SV_GivePlayerHealth(i, h); // Give him health
                Com_sprintf(cmd, sizeof(cmd), "sendclientcommand %i cp \"%s\"\n",i ,"^2You are in a ^1Medic Zone ^7[^1+^7]" ); // needs qvm mod
                Cmd_ExecuteString(cmd);
            }
        }
    }
}

/*
==================
SV_ResetStamina
==================
*/
static void SV_ResetStamina(void) {
    
    int i;
    
    client_t *cl;
    
    playerState_t *ps;
    
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
        
    {
        
        if (cl->state != CS_ACTIVE) {
            continue;
        }
        ps = SV_GameClientNum(i);
        if (!ps->velocity[0] && !ps->velocity[1] && !ps->velocity[2])
        {
            if (++cl->nospeedCount >= sv_regainStamina->integer)
            {
                SV_GivePlayerStamina(i, 100);
                cl->nospeedCount = 0;
            }
        }
        else
            cl->nospeedCount = 0;
    }
}

/*
==================
SV_ResetHealth
==================
*/
static void SV_ResetHealth(void) {
    
    int i;
    
    client_t *cl;
    
    playerState_t *ps;
    
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
        
    {
        
        if (cl->state != CS_ACTIVE) {
            continue;
        }
        ps = SV_GameClientNum(i);
        if (!ps->velocity[0] && !ps->velocity[1] && !ps->velocity[2])
        {
            if (++cl->nospeedCount >= sv_regainHealth->integer)
            {
                SV_GivePlayerHealth(i, 100);
                cl->nospeedCount = 0;
            }
        }
        else
            cl->nospeedCount = 0;
    }
}

/*
==================
SV_Frame

Player movement occurs as a result of packet events, which
happen before SV_Frame is called
==================
*/
void SV_Frame( int msec ) {
	int		frameMsec;
	int		startTime;

	// the menu kills the server with this cvar
	if ( sv_killserver->integer ) {
		SV_Shutdown ("Server was killed");
		Cvar_Set( "sv_killserver", "0" );
		return;
	}

	if (!com_sv_running->integer)
	{
		// Running as a server, but no map loaded
#ifdef DEDICATED
		// Block until something interesting happens
		Sys_Sleep(-1);
#endif

		return;
	}

	// allow pause if only the local client is connected
	if ( SV_CheckPaused() ) {
		return;
	}

	// if it isn't time for the next frame, do nothing
	if ( sv_fps->integer < 1 ) {
		Cvar_Set( "sv_fps", "10" );
	}

	frameMsec = 1000 / sv_fps->integer * com_timescale->value;
	// don't let it scale below 1ms
	if(frameMsec < 1)
	{
		Cvar_Set("timescale", va("%f", sv_fps->integer / 1000.0f));
		frameMsec = 1;
	}

	sv.timeResidual += msec;

	if (!com_dedicated->integer) SV_BotFrame (sv.time + sv.timeResidual);

	if ( com_dedicated->integer && sv.timeResidual < frameMsec ) {
		// NET_Sleep will give the OS time slices until either get a packet
		// or time enough for a server frame has gone by
		NET_Sleep(frameMsec - sv.timeResidual);
		return;
	}

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if ( svs.time > 0x70000000 ) {
		SV_Shutdown( "Restarting server due to time wrapping" );
		Cbuf_AddText( va( "map %s\n", Cvar_VariableString( "mapname" ) ) );
		return;
	}
	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if ( svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities ) {
		SV_Shutdown( "Restarting server due to numSnapshotEntities wrapping" );
		Cbuf_AddText( va( "map %s\n", Cvar_VariableString( "mapname" ) ) );
		return;
	}

	if( sv.restartTime && sv.time >= sv.restartTime ) {
		sv.restartTime = 0;
		Cbuf_AddText( "map_restart 0\n" );
		return;
	}

	// update infostrings if anything has been changed
	if ( cvar_modifiedFlags & CVAR_SERVERINFO ) {
		SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if ( cvar_modifiedFlags & CVAR_SYSTEMINFO ) {
		SV_SetConfigstring( CS_SYSTEMINFO, Cvar_InfoString_Big( CVAR_SYSTEMINFO ) );
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}

	if ( com_speeds->integer ) {
		startTime = Sys_Milliseconds ();
	} else {
		startTime = 0;	// quite a compiler warning
	}

	// update ping based on the all received frames
	SV_CalcPings();

	if (com_dedicated->integer) SV_BotFrame (sv.time);

	// run the game simulation in chunks
	while ( sv.timeResidual >= frameMsec ) {
		sv.timeResidual -= frameMsec;
		svs.time += frameMsec;
		sv.time += frameMsec;

		// let everything in the world think and move
		VM_Call (gvm, GAME_RUN_FRAME, sv.time);
	}

	if ( com_speeds->integer ) {
		time_game = Sys_Milliseconds () - startTime;
	}

	// check timeouts
	SV_CheckTimeouts();
	
	// check user info buffer thingy
	SV_CheckClientUserinfoTimer();

	// send messages back to the clients
	SV_SendClientMessages();

    // reset stamina of players with zero velocity
    
    if (sv_regainStamina->integer > 0 && (Q_stricmp("4.1",Cvar_VariableString("g_modversion")))) {
        SV_ResetStamina();
    }
    // reset health of players with zero velocity
    
    if (sv_regainHealth->integer > 0 && (Q_stricmp("4.1",Cvar_VariableString("g_modversion")))) {
        SV_ResetHealth();
    }
    
	// send a heartbeat to the master if needed
	SV_MasterHeartbeat();

	if (sv_playerDBPassword->modified) {
		Com_DPrintf("Detected playerdb server password change\n");
		Q_strncpyz(svs.playerDatabasePassword, sv_playerDBPassword->string, sizeof(svs.playerDatabasePassword));
		Cvar_Set(sv_playerDBPassword->name, "");
		sv_playerDBPassword->modified = qfalse;
	}

	// These are very inexpensive operations; they don't impact CPU usage even if
	// the playerdb system is turned off.
	static int authFrameCount = 0;
	static int authClientInx = 0;
	client_t *cl;
	authFrameCount++;
	if (authFrameCount == 80) { // 4 seconds during normal gameplay (20 frames per sec)
		authFrameCount = 0;
		if (authClientInx >= sv_maxclients->integer) {
			authClientInx = 0;
		}
		cl = &svs.clients[authClientInx];
		if (cl->state >= CS_CONNECTED && // IPv6 not supported at this point.
				(cl->netchan.remoteAddress.type == NA_IP /* || cl->netchan.remoteAddress.type == NA_IP6 */) &&
				sv_playerDBBanIDs->string[0]) {
			SV_ResolvePlayerDB();
			if (svs.playerDatabaseAddress.type != NA_BAD && !Sys_IsLANAddress(cl->netchan.remoteAddress)) {
				Com_DPrintf("Sending authorizePlayer packet to playerdb for client address %i.%i.%i.%i\n",
						cl->netchan.remoteAddress.ip[0], cl->netchan.remoteAddress.ip[1],
						cl->netchan.remoteAddress.ip[2], cl->netchan.remoteAddress.ip[3]);
				NET_OutOfBandPrint(NS_SERVER,
					svs.playerDatabaseAddress,
					"playerDBRequest\n%s\nauthorizePlayer:%08x\n%s\n%i.%i.%i.%i\n",
					svs.playerDatabasePassword,
					0x80000000 | (cl->challenge),
					sv_playerDBBanIDs->string,
					cl->netchan.remoteAddress.ip[0], cl->netchan.remoteAddress.ip[1],
					cl->netchan.remoteAddress.ip[2], cl->netchan.remoteAddress.ip[3]);
			}
		}
		authClientInx++;
	}
    
    if (sv_MedicStation->integer > 0 && (Q_stricmp("4.1",Cvar_VariableString("g_modversion")))) { // dont run if g_modversion is 4.1
        // example medic stations
        // they are hardcoded
        // FIXME: move cfgs to file?
        // may cause lags?
        // FIXME: find a better spot to bind this?
        SV_MedicStation( "ut4_abbey",50,1500,50,1 );
        SV_MedicStation( "ut4_sanc",-64,544,50,1 );
        SV_MedicStation( "ut4_sanc",3936,128,50,1 );
        SV_MedicStation( "ut4_turnpike",1572,176,50,1 );
        SV_MedicStation( "ut4_turnpike",-596,-1640,50,1 );
	}
}

//============================================================================

