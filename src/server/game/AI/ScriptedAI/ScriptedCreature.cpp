/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "TemporarySummon.h"

// Spell summary for ScriptedAI::SelectSpell
extern struct TSpellSummary* SpellSummary;

void SummonList::DoZoneInCombat(uint32 entry)
{
    for (StorageType::iterator i = storage_.begin(); i != storage_.end();)
    {
        Creature* summon = Unit::GetCreature(*me, *i);
        ++i;
        if (summon && summon->IsAIEnabled
            && (!entry || summon->GetEntry() == entry))
        {
            summon->AI()->DoZoneInCombat();
        }
    }
}

void SummonList::DespawnEntry(uint32 entry)
{
    for (StorageType::iterator i = storage_.begin(); i != storage_.end();)
    {
        Creature* summon = Unit::GetCreature(*me, *i);
        if (!summon)
            i = storage_.erase(i);
        else if (summon->GetEntry() == entry)
        {
            i = storage_.erase(i);
            summon->DespawnOrUnsummon();
        }
        else
            ++i;
    }
}

void SummonList::DespawnAll()
{
    while (!storage_.empty())
    {
        Creature* summon = Unit::GetCreature(*me, storage_.front());
        storage_.pop_front();
        if (summon)
            summon->DespawnOrUnsummon();
    }
}

void SummonList::RemoveNotExisting()
{
    for (StorageType::iterator i = storage_.begin(); i != storage_.end();)
    {
        if (Unit::GetCreature(*me, *i))
            ++i;
        else
            i = storage_.erase(i);
    }
}

bool SummonList::HasEntry(uint32 entry) const
{
    for (StorageType::const_iterator i = storage_.begin(); i != storage_.end(); ++i)
    {
        Creature* summon = Unit::GetCreature(*me, *i);
        if (summon && summon->GetEntry() == entry)
            return true;
    }

    return false;
}

ScriptedAI::ScriptedAI(Creature* creature) : CreatureAI(creature),
me(creature),
IsFleeing(false),
_evadeCheckCooldown(2500),
_isCombatMovementAllowed(true)
{
    _isHeroic = me->GetMap()->IsHeroic();
    _difficulty = DifficultyID(me->GetMap()->GetSpawnMode());
}

bool ScriptedAI::Is25ManRaid() const
{
    switch (_difficulty)
    {
        case DIFFICULTY_25MAN_LFR:
        case DIFFICULTY_25MAN_NORMAL:
        case DIFFICULTY_25MAN_HEROIC:
            return true;
        default:
            break;
    }
    return false;
}

void ScriptedAI::AttackStartNoMove(Unit* who)
{
    if (!who)
        return;

    if (me->Attack(who, true))
        DoStartNoMovement(who);
}

void ScriptedAI::AttackStart(Unit* who)
{
    if (IsCombatMovementAllowed())
        CreatureAI::AttackStart(who);
    else
        AttackStartNoMove(who);
}

void ScriptedAI::UpdateAI(uint32 /*diff*/)
{
    //Check if we have a current target
    if (!UpdateVictim())
        return;

    DoMeleeAttackIfReady();
}

void ScriptedAI::DoStartMovement(Unit* victim, float distance, float angle)
{
    if (victim)
        me->GetMotionMaster()->MoveChase(victim, distance, angle);
}

void ScriptedAI::DoStartNoMovement(Unit* victim)
{
    if (!victim)
        return;

    me->GetMotionMaster()->MoveIdle();
}

void ScriptedAI::DoStopAttack()
{
    if (me->GetVictim())
        me->AttackStop();
}

void ScriptedAI::DoCastSpell(Unit* target, SpellInfo const* spellInfo, bool triggered)
{
    if (!target || me->IsNonMeleeSpellCasted(false))
        return;

    me->StopMoving();
    me->CastSpell(target, spellInfo, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE);
}

void ScriptedAI::DoPlaySoundToSet(WorldObject* source, uint32 soundId)
{
    if (!source)
        return;

    if (!sSoundEntriesStore.LookupEntry(soundId))
    {
        SF_LOG_ERROR("scripts", "Invalid soundId %u used in DoPlaySoundToSet (Source: TypeId %u, GUID %u)", soundId, uint8(source->GetTypeId()), source->GetGUIDLow());
        return;
    }

    source->PlayDirectSound(soundId);
}

Creature* ScriptedAI::DoSpawnCreature(uint32 entry, float offsetX, float offsetY, float offsetZ, float angle, TempSummonType type, uint32 despawntime)
{
    return me->SummonCreature(entry, me->GetPositionX() + offsetX, me->GetPositionY() + offsetY, me->GetPositionZ() + offsetZ, angle, type, despawntime);
}

SpellInfo const* ScriptedAI::SelectSpell(Unit* target, uint32 school, uint32 mechanic, SelectTargetType targets, uint32 powerCostMin, uint32 powerCostMax, float rangeMin, float rangeMax, SelectEffect effects)
{
    //No target so we can't cast
    if (!target)
        return NULL;

    //Silenced so we can't cast
    if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
        return NULL;

    //Using the extended script system we first create a list of viable spells
    SpellInfo const* apSpell[CREATURE_MAX_SPELLS];
    memset(apSpell, 0, CREATURE_MAX_SPELLS * sizeof(SpellInfo*));

    uint32 spellCount = 0;

    SpellInfo const* tempSpell = NULL;

    //Check if each spell is viable(set it to null if not)
    for (uint32 i = 0; i < CREATURE_MAX_SPELLS; i++)
    {
        tempSpell = sSpellMgr->GetSpellInfo(me->m_spells[i]);

        //This spell doesn't exist
        if (!tempSpell)
            continue;

        // Targets and Effects checked first as most used restrictions
        //Check the spell targets if specified
        if (targets && !(SpellSummary[me->m_spells[i]].Targets & (1 << (targets - 1))))
            continue;

        //Check the type of spell if we are looking for a specific spell type
        if (effects && !(SpellSummary[me->m_spells[i]].Effects & (1 << (effects - 1))))
            continue;

        //Check for school if specified
        if (school && (tempSpell->SchoolMask & school) == 0)
            continue;

        //Check for spell mechanic if specified
        if (mechanic && tempSpell->Mechanic != mechanic)
            continue;

        //Make sure that the spell uses the requested amount of power
        //if (powerCostMin && tempSpell->ManaCost < powerCostMin)
        //    continue;

        //if (powerCostMax && tempSpell->ManaCost > powerCostMax)
        //    continue;

        //Continue if we don't have the mana to actually cast this spell
        //if (tempSpell->ManaCost > (uint32)me->GetPower(Powers(tempSpell->PowerType)))
        //    continue;

        //Check if the spell meets our range requirements
        if (rangeMin && me->GetSpellMinRangeForTarget(target, tempSpell) < rangeMin)
            continue;
        if (rangeMax && me->GetSpellMaxRangeForTarget(target, tempSpell) > rangeMax)
            continue;

        //Check if our target is in range
        if (me->IsWithinDistInMap(target, float(me->GetSpellMinRangeForTarget(target, tempSpell))) || !me->IsWithinDistInMap(target, float(me->GetSpellMaxRangeForTarget(target, tempSpell))))
            continue;

        //All good so lets add it to the spell list
        apSpell[spellCount] = tempSpell;
        ++spellCount;
    }

    //We got our usable spells so now lets randomly pick one
    if (!spellCount)
        return NULL;

    return apSpell[std::rand() % spellCount];
}

void ScriptedAI::DoResetThreat()
{
    if (!me->CanHaveThreatList() || me->getThreatManager().isThreatListEmpty())
    {
        SF_LOG_ERROR("scripts", "DoResetThreat called for creature that either cannot have threat list or has empty threat list (me entry = %d)", me->GetEntry());
        return;
    }

    ThreatContainer::StorageType threatlist = me->getThreatManager().getThreatList();

    for (ThreatContainer::StorageType::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
    {
        Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid());
        if (unit && DoGetThreat(unit))
            DoModifyThreatPercent(unit, -100);
    }
}

float ScriptedAI::DoGetThreat(Unit* unit)
{
    if (!unit)
        return 0.0f;
    return me->getThreatManager().getThreat(unit);
}

void ScriptedAI::DoModifyThreatPercent(Unit* unit, int32 pct)
{
    if (!unit)
        return;
    me->getThreatManager().modifyThreatPercent(unit, pct);
}

void ScriptedAI::DoTeleportTo(float x, float y, float z, uint32 time)
{
    me->Relocate(x, y, z);
    float speed = me->GetDistance(x, y, z) / ((float)time * 0.001f);
    me->MonsterMoveWithSpeed(x, y, z, speed);
}

void ScriptedAI::DoTeleportTo(const float position[4])
{
    me->NearTeleportTo(position[0], position[1], position[2], position[3]);
}

void ScriptedAI::DoTeleportPlayer(Unit* unit, float x, float y, float z, float o)
{
    if (!unit)
        return;

    if (Player* player = unit->ToPlayer())
        player->TeleportTo(unit->GetMapId(), x, y, z, o, TELE_TO_NOT_LEAVE_COMBAT);
    else
        SF_LOG_ERROR("scripts", "Creature " UI64FMTD " (Entry: %u) Tried to teleport non-player unit (Type: %u GUID: " UI64FMTD ") to x: %f y:%f z: %f o: %f. Aborted.",
            me->GetGUID(), me->GetEntry(), uint8(unit->GetTypeId()), unit->GetGUID(), x, y, z, o);
}

void ScriptedAI::DoTeleportAll(float x, float y, float z, float o)
{
    Map* map = me->GetMap();
    if (!map->IsInstance())
        return;

    Map::PlayerList const& PlayerList = map->GetPlayers();
    for (Map::PlayerList::const_iterator itr = PlayerList.begin(); itr != PlayerList.end(); ++itr)
        if (Player* player = itr->GetSource())
            if (player->IsAlive())
                player->TeleportTo(me->GetMapId(), x, y, z, o, TELE_TO_NOT_LEAVE_COMBAT);
}

Unit* ScriptedAI::DoSelectLowestHpFriendly(float range, uint32 minHPDiff)
{
    Unit* unit = NULL;
    Skyfire::MostHPMissingInRange u_check(me, range, minHPDiff);
    Skyfire::UnitLastSearcher<Skyfire::MostHPMissingInRange> searcher(me, unit, u_check);
    me->VisitNearbyObject(range, searcher);

    return unit;
}

std::list<Creature*> ScriptedAI::DoFindFriendlyCC(float range)
{
    std::list<Creature*> list;
    Skyfire::FriendlyCCedInRange u_check(me, range);
    Skyfire::CreatureListSearcher<Skyfire::FriendlyCCedInRange> searcher(me, list, u_check);
    me->VisitNearbyObject(range, searcher);

    return list;
}

std::list<Creature*> ScriptedAI::DoFindFriendlyMissingBuff(float range, uint32 uiSpellid)
{
    std::list<Creature*> list;
    Skyfire::FriendlyMissingBuffInRange u_check(me, range, uiSpellid);
    Skyfire::CreatureListSearcher<Skyfire::FriendlyMissingBuffInRange> searcher(me, list, u_check);
    me->VisitNearbyObject(range, searcher);

    return list;
}

Player* ScriptedAI::GetPlayerAtMinimumRange(float minimumRange)
{
    Player* player = NULL;

    CellCoord pair(Skyfire::ComputeCellCoord(me->GetPositionX(), me->GetPositionY()));
    Cell cell(pair);
    cell.SetNoCreate();

    Skyfire::PlayerAtMinimumRangeAway check(me, minimumRange);
    Skyfire::PlayerSearcher<Skyfire::PlayerAtMinimumRangeAway> searcher(me, player, check);
    TypeContainerVisitor<Skyfire::PlayerSearcher<Skyfire::PlayerAtMinimumRangeAway>, GridTypeMapContainer> visitor(searcher);

    cell.Visit(pair, visitor, *me->GetMap(), *me, minimumRange);

    return player;
}

void ScriptedAI::SetEquipmentSlots(bool loadDefault, int32 mainHand /*= EQUIP_NO_CHANGE*/, int32 offHand /*= EQUIP_NO_CHANGE*/, int32 ranged /*= EQUIP_NO_CHANGE*/)
{
    if (loadDefault)
    {
        me->LoadEquipment(me->GetOriginalEquipmentId(), true);
        return;
    }

    if (mainHand >= 0)
        me->SetUInt32Value(UNIT_FIELD_VIRTUAL_ITEM_ID + 0, uint32(mainHand));

    if (offHand >= 0)
        me->SetUInt32Value(UNIT_FIELD_VIRTUAL_ITEM_ID + 1, uint32(offHand));

    if (ranged >= 0)
        me->SetUInt32Value(UNIT_FIELD_VIRTUAL_ITEM_ID + 2, uint32(ranged));
}

void ScriptedAI::SetCombatMovement(bool allowMovement)
{
    _isCombatMovementAllowed = allowMovement;
}

enum NPCs
{
    NPC_BROODLORD = 12017,
    NPC_VOID_REAVER = 19516,
    NPC_JAN_ALAI = 23578,
    NPC_SARTHARION = 28860
};

// Hacklike storage used for misc creatures that are expected to evade of outside of a certain area.
// It is assumed the information is found elswehere and can be handled by the core. So far no luck finding such information/way to extract it.
bool ScriptedAI::EnterEvadeIfOutOfCombatArea(uint32 const diff)
{
    if (_evadeCheckCooldown <= diff)
        _evadeCheckCooldown = 2500;
    else
    {
        _evadeCheckCooldown -= diff;
        return false;
    }

    if (me->IsInEvadeMode() || !me->GetVictim())
        return false;

    float x = me->GetPositionX();
    float y = me->GetPositionY();
    float z = me->GetPositionZ();

    switch (me->GetEntry())
    {
        case NPC_BROODLORD:                                         // broodlord (not move down stairs)
            if (z > 448.60f)
                return false;
            break;
        case NPC_VOID_REAVER:                                         // void reaver (calculate from center of room)
            if (me->GetDistance2d(432.59f, 371.93f) < 105.0f)
                return false;
            break;
        case NPC_JAN_ALAI:                                         // jan'alai (calculate by Z)
            if (z > 12.0f)
                return false;
            break;
        case NPC_SARTHARION:                                         // sartharion (calculate box)
            if (x > 3218.86f && x < 3275.69f && y < 572.40f && y > 484.68f)
                return false;
            break;
        default: // For most of creatures that certain area is their home area.
            SF_LOG_INFO("misc", "TSCR: EnterEvadeIfOutOfCombatArea used for creature entry %u, but does not have any definition. Using the default one.", me->GetEntry());
            uint32 homeAreaId = me->GetMap()->GetAreaId(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ());
            if (me->GetAreaId() == homeAreaId)
                return false;
    }

    EnterEvadeMode();
    return true;
}

// BossAI - for instanced bosses
BossAI::BossAI(Creature* creature, uint32 bossId) : ScriptedAI(creature),
instance(creature->GetInstanceScript()),
summons(creature),
_boundary(instance ? instance->GetBossBoundary(bossId) : NULL),
_bossId(bossId) { }

void BossAI::_Reset()
{
    if (!me->IsAlive())
        return;

    me->ResetLootMode();
    events.Reset();
    summons.DespawnAll();
    if (instance)
        instance->SetBossState(_bossId, NOT_STARTED);
}

void BossAI::_JustDied()
{
    events.Reset();
    summons.DespawnAll();
    if (instance)
    {
        instance->SetBossState(_bossId, DONE);
        instance->SaveToDB();
    }
}

void BossAI::_EnterCombat()
{
    me->setActive(true);
    DoZoneInCombat();
    if (instance)
    {
        // bosses do not respawn, check only on enter combat
        if (!instance->CheckRequiredBosses(_bossId))
        {
            EnterEvadeMode();
            return;
        }
        instance->SetBossState(_bossId, IN_PROGRESS);
    }
}

void BossAI::TeleportCheaters()
{
    float x, y, z;
    me->GetPosition(x, y, z);

    ThreatContainer::StorageType threatList = me->getThreatManager().getThreatList();
    for (ThreatContainer::StorageType::const_iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
        if (Unit* target = (*itr)->getTarget())
            if (target->GetTypeId() == TypeID::TYPEID_PLAYER && !CheckBoundary(target))
                target->NearTeleportTo(x, y, z, 0);
}

bool BossAI::CheckBoundary(Unit* who)
{
    if (!GetBoundary() || !who)
        return true;

    for (BossBoundaryMap::const_iterator itr = GetBoundary()->begin(); itr != GetBoundary()->end(); ++itr)
    {
        switch (itr->first)
        {
            case BOUNDARY_N:
                if (who->GetPositionX() > itr->second)
                    return false;
                break;
            case BOUNDARY_S:
                if (who->GetPositionX() < itr->second)
                    return false;
                break;
            case BOUNDARY_E:
                if (who->GetPositionY() < itr->second)
                    return false;
                break;
            case BOUNDARY_W:
                if (who->GetPositionY() > itr->second)
                    return false;
                break;
            case BOUNDARY_NW:
                if (who->GetPositionX() + who->GetPositionY() > itr->second)
                    return false;
                break;
            case BOUNDARY_SE:
                if (who->GetPositionX() + who->GetPositionY() < itr->second)
                    return false;
                break;
            case BOUNDARY_NE:
                if (who->GetPositionX() - who->GetPositionY() > itr->second)
                    return false;
                break;
            case BOUNDARY_SW:
                if (who->GetPositionX() - who->GetPositionY() < itr->second)
                    return false;
                break;
            default:
                break;
        }
    }

    return true;
}

void BossAI::JustSummoned(Creature* summon)
{
    summons.Summon(summon);
    if (me->IsInCombat())
        DoZoneInCombat(summon);
}

void BossAI::SummonedCreatureDespawn(Creature* summon)
{
    summons.Despawn(summon);
}

void BossAI::UpdateAI(uint32 diff)
{
    if (!UpdateVictim())
        return;

    events.Update(diff);

    if (me->HasUnitState(UNIT_STATE_CASTING))
        return;

    while (uint32 eventId = events.ExecuteEvent())
        ExecuteEvent(eventId);

    DoMeleeAttackIfReady();
}

void BossAI::_DespawnAtEvade()
{
    uint32 corpseDelay = me->GetCorpseDelay();
    uint32 respawnDelay = me->GetRespawnDelay();

    me->SetCorpseDelay(1);
    me->SetRespawnDelay(29);

    me->DespawnOrUnsummon();

    me->SetCorpseDelay(corpseDelay);
    me->SetRespawnDelay(respawnDelay);
}

// WorldBossAI - for non-instanced bosses

WorldBossAI::WorldBossAI(Creature* creature) :
    ScriptedAI(creature),
    summons(creature) { }

void WorldBossAI::_Reset()
{
    if (!me->IsAlive())
        return;

    events.Reset();
    summons.DespawnAll();
}

void WorldBossAI::_JustDied()
{
    events.Reset();
    summons.DespawnAll();
}

void WorldBossAI::_EnterCombat()
{
    Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true);
    if (target)
        AttackStart(target);
}

void WorldBossAI::JustSummoned(Creature* summon)
{
    summons.Summon(summon);
    Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true);
    if (target)
        summon->AI()->AttackStart(target);
}

void WorldBossAI::SummonedCreatureDespawn(Creature* summon)
{
    summons.Despawn(summon);
}

void WorldBossAI::UpdateAI(uint32 diff)
{
    if (!UpdateVictim())
        return;

    events.Update(diff);

    if (me->HasUnitState(UNIT_STATE_CASTING))
        return;

    while (uint32 eventId = events.ExecuteEvent())
        ExecuteEvent(eventId);

    DoMeleeAttackIfReady();
}

// SD2 grid searchers.
Creature* GetClosestCreatureWithEntry(WorldObject* source, uint32 entry, float maxSearchRange, bool alive /*= true*/)
{
    return source->FindNearestCreature(entry, maxSearchRange, alive);
}

GameObject* GetClosestGameObjectWithEntry(WorldObject* source, uint32 entry, float maxSearchRange)
{
    return source->FindNearestGameObject(entry, maxSearchRange);
}

void GetCreatureListWithEntryInGrid(std::list<Creature*>& list, WorldObject* source, uint32 entry, float maxSearchRange)
{
    source->GetCreatureListWithEntryInGrid(list, entry, maxSearchRange);
}

void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& list, WorldObject* source, uint32 entry, float maxSearchRange)
{
    source->GetGameObjectListWithEntryInGrid(list, entry, maxSearchRange);
}

void GetPlayerListInGrid(std::list<Player*>& list, WorldObject* source, float maxSearchRange)
{
    source->GetPlayerListInGrid(list, maxSearchRange);
}

void GetPositionWithDistInOrientation(Unit* pUnit, float dist, float orientation, float& x, float& y)
{
    x = pUnit->GetPositionX() + (dist * cos(orientation));
    y = pUnit->GetPositionY() + (dist * sin(orientation));
}
void GetRandPosFromCenterInDist(float centerX, float centerY, float dist, float& x, float& y)
{
    float randOrientation = frand(0, 2 * M_PI);
    x = centerX + (dist * cos(randOrientation));
    y = centerY + (dist * sin(randOrientation));
}