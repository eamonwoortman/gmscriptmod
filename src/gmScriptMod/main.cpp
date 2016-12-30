#include "version.h"
#include <q_shared.h>
#include <g_local.h>
#include <qmmapi.h>
#include <string>
#include <fstream>
#include <iterator>
#include "tools.h"
#include "gmScan.h"
using std::string;

//new domain: http://maintain-et.net 
pluginres_t* g_result = NULL;
plugininfo_t g_plugininfo = {
	GM_SCRIPTMOD_QMM_PNAME,		//name of plugin
	GM_SCRIPTMOD_QMM_VERSION,	//version of plugin
	GM_SCRIPTMOD_QMM_PDESC,		//description of plugin
	GM_SCRIPTMOD_QMM_BUILDER,	//author of plugin
	GM_SCRIPTMOD_QMM_WEBSITE,	//website of plugin
	0,				//can this plugin be paused?
	0,				//can this plugin be loaded via cmd
	1,				//can this plugin be unloaded via cmd
	QMM_PIFV_MAJOR,			//plugin interface version major
	QMM_PIFV_MINOR			//plugin interface version minor
};
eng_syscall_t g_syscall = NULL;
mod_vmMain_t g_vmMain = NULL;
pluginfuncs_t* g_pluginfuncs = NULL;
int g_vmbase = 0;

gentity_t* g_gents = NULL;
int g_gentsize = sizeof(gentity_t);
gclient_t* g_clients = NULL;
int g_clientsize = sizeof(gclient_t);

char fileP[150]; 

//2good - reserved entity stuff
#define NUM_OF_ENTITYS_TO_RESERVE 5

int GM_Entity_Num = 0;
int entitytok = 0;
int entitysadded = 1;
char* enttoken[] = { "{", "classname","info_notnull", "origin","0 0 0",  "}" };
//

//camping system
int g_campshuffle;
int g_camptimeout;
int g_campprotect;
char g_campwarnmessage[MAX_STRING_CHARS];
char g_campslapmessage[MAX_STRING_CHARS];
int g_time = 0;
int running = -1;
typedef struct playerinfo_s {
	bool begun;
} playerinfo_t;

playerinfo_t g_playerinfo[MAX_CLIENTS];

//GameMonkey
#include "gmThread.h"
#include "gmGoNeLib.h"
#include "gmMachine.h"
#include "gmCall.h"   // Header contains helper class
#include "gmTableObject.h"

#define cp(x) g_syscall(G_SEND_SERVER_COMMAND, -1, QMM_VARARGS("cp \"%s\" 1",x))

gmMachine *g_Machine = 0;

int gmLoadAndExecuteScript( gmMachine &a_machine, char * a_filename )
{
	std::ifstream file((const char *)a_filename);
	char filename[200];

	strcpy(filename, a_filename);

	g_syscall(G_PRINT, QMM_VARARGS( "[GMS Mod] Trying to load '%s'\n",a_filename));

	if (!file){
		g_syscall(G_PRINT, QMM_VARARGS( "[GMS Mod] Error, Could not open '%s'\n",a_filename));
		return GM_EXCEPTION;
	}
	g_syscall(G_PRINT,filename);
	std::string fileString = std::string(std::istreambuf_iterator<char>(file),
	std::istreambuf_iterator<char>());
	file.close();

	a_machine.GetLog().Reset();
	int num_errors;
	if (num_errors = a_machine.ExecuteString(fileString.c_str()) > 0)
	{
		g_syscall(G_PRINT,QMM_VARARGS("\n[GMS Mod] Compile failed for '%s'\n\n",filename));
		g_syscall(G_PRINT,"------------------------------------------------------------\n");

		bool first = true;

		while ( const char* error=a_machine.GetLog().GetEntry( first ) )
         	{
			g_syscall(G_PRINT,QMM_VARARGS ( "%s\n", error ));
		}

		g_syscall(G_PRINT,"------------------------------------------------------------\n");
		g_syscall(G_PRINT,"[GMS Mod] end of compile debugging\n\n");
       }else{
		g_syscall(G_PRINT,QMM_VARARGS("[GMS Mod] scriptfile successfully loaded: %s\n",filename));
	}

}

//end of GameMonkey Stuff

//2good - reserved entity stuff
int SendEntityToken(char* buffer,int buffersize){
	//copy Nth entity token
	strncpy(buffer,enttoken[entitytok],buffersize);
	//increment our counter
	entitytok++;
}
//
void COM_BitSet(int array[],int bitNum){
	int i = 0;
	while(bitNum > 31){
		i++;
		bitNum -= 32;
	}
	array[i] |= (1<<bitNum);
}
/*
=============
Q_strncpyz
 
Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {
	if ( !src ) {
		return;
	}
	if ( destsize < 1 ) {
		return;
	}

	strncpy( dest, src, destsize-1 );
    dest[destsize-1] = 0;
}
// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		return;
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}

C_DLLEXPORT void QMM_Query(plugininfo_t** pinfo) {
	QMM_GIVE_PINFO();
}

C_DLLEXPORT int QMM_Attach(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, int vmbase, int iscmd) {
	QMM_SAVE_VARS();

	iscmd = 0;
	g_pluginfuncs = pluginfuncs; 

	g_syscall( G_CVAR_VARIABLE_STRING_BUFFER, "fs_homepath", fileP, sizeof(fileP));

	g_Machine = new gmMachine();
	gmBindGoNeLib(g_Machine);		//Bind the GoNe Lib to our Machine


	gmScan_Register(g_Machine);

	gmLoadAndExecuteScript(*g_Machine, QMM_VARARGS("%s/gmScriptMod/GoNeScript.gm",fileP));

	memset(g_playerinfo, 0, sizeof(g_playerinfo));


	return 1;
}

C_DLLEXPORT void QMM_Detach(int iscmd) {
	iscmd = 0;
	delete g_Machine;
}

//Dutchmeat, anti-camp system, I've added 3 vars to the g_local.h file in ../sdks/rtcwet
int VectorCompareShuffle(vec3_t a, vec3_t b, float shuffle) {
	if(!(b[0] > a[0]-shuffle && b[0] < a[0]+shuffle))
		return 0;
	if(!(b[1] > a[1]-shuffle && b[1] < a[1]+shuffle))
		return 0;
	if(!(b[2] > a[2]-shuffle && b[2] < a[2]+shuffle))
		return 0;
	return 1;
}

void CheckForCamp(gentity_t *ent){
	//added adamw
	char tempmsg[128];
	int timeatlocale, totaltime;

	if ( ent->client->ps.pm_type == 2 ) 
		return;

	if((g_time%500 == 0)  || ent->lastcheck != g_time){

		ent->lastcheck = g_time;
			
		if(g_campshuffle < 1)
			g_campshuffle = 1;

		if(VectorCompareShuffle(ent->lastlocale, ent->r.currentOrigin, (float)g_campshuffle)) {
			//They are STILL here
			//How long are have they been there for....
			timeatlocale = g_time - ent->beganhold;

			if(timeatlocale%1000==0) {
				//OK, they have been here for a multiple of 1 second
				totaltime = timeatlocale/1000;

				if(totaltime==g_camptimeout-5) 
					g_syscall(G_SEND_SERVER_COMMAND,ent->client->ps.clientNum, QMM_VARARGS("cp \"%s\n\"", g_campwarnmessage));
					//g_syscall(G_PRINT, "SOMEONE'S CAMPING FFS!");

				if(totaltime==g_camptimeout) {
					strcpy(tempmsg, QMM_VARARGS(g_campslapmessage, ent->client->pers.netname));
					//trap_SendServerCommand(-1, va("print \"%s\n\"", tempmsg));
					//g_syscall(G_PRINT, "SLAP HIM!");
					g_syscall(G_SEND_SERVER_COMMAND, -1, QMM_VARARGS("print \"%s\n\"", tempmsg));

					ent->client->ps.velocity[2] = ent->client->ps.velocity[2] + 1000;
					ent->client->ps.velocity[2] = ent->client->ps.velocity[2] - 600;
					ent->client->ps.velocity[2] = ent->client->ps.velocity[2] + 1000;

					//Nuke that bitch!
					//G_Damage(ent, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
				}

			}
		} else {
			VectorCopy(ent->r.currentOrigin, ent->lastlocale);
			ent->beganhold = g_time;
		}
	}
}

C_DLLEXPORT int QMM_vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	switch(cmd){	
	//hook client console commands
	case GAME_CONSOLE_COMMAND: {
		int myReturn = 0;

		//check what command it is
		char tempbuf[128];
		g_syscall(G_ARGV, 0, tempbuf, sizeof(tempbuf));

		//exec("slap cnum 1000"); 

		if (!strncmp(tempbuf, "rslap",5)){
			char arg1[3];
			char arg2[4];

			g_syscall(G_ARGV, 1, arg1, sizeof(arg1));  //clientnum
			g_syscall(G_ARGV, 2, arg2, sizeof(arg2));  //Z vector (height)

			//g_syscall(G_PRINT, QMM_VARARGS("DEBUG '%d'",atoi(arg2)));

			CLIENT_FROM_NUM (atoi(arg1))->ps.velocity[2] =  CLIENT_FROM_NUM (atoi(arg1))->ps.velocity[2] + atoi(arg2);
			CLIENT_FROM_NUM (atoi(arg1))->ps.velocity[1] =  (crandom() * 100) * 10;
			CLIENT_FROM_NUM (atoi(arg1))->ps.velocity[0] =  (crandom() * 100) * 10; 
		
			QMM_RET_SUPERCEDE(1);
		}
		
		if (!strncmp(tempbuf, "slap",4)){
		        char arg1[3];
		        char arg2[4];

			g_syscall(G_ARGV, 1, arg1, sizeof(arg1));  //clientnum
			g_syscall(G_ARGV, 2, arg2, sizeof(arg2));  //Z vector (height)
                        CLIENT_FROM_NUM (atoi(arg1))->ps.velocity[2] =  CLIENT_FROM_NUM (atoi(arg1))->ps.velocity[2] + atoi(arg2);
                        QMM_RET_SUPERCEDE(1);
                }

		gmCall call;

		if ( call.BeginGlobalFunction( g_Machine, "GAME_CONSOLE_COMMAND" ) ){

				call.AddParamString(tempbuf);
				call.AddParamString(ConcatArgs( 1 ));
				call.End();

			 	call.GetReturnedInt( myReturn );
	    	}
	
		if (myReturn == 1){
			QMM_RET_SUPERCEDE(1);
		}else{
			QMM_RET_IGNORED(1);
		}

		}
		break;
	
	//hook client console commands
	case GAME_CLIENT_COMMAND: {
		int myReturn = 0;
		gmCall monkeycall;
		int velocity_x=0;
		int velocity_y=0;
		int velocity_z=0;
		if( monkeycall.BeginGlobalFunction( g_Machine, "GET_CLIENT_VELOCITY_X_IMPACT" ) ){
			monkeycall.AddParamInt(arg0); 
			monkeycall.End();
			monkeycall.GetReturnedInt( velocity_x );
		 }
		
		if ( monkeycall.BeginGlobalFunction( g_Machine, "GET_CLIENT_VELOCITY_Y_IMPACT" ) ){
			monkeycall.AddParamInt(arg0);
			monkeycall.End();
                        monkeycall.GetReturnedInt( velocity_y );
		 }
	         if ( monkeycall.BeginGlobalFunction( g_Machine, "GET_CLIENT_VELOCITY_Z_IMPACT" ) ){
			monkeycall.AddParamInt(arg0);
			monkeycall.End();
			 monkeycall.GetReturnedInt( velocity_z );
                  }


		//check what command it is
		char tempbuf[128];
		g_syscall(G_ARGV, 0, tempbuf, sizeof(tempbuf));
		if (!strncmp(tempbuf, "hjump",5)){

		
			gmCall monkeycall;
			int velocity_z=0;
			if ( monkeycall.BeginGlobalFunction( g_Machine, "GET_CLIENT_VELOCITY_Z_IMPACT" ) ){
				monkeycall.AddParamInt(arg0);
				monkeycall.End();
				monkeycall.GetReturnedInt( velocity_z );
			}
			CLIENT_FROM_NUM(arg0)->ps.velocity[2] = CLIENT_FROM_NUM(arg0)->ps.velocity[2] + velocity_z;
		}
		
		if (!strncmp(tempbuf, "health",6)){
			//cp(QMM_VARARGS("DEBUG: %d",CLIENT_FROM_NUM(arg0)->ps.stats[STAT_HEALTH]));
			CLIENT_FROM_NUM(arg0)->ps.stats[STAT_HEALTH] = 100;
			//cp(QMM_VARARGS("DEBUG: %d",CLIENT_FROM_NUM(arg0)->ps.stats[STAT_HEALTH]));
		}

		if(!strncmp(tempbuf,"break",5)){
			CLIENT_FROM_NUM(arg0)->ps.velocity[2]=0;
			//THIS COULD BE FUNNY
			CLIENT_FROM_NUM(arg0)->ps.persistant[3]=2;//team blue
		}
		if(!strncmp(tempbuf,"test",4)){
			g_syscall(G_PRINT, QMM_VARARGS("DEBUG eFlags: %d\n",CLIENT_FROM_NUM(arg0)->ps.eFlags));
			g_syscall(G_PRINT, QMM_VARARGS("DEBUG pm_flags: %d\n",CLIENT_FROM_NUM(arg0)->ps.pm_flags));
			g_syscall(G_PRINT, QMM_VARARGS("DEBUG pm_type: %d\n",CLIENT_FROM_NUM(arg0)->ps.pm_type));
			g_syscall(G_PRINT, QMM_VARARGS("DEBUG PM_NOCLIP: %d\n",PM_NOCLIP));
			g_syscall(G_PRINT, QMM_VARARGS("DEBUG origin0: %d\n",CLIENT_FROM_NUM(arg0)->ps.origin[0]));
			g_syscall(G_PRINT, QMM_VARARGS("DEBUG origin1: %d\n",CLIENT_FROM_NUM(arg0)->ps.origin[1]));
			g_syscall(G_PRINT, QMM_VARARGS("DEBUG origin2: %d\n",CLIENT_FROM_NUM(arg0)->ps.origin[2]));
			for(int i=0;i<16;i++){
				g_syscall(G_PRINT, QMM_VARARGS("DEBUG stats[%d]: %d\n",i,CLIENT_FROM_NUM(arg0)->ps.stats[i]));
				g_syscall(G_PRINT, QMM_VARARGS("DEBUG persist[%d]: %d\n",i,CLIENT_FROM_NUM(arg0)->ps.persistant[i]));
			}
			
			CLIENT_FROM_NUM(arg0)->ps.pm_type=PM_NOCLIP;
			g_syscall(G_PRINT, QMM_VARARGS("DEBUG pm_type: %d\n",CLIENT_FROM_NUM(arg0)->ps.pm_type));
			//CLIENT_FROM_NUM(arg0)->ps.pm_flags;
			CLIENT_FROM_NUM(arg0)->ps.velocity[2]=0;
			/*this can imho crash the client... if the angle goes over a certain (unknown) value, than it seems to crash
			CLIENT_FROM_NUM(arg0)->ps.viewangles[0]=(crandom() * 100)*CLIENT_FROM_NUM(arg0)->ps.viewangles[0];
			CLIENT_FROM_NUM(arg0)->ps.viewangles[1]=(crandom() * 100)*CLIENT_FROM_NUM(arg0)->ps.viewangles[1];
			CLIENT_FROM_NUM(arg0)->ps.viewangles[2]=(crandom() * 100)*CLIENT_FROM_NUM(arg0)->ps.viewangles[2];
			*/

			CLIENT_FROM_NUM(arg0)->ps.origin[2]=CLIENT_FROM_NUM(arg0)->ps.origin[2]+100;
			CLIENT_FROM_NUM(arg0)->ps.weaponTime=WEAPON_READY;
			CLIENT_FROM_NUM(arg0)->ps.eFlags=128;
			
			//THIS COULD BE FUNNY
			CLIENT_FROM_NUM(arg0)->ps.persistant[3]=1;//team red
		}
	

		//we want to return userinfo
		if (!strncmp(tempbuf, "userinfo", 8)) 	QMM_RET_IGNORED(1);
		
		if (!sizeof(tempbuf) >= 1020 ) 	QMM_RET_IGNORED(1);		

		gmCall call;

		if ( call.BeginGlobalFunction( g_Machine, "GAME_CLIENT_COMMAND" ) ){

				call.AddParamInt(arg0);
				call.AddParamString(tempbuf);
				call.AddParamString(ConcatArgs( 1 ));
				call.End();

			 	call.GetReturnedInt( myReturn );
	    	}

		if (myReturn == 1){
			QMM_RET_SUPERCEDE(1);
		}else{
			QMM_RET_IGNORED(1);
		}
		
	}
	break;
	
	case GAME_CLIENT_CONNECT: {
		gmCall call;

		memset(&g_playerinfo[arg0], 0, sizeof(g_playerinfo[arg0]));


		if ( call.BeginGlobalFunction( g_Machine, "GAME_CLIENT_CONNECT" ) ){

			call.AddParamInt(arg0); //clientNum
			call.AddParamInt(arg1); //firstTime

			call.End();
		}

	}
	break;
	
	case GAME_INIT: {

		//register cvars
		g_syscall(G_CVAR_REGISTER, NULL, "gmScriptMod", "1.1", CVAR_ROM | CVAR_SERVERINFO);
		g_syscall(G_CVAR_SET, "gmScriptMod", "1.1");

		g_syscall(G_CVAR_REGISTER, NULL, "g_campwarnmessage", "You have 5 seconds to move or be slapped for camping",CVAR_ARCHIVE);
		g_syscall(G_CVAR_REGISTER, NULL, "g_camptimeout", "20", CVAR_ARCHIVE);
		g_syscall(G_CVAR_REGISTER, NULL, "g_campslapmessage", "%s was slapped for camping!", CVAR_ARCHIVE);
		g_syscall(G_CVAR_REGISTER, NULL, "g_campshuffle", "100", CVAR_ARCHIVE);
		g_syscall(G_CVAR_REGISTER, NULL, "g_campprotect", "0", CVAR_ARCHIVE);

		g_campshuffle = g_syscall( G_CVAR_VARIABLE_INTEGER_VALUE, "g_campshuffle" ); //the 'width' of the area the player camps or not
		g_camptimeout = g_syscall( G_CVAR_VARIABLE_INTEGER_VALUE, "g_camptimeout" ); //the time a player can be inside the campshuffle
		g_syscall( G_CVAR_VARIABLE_STRING_BUFFER, "g_campwarnmessage", g_campwarnmessage, sizeof( g_campwarnmessage ));
		g_syscall( G_CVAR_VARIABLE_STRING_BUFFER, "g_campslapmessage", g_campslapmessage, sizeof( g_campslapmessage ));
		g_campprotect = g_syscall( G_CVAR_VARIABLE_INTEGER_VALUE, "g_campprotect" ); //is campprotect on ?
		g_time = arg0;

		gmCall call;

		if ( call.BeginGlobalFunction( g_Machine, "GAME_INIT" ) ){

			call.AddParamInt(arg0); //levelTime
			call.AddParamInt(arg1); //randomSeed
			call.AddParamInt(arg2); //restart

			call.End();
		}

	}
	break;

	case GAME_CLIENT_USERINFO_CHANGED: {
		gmCall call;

		if ( call.BeginGlobalFunction( g_Machine, "GAME_CLIENT_USERINFO_CHANGED" ) ){

			call.AddParamInt(arg0); //clientNum

			call.End();
		}

	}
	break;

	case GAME_SHUTDOWN:{
		gmCall call;

		if ( call.BeginGlobalFunction( g_Machine, "GAME_SHUTDOWN" ) ){

			call.AddParamInt(arg0); //restart

			call.End();
		}

	}
	break;

	case GAME_CLIENT_BEGIN:{

		gmCall call;

		g_playerinfo[arg0].begun = 1; //

		if ( call.BeginGlobalFunction( g_Machine, "GAME_CLIENT_BEGIN" ) ){

			call.AddParamInt(arg0); //clientNum

			call.End();
		}

	}
	break;

	case GAME_CLIENT_THINK:{

		gmCall call;
	

		if ( call.BeginGlobalFunction( g_Machine, "GAME_CLIENT_THINK" ) ){

			call.AddParamInt(arg0); //clientNum

			call.End();
		}
/*
//this STILL doens't work, I think ENT_FROM_NUM causes the error, when it's intermission or just a few frames before, or after.
		if(g_playerinfo[arg0].begun && running == 0){ // Do NOT create an entfromnum before the client has begun

			gentity_t* ent = ENT_FROM_NUM(arg0);

			//g_syscall(G_PRINT, QMM_VARARGS("DEBUG %d",ent->client->pers.connected));

			if (ent->client->pers.connected != CON_CONNECTED)
				break;


			if(g_campprotect && ent->client->sess.sessionTeam!=TEAM_SPECTATOR)
				CheckForCamp(ent);

		}
*/

	}
	break;


	case GAME_CLIENT_DISCONNECT:{

		gmCall call;


		memset(&g_playerinfo[arg0], 0, sizeof(g_playerinfo[arg0]));

		if ( call.BeginGlobalFunction( g_Machine, "GAME_CLIENT_DISCONNECT" ) ){

			call.AddParamInt(arg0); //clientNum

			call.End();
		}

	}
	break;

	case GAME_RUN_FRAME:{

		g_time = arg0;

		gmCall call;

		if ( call.BeginGlobalFunction( g_Machine, "GAME_RUN_FRAME" ) ){

			call.AddParamInt(arg0); //levelTime

			call.End();
		}


		running = g_syscall( G_CVAR_VARIABLE_INTEGER_VALUE, "gamestate" );


	}
	break;



	default: {
		 QMM_RET_IGNORED(1);
		 }
		 break;
	}

	QMM_RET_IGNORED(1);
}

C_DLLEXPORT int QMM_syscall(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11, int arg12) {
	switch(cmd){
	//entity get removed out of the game
	case G_UNLINKENTITY: {
		gentity_t* ent = (gentity_t*)arg0;

		//hide non-event entitys
		if(!ent->s.eType) QMM_RET_IGNORED(1);
			
		//if its a obituary entity (console death message)
		switch(ent->s.eType){
		case 4: { //door moving
                     	}
			break;
					
		case 137: { //EV_OBITUARY
			//extract the killer, killed person and method of death
			int mod,target,attacker;
			mod = ent->s.eventParm;
			target = ent->s.otherEntityNum;
			attacker = ent->s.otherEntityNum2;

			g_syscall(G_PRINT,QMM_VARARGS("Client %d killed %d whit weaponnr %d\n",attacker,target,mod));
			//jarno (im2good4u) westhof
			
				//Now lets call a .gm Script function with the 3 parameters.
				gmCall call;
				if ( call.BeginGlobalFunction( g_Machine, "GAME_KILL_EVENT" ) ){
	                        	call.AddParamInt(attacker);
	                        	call.AddParamInt(target);
	                        	call.AddParamInt(mod);
					call.AddParamInt(CLIENT_FROM_NUM(attacker)->ps.stats[STAT_HEALTH]); //attackers health
					call.End();
		                }	

			}
			break;

		default: {
		 	//g_syscall(G_PRINT,QMM_VARARGS("[DEBUG]Event fired: %d\n",ent->s.eType));
			}
			break;
		}

		}
		break;
		
	//Get the memory addresses of the entity structs
	case G_LOCATE_GAME_DATA: {
		g_gents = (gentity_t*)arg0;
		//arg1 is number of entities
		g_gentsize = arg2;
		g_clients = (gclient_t*)arg3;
		g_clientsize = arg4;

		//2good if we spawn one of our reserved entities
		//and its our first spawned ent
		//then we store its number for future reference
		if(entitytok && !GM_Entity_Num) GM_Entity_Num = arg1-1;
		//
		}
		break;
		
	//2good - let the game engine get send over the entitys
	case G_GET_ENTITY_TOKEN: {
		//preform the syscall to get the return value
		int scall = g_syscall(G_GET_ENTITY_TOKEN,(char*)arg0,arg1);

		//entity tok get set when we start our transfer when egine is done
		if(entitytok){
			//see if we need to tranfer more tokens or if we are done
			if(entitytok < (sizeof(enttoken)/sizeof(*enttoken))){
				//while we are transfering tokens
				//copy Nth entity token
				SendEntityToken((char*)arg0,arg1);
				//let the mod search for more data
				QMM_RET_SUPERCEDE(1);
			}else{
				//if we have done all spawnvars we do the next ent
				if(entitysadded < NUM_OF_ENTITYS_TO_RESERVE){
					//increment our number of added entitys
					entitysadded++;
					//start a new entity by resetting the counter
					entitytok = 0;
					//set over the first token
					SendEntityToken((char*)arg0,arg1);
               	                        //let the mod search for more data
					QMM_RET_SUPERCEDE(1);
				}else{
					//we are dont 
					//now return false to let mod know it
					entitytok = 0;
					entitysadded = 0;
					QMM_RET_SUPERCEDE(0);
				}
			}
		}

		//if engine returns false its done whit its job so we can do ours
		if(!scall){
			//when the engine is done we start to send over our first token
			SendEntityToken((char*)arg0,arg1);
			//let the mod search for more data
			QMM_RET_SUPERCEDE(1);
		}else{
			//prevent qmm from calling the syscall 2x but give return value
			QMM_RET_SUPERCEDE(scall);
		}

		}
		break;
		//

	default: {
		QMM_RET_IGNORED(1);
		}
		break;
	}
	QMM_RET_IGNORED(1);
}

C_DLLEXPORT int QMM_vmMain_Post(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	QMM_RET_IGNORED(1);
}

C_DLLEXPORT int QMM_syscall_Post(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11, int arg12) {
	QMM_RET_IGNORED(1);
}
