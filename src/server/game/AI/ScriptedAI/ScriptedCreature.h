/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SCRIPTEDCREATURE_H_
#define SCRIPTEDCREATURE_H_

#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "InstanceScript.h"

#define CAST_AI(a, b)   (dynamic_cast<a*>(b))

class InstanceScript;

class SummonList
{
public:
    typedef std::list<uint64> StorageType;
    typedef StorageType::iterator iterator;
    typedef StorageType::const_iterator const_iterator;
    typedef StorageType::size_type size_type;
    typedef StorageType::value_type value_type;

    explicit SummonList(Creature* creature)
        : me(creature)
    { }

    // And here we see a problem of original inheritance approach. People started
    // to exploit presence of std::list members, so I have to provide wrappers

    iterator begin()
    {
        return storage_.begin();
    }

    const_iterator begin() const
    {
        return storage_.begin();
    }

    iterator end()
    {
        return storage_.end();
    }

    const_iterator end() const
    {
        return storage_.end();
    }

    iterator erase(iterator i)
    {
        return storage_.erase(i);
    }

    bool empty() const
    {
        return storage_.empty();
    }

    size_type size() const
    {
        return storage_.size();
    }

    void Summon(Creature const* summon)
    {
        storage_.push_back(summon->GetGUID());
    }
    void Despawn(Creature const* summon)
    {
        storage_.remove(summon->GetGUID());
    }
    void DespawnEntry(uint32 entry);
    void DespawnAll();

    template <typename T>
    void DespawnIf(T const& predicate)
    {
        storage_.remove_if(predicate);
    }

    template <class Predicate>
    void DoAction(int32 info, Predicate& predicate, uint16 max = 0)
    {
        // We need to use a copy of SummonList here, otherwise original SummonList would be modified
        StorageType listCopy = storage_;
        Skyfire::Containers::RandomResizeList<uint64, Predicate>(listCopy, predicate, max);
        for (StorageType::iterator i = listCopy.begin(); i != listCopy.end();)
        {
            Creature* summon = Unit::GetCreature(*me, *i++);
            if (summon && summon->IsAIEnabled)
                summon->AI()->DoAction(info);
        }
    }

    void DoZoneInCombat(uint32 entry = 0);
    void RemoveNotExisting();
    bool HasEntry(uint32 entry) const;

private:
    Creature* me;
    StorageType storage_;
};

class EntryCheckPredicate
{
public:
    EntryCheckPredicate(uint32 entry) : _entry(entry)
    { }
    bool operator()(uint64 guid)
    {
        return GUID_ENPART(guid) == _entry;
    }

private:
    uint32 _entry;
};

class DummyEntryCheckPredicate
{
public:
    bool operator()(uint64)
    {
        return true;
    }
};

struct ScriptedAI : public CreatureAI
{
    explicit ScriptedAI(Creature* creature);
    virtual ~ScriptedAI()
    { }

    // *************
    //CreatureAI Functions
    // *************

    void AttackStartNoMove(Unit* target);

    // Called at any Damage from any attacker (before damage apply)
    void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) OVERRIDE { }

    //Called at World update tick
    void UpdateAI(uint32 diff) OVERRIDE;

    void ExecuteEvent(uint32 /*eventId*/) OVERRIDE { }

    //Called at creature death
    void JustDied(Unit* /*killer*/) OVERRIDE { }

    //Called at creature killing another unit
    void KilledUnit(Unit* /*victim*/) OVERRIDE { }

    // Called when the creature summon successfully other creature
    void JustSummoned(Creature* /*summon*/) OVERRIDE { }

    // Called when a summoned creature is despawned
    void SummonedCreatureDespawn(Creature* /*summon*/) OVERRIDE { }

    // Called when hit by a spell
    void SpellHit(Unit* /*caster*/, SpellInfo const* /*spell*/) OVERRIDE { }

    // Called when spell hits a target
    void SpellHitTarget(Unit* /*target*/, SpellInfo const* /*spell*/) OVERRIDE { }

    //Called at waypoint reached or PointMovement end
    void MovementInform(uint32 /*type*/, uint32 /*id*/) OVERRIDE { }

    // Called when AI is temporarily replaced or put back when possess is applied or removed
    void OnPossess(bool /*apply*/)
    { }

    // *************
    // Variables
    // *************

    //Pointer to creature we are manipulating
    Creature* me;

    //For fleeing
    bool IsFleeing;

    // *************
    //Pure virtual functions
    // *************

    //Called at creature reset either by death or evade
    void Reset() OVERRIDE { }

    //Called at creature aggro either by MoveInLOS or Attack Start
    void EnterCombat(Unit* /*victim*/) OVERRIDE { }

    // Called before EnterCombat even before the creature is in combat.
    void AttackStart(Unit* /*target*/) OVERRIDE;

    // *************
    //AI Helper Functions
    // *************

    //Start movement toward victim
    void DoStartMovement(Unit* target, float distance = 0.0f, float angle = 0.0f);

    //Start no movement on victim
    void DoStartNoMovement(Unit* target);

    //Stop attack of current victim
    void DoStopAttack();

    //Cast spell by spell info
    void DoCastSpell(Unit* target, SpellInfo const* spellInfo, bool triggered = false);

    //Plays a sound to all nearby players
    void DoPlaySoundToSet(WorldObject* source, uint32 soundId);

    //Drops all threat to 0%. Does not remove players from the threat list
    void DoResetThreat();

    float DoGetThreat(Unit* unit);
    void DoModifyThreatPercent(Unit* unit, int32 pct);

    void DoTeleportTo(float x, float y, float z, uint32 time = 0);
    void DoTeleportTo(float const pos[4]);

    //Teleports a player without dropping threat (only teleports to same map)
    void DoTeleportPlayer(Unit* unit, float x, float y, float z, float o);
    void DoTeleportAll(float x, float y, float z, float o);

    //Returns friendly unit with the most amount of hp missing from max hp
    Unit* DoSelectLowestHpFriendly(float range, uint32 minHPDiff = 1);

    //Returns a list of friendly CC'd units within range
    std::list<Creature*> DoFindFriendlyCC(float range);

    //Returns a list of all friendly units missing a specific buff within range
    std::list<Creature*> DoFindFriendlyMissingBuff(float range, uint32 spellId);

    //Return a player with at least minimumRange from me
    Player* GetPlayerAtMinimumRange(float minRange);

    //Spawns a creature relative to me
    Creature* DoSpawnCreature(uint32 entry, float offsetX, float offsetY, float offsetZ, float angle, TempSummonType type, uint32 despawntime);

    bool HealthBelowPct(uint32 pct) const
    {
        return me->HealthBelowPct(pct);
    }
    bool HealthAbovePct(uint32 pct) const
    {
        return me->HealthAbovePct(pct);
    }

    //Returns spells that meet the specified criteria from the creatures spell list
    SpellInfo const* SelectSpell(Unit* target, uint32 school, uint32 mechanic, SelectTargetType targets, uint32 powerCostMin, uint32 powerCostMax, float rangeMin, float rangeMax, SelectEffect effect);

    void SetEquipmentSlots(bool loadDefault, int32 mainHand = EQUIP_NO_CHANGE, int32 offHand = EQUIP_NO_CHANGE, int32 ranged = EQUIP_NO_CHANGE);

    // Used to control if MoveChase() is to be used or not in AttackStart(). Some creatures does not chase victims
    // NOTE: If you use SetCombatMovement while the creature is in combat, it will do NOTHING - This only affects AttackStart
    //       You should make the necessary to make it happen so.
    //       Remember that if you modified _isCombatMovementAllowed (e.g: using SetCombatMovement) it will not be reset at Reset().
    //       It will keep the last value you set.
    void SetCombatMovement(bool allowMovement);
    bool IsCombatMovementAllowed() const
    {
        return _isCombatMovementAllowed;
    }

    bool EnterEvadeIfOutOfCombatArea(uint32 const diff);

    // return true for heroic mode. i.e.
    //   - for dungeon in mode 10-heroic,
    //   - for raid in mode 10-Heroic
    //   - for raid in mode 25-heroic
    // DO NOT USE to check raid in mode 25-normal.
    bool IsHeroic() const
    {
        return _isHeroic;
    }

    // return the dungeon or raid difficulty
    DifficultyID GetDifficulty() const
    {
        return _difficulty;
    }

    // return true for 25 man or 25 man heroic mode
    bool Is25ManRaid() const;

    template<class T> inline
        const T& DUNGEON_MODE(const T& normal5, const T& heroic5) const
    {
        switch (_difficulty)
        {
            case DIFFICULTY_NORMAL:
                return normal5;
            case DIFFICULTY_HEROIC:
                return heroic5;
            default:
                break;
        }

        return heroic5;
    }

    template<class T> inline
        const T& RAID_MODE(const T& normal10, const T& normal25) const
    {
        switch (_difficulty)
        {
            case DIFFICULTY_10MAN_NORMAL:
                return normal10;
            case DIFFICULTY_25MAN_NORMAL:
                return normal25;
            default:
                break;
        }

        return normal25;
    }

    template<class T> inline
        const T& RAID_MODE(const T& normal10, const T& normal25, const T& flex, const T& lfr) const
    {
        switch (_difficulty)
        {
            case DIFFICULTY_10MAN_NORMAL:
                return normal10;
            case DIFFICULTY_25MAN_NORMAL:
                return normal25;
            case DIFFICULTY_FLEX:
                return flex;
            case DIFFICULTY_25MAN_LFR:
                return lfr;
            default:
                break;
        }

        return normal25;
    }

    template<class T> inline
        const T& RAID_MODE(const T& normal10, const T& normal25, const T& heroic10, const T& heroic25, const T& flex, const T& lfr) const
    {
        switch (_difficulty)
        {
            case DIFFICULTY_10MAN_NORMAL:
                return normal10;
            case DIFFICULTY_25MAN_NORMAL:
                return normal25;
            case DIFFICULTY_10MAN_HEROIC:
                return heroic10;
            case DIFFICULTY_25MAN_HEROIC:
                return heroic25;
            case DIFFICULTY_FLEX:
                return flex;
            case DIFFICULTY_25MAN_LFR:
                return lfr;
            default:
                break;
        }

        return heroic25;
    }

private:
    DifficultyID _difficulty;
    uint32 _evadeCheckCooldown;
    bool _isCombatMovementAllowed;
    bool _isHeroic;
};

class BossAI : public ScriptedAI
{
public:
    BossAI(Creature* creature, uint32 bossId);
    virtual ~BossAI()
    { }

    InstanceScript* const instance;
    BossBoundaryMap const* GetBoundary() const
    {
        return _boundary;
    }

    void JustSummoned(Creature* summon) OVERRIDE;
    void SummonedCreatureDespawn(Creature* summon) OVERRIDE;

    void UpdateAI(uint32 diff) OVERRIDE;

    // Hook used to execute events scheduled into EventMap without the need
    // to override UpdateAI
    // note: You must re-schedule the event within this method if the event
    // is supposed to run more than once
    void ExecuteEvent(uint32 /*eventId*/) OVERRIDE { }

    void Reset() OVERRIDE
    {
        _Reset();
    }
    void EnterCombat(Unit* /*who*/) OVERRIDE
    {
        _EnterCombat();
    }
    void JustDied(Unit* /*killer*/) OVERRIDE
    {
        _JustDied();
    }
    void JustReachedHome() OVERRIDE
    {
        _JustReachedHome();
    }

protected:
    void _Reset();
    void _EnterCombat();
    void _JustDied();
    void _JustReachedHome()
    {
        me->setActive(false);
    }
    void _DespawnAtEvade();

    bool CheckInRoom()
    {
        if (CheckBoundary(me))
            return true;

        EnterEvadeMode();
        return false;
    }

    bool CheckBoundary(Unit* who);
    void TeleportCheaters();

    EventMap events;
    SummonList summons;

private:
    BossBoundaryMap const* const _boundary;
    uint32 const _bossId;
};

class WorldBossAI : public ScriptedAI
{
public:
    WorldBossAI(Creature* creature);
    virtual ~WorldBossAI()
    { }

    void JustSummoned(Creature* summon) OVERRIDE;
    void SummonedCreatureDespawn(Creature* summon) OVERRIDE;

    void UpdateAI(uint32 diff) OVERRIDE;

    // Hook used to execute events scheduled into EventMap without the need
    // to override UpdateAI
    // note: You must re-schedule the event within this method if the event
    // is supposed to run more than once
    void ExecuteEvent(uint32 /*eventId*/) OVERRIDE { }

    void Reset() OVERRIDE
    {
        _Reset();
    }
    void EnterCombat(Unit* /*who*/) OVERRIDE
    {
        _EnterCombat();
    }
    void JustDied(Unit* /*killer*/) OVERRIDE
    {
        _JustDied();
    }

protected:
    void _Reset();
    void _EnterCombat();
    void _JustDied();

    EventMap events;
    SummonList summons;
};

// SD2 grid searchers.
Creature* GetClosestCreatureWithEntry(WorldObject* source, uint32 entry, float maxSearchRange, bool alive = true);
GameObject* GetClosestGameObjectWithEntry(WorldObject* source, uint32 entry, float maxSearchRange);
void GetCreatureListWithEntryInGrid(std::list<Creature*>& list, WorldObject* source, uint32 entry, float maxSearchRange);
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& list, WorldObject* source, uint32 entry, float maxSearchRange);
void GetPlayerListInGrid(std::list<Player*>& list, WorldObject* source, float maxSearchRange);
void GetPositionWithDistInOrientation(Unit* pUnit, float dist, float orientation, float& x, float& y);
void GetRandPosFromCenterInDist(float centerX, float centerY, float dist, float& x, float& y);
#endif // SCRIPTEDCREATURE_H_
