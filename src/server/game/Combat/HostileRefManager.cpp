/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "DBCStructure.h"
#include "HostileRefManager.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "Unit.h"

HostileRefManager::~HostileRefManager()
{
    deleteReferences();
}

//=================================================
// send threat to all my hateres for the victim
// The victim is hated than by them as well
// use for buffs and healing threat functionality

void HostileRefManager::threatAssist(Unit* victim, float baseThreat, SpellInfo const* threatSpell)
{
    if (getSize() == 0)
        return;

    HostileReference* ref = getFirst();
    float threat = ThreatCalcHelper::calcThreat(victim, iOwner, baseThreat, (threatSpell ? threatSpell->GetSchoolMask() : SPELL_SCHOOL_MASK_NORMAL), threatSpell);
    threat /= getSize();
    while (ref)
    {
        if (ThreatCalcHelper::isValidProcess(victim, ref->GetSource()->GetOwner(), threatSpell))
            ref->GetSource()->doAddThreat(victim, threat);

        ref = ref->next();
    }
}

//=================================================

void HostileRefManager::addTempThreat(float threat, bool apply)
{
    HostileReference* ref = getFirst();

    while (ref)
    {
        if (apply)
        {
            if (ref->getTempThreatModifier() == 0.0f)
                ref->addTempThreat(threat);
        }
        else
            ref->resetTempThreat();

        ref = ref->next();
    }
}

//=================================================

void HostileRefManager::addThreatPercent(int32 percent)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        ref->addThreatPercent(percent);
        ref = ref->next();
    }
}

//=================================================
// The online / offline status is given to the method. The calculation has to be done before

void HostileRefManager::setOnlineOfflineState(bool isOnline)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        ref->setOnlineOfflineState(isOnline);
        ref = ref->next();
    }
}

//=================================================
// The online / offline status is calculated and set

void HostileRefManager::updateThreatTables()
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        ref->updateOnlineStatus();
        ref = ref->next();
    }
}

//=================================================
// The references are not needed anymore
// tell the source to remove them from the list and free the mem

void HostileRefManager::deleteReferences()
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        ref->removeReference();
        delete ref;
        ref = nextRef;
    }
}

//=================================================
// delete one reference, defined by faction

void HostileRefManager::deleteReferencesForFaction(uint32 faction)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        if (ref->GetSource()->GetOwner()->GetFactionTemplateEntry()->faction == faction)
        {
            ref->removeReference();
            delete ref;
        }
        ref = nextRef;
    }
}

//=================================================
// delete one reference, defined by Unit

void HostileRefManager::deleteReference(Unit* creature)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        if (ref->GetSource()->GetOwner() == creature)
        {
            ref->removeReference();
            delete ref;
            break;
        }
        ref = nextRef;
    }
}

//=================================================
// set state for one reference, defined by Unit

void HostileRefManager::setOnlineOfflineState(Unit* creature, bool isOnline)
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        if (ref->GetSource()->GetOwner() == creature)
        {
            ref->setOnlineOfflineState(isOnline);
            break;
        }
        ref = nextRef;
    }
}

//=================================================

void HostileRefManager::UpdateVisibility()
{
    HostileReference* ref = getFirst();
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        if (!ref->GetSource()->GetOwner()->CanSeeOrDetect(GetOwner()))
        {
            nextRef = ref->next();
            ref->removeReference();
            delete ref;
        }
        ref = nextRef;
    }
}
