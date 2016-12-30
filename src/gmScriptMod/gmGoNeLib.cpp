/*
    _____               __  ___          __            ____        _      __
   / ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
  / (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
  \___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
                                               /___/             /_/
                                             
  See Copyright Notice in gmMachine.h

*/

#include "gmConfig.h"
#include "gmGoNeLib.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmHelpers.h"
#include "gmUtil.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
//#include <strstream>
#include <string>
//TEST INCLUDE
#include <qmmapi.h>
#include <g_local.h>

//
//
// system functions
//
//

extern void gmConcat(gmMachine * a_machine, char * &a_dst, int &a_len, int &a_size, const char * a_src, int a_growBy = 32);

//logfile handle
static int s_fh = -1;

//Eamon, added functions:
static int GM_CDECL gmfCvarSet(gmThread * a_thread)
{
        // Get data from params
        const char* string = a_thread->ParamString(0);
        const char* string2 = a_thread->ParamString(1);
        if(string)
        {
		g_syscall( G_CVAR_SET, string, string2 );            
        }
       return GM_OK;
}

//Apo: Hope its the fixed version :)
//convert integer to string
////Basic function setup is copied from gmStringLib.cpp -->gmStringSetAt()
/*
static int GM_CDECL gmfitoa(gmThread * a_thread)
{
  int i;
    char buffer [33];
      GM_CHECK_NUM_PARAMS(0);
        if(a_thread->ParamType(0)!=GM_INT)
    	 return GM_EXCEPTION;
      i=a_thread->Param(0).m_value.m_int;
       itoa(i,buffer,10);
       gmStringObject* itostring=a_thread->GetMachine()->AllocStringObject(buffer);
      a_thread->PushString(itostring);
        return GM_OK;
}
*/
//convert string to integer
static int GM_CDECL gmfatoi(gmThread * a_thread)
{
const gmVariable * var = a_thread->GetThis();
gmStringObject* strobj=var->GetStringObjectSafe();
GM_ASSERT(var->m_type != GM_STRING);
const char* scriptstring = a_thread->ParamString(0);
int i;
i = atoi(scriptstring);
a_thread->PushInt(i);
//  delete var;
return GM_OK;
}
	

static int GM_CDECL gmfRegisterCvar(gmThread * a_thread)
{
        // Get data from params
	const char* string = a_thread->ParamString(0);
	const char* string2 = a_thread->ParamString(1);

	if(string)
	{
		g_syscall(G_CVAR_REGISTER, NULL, string, string2, CVAR_ARCHIVE);

	}
		return GM_OK;
}
	                                                        
static int GM_CDECL gmfGetConfigString(gmThread * a_thread)
{
	int clientNum;
	char	cstring[1024];

  	// Get data from params
  	clientNum = a_thread->Param(0).m_value.m_int;

	g_syscall( G_GET_CONFIGSTRING, CS_PLAYERS + clientNum, cstring, sizeof( cstring ) );


  	// do we have a userinfo string?
  	if(strlen(cstring) <= 0){
		return GM_EXCEPTION;
  	}

  	// check for malformed or illegal info strings
  	if ( !Info_Validate(cstring) ) {
		return GM_EXCEPTION;
  	}

  	gmStringObject *userstr = a_thread->GetMachine()->AllocStringObject( cstring ); 
  	//gmStringObject *userstr = a_thread->GetMachine()->AllocStringObject( userinfo, sizeof(userinfo) ); 
  
  	a_thread->PushString(userstr);

	
	return GM_OK;
}


int gmIncludeFile( gmMachine *a_machine, const char * include_filename )
{

	const char *filepath = QMM_GETSTRCVAR("fs_homepath");
	include_filename = QMM_VARARGS("%s/gmScriptMod/%s",filepath,include_filename);

	std::ifstream file((const char *)include_filename);

	g_syscall(G_PRINT, QMM_VARARGS( "[GMS Mod] Include, Trying to load '%s'\n",include_filename));

	if (!file){
		g_syscall(G_PRINT, QMM_VARARGS( "[GMS Mod] Include, Error, Could not open '%s'\n",include_filename));
		return GM_EXCEPTION;
	}
	
	std::string fileString = std::string(std::istreambuf_iterator<char>(file),
	std::istreambuf_iterator<char>());
	file.close();

	a_machine->GetLog().Reset();
	int num_errors;
	if (num_errors = a_machine->ExecuteString(fileString.c_str()) > 0)
	{
		g_syscall(G_PRINT,QMM_VARARGS("\n[GMS Mod] Include, Compile failed for '%s'\n\n",include_filename));
		g_syscall(G_PRINT,"------------------------------------------------------------\n");

		bool first = true;

		while ( const char* error=a_machine->GetLog().GetEntry( first ) )
         	{
			g_syscall(G_PRINT,QMM_VARARGS ( "%s\n", error ));
		}

		g_syscall(G_PRINT,"------------------------------------------------------------\n");
		g_syscall(G_PRINT,"[GMS Mod] Include, end of compile debugging\n\n");
       }else{
		g_syscall(G_PRINT,QMM_VARARGS("[GMS Mod] scriptfile '%s' successfully included\n",include_filename));
	}
}

static int GM_CDECL gmfInclude(gmThread * a_thread)
{
	// Get data from params
	const char* string = a_thread->ParamString(0);
	
	if(string)
	{
		gmIncludeFile( a_thread->GetMachine(), string );
	}
	
	return GM_OK;
}

//Strip a string from colorcode
//std::string strip(std::string input)
static int GM_CDECL gmfstrip(gmThread* a_thread)
{
	std::string input=a_thread->ParamString(0);
	std::stringstream s;
	/*
	//OLD AND BUGGY: Had problems with ^5^3 (two color codes´)
	//Fixed in new version
	for(int i =0;i<input.length();i++)
	{
		if(i==input.length()-1){
			if(input[i]=='^'){
				input[i]='\0';
			}
		}	
		if(input[i]=='^')
		{
			i=i+2;
		}
		s << input[i];
	}
	*/
	for(int i =0;i<input.length();i++)
	{
		if(i==input.length()-1){
			if(input[i]=='^'){
				input[i]='\0';
			}
		}
		if(input[i]=='^')
		{
			i=i+2;
			if(input[i]=='^'){
				i=i++;
			}
			else{
				s << input[i];
			}
		}
		else{
			s << input[i];
		}
	}
	gmStringObject* stripstring=a_thread->GetMachine()->AllocStringObject(s.str().c_str());
	a_thread->PushString(stripstring);
	return GM_OK;
};


static int GM_CDECL gmfPlaySound(gmThread * a_thread)
{
	int clientnum, pos;

  	// Get data from params
  	clientnum = a_thread->Param(0).m_value.m_int;

	// Get data from params
	const char* string = a_thread->ParamString(1);
	
	if(string)
	{

		//mu_fade
		//mu_stop
		//mu_play
		//mu_start
		g_syscall(G_SEND_SERVER_COMMAND, clientnum, "mu_stop");
		//va("mu_start %s 1000\n", musicString)
													 
		g_syscall(G_SEND_SERVER_COMMAND, clientnum, QMM_VARARGS("mu_play %s 1000", string));
		
	}
	
	return GM_OK;
}
/*
//=============
//Q_strncpyz
 
//Safe strncpy that ensures a trailing zero
//=============

void Q_strncpyz( char *dest, const char *src, int destsize ) {
	if ( !src ) {
		//Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}
	if ( destsize < 1 ) {
		//Com_Error(ERR_FATAL,"Q_strncpyz: destsize < 1" ); 
	}

	strncpy( dest, src, destsize-1 );
    dest[destsize-1] = 0;
}
   
*/
void QDECL Com_sprintf( char *dest, int size, const char *fmt, ...) {
	int		len;
	va_list		argptr;
	char	bigbuffer[32000];	// big, but small enough to fit in PPC stack

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);
	if ( len >= sizeof( bigbuffer ) ) {
		//Com_Error( ERR_FATAL, "Com_sprintf: overflowed bigbuffer" );
	}
	if (len >= size) {
		//Com_Printf ("Com_sprintf: overflow of %i in %i\n", len, size);
	}
	Q_strncpyz (dest, bigbuffer, size );
}

static int GM_CDECL gmfLogWrite(gmThread * a_thread)
{
  char	*s;
  int len = -1;
  int levelTime = 0;
  int min, tens, sec;
  char	string[1024];

  //check if both parameters are strings
  if (a_thread->ParamType(1) != GM_STRING){
	return GM_EXCEPTION;
  }

  if (a_thread->ParamType(0) != GM_INT)
  	return GM_EXCEPTION;

  // Get data from params
  levelTime = a_thread->Param(0).m_value.m_int;

  // Get data from params
  const char* text = a_thread->ParamString(1);

  if(strlen(text) <= 0){
	return GM_EXCEPTION;
  }
  sec = levelTime / 1000;

  min = sec / 60;
  sec -= min * 60;
  tens = sec / 10;
  sec -= tens * 10;

  Com_sprintf( string, sizeof(string), "%i:%i%i ", min, tens, sec );

  //logstring(QMM_VARARGS("\nDEBUG: '%s'\n\n",text));
  QMM_WRITEGAMELOG(QMM_VARARGS("%s%s\n",string,text),-1);

  return GM_OK;
}

/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

static int GM_CDECL gmfGetUserInfo(gmThread * a_thread)
{
  char	userinfo[1024];
  int clientnum;
  if (a_thread->ParamType(0) != GM_INT)
  	return GM_EXCEPTION;

  // Get data from params
  clientnum = a_thread->Param(0).m_value.m_int;

  g_syscall( G_GET_USERINFO, clientnum, userinfo, sizeof(userinfo));

  // do we have a userinfo string?
  if(strlen(userinfo) <= 0){
	return GM_EXCEPTION;
  }

  // check for malformed or illegal info strings
  if ( !Info_Validate(userinfo) ) {
	return GM_EXCEPTION;
  }

  gmStringObject *userstr = a_thread->GetMachine()->AllocStringObject( userinfo ); 
  //gmStringObject *userstr = a_thread->GetMachine()->AllocStringObject( userinfo, sizeof(userinfo) ); 
  
  a_thread->PushString(userstr);

  return GM_OK;
}

int Q_stricmpn (const char *s1, const char *s2, int n) {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_strncmp (const char *s1, const char *s2, int n) {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_stricmp (const char *s1, const char *s2) {
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
#define BIG_INFO_KEY		8192
char *Info_ValueForKey( const char *s, const char *key ) {
	char	pkey[BIG_INFO_KEY];
	static	char value[2][BIG_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;
	
	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		return "";
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}

static int GM_CDECL gmfGetValueForKey(gmThread * a_thread)
{
  char	*s;

  //check if both parameters are strings
  if (a_thread->ParamType(0) != GM_STRING && a_thread->ParamType(1) != GM_STRING){
	return GM_EXCEPTION;
  }

  // Get data from params
  const char* userinfostr = a_thread->ParamString(0);
  const char* value = a_thread->ParamString(1);

  // do we have a userinfo string?
  if(strlen(userinfostr) <= 0 || strlen(value) <= 0){
	return GM_EXCEPTION;
  }

  s = Info_ValueForKey( userinfostr, value );

  gmStringObject *infokey = a_thread->GetMachine()->AllocStringObject( s ); 

  a_thread->PushString(infokey);

  return GM_OK;
}

static int GM_CDECL gmfStrCmp(gmThread * a_thread)
{
  int equal;

  //check if both parameters are strings
  if (a_thread->ParamType(0) != GM_STRING && a_thread->ParamType(1) != GM_STRING){
	return GM_EXCEPTION;
  }

  // Get data from params
  const char* string1 = a_thread->ParamString(0);
  const char* string2 = a_thread->ParamString(1);

  // do we have a userinfo string?
  if(strlen(string1) <= 0 || strlen(string2) <= 0){
	return GM_EXCEPTION;
  }

  equal = Q_strncmp (string2, string1,strlen(string1)); //will return 0 if equal

  a_thread->PushInt(equal);

  return GM_OK;
}

static int GM_CDECL gmfGetStringCvar(gmThread * a_thread)
{
  char	s[MAX_STRING_CHARS];

  //check if parameter is a string
  if (a_thread->ParamType(0) != GM_STRING){
	return GM_EXCEPTION;
  }

  // Get data from params
  const char* var_name = a_thread->ParamString(0);

  if(strlen(var_name) <= 0){
	return GM_EXCEPTION;
  }

  g_syscall( G_CVAR_VARIABLE_STRING_BUFFER, var_name, s, sizeof(s) );

  gmStringObject *returnStr = a_thread->GetMachine()->AllocStringObject( s );   

  a_thread->PushString(returnStr);

  return GM_OK;
}

static int GM_CDECL gmfGetIntegerCvar(gmThread * a_thread)
{
  int returnInt;

  //check if parameter is a string
  if (a_thread->ParamType(0) != GM_STRING){
	return GM_EXCEPTION;
  }

  // Get data from params
  const char* var_name = a_thread->ParamString(0);

  if(strlen(var_name) <= 0){
	return GM_EXCEPTION;
  }

  returnInt = g_syscall( G_CVAR_VARIABLE_INTEGER_VALUE, var_name );

  a_thread->PushInt(returnInt);

  return GM_OK;
}


static int GM_CDECL gmfStringLength(gmThread * a_thread)
{
  int	length = 0;

  //check if parameter is a string
  if (a_thread->ParamType(0) != GM_STRING){
	return GM_EXCEPTION;
  }

  // Get data from params
  const char* var_name = a_thread->ParamString(0);

  length = strlen(var_name);

  a_thread->PushInt(length);

  return GM_OK;
}

static int GM_CDECL gmfCP(gmThread* a_thread)
{
	int clientnum, pos;

  	// Get data from params
  	clientnum = a_thread->Param(0).m_value.m_int;

	// Get data from params
	const char* string = a_thread->ParamString(1);
	
	if(string)
	{
		//now lets send the string to the client
		g_syscall(G_SEND_SERVER_COMMAND, clientnum, QMM_VARARGS("cp \"%s\"", string));
	}
	
	return GM_OK;
}

static int GM_CDECL gmfCPM(gmThread* a_thread)
{
	int clientnum, pos;

  	// Get data from params
  	clientnum = a_thread->Param(0).m_value.m_int;

	// Get data from params
	const char* string = a_thread->ParamString(1);
	
	if(string)
	{
		//now lets send the string to the server
		g_syscall(G_SEND_SERVER_COMMAND, clientnum, QMM_VARARGS("cpm \"%s\n\"", string));
	}
	
	return GM_OK;
}

static int GM_CDECL gmfClientPrint(gmThread* a_thread)
{
	int clientnum, pos;

  	// Get data from params
  	clientnum = a_thread->Param(0).m_value.m_int;

	// Get data from params
	const char* string = a_thread->ParamString(1);
	
	if(string)
	{
		//now lets send the string to the server
		g_syscall(G_SEND_SERVER_COMMAND, clientnum, QMM_VARARGS("print \"%s\n\"", string));
		
	}
	
	return GM_OK;
}


//Eamon, end of my functions



static int GM_CDECL gmfSystem(gmThread * a_thread)
{
  const int bufferSize = 256;
  int len = 0, size = 0, i, ret = -1;
  char * str = NULL, buffer[bufferSize];

  // build the string
  for(i = 0; i < a_thread->GetNumParams(); ++i)
  {
    gmConcat(a_thread->GetMachine(), str, len, size, a_thread->Param(i).AsString(a_thread->GetMachine(), buffer, bufferSize), 64);

    if(str)
    {
      GM_ASSERT(len < size);
      str[len++] = ' ';
      str[len] = '\0';
    }
  }

  // print the string
  if(str)
  {
    ret = system(str);
    a_thread->GetMachine()->Sys_Free(str);
  }

  a_thread->PushInt(ret);

  return GM_OK;
}


static int GM_CDECL gmfSay(gmThread* a_thread)
{
	const int bufferSize=256;
	int len=0, size=0, i, ret=-1;
	char* str =NULL, buffer[bufferSize];
	for(i=0;i<a_thread->GetNumParams();++i)
	{
		gmConcat(a_thread->GetMachine(),str,len, size, a_thread->Param(i).AsString(a_thread->GetMachine(), buffer, bufferSize),64);
		if(str)
		{
			GM_ASSERT(len<size);
			str[len++]=' ';
			str[len]='\0';
		}
	}
	
	if(str)
	{
		//now lets send the string to the server
		g_syscall(G_SEND_SERVER_COMMAND, -1, QMM_VARARGS("chat \"%s\"", str));
	
    		a_thread->GetMachine()->Sys_Free(str);
		
	}

	return GM_OK;
}


static int GM_CDECL gmfPrintConsole(gmThread* a_thread)
{
	const int bufferSize=256;
	int len=0, size=0,i,ret=-1;
	char* str=NULL,buffer[bufferSize];
	for(i=0;i<a_thread->GetNumParams();++i)
	{
		gmConcat(a_thread->GetMachine(),str,len,size,a_thread->Param(i).AsString(a_thread->GetMachine(),buffer,bufferSize),64);
		if(str)
		{
			GM_ASSERT(len<size);
			str[len++]=' ';
			str[len]='\0';
		}
	}
	if(str)
	{
		g_syscall(G_PRINT,QMM_VARARGS("%s\n",str));
    		a_thread->GetMachine()->Sys_Free(str);
	}
	return GM_OK;
}
//function to communicate with the server console.
//example: "set timelimit 5"
static int GM_CDECL gmfSet(gmThread* a_thread)
{
	const int bufferSize=256;
	int len=0, size=0, i,ret=-1;
	char* str=NULL, buffer[bufferSize];
	for(i=0;i<a_thread->GetNumParams();++i)
	{
		gmConcat(a_thread->GetMachine(),str,len,size,a_thread->Param(i).AsString(a_thread->GetMachine(),buffer,bufferSize),64);
		if(str)
		{
			GM_ASSERT(len<size);
			str[len++]=' ';
			str[len]='\0';
		}
	}
	if(str)
	{
		g_syscall(G_SEND_CONSOLE_COMMAND, EXEC_APPEND, QMM_VARARGS("%s\n",str));
    		a_thread->GetMachine()->Sys_Free(str);
	}
	
	return GM_OK;
}

//Search Function
static int GM_CDECL gmfSearchString(gmThread * a_thread)
{
	if (a_thread->ParamType(0) != GM_STRING && a_thread->ParamType(1) != GM_STRING){
		return GM_EXCEPTION;
	}
	std::string search   = a_thread->ParamString(0);
	std::string searchin = a_thread->ParamString(1);
	//std::sstream ss;
	std::ostringstream ss;
	std::string uppersearchstring, uppersearchinstring;
	uppersearchstring.resize(254);
	uppersearchinstring.resize(254);
	ss.str();
	int i;
	for(i=0;i<search.length();i++){
		uppersearchstring[i]=(char)toupper(search[i]);
	}
	for(i=0;i<searchin.length();i++){
		uppersearchinstring[i]=(char)toupper(searchin[i]);
	}
	if(uppersearchinstring.find(uppersearchstring.c_str())==uppersearchinstring.npos){
		a_thread->PushInt(0);
		return GM_OK;
	}
	else{
		a_thread->PushInt(1);
		return GM_OK;
	}
	return GM_OK;
}

//sscanf
static int GM_CDECL gmfsscanf(gmThread * a_thread)
{
	int NumParams=GM_NUM_PARAMS;
	int type=-1;
	if(NumParams<3)
		return GM_EXCEPTION;
	std::string input=a_thread->ParamString(0);
	std::string expression=a_thread->ParamString(1);
	std::string string_erg="";
	gmStringObject* return_string=0;
	int int_erg=0;
	float float_erg=0.0;
	type= a_thread->ParamInt(2); 
	switch(type){
		case 0: 
			return GM_OK;
			//break;
		case 1: 
			sscanf (input.c_str(),expression.c_str(),&int_erg);
			a_thread->PushInt(int_erg);
			return GM_OK;
			//break;
		case 2: 
			sscanf (input.c_str(),expression.c_str(),&float_erg);
			a_thread->PushFloat(float_erg);
			return GM_OK;
			//break;
		case 3: 
			sscanf (input.c_str(),expression.c_str(),string_erg.c_str());
			return_string=a_thread->GetMachine()->AllocStringObject(string_erg.c_str());
			a_thread->PushString(return_string);
			return GM_OK;
			//break;
		default:
			return GM_OK;
			break;	
	}
	return GM_OK;
}





//
// Libs and bindings
//


static gmFunctionEntry s_GoNeLib[] = 
{ 
  /*gm
    \lib system
    \brief system functions are bound in a "system" table.
  */
  /*gm
   *\lib GoNe
   *Allows you to have a ssanf like function
   * */
 {"sscanf",gmfsscanf},
  /*gm
   *\lib GoNe
   *Allows you to search a string in a string
   * */
 {"search",gmfSearchString},
 /*
  *\lib GoNe
  * removes the color code from a player/string
  * */
  {"strip",gmfstrip},
  /*gm
    \function Exec
    \brief Exec will execute a system command
    \param string params will be concatinated together with a single space to form the final system command string
    \return integer value returned from system exec call, -1 on error
  */
  {"Exec", gmfSystem},
  /*
    \function say
    \brief say will send a console say message
  */
  {"say", gmfSay},
  /*
   \function "echo"
   \brief echo will echo a string to the serverconsole output
  */
  {"echo",gmfPrintConsole},
  /*
   \function set
   \brief set will set a GameServer Var
   \param string param will be concatinated and piped to the gameserver
  */ 
  {"set",gmfSet},

  /* NOT REALLY WORKING GOOD YET
   \function gmfStrCmp
   \syntax: gmfStrCmp(string1,string2)
   \this function will compare two string if they are equal, 0 is equal
  */
  {"strcmp",gmfStrCmp},

  // Eamon: Last added/modified

  /*
   \function gmfCP
   \syntax: cp(int clientnum, some char text)
   \CP will send 'text' as a Center Print message to 'clientnum'
   */ 
  {"cp",gmfCP},

  /*
   \function gmfCPM
   \syntax: gmfCPM(clientnumber, string)
   \this function will send string to clientnum in CPM mode
   */ 
  {"cpm", gmfCPM},

  /*
   \function gmfClientPrint
   \syntax: gmfClientPrint(clientnumber, string)
   \this function will send string to clientnum in print mode(console)
   */ 
  {"cprint",gmfClientPrint},

  /*
   \function gmfGetValueForKey
   \syntax: GetValueForKey(string)
   \this function will get a certain value from the userinfo string
   */ 
  {"GetValueForKey",gmfGetValueForKey},

  /*
   \function gmfGetUserInfo
   \syntax: gmfGetUserInfo(clientnum)
   \this function will return a userinfostring
   */ 
  {"GetUserInfo",gmfGetUserInfo},

  /* 
   \function gmfGetStringCvar
   \syntax: gmfGetStringCvar(var_name)
   \this function will return the string of the cvar 'var_name'
  */
  {"getStringCvar",gmfGetStringCvar},

  /*
   \function gmfGetIntegerCvar
   \syntax: gmfGetIntegerCvar(var_name)
   \this function will return the integer of the cvar 'var_name'
  */
  {"getIntCvar",gmfGetIntegerCvar},

  /*
   \function gmfStringLength
   \syntax: gmfStringLength(string)
   \this function will return the length of the string
  */
  {"strlen",gmfStringLength},

  /*
   \function gmfLogWrite
   \syntax: gmfLogWrite(string)
   \this function will write 'string' in the game logfile
  */
  {"logWrite",gmfLogWrite},

  /*
   \function gmfPlaySound
   \syntax: gmfPlaySound(string)
   \this function will play the sound 'string'
  */
  {"playsound",gmfPlaySound},

  /*
   \function gmfInclude
   \syntax: gmfInclude(string)
   \this function will include the file 'string'
  */
  {"include",gmfInclude},

  /*
   \function gmfGetConfigString
   \syntax: gmfGetConfigString(clientnum)
   \
  */
  {"getConfigString",gmfGetConfigString},
    
  /*
    \function gmfRegisterCvar
    \syntax: gmfRegisterCvar(CvarName,DefaultValue)
    \
  */
  {"registerCvar", gmfRegisterCvar},

  /*
  \function gmfsetCvar
  \syntax: gmfsetCvar(CvarName,Value)
  \
  */
  {"setCvar", gmfCvarSet},

 /*
  \function gmfsetCvar
  \syntax: gmfsetCvar(CvarName,Value)
 */
  {"atoi", gmfatoi}

   // Eamon: end added.

  
};


void gmBindGoNeLib(gmMachine * a_machine)
{
  a_machine->RegisterLibrary(s_GoNeLib, sizeof(s_GoNeLib) / sizeof(s_GoNeLib[0]));
}

