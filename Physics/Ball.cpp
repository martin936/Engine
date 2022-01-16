#include <string.h>
#include "Engine/Engine.h"
#include "Engine/Physics/Physics.h"



void CSoftbody::SetForceSpeedTarget(const char* Name, float3 Target)
{
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (!strcmp(pForce->GetName(), Name))
		{
			pForce->m_bSpeedTarget = true;
			pForce->m_Force = Target;
			break;
		}
	}
}


void CSoftbody::EditForce(const char* Name, float3 NewForce)
{
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (!strcmp(pForce->GetName(), Name))
		{
			pForce->m_bSpeedTarget = false;
			pForce->m_Force = NewForce;
			break;
		}
	}
}


void CSoftbody::EditForceAdditive(const char* Name, float3 NewForce)
{
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (!strcmp(pForce->GetName(), Name))
		{
			pForce->m_bSpeedTarget = false;
			pForce->m_Force += NewForce;
			break;
		}
	}
}


void CSoftbody::EditForceDuration(const char* Name, float fDuration)
{
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (!strcmp(pForce->GetName(), Name))
		{
			if (fDuration < 0.f)
			{
				pForce->m_Usage		= FORCE_USAGE_ALWAYS;
				pForce->m_bActive	= true;
			}

			else
			{
				pForce->m_Usage		= FORCE_USAGE_TEMPORARY;
				pForce->m_fDuration = fDuration;
				pForce->m_bPending	= false;
				pForce->m_bActive	= false;
			}

			break;
		}
	}
}


void CSoftbody::AddForce(const char* Name, float3 force, ForceUsage usage, float duration)
{
	CForce* pForce = new CForce(Name, force, usage, duration);
	m_pForces.push_back(pForce);
}


void CSoftbody::SumForces()
{
	m_ExternalForce = 0.f;

	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if ((pForce->m_Usage == FORCE_USAGE_ALWAYS || !pForce->m_bPending) && pForce->m_bActive)
		{
			m_ExternalForce = m_ExternalForce + pForce->m_Force;

			if (pForce->m_Usage == FORCE_USAGE_TEMPORARY && pForce->m_fDuration > 0.f)
			{
				pForce->m_fTimeLeft -= 1e-3f * (float)CEngine::GetFrameDuration();
				if (pForce->m_fTimeLeft < 1e-8f)
				{
					pForce->m_bActive = 0;
					pForce->m_Force = 0.f;
				}
			}
		}
	}
}


bool CSoftbody::IsForceEnabled(const char* Name)
{
	int result = 0;
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (!strcmp(pForce->GetName(), Name))
		{
			return pForce->m_bActive;
		}
	}

	return false;
}


void CSoftbody::EnableForce(const char* Name)
{
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (!strcmp(pForce->GetName(), Name))
		{
			pForce->Enable();
			break;
		}
	}
}


void CSoftbody::DisableForce(const char* Name)
{
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (!strcmp(pForce->GetName(), Name))
		{
			pForce->Disable();
			break;
		}
	}
}


void CSoftbody::FlushForce(const char* Name)
{
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (!strcmp(pForce->GetName(), Name) && pForce->m_bPending)
		{
			pForce->m_bPending	= false;
			pForce->m_bActive	= true;

			if (pForce->m_bSpeedTarget)
				for (int j = 0; j < 3; j++)
					pForce->m_Force.v()[j] /= pForce->m_fDuration;
		}
	}
}


void CSoftbody::FlushForces()
{
	CForce* pForce = NULL;
	std::vector<CForce*>::iterator it;

	for (it = m_pForces.begin(); it < m_pForces.end(); it++)
	{
		pForce = *it;
		if (pForce->m_Usage == FORCE_USAGE_TEMPORARY && pForce->m_bPending)
		{
			pForce->m_bPending	= false;
			pForce->m_bActive	= true;

			if (pForce->m_bSpeedTarget)
			{
				for (int j = 0; j < 3; j++)
					pForce->m_Force.v()[j] /= pForce->m_fDuration;
			}
		}
	}
}
