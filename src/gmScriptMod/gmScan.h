/***********************************
 * Scan function, written by Seth. *
 * Brief: GM version of C scanf	   *
 ***********************************/

#pragma once
#ifndef GMSCAN_H
#define GMSCAN_H

#include "gmConfig.h"
#include "gmGoNeLib.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmHelpers.h"
#include "gmUtil.h"
#include <math.h>
#include <fstream>

#include <stdio.h>

int gmScan(gmThread* thread)
{
	int NumParams = thread->GetNumParams();



	GM_ASSERT( NumParams > 0 && NumParams < 10 );
	GM_ASSERT( thread->ParamType(0) == GM_STRING );




	const char* Format = ((gmStringObject*)thread->ParamRef(0))->GetString();

	const char* String = ((gmStringObject*)thread->ParamRef(3))->GetString();

g_syscall(G_PRINT, "DEBUG3\n");

	if( Format )
	{
		gmMachine* VM = thread->GetMachine();

		if( VM )
		{
			gmTableObject* ReturnTable = NULL;
			gmTableObject* MemberName = NULL;
			gmTableObject* TextToDisplay = NULL;

			const char* Dummy = strstr( Format, "%" );

			

			if( Dummy && strstr( Dummy+1, "%" ) )
				ReturnTable = VM->AllocTableObject();

			if( NumParams > 1 )
			{
				GM_ASSERT( thread->ParamType(1) == GM_TABLE || thread->ParamType(1) == GM_NULL );
				MemberName = (gmTableObject*)thread->ParamRef(1);
			}

			if( NumParams > 2 )
			{
				GM_ASSERT( thread->ParamType(2) == GM_TABLE );
				TextToDisplay = (gmTableObject*)thread->ParamRef(2);
			}

			gmVariable Temp;
			int index = 0;

			while( *Format != '\0' )
			{
				if( *Format++ == '%' )
				{
					GM_ASSERT( *Format == 'd' || *Format == 'f' || *Format == 's' || *Format == '[' );			
					fflush(stdin);
					Temp.Nullify();

					if( TextToDisplay )
					{
						GM_ASSERT( index < TextToDisplay->Count() );
						printf( "%s", ((gmStringObject*)TextToDisplay->Get( gmVariable( index ) ).m_value.m_ref)->GetString() );
					}
					g_syscall(G_PRINT, "TEST2\n");
					switch( *Format )
					{
					case 'd':
						if( sscanf(String, "%d", &Temp.m_value.m_int ) )
							Temp.m_type = GM_INT; break;
		
					case 'f':
						if( sscanf(String, "%f", &Temp.m_value.m_float ) )
							Temp.m_type = GM_FLOAT; break;

					case 's':
						{
						char String[128+1];
						if( sscanf(String, "%128s", String, 129 ) )
							Temp.SetString( VM->AllocPermanantStringObject( String ) ); break;
						}

					case '[':
						{
						int j = 0;

						while( *++Format != ']' )
						{
							GM_ASSERT( *Format != '\0' );
							++j;
						}

						int size = 5+j+1+1;
						char* Set = new char[size];
						//strcpy ( char * destination, const char * source ); 
						strcpy( Set, "%128[" );
						strncat( Set, Format-j, j );
						strcat( Set, "]" );

						char String[128+1];
						if( scanf( Set, String, 129 ) )
							Temp.SetString( VM->AllocPermanantStringObject( String ) );

						if(Set) { delete[] Set; Set = NULL; }; break;
						}
					}
					g_syscall(G_PRINT, "TEST3\n");
					if( ReturnTable )
					{	
						if( MemberName )
						{
							GM_ASSERT( index < MemberName->Count() );
							ReturnTable->Set( VM, MemberName->Get( gmVariable( index++ ) ) , Temp );
						}
						else
							ReturnTable->Set( VM, index++, Temp );
					}
				}
			}
			
			if( ReturnTable )
				thread->PushTable( ReturnTable );
			else
				thread->Push( Temp );
		}
	}

	return GM_OK;
}

void gmScan_Register(gmMachine* VM)
{
	gmFunctionEntry gmScanLib = { "scan", gmScan };
	VM->RegisterLibrary( &gmScanLib, 1 );
}

#endif
