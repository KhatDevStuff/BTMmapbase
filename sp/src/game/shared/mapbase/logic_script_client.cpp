//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Custom client-side equivalent of logic_script.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vscript_shared.h"
#include "tier1/fmtstr.h"

//-----------------------------------------------------------------------------
// Purpose: An entity that acts as a container for client-side game scripts.
//-----------------------------------------------------------------------------

#define MAX_SCRIPT_GROUP_CLIENT 8

class CLogicScriptClient : public CBaseEntity
{
public:
	DECLARE_CLASS( CLogicScriptClient, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	void OnDataChanged( DataUpdateType_t type )
	{
		BaseClass::OnDataChanged( type );
	
		if ( !m_ScriptScope.IsInitialized() )
		{
			RunVScripts();
		}
	}
#else
	int		UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }
#endif

	bool KeyValue( const char *szKeyName, const char *szValue )
	{
		if ( FStrEq( szKeyName, "vscripts" ) )
		{
			Q_strcpy( m_iszClientScripts.GetForModify(), szValue );
		}

		return BaseClass::KeyValue( szKeyName, szValue );
	}

	void RunVScripts()
	{
#ifdef CLIENT_DLL
		if (m_iszClientScripts == NULL_STRING)
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "%s has no client scripts", GetDebugName() );
			return;
		}

		if (g_pScriptVM == NULL)
		{
			return;
		}

		ValidateScriptScope();

		// All functions we want to have call chained instead of overwritten
		// by other scripts in this entities list.
		static const char* sCallChainFunctions[] =
		{
			"OnPostSpawn",
			"Precache"
		};

		ScriptLanguage_t language = g_pScriptVM->GetLanguage();

		// Make a call chainer for each in this entities scope
		for (int j = 0; j < ARRAYSIZE( sCallChainFunctions ); ++j)
		{

			if (language == SL_PYTHON)
			{
				// UNDONE - handle call chaining in python
				;
			}
			else if (language == SL_SQUIRREL)
			{
				//TODO: For perf, this should be precompiled and the %s should be passed as a parameter
				HSCRIPT hCreateChainScript = g_pScriptVM->CompileScript( CFmtStr( "%sCallChain <- CSimpleCallChainer(\"%s\", self.GetScriptScope(), true)", sCallChainFunctions[j], sCallChainFunctions[j] ) );
				g_pScriptVM->Run( hCreateChainScript, (HSCRIPT)m_ScriptScope );
			}
		}

		char szScriptsList[255];
		Q_strcpy( szScriptsList, m_iszClientScripts.Get() );
		CUtlStringList szScripts;

		V_SplitString( szScriptsList, " ", szScripts );

		for (int i = 0; i < szScripts.Count(); i++)
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "%s executing script: %s\n", GetDebugName(), szScripts[i] );

			RunScriptFile( szScripts[i], IsWorld() );

			for (int j = 0; j < ARRAYSIZE( sCallChainFunctions ); ++j)
			{
				if (language == SL_PYTHON)
				{
					// UNDONE - handle call chaining in python
					;
				}
				else if (language == SL_SQUIRREL)
				{
					//TODO: For perf, this should be precompiled and the %s should be passed as a parameter.
					HSCRIPT hRunPostScriptExecute = g_pScriptVM->CompileScript( CFmtStr( "%sCallChain.PostScriptExecute()", sCallChainFunctions[j] ) );
					g_pScriptVM->Run( hRunPostScriptExecute, (HSCRIPT)m_ScriptScope );
				}
			}
		}
#else
		// Avoids issues from having m_iszVScripts set without actually having a script scope
		ValidateScriptScope();

		if (m_bRunOnServer)
		{
			BaseClass::RunVScripts();
		}
#endif
	}

#ifndef CLIENT_DLL
	void InputCallScriptFunctionClient( inputdata_t &inputdata )
	{
		// TODO: Support for specific players?
		CBroadcastRecipientFilter filter;
		filter.MakeReliable();

		const char *pszFunction = inputdata.value.String();
		if (strlen( pszFunction ) > 64)
		{
			Msg("%s CallScriptFunctionClient: \"%s\" is too long at %i characters, must be 64 or less\n", GetDebugName(), pszFunction, strlen(pszFunction));
			return;
		}

		UserMessageBegin( filter, "CallClientScriptFunction" );
			WRITE_STRING( pszFunction ); // function
			WRITE_SHORT( entindex() ); // entity
		MessageEnd();
	}
#endif

	//CNetworkArray( string_t, m_iszGroupMembers, MAX_SCRIPT_GROUP_CLIENT );
	CNetworkString( m_iszClientScripts, 128 );

	bool m_bRunOnServer;
};

LINK_ENTITY_TO_CLASS( logic_script_client, CLogicScriptClient );

BEGIN_DATADESC( CLogicScriptClient )

	//DEFINE_KEYFIELD( m_iszGroupMembers[0], FIELD_STRING, "Group00"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[1], FIELD_STRING, "Group01"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[2], FIELD_STRING, "Group02"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[3], FIELD_STRING, "Group03"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[4], FIELD_STRING, "Group04"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[5], FIELD_STRING, "Group05"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[6], FIELD_STRING, "Group06"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[7], FIELD_STRING, "Group07"),

	DEFINE_KEYFIELD( m_bRunOnServer, FIELD_BOOLEAN, "RunOnServer" ),

#ifndef CLIENT_DLL
	DEFINE_INPUTFUNC( FIELD_STRING, "CallScriptFunctionClient", InputCallScriptFunctionClient ),
#endif

END_DATADESC()

IMPLEMENT_NETWORKCLASS_DT( CLogicScriptClient, DT_LogicScriptClient )

#ifdef CLIENT_DLL
	//RecvPropArray( RecvPropString( RECVINFO( m_iszGroupMembers[0] ) ), m_iszGroupMembers ),
	RecvPropString( RECVINFO( m_iszClientScripts ) ),
#else
	//SendPropArray( SendPropStringT( SENDINFO_ARRAY( m_iszGroupMembers ) ), m_iszGroupMembers ),
	SendPropString( SENDINFO( m_iszClientScripts ) ),
#endif

END_NETWORK_TABLE()
