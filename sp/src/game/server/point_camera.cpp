//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "point_camera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CAM_THINK_INTERVAL 0.05

// Spawnflags
#define SF_CAMERA_START_OFF				0x01
#ifdef MAPBASE
#define SF_CAMERA_KEEP_RT_TEXTURE		0x02
#endif

// UNDONE: Share properly with the client code!!!
#define POINT_CAMERA_MSG_SETACTIVE		1

CEntityClassList<CPointCamera> g_PointCameraList;
template <> CPointCamera *CEntityClassList<CPointCamera>::m_pClassList = NULL;

CPointCamera* GetPointCameraList()
{
	return g_PointCameraList.m_pClassList;
}

// These are already built into CBaseEntity
//	DEFINE_KEYFIELD( m_iName, FIELD_STRING, "targetname" ),
//	DEFINE_KEYFIELD( m_iParent, FIELD_STRING, "parentname" ),
//	DEFINE_KEYFIELD( m_target, FIELD_STRING, "target" ),

LINK_ENTITY_TO_CLASS( point_camera, CPointCamera );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointCamera::~CPointCamera()
{
	g_PointCameraList.Remove( this );
}

CPointCamera::CPointCamera()
{
	// Set these to opposites so that it'll be sent the first time around.
	m_bActive = false;
	m_bIsOn = false;
	
	m_bFogEnable = false;

	m_ResolutionHeight = m_ResolutionWidth = 256;

#ifdef MAPBASE
	// Equivalent to SKYBOX_2DSKYBOX_VISIBLE, the original sky setting
	m_iSkyMode = 2;
#endif

	g_PointCameraList.Insert( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::Spawn( void )
{
	BaseClass::Spawn();

	m_bIsOn = ( m_spawnflags & SF_CAMERA_START_OFF ) == 0;
#ifdef MAPBASE
	m_KeepRTTexture = ( m_spawnflags & SF_CAMERA_KEEP_RT_TEXTURE ) != 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Override ShouldTransmit since we want to be sent even though we don't have a model, etc.
//			All that matters is if we are in the pvs.
//-----------------------------------------------------------------------------
int CPointCamera::UpdateTransmitState()
{
	if ( m_bActive )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	else
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::SetActive( bool bActive )
{
	// If the mapmaker's told the camera it's off, it enforces inactive state
	if ( !m_bIsOn )
	{
		bActive = false;
	}

	if ( m_bActive != bActive )
	{
		m_bActive = bActive;
		DispatchUpdateTransmitState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputChangeFOV( inputdata_t &inputdata )
{
	// Parse the keyvalue data
	char parseString[255];

	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get FOV
	char *pszParam = strtok(parseString," ");
	if(pszParam)
	{
		m_TargetFOV = atof( pszParam );
	}
	else
	{
		// Assume no change
		m_TargetFOV = m_FOV;
	}

	// Get Time
	float flChangeTime;
	pszParam = strtok(NULL," ");
	if(pszParam)
	{
		flChangeTime = atof( pszParam );
	}
	else
	{
		// Assume 1 second.
		flChangeTime = 1.0;
	}

	m_DegreesPerSecond = ( m_TargetFOV - m_FOV ) / flChangeTime;

	SetThink( &CPointCamera::ChangeFOVThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::ChangeFOVThink( void )
{
	SetNextThink( gpGlobals->curtime + CAM_THINK_INTERVAL );

	float newFOV = m_FOV;

	newFOV += m_DegreesPerSecond * CAM_THINK_INTERVAL;

	if( m_DegreesPerSecond < 0 )
	{
		if( newFOV <= m_TargetFOV )
		{
			newFOV = m_TargetFOV;
			SetThink( NULL );
		}
	}
	else
	{
		if( newFOV >= m_TargetFOV )
		{
			newFOV = m_TargetFOV;
			SetThink( NULL );
		}
	}

	m_FOV = newFOV;
}

//-----------------------------------------------------------------------------
// Purpose: Turn this camera on, and turn all other cameras off
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOnAndTurnOthersOff( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "point_camera" )) != NULL)
	{
		CPointCamera *pCamera = (CPointCamera*)pEntity;
		pCamera->InputSetOff( inputdata );
	}

	// Now turn myself on
	InputSetOn( inputdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOn( inputdata_t &inputdata )
{
	m_bIsOn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOff( inputdata_t &inputdata )
{
	m_bIsOn = false;
		( false );
}

BEGIN_DATADESC( CPointCamera )

	// Save/restore Keyvalue fields
	DEFINE_KEYFIELD( m_FOV,			FIELD_FLOAT, "FOV" ),
	DEFINE_KEYFIELD( m_ResolutionHeight,	FIELD_INTEGER, "resolution_height" ),
	DEFINE_KEYFIELD( m_ResolutionWidth,	FIELD_INTEGER, "resolution_width" ),
	DEFINE_KEYFIELD( m_bFogEnable,	FIELD_BOOLEAN, "fogEnable" ),
	DEFINE_KEYFIELD( m_FogColor,	FIELD_COLOR32,	"fogColor" ),
	DEFINE_KEYFIELD( m_flFogStart,	FIELD_FLOAT, "fogStart" ),
	DEFINE_KEYFIELD( m_flFogEnd,	FIELD_FLOAT, "fogEnd" ),
	DEFINE_KEYFIELD( m_flFogMaxDensity,	FIELD_FLOAT, "fogMaxDensity" ),
	DEFINE_KEYFIELD( m_bUseScreenAspectRatio, FIELD_BOOLEAN, "UseScreenAspectRatio" ),
#ifdef MAPBASE
	DEFINE_KEYFIELD( m_iSkyMode, FIELD_INTEGER, "SkyMode" ),
	DEFINE_FIELD( m_KeepRTTexture, FIELD_BOOLEAN),
#endif
	DEFINE_FIELD( m_bActive,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsOn,			FIELD_BOOLEAN ),


	DEFINE_FIELD( m_TargetFOV,		FIELD_FLOAT ),
	DEFINE_FIELD( m_DegreesPerSecond, FIELD_FLOAT ),
	// This is re-set up in the constructor
	//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),

	DEFINE_FUNCTION( ChangeFOVThink ),

	// Input
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeFOV", InputChangeFOV ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOnAndTurnOthersOff", InputSetOnAndTurnOthersOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOn", InputSetOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOff", InputSetOff ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetSkyMode", InputSetSkyMode ),
#endif

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPointCamera, DT_PointCamera )
	SendPropFloat( SENDINFO( m_FOV ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_ResolutionHeight ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_ResolutionWidth ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_bFogEnable ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_STRUCTELEM( m_FogColor ), 32, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flFogStart ), 0, SPROP_NOSCALE ),	
	SendPropFloat( SENDINFO( m_flFogEnd ), 0, SPROP_NOSCALE ),	
	SendPropFloat( SENDINFO( m_flFogMaxDensity ), 0, SPROP_NOSCALE ),	
	SendPropInt( SENDINFO( m_bActive ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bUseScreenAspectRatio ), 1, SPROP_UNSIGNED ),
#ifdef MAPBASE
	SendPropInt( SENDINFO( m_iSkyMode ) ),
	SendPropInt( SENDINFO( m_KeepRTTexture ), 1, SPROP_UNSIGNED ),
#endif
END_SEND_TABLE()

#ifdef MAPBASE

//=============================================================================
// Orthographic point_camera 
//=============================================================================

BEGIN_DATADESC( CPointCameraOrtho )

	DEFINE_KEYFIELD( m_bOrtho, FIELD_BOOLEAN, "IsOrtho" ),
	DEFINE_ARRAY( m_OrthoDimensions, FIELD_FLOAT, CPointCameraOrtho::NUM_ORTHO_DIMENSIONS ),

	DEFINE_ARRAY( m_TargetOrtho, FIELD_FLOAT, CPointCameraOrtho::NUM_ORTHO_DIMENSIONS ),
	DEFINE_FIELD( m_TargetOrthoDPS, FIELD_FLOAT ),

	DEFINE_FUNCTION( ChangeOrthoThink ),

	// Input
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetOrthoEnabled", InputSetOrthoEnabled ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ScaleOrtho", InputScaleOrtho ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetOrthoTop", InputSetOrthoTop ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetOrthoBottom", InputSetOrthoBottom ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetOrthoLeft", InputSetOrthoLeft ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetOrthoRight", InputSetOrthoRight ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPointCameraOrtho, DT_PointCameraOrtho )
	SendPropInt( SENDINFO( m_bOrtho ), 1, SPROP_UNSIGNED ),
	SendPropArray( SendPropFloat(SENDINFO_ARRAY(m_OrthoDimensions), CPointCameraOrtho::NUM_ORTHO_DIMENSIONS, SPROP_NOSCALE ), m_OrthoDimensions ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( point_camera_ortho, CPointCameraOrtho );

CPointCameraOrtho::~CPointCameraOrtho()
{
}

CPointCameraOrtho::CPointCameraOrtho()
{
}

void CPointCameraOrtho::Spawn( void )
{
	BaseClass::Spawn();

	// If 0, get the FOV
	if (m_OrthoDimensions[ORTHO_TOP] == 0.0f)
		m_OrthoDimensions.Set( ORTHO_TOP, GetFOV() );

	// If 0, get the negative top ortho
	if (m_OrthoDimensions[ORTHO_BOTTOM] == 0.0f)
		m_OrthoDimensions.Set( ORTHO_BOTTOM, -m_OrthoDimensions[ORTHO_TOP] );

	// If 0, get the top ortho
	if (m_OrthoDimensions[ORTHO_LEFT] == 0.0f)
		m_OrthoDimensions.Set( ORTHO_LEFT, m_OrthoDimensions[ORTHO_TOP] );

	// If 0, get the negative left ortho
	if (m_OrthoDimensions[ORTHO_RIGHT] == 0.0f)
		m_OrthoDimensions.Set( ORTHO_RIGHT, -m_OrthoDimensions[ORTHO_LEFT] );
}

bool CPointCameraOrtho::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( strncmp( szKeyName, "Ortho", 5 ) == 0 )
	{
		int iOrtho = atoi(szKeyName + 5);
		m_OrthoDimensions.Set( iOrtho, atof( szValue ) );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

bool CPointCameraOrtho::GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen )
{
	if ( strncmp( szKeyName, "Ortho", 5 ) )
	{
		int iOrtho = atoi(szKeyName + 5);
		Q_snprintf( szValue, iMaxLen, "%f", m_OrthoDimensions[iOrtho] );
	}
	else
		return BaseClass::GetKeyValue( szKeyName, szValue, iMaxLen );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCameraOrtho::ChangeOrtho( int iType, const char *szChange )
{
	// Parse the keyvalue data
	char parseString[255];
	Q_strncpy( parseString, szChange, sizeof( parseString ) );

	// Get Ortho
	char *pszParam = strtok( parseString, " " );
	if (pszParam)
	{
		m_TargetOrtho[iType] = atof( pszParam );
	}
	else
	{
		// Assume no change
		m_TargetOrtho[iType] = m_OrthoDimensions[iType];
	}

	// Get Time
	float flChangeTime;
	pszParam = strtok( NULL, " " );
	if (pszParam)
	{
		flChangeTime = atof( pszParam );
	}
	else
	{
		// Assume 1 second
		flChangeTime = 1.0;
	}

	m_TargetOrthoDPS = ( m_TargetOrtho[iType] - m_OrthoDimensions[iType] ) / flChangeTime;

	SetThink( &CPointCameraOrtho::ChangeOrthoThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCameraOrtho::InputScaleOrtho( inputdata_t &inputdata )
{
	// Parse the keyvalue data
	char parseString[255];
	Q_strncpy( parseString, inputdata.value.String(), sizeof( parseString ) );

	// Get Scale
	float flScale = 1.0f;
	char *pszParam = strtok( parseString, " " );
	if (pszParam)
	{
		flScale = atof( pszParam );
	}

	// Get Time
	float flChangeTime = 1.0f;
	pszParam = strtok( NULL, " " );
	if (pszParam)
	{
		flChangeTime = atof( pszParam );
	}

	int iLargest = 0;
	for (int i = 0; i < NUM_ORTHO_DIMENSIONS; i++)
	{
		m_TargetOrtho[i] = flScale * m_OrthoDimensions[i];

		if (m_TargetOrtho[iLargest] <= m_TargetOrtho[i])
			iLargest = i;
	}

	m_TargetOrthoDPS = (m_TargetOrtho[iLargest] - m_OrthoDimensions[iLargest]) / flChangeTime;

	SetThink( &CPointCameraOrtho::ChangeOrthoThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCameraOrtho::ChangeOrthoThink( void )
{
	SetNextThink( gpGlobals->curtime + CAM_THINK_INTERVAL );

	int iChanging = 0;
	for (int i = 0; i < NUM_ORTHO_DIMENSIONS; i++)
	{
		float newDim = m_OrthoDimensions[i];
		if (newDim == m_TargetOrtho[i])
			continue;

		newDim += m_TargetOrthoDPS * CAM_THINK_INTERVAL;

		if (m_TargetOrthoDPS < 0)
		{
			if (newDim <= m_TargetOrtho[i])
			{
				newDim = m_TargetOrtho[i];
			}
		}
		else
		{
			if (newDim >= m_TargetOrtho[i])
			{
				newDim = m_TargetOrtho[i];
			}
		}

		m_OrthoDimensions.Set(i, newDim);
	}

	if (iChanging == 0)
		SetThink( NULL );
}
#endif
