#include "Engine/Physics/Physics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


CForce::CForce(const char* cName, float3 Force, ForceUsage Usage, float fDuration)
{
	strncpy(m_cName, cName, 256);

	m_Force		= Force;
	m_Usage		= Usage;
	m_fDuration = fDuration;
	m_bPending	= false;
	
	m_bActive	= (Usage == FORCE_USAGE_ALWAYS) ? true : false;

	m_fTimeLeft = fDuration;
	m_bSpeedTarget = false;
}


void CForce::Enable()
{
	if (m_Usage == FORCE_USAGE_ALWAYS)
		m_bActive = true;

	else
	{
		m_bPending	= true;
		m_bActive	= false;
		m_fTimeLeft = m_fDuration;
	}
}


void CForce::Disable()
{
	m_bActive	= false;
	m_bPending	= false;
}



CForceField::CForceField(const char* pcName, float(*pFieldFunc)(float3& Position, float3& Velocity))
{
	strncpy(m_cName, pcName, 256);

	m_pFieldFunc = pFieldFunc;
}