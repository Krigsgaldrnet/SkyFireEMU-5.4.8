/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Creature.h"
#include "CreatureAI.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "Unit.h"
#include "UnitEvents.h"

//==============================================================
//================= ThreatCalcHelper ===========================
//==============================================================

// The hatingUnit is not used yet
float ThreatCalcHelper::calcThreat(Unit* hatedUnit, Unit* /*hatingUnit*/, float threat, SpellSchoolMask schoolMask, SpellInfo const* threatSpell)
{
    if (threatSpell)
    {
        if (SpellThreatEntry const* threatEntry = sSpellMgr->GetSpellThreatEntry(threatSpell->Id))
            if (threatEntry->pctMod != 1.0f)
                threat *= threatEntry->pctMod;

        // Energize is not affected by Mods
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
            if (threatSpell->Effects[i].Effect == SPELL_EFFECT_ENERGIZE || threatSpell->Effects[i].ApplyAuraName == SPELL_AURA_PERIODIC_ENERGIZE)
                return threat;

        if (Player* modOwner = hatedUnit->GetSpellModOwner())
            modOwner->ApplySpellMod(threatSpell->Id, SPELLMOD_THREAT, threat);
    }

    return hatedUnit->ApplyTotalThreatModifier(threat, schoolMask);
}

bool ThreatCalcHelper::isValidProcess(Unit* hatedUnit, Unit* hatingUnit, SpellInfo const* threatSpell)
{
    //function deals with adding threat and adding players and pets into ThreatList
    //mobs, NPCs, guards have ThreatList and HateOfflineList
    //players and pets have only InHateListOf
    //HateOfflineList is used co contain unattackable victims (in-flight, in-water, GM etc.)

    if (!hatedUnit || !hatingUnit)
        return false;

    // not to self
    if (hatedUnit == hatingUnit)
        return false;

    // not to GM
    if (hatedUnit->GetTypeId() == TypeID::TYPEID_PLAYER && hatedUnit->ToPlayer()->IsGameMaster())
        return false;

    // not to dead and not for dead
    if (!hatedUnit->IsAlive() || !hatingUnit->IsAlive())
        return false;

    // not in same map or phase
    if (!hatedUnit->IsInMap(hatingUnit) || !hatedUnit->InSamePhase(hatingUnit))
        return false;

    // spell not causing threat
    if (threatSpell && threatSpell->AttributesEx & SPELL_ATTR1_NO_THREAT)
        return false;

    ASSERT(hatingUnit->GetTypeId() == TypeID::TYPEID_UNIT);

    return true;
}

//============================================================
//================= HostileReference ==========================
//============================================================

HostileReference::HostileReference(Unit* refUnit, ThreatManager* threatManager, float threat)
{
    iThreat = threat;
    iTempThreatModifier = 0.0f;
    link(refUnit, threatManager);
    iUnitGuid = refUnit->GetGUID();
    iOnline = true;
    iAccessible = true;
}

//============================================================
// Tell our refTo (target) object that we have a link
void HostileReference::targetObjectBuildLink()
{
    getTarget()->addHatedBy(this);
}

//============================================================
// Tell our refTo (taget) object, that the link is cut
void HostileReference::targetObjectDestroyLink()
{
    getTarget()->removeHatedBy(this);
}

//============================================================
// Tell our refFrom (source) object, that the link is cut (Target destroyed)

void HostileReference::sourceObjectDestroyLink()
{
    setOnlineOfflineState(false);
}

//============================================================
// Inform the source, that the status of the reference changed

void HostileReference::fireStatusChanged(ThreatRefStatusChangeEvent& threatRefStatusChangeEvent)
{
    if (GetSource())
        GetSource()->processThreatEvent(&threatRefStatusChangeEvent);
}

//============================================================

void HostileReference::addThreat(float modThreat)
{
    iThreat += modThreat;
    // the threat is changed. Source and target unit have to be available
    // if the link was cut before relink it again
    if (!isOnline())
        updateOnlineStatus();
    if (modThreat != 0.0f)
    {
        ThreatRefStatusChangeEvent event(UEV_THREAT_REF_THREAT_CHANGE, this, modThreat);
        fireStatusChanged(event);
    }

    if (isValid() && modThreat >= 0.0f)
    {
        Unit* victimOwner = getTarget()->GetCharmerOrOwner();
        if (victimOwner && victimOwner->IsAlive())
            GetSource()->addThreat(victimOwner, 0.0f);     // create a threat to the owner of a pet, if the pet attacks
    }
}

void HostileReference::addThreatPercent(int32 percent)
{
    float tmpThreat = iThreat;
    AddPct(tmpThreat, percent);
    addThreat(tmpThreat - iThreat);
}

//============================================================
// check, if source can reach target and set the status

void HostileReference::updateOnlineStatus()
{
    bool online = false;
    bool accessible = false;

    if (!isValid())
        if (Unit* target = ObjectAccessor::GetUnit(*GetSourceUnit(), getUnitGuid()))
            link(target, GetSource());

    // only check for online status if
    // ref is valid
    // target is no player or not gamemaster
    // target is not in flight
    if (isValid()
        && (getTarget()->GetTypeId() != TypeID::TYPEID_PLAYER || !getTarget()->ToPlayer()->IsGameMaster())
        && !getTarget()->HasUnitState(UNIT_STATE_IN_FLIGHT)
        && getTarget()->IsInMap(GetSourceUnit())
        && getTarget()->InSamePhase(GetSourceUnit())
        )
    {
        Creature* creature = GetSourceUnit()->ToCreature();
        online = getTarget()->isInAccessiblePlaceFor(creature);
        if (!online)
        {
            if (creature->IsWithinCombatRange(getTarget(), creature->m_CombatDistance))
                online = true;                              // not accessible but stays online
        }
        else
            accessible = true;
    }
    setAccessibleState(accessible);
    setOnlineOfflineState(online);
}

//============================================================
// set the status and fire the event on status change

void HostileReference::setOnlineOfflineState(bool isOnline)
{
    if (iOnline != isOnline)
    {
        iOnline = isOnline;
        if (!iOnline)
            setAccessibleState(false);                      // if not online that not accessable as well

        ThreatRefStatusChangeEvent event(UEV_THREAT_REF_ONLINE_STATUS, this);
        fireStatusChanged(event);
    }
}

//============================================================

void HostileReference::setAccessibleState(bool isAccessible)
{
    if (iAccessible != isAccessible)
    {
        iAccessible = isAccessible;

        ThreatRefStatusChangeEvent event(UEV_THREAT_REF_ASSECCIBLE_STATUS, this);
        fireStatusChanged(event);
    }
}

//============================================================
// prepare the reference for deleting
// this is called be the target

void HostileReference::removeReference()
{
    invalidate();

    ThreatRefStatusChangeEvent event(UEV_THREAT_REF_REMOVE_FROM_LIST, this);
    fireStatusChanged(event);
}

//============================================================

Unit* HostileReference::GetSourceUnit()
{
    return (GetSource()->GetOwner());
}

//============================================================
//================ ThreatContainer ===========================
//============================================================

void ThreatContainer::clearReferences()
{
    for (ThreatContainer::StorageType::const_iterator i = iThreatList.begin(); i != iThreatList.end(); ++i)
    {
        (*i)->unlink();
        delete (*i);
    }

    iThreatList.clear();
}

//============================================================
// Return the HostileReference of NULL, if not found
HostileReference* ThreatContainer::getReferenceByTarget(Unit* victim) const
{
    if (!victim)
        return NULL;

    uint64 const guid = victim->GetGUID();
    for (ThreatContainer::StorageType::const_iterator i = iThreatList.begin(); i != iThreatList.end(); ++i)
    {
        HostileReference* ref = (*i);
        if (ref && ref->getUnitGuid() == guid)
            return ref;
    }

    return NULL;
}

//============================================================
// Add the threat, if we find the reference

HostileReference* ThreatContainer::addThreat(Unit* victim, float threat)
{
    HostileReference* ref = getReferenceByTarget(victim);
    if (ref)
        ref->addThreat(threat);
    return ref;
}

//============================================================

void ThreatContainer::modifyThreatPercent(Unit* victim, int32 percent)
{
    if (HostileReference* ref = getReferenceByTarget(victim))
        ref->addThreatPercent(percent);
}

//============================================================
// Check if the list is dirty and sort if necessary

void ThreatContainer::update()
{
    if (iDirty && iThreatList.size() > 1)
        iThreatList.sort(Skyfire::ThreatOrderPred());

    iDirty = false;
}

//============================================================
// return the next best victim
// could be the current victim

HostileReference* ThreatContainer::selectNextVictim(Creature* attacker, HostileReference* currentVictim) const
{
    HostileReference* currentRef = NULL;
    bool found = false;
    bool noPriorityTargetFound = false;

    ThreatContainer::StorageType::const_iterator lastRef = iThreatList.end();
    --lastRef;

    for (ThreatContainer::StorageType::const_iterator iter = iThreatList.begin(); iter != iThreatList.end() && !found;)
    {
        currentRef = (*iter);

        Unit* target = currentRef->getTarget();
        ASSERT(target);                                     // if the ref has status online the target must be there !

        // some units are prefered in comparison to others
        if (!noPriorityTargetFound && (target->IsImmunedToDamage(attacker->GetMeleeDamageSchoolMask()) || target->HasNegativeAuraWithInterruptFlag(AURA_INTERRUPT_FLAG_TAKE_DAMAGE)))
        {
            if (iter != lastRef)
            {
                // current victim is a second choice target, so don't compare threat with it below
                if (currentRef == currentVictim)
                    currentVictim = NULL;
                ++iter;
                continue;
            }
            else
            {
                // if we reached to this point, everyone in the threatlist is a second choice target. In such a situation the target with the highest threat should be attacked.
                noPriorityTargetFound = true;
                iter = iThreatList.begin();
                continue;
            }
        }

        if (attacker->CanCreatureAttack(target))           // skip non attackable currently targets
        {
            if (currentVictim)                              // select 1.3/1.1 better target in comparison current target
            {
                // list sorted and and we check current target, then this is best case
                if (currentVictim == currentRef || currentRef->getThreat() <= 1.1f * currentVictim->getThreat())
                {
                    if (currentVictim != currentRef && attacker->CanCreatureAttack(currentVictim->getTarget()))
                        currentRef = currentVictim;            // for second case, if currentvictim is attackable

                    found = true;
                    break;
                }

                if (currentRef->getThreat() > 1.3f * currentVictim->getThreat() ||
                    (currentRef->getThreat() > 1.1f * currentVictim->getThreat() &&
                        attacker->IsWithinMeleeRange(target)))
                {                                           //implement 110% threat rule for targets in melee range
                    found = true;                           //and 130% rule for targets in ranged distances
                    break;                                  //for selecting alive targets
                }
            }
            else                                            // select any
            {
                found = true;
                break;
            }
        }
        ++iter;
    }
    if (!found)
        currentRef = NULL;

    return currentRef;
}

//============================================================
//=================== ThreatManager ==========================
//============================================================

ThreatManager::ThreatManager(Unit* owner) : iCurrentVictim(NULL), iOwner(owner), iUpdateTimer(THREAT_UPDATE_INTERVAL) { }

//============================================================

void ThreatManager::clearReferences()
{
    iThreatContainer.clearReferences();
    iThreatOfflineContainer.clearReferences();
    iCurrentVictim = NULL;
    iUpdateTimer = THREAT_UPDATE_INTERVAL;
}

//============================================================

void ThreatManager::addThreat(Unit* victim, float threat, SpellSchoolMask schoolMask, SpellInfo const* threatSpell)
{
    if (!ThreatCalcHelper::isValidProcess(victim, GetOwner(), threatSpell))
        return;

    doAddThreat(victim, ThreatCalcHelper::calcThreat(victim, iOwner, threat, schoolMask, threatSpell));
}

void ThreatManager::doAddThreat(Unit* victim, float threat)
{
    uint32 redirectThreadPct = victim->GetRedirectThreatPercent();

    // must check > 0.0f, otherwise dead loop
    if (threat > 0.0f && redirectThreadPct)
    {
        if (Unit* redirectTarget = victim->GetRedirectThreatTarget())
        {
            float redirectThreat = CalculatePct(threat, redirectThreadPct);
            threat -= redirectThreat;
            _addThreat(redirectTarget, redirectThreat);
        }
    }

    _addThreat(victim, threat);
}

void ThreatManager::_addThreat(Unit* victim, float threat)
{
    HostileReference* ref = iThreatContainer.addThreat(victim, threat);
    // Ref is not in the online refs, search the offline refs next
    if (!ref)
        ref = iThreatOfflineContainer.addThreat(victim, threat);

    if (!ref) // there was no ref => create a new one
    {
        // threat has to be 0 here
        HostileReference* hostileRef = new HostileReference(victim, this, 0);
        iThreatContainer.addReference(hostileRef);
        hostileRef->addThreat(threat); // now we add the real threat
        if (victim->GetTypeId() == TypeID::TYPEID_PLAYER && victim->ToPlayer()->IsGameMaster())
            hostileRef->setOnlineOfflineState(false); // GM is always offline
    }
}

//============================================================

void ThreatManager::modifyThreatPercent(Unit* victim, int32 percent)
{
    iThreatContainer.modifyThreatPercent(victim, percent);
}

//============================================================

Unit* ThreatManager::getHostilTarget()
{
    iThreatContainer.update();
    HostileReference* nextVictim = iThreatContainer.selectNextVictim(GetOwner()->ToCreature(), getCurrentVictim());
    setCurrentVictim(nextVictim);
    return getCurrentVictim() != NULL ? getCurrentVictim()->getTarget() : NULL;
}

//============================================================

float ThreatManager::getThreat(Unit* victim, bool alsoSearchOfflineList)
{
    float threat = 0.0f;
    HostileReference* ref = iThreatContainer.getReferenceByTarget(victim);
    if (!ref && alsoSearchOfflineList)
        ref = iThreatOfflineContainer.getReferenceByTarget(victim);
    if (ref)
        threat = ref->getThreat();
    return threat;
}

//============================================================

void ThreatManager::tauntApply(Unit* taunter)
{
    HostileReference* ref = iThreatContainer.getReferenceByTarget(taunter);
    if (getCurrentVictim() && ref && (ref->getThreat() < getCurrentVictim()->getThreat()))
    {
        if (ref->getTempThreatModifier() == 0.0f) // Ok, temp threat is unused
            ref->setTempThreat(getCurrentVictim()->getThreat());
    }
}

//============================================================

void ThreatManager::tauntFadeOut(Unit* taunter)
{
    HostileReference* ref = iThreatContainer.getReferenceByTarget(taunter);
    if (ref)
        ref->resetTempThreat();
}

//============================================================

void ThreatManager::setCurrentVictim(HostileReference* pHostileReference)
{
    if (pHostileReference && pHostileReference != iCurrentVictim)
    {
        iOwner->SendChangeCurrentVictimOpcode(pHostileReference);
    }
    iCurrentVictim = pHostileReference;
}

//============================================================
// The hated unit is gone, dead or deleted
// return true, if the event is consumed

void ThreatManager::processThreatEvent(ThreatRefStatusChangeEvent* threatRefStatusChangeEvent)
{
    threatRefStatusChangeEvent->setThreatManager(this);     // now we can set the threat manager

    HostileReference* hostilRef = threatRefStatusChangeEvent->getReference();

    switch (threatRefStatusChangeEvent->getType())
    {
        case UEV_THREAT_REF_THREAT_CHANGE:
            if ((getCurrentVictim() == hostilRef && threatRefStatusChangeEvent->getFValue() < 0.0f) ||
                (getCurrentVictim() != hostilRef && threatRefStatusChangeEvent->getFValue() > 0.0f))
                setDirty(true);                             // the order in the threat list might have changed
            break;
        case UEV_THREAT_REF_ONLINE_STATUS:
            if (!hostilRef->isOnline())
            {
                if (hostilRef == getCurrentVictim())
                {
                    setCurrentVictim(NULL);
                    setDirty(true);
                }
                iOwner->SendRemoveFromThreatListOpcode(hostilRef);
                iThreatContainer.remove(hostilRef);
                iThreatOfflineContainer.addReference(hostilRef);
            }
            else
            {
                if (getCurrentVictim() && hostilRef->getThreat() > (1.1f * getCurrentVictim()->getThreat()))
                    setDirty(true);
                iThreatContainer.addReference(hostilRef);
                iThreatOfflineContainer.remove(hostilRef);
            }
            break;
        case UEV_THREAT_REF_REMOVE_FROM_LIST:
            if (hostilRef == getCurrentVictim())
            {
                setCurrentVictim(NULL);
                setDirty(true);
            }
            iOwner->SendRemoveFromThreatListOpcode(hostilRef);
            if (hostilRef->isOnline())
                iThreatContainer.remove(hostilRef);
            else
                iThreatOfflineContainer.remove(hostilRef);
            break;
    }
}

bool ThreatManager::isNeedUpdateToClient(uint32 time)
{
    if (isThreatListEmpty())
        return false;

    if (time >= iUpdateTimer)
    {
        iUpdateTimer = THREAT_UPDATE_INTERVAL;
        return true;
    }
    iUpdateTimer -= time;
    return false;
}

// Reset all aggro without modifying the threatlist.
void ThreatManager::resetAllAggro()
{
    ThreatContainer::StorageType& threatList = iThreatContainer.iThreatList;
    if (threatList.empty())
        return;

    for (ThreatContainer::StorageType::iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
        (*itr)->setThreat(0);

    setDirty(true);
}
