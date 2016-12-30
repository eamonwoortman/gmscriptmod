// quickly hacked by Deadsoul ( jed at deadsoul dot com ) 

#include "gmConfig.h"
#include "gmSqliteLib.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmHelpers.h"

extern "C" 
{
	#include "sqlite/sqlite.h"
};

#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
//#include <process.h>

#undef		GetObject
gmMachine 	*sqlite_machine;

static gmType s_gmSqliteType = GM_NULL;

static int GM_CDECL gmfSqlite(gmThread * a_thread)
{
	a_thread->PushNewUser(NULL, s_gmSqliteType);
	return GM_OK;
};

#if GM_USE_INCGC
static void GM_CDECL gmGCDestructSqliteUserType(gmMachine * a_machine, gmUserObject* a_object)
{
	if(a_object->m_user)
		sqlite_close((sqlite*)a_object->m_user);
	a_object->m_user = NULL;
}
#else //GM_USE_INCGC
static void GM_CDECL gmGCSqliteUserType(gmMachine * a_machine, gmUserObject * a_object, gmuint32 a_mark)
{
	if(a_object->m_user)
		sqlite_close((sqlite*)a_object->m_user);
	a_object->m_user = NULL;
}
#endif //GM_USE_INCGC

static int GM_CDECL gmfSqliteOpen(gmThread * a_thread) // path
{
	GM_CHECK_STRING_PARAM(filename, 0);
	char *zErrMsg = 0;
	gmUserObject * sqliteObject = a_thread->ThisUserObject();

	GM_ASSERT(sqliteObject->m_userType == s_gmSqliteType);

	if(sqliteObject->m_user)
		sqlite_close((sqlite*) sqliteObject->m_user);

	sqliteObject->m_user = (void *) sqlite_open(filename,0,&zErrMsg);

	if(sqliteObject->m_user)
		a_thread->PushInt(1);
	else
		a_thread->PushInt(0);
	return GM_OK;
};

static int GM_CDECL gmfSqliteClose(gmThread * a_thread)
{
	gmUserObject * sqliteObject = a_thread->ThisUserObject();
	GM_ASSERT(sqliteObject->m_userType == s_gmSqliteType);
	if(sqliteObject->m_user)
	{
		sqlite_close((sqlite*)sqliteObject->m_user);
	}
	sqliteObject->m_user = NULL;
	return GM_OK;
}

static int GM_CDECL gmfSqliteQuery(gmThread * a_thread) // path
{
GM_CHECK_STRING_PARAM(query, 0);
char		*zErrMsg = 0;

	gmUserObject * sqliteObject = a_thread->ThisUserObject();
	GM_ASSERT(sqliteObject->m_userType == s_gmSqliteType);
	if(sqliteObject->m_user)
	{
		int	nRows;
		int	nColumns;
		char	**results;

		long	nResult=sqlite_get_table((sqlite*)sqliteObject->m_user,query,
						&results,
						&nRows,
						&nColumns,
						&zErrMsg);
		if(nResult != SQLITE_OK)
		{
			printf("SQLITE ERROR: %s\n",zErrMsg);
			a_thread->PushInt(0);
		}
		else
		{
			//	this creates a table of tables
			//	probably not the best approach but it works
			//	note only strings 

			if (nColumns!=0)
			{
				gmTableObject *base;
				base = a_thread->PushNewTable();
				for (int q=0;q<nColumns;q++)
				{
					gmTableObject *rows;
					if (nRows!=0)
					{
						rows = sqlite_machine->AllocTableObject();
						base->Set(sqlite_machine,q,gmVariable(GM_TABLE,rows->GetRef()) );
						for (int z=0;z<nRows;z++)
						{
							gmStringObject *str = sqlite_machine->AllocStringObject(results[q+(z*nColumns)]);
							rows->Set(sqlite_machine,z,gmVariable(GM_STRING,str->GetRef()) );
						}	
					}							
				}
			}
		}
	}
	return GM_OK;
};


static gmFunctionEntry s_SqliteLib[] =
{
	{"sqlite", gmfSqlite},
};
static gmFunctionEntry s_sqliteLib[] = 
{ 
  /*gm
    \lib sqlite
    \brief sqlite functions , a simple embeddable sql engine
  */
  /*gm
    \function Open
    \brief Open will open a sqllite database , creating if necessary 
    \param string filename
    \return integer value returned , 0 on error
  */
	{"Open", gmfSqliteOpen},
  /*gm
    \function Close
    \brief Close will close a sqllite database 
    \param none
    \return none
  */
	{"Close", gmfSqliteClose},
  /*gm
    \function Query
    \brief perform an sql query on the open database
    \param string sql query 
    \return a table of columns with a table of rows inside each one
	*/
	{"Query", gmfSqliteQuery},
};

void gmBindSqliteLib(gmMachine * a_machine)
{
	sqlite_machine = a_machine;
	a_machine->RegisterLibrary(s_SqliteLib, sizeof(s_SqliteLib) / sizeof(s_SqliteLib[0]));
	// sql
	s_gmSqliteType = a_machine->CreateUserType("sqlite");
#if GM_USE_INCGC
	a_machine->RegisterUserCallbacks(s_gmSqliteType, NULL, gmGCDestructSqliteUserType);
#else //GM_USE_INCGC
	a_machine->RegisterUserCallbacks(s_gmSqliteType, NULL, gmGCSqliteUserType);
#endif //GM_USE_INCGC
	a_machine->RegisterTypeLibrary(s_gmSqliteType, s_sqliteLib, sizeof(s_sqliteLib) / sizeof(s_sqliteLib[0]));

}

