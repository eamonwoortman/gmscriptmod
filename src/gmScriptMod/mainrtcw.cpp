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

//GameMonkey
#include "gmThread.h"
#include "gmGoNeLib.h"
#include "gmMachine.h"
#include "gmCall.h"   // Header contains helper class
#include "gmTableObject.h"

/*
#include <gloox/client.h>
#include <gloox/messagehandler.h>
using namespace gloox;
*/

#include <iostream>

#define RTCWMP 1

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

level_locals_t* g_level = NULL; 

char fileP[150]; 

//2good - reserved entity stuff
#define NUM_OF_ENTITYS_TO_RESERVE 5

int GM_Entity_Num = 0;
int GM_Entities = 0; //net nieuw gemaakt...

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
int g_starttime = 0;

int g_cloaking = 0;

int eventDebug = 0;

typedef struct playerinfo_s {
	bool begun;
	bool smoke;
} playerinfo_t;

playerinfo_t g_playerinfo[MAX_CLIENTS];

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


int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create ) {
	int		i;
	char	s[MAX_STRING_CHARS];

	if ( !name || !name[0] ) return 0;
	
	for ( i=1 ; i<max ; i++ ) {

		g_syscall( G_GET_CONFIGSTRING,start + i, s, sizeof( s ) );
		if ( !s[0] ) break;
		
		if ( !strcmp( s, name ) ) return i;
	}

	if ( !create ) return 0;												     
	if ( i == max ) g_syscall(G_ERROR,"G_FindConfigstringIndex: overflow");
					  //Setting C: '341' 'sound/q3tc/siren.wav' '320'
	g_syscall(G_PRINT,QMM_VARARGS("Setting C: '%d' '%s' '%d' \n",i,name, (CS_MODELS + MAX_MODELS) )); 
	g_syscall( G_SET_CONFIGSTRING, start + i, name );
	
	return i;
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

/*
class Bot : public MessageHandler
{
 public:
	Bot()
       	{
       		JID jid( "bot@server/resource" );
		j = new Client( jid, "pwd" );
		j->registerMessageHandler( this );
		j->setPresence( PresenceAvailable, 5 );
		j->connect();
	}

	virtual void handleMessage( Stanza* stanza,
                               MessageSession* session = 0 )
	{
		Stanza *s = Stanza::createMessageStanza(
		stanza->from().full(), "hello world" );
		j->send( s );
	}
 private:
 	Client* j;

};
*/

C_DLLEXPORT void QMM_Query(plugininfo_t** pinfo) {
	QMM_GIVE_PINFO();
}
C_DLLEXPORT int QMM_Attach(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, int vmbase, int iscmd) {
	QMM_SAVE_VARS();

	iscmd = 0;
	g_pluginfuncs = pluginfuncs; 

	g_syscall( G_CVAR_VARIABLE_STRING_BUFFER, "fs_homepath", fileP, sizeof(fileP));

	eventDebug = g_syscall(G_CVAR_VARIABLE_INTEGER_VALUE, "g_debugEvents");

	//rtcw cloaking 
	g_cloaking = g_syscall(G_CVAR_VARIABLE_INTEGER_VALUE, "g_cloaking1");

	g_Machine = new gmMachine();
	gmBindGoNeLib(g_Machine);		//Bind the GoNe Lib to our Machine

	gmScan_Register(g_Machine);

	gmLoadAndExecuteScript(*g_Machine, QMM_VARARGS("%s/gmScriptMod/NeelixScript.gm",fileP));

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

/*
//currently not in use!
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
*/
/*


//this one does the referee.wav
void BACKUPSpawnEnt(){

			g_syscall(G_PRINT, QMM_VARARGS("[ENTITY DEBUG] Playing Sound on %d\n",GM_Entity_Num));
			gentity_t *e = ENT_FROM_NUM(GM_Entity_Num);

			

			vec3_t	snapped;
			//vec3_t  origin;

			//GlobalSound()
			////TempEntity()
			vec3_t origin = {0,0,0};

			VectorCopy( e->r.currentOrigin, origin);

			//////InitEntity()
			e->inuse = qtrue;
			e->classname = "noclass";
			e->s.number = GM_Entity_Num;
			e->r.ownerNum = ENTITYNUM_NONE;
			e->aiInactive = 0xffffffff;
			e->nextthink = 0;
			memset(e->goalPriority, 0, sizeof(e->goalPriority));
			e->free = NULL;
			e->scriptStatus.scriptEventIndex = -1;
			e->spawnCount++;
			e->spawnTime = g_time;

			//prevent interaction with the game
			g_syscall(G_UNLINKENTITY, e);

			//////TempEntity
			//e->s.eType = EV_GLOBAL_SOUND;
			//e->s.eType = (entityType_t)(ET_EVENTS + EV_GLOBAL_SOUND);
			//e->s.eType = (entityType_t)1;
			//
			e->s.eType = (entityType_t)120; //120 (entityType_t)(ET_EVENTS + EV_GLOBAL_SOUND);
			//

			e->classname = "tempEntity";
			e->eventTime = g_time;
			e->r.eventTime = g_time;
			e->freeAfterEvent = qtrue;
			VectorCopy( origin, snapped );
			SnapVector( snapped );		// save network bandwidth
			
			//////SetOrigin()
			VectorCopy( snapped, e->s.pos.trBase );
			e->s.pos.trType = TR_STATIONARY;
			e->s.pos.trTime = 0;
			e->s.pos.trDuration = 0;
			VectorClear( e->s.pos.trDelta );
			VectorCopy( snapped, e->r.currentOrigin );

			//////TrapLink
			g_syscall(G_LINKENTITY,e);

			//sound/GoNe/bananaphone.wav
			//Getting Configstring:322: sound/misc/referee.wav
			//
			////Unliked EventID: 120 Parm: 1 other1: 0 other2 0

			//grenade:
			//Unliked EventID: 187 Parm: 0 other1: 0 other2: 0 svFlags: 32

			//endround
			//Unliked EventID: 122 Parm: 23 other1: 0 other2: 0 svFlags: 32 classname: tempEntity 

			e->s.eventParm = 1;
			//22; //G_FindConfigstringIndex("sound/q3tc/siren.wav",CS_SOUNDS,MAX_SOUNDS,qtrue);

 			//120 Parm: 22 other1: 0 other2: 0 svFlags: 32 classname: tempEntity eventtime: 218400 runthisframe: 1


			e->r.svFlags |= SVF_BROADCAST;
			//e->r.svFlags = 32;

			g_syscall (G_PRINT, QMM_VARARGS( "g_gents: %d\n", g_gents[GM_Entity_Num] ));

}

void TCE_SpawnEnt(int index){

	//dit word een bitflag die bijhoud of zone entites wel of niet ingewbruikt zijn ;P
	//GM_Entities

	int maxSlots = GM_Entity_Num + NUM_OF_ENTITYS_TO_RESERVE;

	//find out witch entity slot is useable
	for(int i=GM_Entity_Num;i<maxSlots;i++){

		gentity_t *e = ENT_FROM_NUM(i);

		if((e) && !(GM_Entities & (1 << i))){

			g_syscall(G_PRINT, QMM_VARARGS("[ENTITY DEBUG] Playing Sound on %d\n",i));

			gentity_t *e = ENT_FROM_NUM(GM_Entity_Num);
			vec3_t	snapped;

			////TempEntity()
			vec3_t origin = {0,0,0};

			VectorCopy( e->r.currentOrigin, origin);

			//////InitEntity()
			e->inuse = qtrue;
			e->classname = "noclass";
			e->s.number = GM_Entity_Num;
			e->r.ownerNum = ENTITYNUM_NONE;
			e->aiInactive = 0xffffffff;
			e->nextthink = 0;
			memset(e->goalPriority, 0, sizeof(e->goalPriority));
			e->free = NULL;
			e->scriptStatus.scriptEventIndex = -1;
			e->spawnCount++;
			e->spawnTime = g_time;

			//////TempEntity
			//EV_GLOBAL_SOUND
			e->s.eType = (entityType_t)120; //120 (entityType_t)(ET_EVENTS + EV_GLOBAL_SOUND);

			e->classname = "tempEntity";
			e->eventTime = g_time;
			e->r.eventTime = g_time;
			e->freeAfterEvent = qtrue;
			VectorCopy( origin, snapped );
			SnapVector( snapped );		// save network bandwidth
			
			//////SetOrigin()
			VectorCopy( snapped, e->s.pos.trBase );
			e->s.pos.trType = TR_STATIONARY;
			e->s.pos.trTime = 0;
			e->s.pos.trDuration = 0;
			VectorClear( e->s.pos.trDelta );
			VectorCopy( snapped, e->r.currentOrigin );

			//////TrapLink
			g_syscall(G_LINKENTITY,e);
	
			e->s.eventParm = index;
			e->r.svFlags |= SVF_BROADCAST; //32

			//set the bitflag of the GM_Entities thingie
			GM_Entities |= i;

			g_syscall(G_PRINT,QMM_VARARGS("Linked  %d is now used",i));

			return;
		}
	}

	g_syscall(G_PRINT, QMM_VARARGS("[ENTITY DEBUG] No free spaces found"));

}

*/
void artilleryGoAway2(gentity_t *ent) {
	ent->freeAfterEvent = qtrue;
	g_syscall(G_LINKENTITY, ent);
}

void GenerateSmoke( gentity_t *ent ) {

	SnapVector( ent->s.pos.trBase );

	int maxSlots = GM_Entity_Num + NUM_OF_ENTITYS_TO_RESERVE;

	//find out witch entity slot is useable
	for(int i=GM_Entity_Num;i<maxSlots;i++){

		gentity_t *e = ENT_FROM_NUM(i);

		if((e) && !(GM_Entities & (1 << i))){

			//g_syscall(G_PRINT, QMM_VARARGS("[ENTITY DEBUG] Playing Sound on %d\n",i));

			gentity_t *bomb = ENT_FROM_NUM(GM_Entity_Num);

			//////TrapLink
			//g_syscall(G_LINKENTITY,e);
	
			bomb->s.eType		= ET_MISSILE;
			bomb->r.svFlags		= SVF_USE_CURRENT_ORIGIN;
			bomb->r.ownerNum	= ent->s.number;
			bomb->parent		= ent;
			bomb->nextthink = g_time + 100;
			bomb->classname	= "WP"; // WP == White Phosphorous, so we can check for bounce noise in grenade bounce routine
			bomb->damage		= 000; // maybe should un-hard-code these?
			bomb->splashDamage  = 000;
			bomb->splashRadius	= 000;
			bomb->s.weapon	= WP_SMOKETRAIL;
			bomb->think = artilleryGoAway2;
			bomb->s.eFlags |= EF_BOUNCE;
			bomb->clipmask = MASK_MISSILESHOT;
			//bomb->s.pos.trType = TR_GRAVITY_LOW; // was TR_GRAVITY,  might wanna go back to this and drop from height
			bomb->s.pos.trTime = g_time;		// move a bit on the very first frame
		
			if (ent->client->sess.sessionTeam == TEAM_RED) // use team so we can generate red or blue smoke
				bomb->s.otherEntityNum2 = 1;
			else
				bomb->s.otherEntityNum2 = 0;

			SnapVector( bomb->s.pos.trDelta );			// save net bandwidth
			VectorCopy(ent->s.pos.trBase,bomb->s.pos.trBase);
			VectorCopy(ent->s.pos.trBase,bomb->r.currentOrigin);

			//set the bitflag of the GM_Entities thingie
			GM_Entities |= i;

			//g_syscall(G_PRINT,QMM_VARARGS("Linked  %d is now used",i));

			return;
		}
	}

	g_syscall(G_PRINT, QMM_VARARGS("[ENTITY DEBUG] No free spaces found"));		


		
}

void RTCW_SpawnEnt(int index){

	//dit word een bitflag die bijhoud of zone entites wel of niet ingewbruikt zijn ;P
	//GM_Entities

	int maxSlots = GM_Entity_Num + NUM_OF_ENTITYS_TO_RESERVE;

	//find out witch entity slot is useable
	for(int i=GM_Entity_Num;i<maxSlots;i++){

		gentity_t *e = ENT_FROM_NUM(i);

		if((e) && !(GM_Entities & (1 << i))){

			//g_syscall(G_PRINT, QMM_VARARGS("[ENTITY DEBUG] Playing Sound on %d\n",i));

			gentity_t *e = ENT_FROM_NUM(GM_Entity_Num);
			vec3_t	snapped;

			////TempEntity()
			vec3_t origin = {0,0,0};

			VectorCopy( e->r.currentOrigin, origin);

			//////InitEntity()
			e->inuse = qtrue;
			e->classname = "noclass";
			e->s.number = GM_Entity_Num;
			e->r.ownerNum = ENTITYNUM_NONE;
			e->nextthink = 0;
			e->scriptStatus.scriptEventIndex = -1;

			//////TempEntity
			//EV_GLOBAL_SOUND
			e->s.eType = (entityType_t)(ET_EVENTS + EV_GLOBAL_SOUND);

			e->classname = "tempEntity";
			e->eventTime = g_time;
			e->r.eventTime = g_time;
			e->freeAfterEvent = qtrue;
			VectorCopy( origin, snapped );
			SnapVector( snapped );		// save network bandwidth
			
			//////SetOrigin()
			VectorCopy( snapped, e->s.pos.trBase );
			e->s.pos.trType = TR_STATIONARY;
			e->s.pos.trTime = 0;
			e->s.pos.trDuration = 0;
			VectorClear( e->s.pos.trDelta );
			VectorCopy( snapped, e->r.currentOrigin );

			//////TrapLink
			g_syscall(G_LINKENTITY,e);
	
			e->s.eventParm = index;
			e->r.svFlags |= SVF_BROADCAST; //32

			//set the bitflag of the GM_Entities thingie
			GM_Entities |= i;

			//g_syscall(G_PRINT,QMM_VARARGS("Linked  %d is now used",i));

			return;
		}
	}

	g_syscall(G_PRINT, QMM_VARARGS("[ENTITY DEBUG] No free spaces found"));

}
//linked EventID: 3 Parm: 0 other1: 17 other2: 0 svFlags: 32 classname: smoke_grenade eventtime: 305700 runthisframe: 1

C_DLLEXPORT int QMM_vmMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	switch(cmd){	

	//2good on top since it gets fires a lot
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

	//2good top since it gets called a lot

        case GAME_CLIENT_THINK:{
 	       gmCall call;

		 if(!g_playerinfo[arg0].begun) QMM_RET_IGNORED(1);


               if ( call.BeginGlobalFunction( g_Machine, "GAME_CLIENT_THINK" ) ){
	               call.AddParamInt(arg0); //clientNum
                      call.End();
               }

		gentity_t* ent = ENT_FROM_NUM(arg0);

		usercmd_t	*ucmd;
			
		ucmd = &ent->client->pers.cmd;

		if (g_playerinfo[arg0].smoke){
			gclient_t	*client = CLIENT_FROM_NUM(arg0);
			


			if (ucmd->upmove > 0)
				g_syscall(G_SEND_SERVER_COMMAND, arg0,"print \"Up move\"");
			else
				g_syscall(G_SEND_SERVER_COMMAND, arg0,"print \"NO\"");

			GenerateSmoke(ent);
		}

	
		if(running == 0){ 
			
			if (g_cloaking){


				if (ent->client->ps.eFlags & EF_NODRAW && ent->health > 10){
					ent->s.onFireEnd = g_time + 500;
					ent->health = ent->health - 10;
				}else{
					ent->s.onFireEnd = g_time;
				}

				if (ent->client->ps.eFlags & EF_NODRAW && (ent->client->pers.cmd.buttons & BUTTON_ATTACK || ent->client->ps.stats[STAT_HEALTH] <= 10)){
					ent->client->ps.eFlags &= ~EF_NODRAW;
					ent->s.onFireEnd = g_time;
					g_syscall(G_SEND_SERVER_COMMAND, arg0,"print \"Cloaking DISABLED\"");
	 			}
			}
		}

	/*
	 	//this STILL doens't work, I think ENT_FROM_NUM causes the error, when it's intermission or just a few frames before, or after.
		if(g_playerinfo[arg0].begun && running == 0){ // Do NOT create an entfromnum before the client has begun
	                gentity_t* ent = ENT_FROM_NUM(arg0);

			//g_syscall(G_PRINT, QMM_VARARGS("DEBUG %d",ent->client->pers.connected));
			
			if (ent->client->pers.connected != CON_CONNECTED) break;
			
			if(g_campprotect && ent->client->sess.sessionTeam!=TEAM_SPECTATOR) CheckForCamp(ent);
		}
	*/
	}
	break;
	
	//server consome command
	case GAME_CONSOLE_COMMAND: {
		int myReturn = 0;

		//check what command it is
		char tempbuf[128];
		g_syscall(G_ARGV, 0, tempbuf, sizeof(tempbuf));

		//exec("slap cnum 1000"); 
		
		//2good debug command for the entity thingy
		if (!strncmp(tempbuf, "entity",6)){
			
			char arg1[256];
			g_syscall(G_ARGV, 1, arg1, sizeof(arg1));

			int index = G_FindConfigstringIndex(arg1,CS_SOUNDS,MAX_SOUNDS,qtrue);
		
			if (RTCWMP)
				RTCW_SpawnEnt(index);
			//else
			//	TCE_SpawnEnt(index);

			QMM_RET_SUPERCEDE(1);

		}


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
		


/*gmCall monkeycall;
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
			//this can imho crash the client... if the angle goes over a certain (unknown) value, than it seems to crash
			CLIENT_FROM_NUM(arg0)->ps.viewangles[0]=(crandom() * 100)*CLIENT_FROM_NUM(arg0)->ps.viewangles[0];
			CLIENT_FROM_NUM(arg0)->ps.viewangles[1]=(crandom() * 100)*CLIENT_FROM_NUM(arg0)->ps.viewangles[1];
			CLIENT_FROM_NUM(arg0)->ps.viewangles[2]=(crandom() * 100)*CLIENT_FROM_NUM(arg0)->ps.viewangles[2];
			

			CLIENT_FROM_NUM(arg0)->ps.origin[2]=CLIENT_FROM_NUM(arg0)->ps.origin[2]+100;
			CLIENT_FROM_NUM(arg0)->ps.weaponTime=WEAPON_READY;
			CLIENT_FROM_NUM(arg0)->ps.eFlags=128;
			
			//THIS COULD BE FUNNY
			CLIENT_FROM_NUM(arg0)->ps.persistant[3]=1;//team red
		}
*/

		//check what command it is
		char tempbuf[128];
		g_syscall(G_ARGV, 0, tempbuf, sizeof(tempbuf));
	

		if (!strncmp(tempbuf, "smoketest",5)){
			
			if (!g_playerinfo[arg0].smoke){
				g_playerinfo[arg0].smoke = qtrue;
				g_syscall(G_SEND_SERVER_COMMAND, arg0,"print \"Smoke On\"");
			}else{
				g_playerinfo[arg0].smoke = false;
				g_syscall(G_SEND_SERVER_COMMAND, arg0,"print \"Smoke OFF\"");
			}

		}

		if (!strncmp(tempbuf, "testcloak",5)){

			gentity_t *ent = ENT_FROM_NUM(arg0);

			/*if (ent->client->ps.eFlags & EF_NODRAW){
				ent->client->ps.eFlags &= ~EF_NODRAW;	//disable cloaking
				cp("Cloaking DISABLED!");
			}else{
				ent->client->ps.eFlags |= EF_NODRAW;		//enable cloaking	
				cp("Cloaking ENABLED!");
			}*/

			char *msg;

			if(!g_cloaking) {
				g_syscall(G_SEND_SERVER_COMMAND, arg0,"print \"cloaking has been disabled on this server.\n\"");
				QMM_RET_SUPERCEDE(1);
			}

			if(ent->health <= 10){
				g_syscall(G_SEND_SERVER_COMMAND, arg0,"print \"You do not have enough health to cloak\"");
				QMM_RET_SUPERCEDE(1);
			}
	
			if(ent->client->sess.sessionTeam == TEAM_SPECTATOR){
				g_syscall(G_SEND_SERVER_COMMAND, arg0,"print \"You can't cloak while spectator\"");
				QMM_RET_SUPERCEDE(1);
			}
	
			if (ent->client->ps.eFlags & EF_NODRAW){
				ent->client->ps.eFlags &= ~EF_NODRAW;	//disable cloaking
				msg = "Cloaking DISABLED!";
			}else{
				ent->client->ps.eFlags |= EF_NODRAW;		//enable cloaking	
				msg = "Cloaking ENABLED!";
			}

			g_syscall(G_SEND_SERVER_COMMAND, arg0,QMM_VARARGS("print \"%s\"",msg));

			QMM_RET_SUPERCEDE(1);
		}
/*
		if (!strncmp(tempbuf, "third",5)){
	
			char buffer[1024];

			g_syscall(G_GET_CONFIGSTRING, CS_SYSTEMINFO, buffer, sizeof(buffer) );
			
			strcpy (buffer,"\\cg_thirdperson\\1");

			g_syscall(G_SET_CONFIGSTRING, CS_SYSTEMINFO, buffer );
			
			g_syscall(G_PRINT, QMM_VARARGS("SYSTEMINFO '%s'",buffer));
		}
*/
		//if (!strncmp(tempbuf, "black",5)){
		//	g_syscall( G_SET_CONFIGSTRING, CS_SCREENFADE, QMM_VARARGS("1 %i 1", g_time - 10 ) );
		//}

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

/*
//not used
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
*/
		g_time = arg0;
		g_starttime = arg0;
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

	case GAME_CLIENT_BEGIN:{

		gmCall call;

		g_playerinfo[arg0].begun = 1; //

		if ( call.BeginGlobalFunction( g_Machine, "GAME_CLIENT_BEGIN" ) ){

			call.AddParamInt(arg0); //clientNum

			call.End();
		}

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

	//at the bottom since it only gets called 1 time
	case GAME_SHUTDOWN:{
	 	gmCall call;

		if ( call.BeginGlobalFunction( g_Machine, "GAME_SHUTDOWN" ) ){
	                call.AddParamInt(arg0); //restart
                	call.End();
		}

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
	
	 if (cmd == G_FS_FOPEN_FILE) { 
                //check if the args match the logging call 
                if ((arg2 == FS_APPEND_SYNC || arg2 == FS_APPEND) && !strcmp((char*)arg0, QMM_GETSTRCVAR("g_log"))) { 
                        g_level = (level_locals_t*)(arg1 - sizeof(struct gclient_s*) - sizeof(struct gentity_s*) - (sizeof(int) * 3)); 
                } 
        } 
	


	switch(cmd){
	
	/*
	g_syscall( G_CVAR_VARIABLE_STRING_BUFFER, "gamename", originalGameName, sizeof(originalGameName));

	g_syscall(G_CVAR_SET, "gamename", QMM_VARARGS("%s|GMSM",originalGameName));
	*/

	case G_LINKENTITY:{
		
		gentity_t* ent = (gentity_t*)arg0;



		//dus slaat deze if nergens op yep :P
		if(NUM_FROM_ENT(ent) == GM_Entity_Num){
			if(eventDebug)g_syscall(G_PRINT, "LINK GMENT\n");
		}

		//hide non-event entitys
		if(!ent->s.eType) QMM_RET_IGNORED(1);
		
 		int event,eventParm, otherEntityNum,otherEntityNum2,svFlags;

		event = eventParm = otherEntityNum = otherEntityNum2 = svFlags = -1; // we have to init them in case an event doesn't use them
		//char classname[60];
		//e->r.svFlags

		event = ent->s.eType;
		eventParm = ent->s.eventParm;
		otherEntityNum = ent->s.otherEntityNum;
		otherEntityNum2 = ent->s.otherEntityNum2;
		svFlags = ent->r.svFlags;


              //if (event != 1)g_syscall(G_PRINT,  QMM_VARARGS("linked EventID: %d Parm: %d other1: %d other2: %d svFlags: %d classname: %s eventtime: %d \n",event,eventParm, otherEntityNum,otherEntityNum2,svFlags,ent->classname,ent->eventTime));


	}break;

	//entity get removed out of the game
	case G_UNLINKENTITY: {
 		gentity_t* ent = (gentity_t*)arg0; //kiek maar >.<
		int num = NUM_FROM_ENT(ent);
		int maxSlots = GM_Entity_Num + NUM_OF_ENTITYS_TO_RESERVE;
		
		for (int i = GM_Entity_Num; i < maxSlots; i++){
			if(num  == i){
				GM_Entities &= ~num; 
				if(eventDebug)g_syscall(G_PRINT, "UNLINK GMENT\n");
			}
		}

		//if one of our entties is given free
		if((num >= GM_Entity_Num) && (num < NUM_OF_ENTITYS_TO_RESERVE)){
			GM_Entities ^= (1 << num);
			if(eventDebug)g_syscall(G_PRINT,QMM_VARARGS("Unlinked %d is now free",num));
		}

		//hide non-event entitys
		if(!ent->s.eType) QMM_RET_IGNORED(1);
		
 		int event,eventParm, otherEntityNum,otherEntityNum2,svFlags;

		event = eventParm = otherEntityNum = otherEntityNum2 = svFlags = -1; // we have to init them in case an event doesn't use them
		//char classname[60];
		//e->r.svFlags

		event = ent->s.eType;
		eventParm = ent->s.eventParm;
		otherEntityNum = ent->s.otherEntityNum;
		otherEntityNum2 = ent->s.otherEntityNum2;
		svFlags = ent->r.svFlags;


             if(eventDebug){
			if (event != 1 && event != 4)g_syscall(G_PRINT,  QMM_VARARGS("Unlinked EventID: %d Parm: %d other1: %d other2: %d svFlags: %d classname: %s eventtime: %d \n",event,eventParm, otherEntityNum,otherEntityNum2,svFlags,ent->classname,ent->eventTime));
		}
		     
		//Unliked EventID: 120 Parm: 1 other1: 0 other2 0

		gmCall call;
		if ( call.BeginGlobalFunction( g_Machine, "GMSM_GAME_EVENTS" ) ){
			call.AddParamInt(event); //event
			call.AddParamInt(otherEntityNum); //target
			call.AddParamInt(otherEntityNum2); //attacker
			call.AddParamInt(eventParm); //mod
			if (event = 137)
				call.AddParamInt(CLIENT_FROM_NUM(otherEntityNum2)->ps.stats[STAT_HEALTH]); //attackers health

			call.End();
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
		//
		//
	

	case G_SET_CONFIGSTRING:{
		if(eventDebug){
			if(arg0 != 14)g_syscall(G_PRINT,  QMM_VARARGS("Sending Configstring:%d: %s\n",(arg0),(char*)arg1));
		}
		//if(arg0 == ( CS_MODELS + MAX_MODELS)) g_syscall(G_PRINT,  QMM_VARARGS("Sending Configstring:%d: %s\n",(arg0),(char*)arg1));

	
	//wacht ff
	}
 	break;

	case G_GET_CONFIGSTRING:{

		if(eventDebug){
			if(arg0 != 14)g_syscall(G_PRINT,  QMM_VARARGS("Getting Configstring:%d: %s\n",(arg0),(char*)arg1));
		}

		//if(arg0 != 14)g_syscall(G_PRINT,  QMM_VARARGS("Sending Configstring:%d: %s\n",(arg0),(char*)arg1));
		//if(arg0 == ( CS_MODELS + MAX_MODELS)) g_syscall(G_PRINT,  QMM_VARARGS("Sending Configstring:%d: %s\n",(arg0),(char*)arg1));

/*
Getting Configstring:321: sound/movers/doors/door1_loopo.wav
Getting Configstring:322: sound/misc/referee.wav
Getting Configstring:323: sound/misc/vote.wav
Getting Configstring:324: sound/player/gurp1.wav
Getting Configstring:325: sound/player/gurp2.wav
Getting Configstring:326: sound/q3tc/aircondition.wav
Getting Configstring:327: sound/movers/doors/door1_open.wav
Getting Configstring:328: sound/movers/doors/door1_endo.wav
Getting Configstring:329: sound/movers/doors/door1_close.wav
Getting Configstring:330: sound/movers/doors/door1_endc.wav
Getting Configstring:331: sound/movers/doors/door1_loopo.wav
Getting Configstring:321: sound/movers/doors/door1_loopc.wav
Getting Configstring:322: sound/misc/referee.wav
Getting Configstring:323: sound/misc/vote.wav
Getting Configstring:324: sound/player/gurp1.wav
Getting Configstring:325: sound/player/gurp2.wav
Getting Configstring:326: sound/q3tc/aircondition.wav
Getting Configstring:327: sound/movers/doors/door1_open.wav
Getting Configstring:328: sound/movers/doors/door1_endo.wav
Getting Configstring:329: sound/movers/doors/door1_close.wav
Getting Configstring:330: sound/movers/doors/door1_endc.wav
Getting Configstring:331: sound/movers/doors/door1_loopo.wav
Getting Configstring:332: sound/movers/doors/door1_loopc.wav
Getting Configstring:321: sound/movers/doors/door1_locked.wav
Getting Configstring:322: sound/misc/referee.wav
Getting Configstring:323: sound/misc/vote.wav
Getting Configstring:324: sound/player/gurp1.wav
Getting Configstring:325: sound/player/gurp2.wav

*/

	}
 	break;

case G_ERROR:{

if (strlen((char *)arg0) <= 0) QMM_RET_SUPERCEDE(1);

}break;
	default: {
		QMM_RET_IGNORED(1);
		}
		break;
	}
	QMM_RET_IGNORED(1);
}

C_DLLEXPORT int QMM_vmMain_Post(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {

	if(cmd == GAME_INIT){

		char gameName[150];

		g_syscall( G_CVAR_VARIABLE_STRING_BUFFER, "gamename", gameName, sizeof(gameName));

	  	string str (gameName);
		string str2 ("|GMSM");
		size_t found;

		// different member versions of find in the same order as above:
		found=str.find(str2);
		
		if (found==string::npos)
			g_syscall(G_CVAR_SET,"gamename",QMM_VARARGS("%s|GMSM",gameName));

		
	}

	QMM_RET_IGNORED(1);
}

C_DLLEXPORT int QMM_syscall_Post(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11, int arg12) {
	QMM_RET_IGNORED(1);
}

