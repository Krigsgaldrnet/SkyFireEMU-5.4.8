/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SF_HOSTILEREFMANAGER
#define SF_HOSTILEREFMANAGER

#include "Common.h"
#include "RefManager.h"

class Unit;
class ThreatManager;
class HostileReference;
class SpellInfo;

//=================================================

class HostileRefManager : public RefManager<Unit, ThreatManager>
{
private:
    Unit* iOwner;
public:
    explicit HostileRefManager(Unit* owner) { iOwner = owner; }
    ~HostileRefManager();

    Unit* GetOwner() { return iOwner; }

    // send threat to all my hateres for the victim
    // The victim is hated than by them as well
    // use for buffs and healing threat functionality
    void threatAssist(Unit* victim, float baseThreat, SpellInfo const* threatSpell = NULL);

    void addTempThreat(float threat, bool apply);

    void addThreatPercent(int32 percent);

    // The references are not needed anymore
    // tell the source to remove them from the list and free the mem
    void deleteReferences();

    // Remove specific faction references
    void deleteReferencesForFaction(uint32 faction);

    HostileReference* getFirst() { return ((HostileReference*)RefManager<Unit, ThreatManager>::getFirst()); }

    void updateThreatTables();

    void setOnlineOfflineState(bool isOnline);

    // set state for one reference, defined by Unit
    void setOnlineOfflineState(Unit* creature, bool isOnline);

    // delete one reference, defined by Unit
    void deleteReference(Unit* creature);

    void UpdateVisibility();
};
//=================================================
#endif
