/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SKYFIRE_SMARTAI_H
#define SKYFIRE_SMARTAI_H

#include "Common.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "Spell.h"
#include "Unit.h"

#include "GameObjectAI.h"
#include "SmartScript.h"

enum SmartEscortState
{
    SMART_ESCORT_NONE = 0x000,                        //nothing in progress
    SMART_ESCORT_ESCORTING = 0x001,                        //escort is in progress
    SMART_ESCORT_RETURNING = 0x002,                        //escort is returning after being in combat
    SMART_ESCORT_PAUSED = 0x004                         //will not proceed with waypoints before state is removed
};

enum SmartEscortVars
{
    SMART_ESCORT_MAX_PLAYER_DIST = 50,
    SMART_MAX_AID_DIST = SMART_ESCORT_MAX_PLAYER_DIST / 2
};

class SmartAI : public CreatureAI
{
public:
    ~SmartAI() { }
    explicit SmartAI(Creature* c);

    // Start moving to the desired MovePoint
    void StartPath(bool run = false, uint32 path = 0, bool repeat = false, Unit* invoker = NULL);
    bool LoadPath(uint32 entry);
    void PausePath(uint32 delay, bool forced = false);
    void StopPath(uint32 DespawnTime = 0, uint32 quest = 0, bool fail = false);
    void EndPath(bool fail = false);
    void ResumePath();
    WayPoint* GetNextWayPoint();
    bool HasEscortState(uint32 uiEscortState) const { return (mEscortState & uiEscortState); }
    void AddEscortState(uint32 uiEscortState) { mEscortState |= uiEscortState; }
    void RemoveEscortState(uint32 uiEscortState) { mEscortState &= ~uiEscortState; }
    void SetAutoAttack(bool on) { mCanAutoAttack = on; }
    void SetCombatMove(bool on);
    bool CanCombatMove() const { return mCanCombatMove; }
    void SetFollow(Unit* target, float dist = 0.0f, float angle = 0.0f, uint32 credit = 0, uint32 end = 0, uint32 creditType = 0);

    void SetScript9(SmartScriptHolder& e, uint32 entry, Unit* invoker);
    SmartScript* GetScript() { return &mScript; }
    bool IsEscortInvokerInRange();

    // Called when creature is spawned or respawned
    void JustRespawned() OVERRIDE;

    // Called after InitializeAI(), EnterEvadeMode() for resetting variables
    void Reset() OVERRIDE;

    // Called at reaching home after evade
    void JustReachedHome() OVERRIDE;

    // Called for reaction at enter to combat if not in combat yet (enemy can be NULL)
    void EnterCombat(Unit* enemy) OVERRIDE;

    // Called for reaction at stopping attack at no attackers or targets
    void EnterEvadeMode() OVERRIDE;

    // Called when the creature is killed
    void JustDied(Unit* killer) OVERRIDE;

    // Called when the creature kills a unit
    void KilledUnit(Unit* victim) OVERRIDE;

    // Called when the creature summon successfully other creature
    void JustSummoned(Creature* creature) OVERRIDE;

    // Tell creature to attack and follow the victim
    void AttackStart(Unit* who) OVERRIDE;

    // Called if IsVisible(Unit* who) is true at each *who move, reaction at visibility zone enter
    void MoveInLineOfSight(Unit* who) OVERRIDE;

    // Called when hit by a spell
    void SpellHit(Unit* unit, const SpellInfo* spellInfo) OVERRIDE;

    // Called when spell hits a target
    void SpellHitTarget(Unit* target, const SpellInfo* spellInfo) OVERRIDE;

    // Called at any Damage from any attacker (before damage apply)
    void DamageTaken(Unit* doneBy, uint32& damage) OVERRIDE;

    // Called when the creature receives heal
    void HealReceived(Unit* doneBy, uint32& addhealth) OVERRIDE;

    // Called at World update tick
    void UpdateAI(uint32 diff) OVERRIDE;

    // Called at text emote receive from player
    void ReceiveEmote(Player* player, uint32 textEmote) OVERRIDE;

    // Called at waypoint reached or point movement finished
    void MovementInform(uint32 MovementType, uint32 Data) OVERRIDE;

    // Called when creature is summoned by another unit
    void IsSummonedBy(Unit* summoner) OVERRIDE;

    // Called at any Damage to any victim (before damage apply)
    void DamageDealt(Unit* doneTo, uint32& damage, DamageEffectType /*damagetype*/) OVERRIDE;

    // Called when a summoned creature dissapears (UnSommoned)
    void SummonedCreatureDespawn(Creature* unit) OVERRIDE;

    // called when the corpse of this creature gets removed
    void CorpseRemoved(uint32& respawnDelay) OVERRIDE;

    // Called at World update tick if creature is charmed
    void UpdateAIWhileCharmed(const uint32 diff);

    // Called when a Player/Creature enters the creature (vehicle)
    void PassengerBoarded(Unit* who, int8 seatId, bool apply) OVERRIDE;

    // Called when gets initialized, when creature is added to world
    void InitializeAI() OVERRIDE;

    // Called when creature gets charmed by another unit
    void OnCharmed(bool apply) OVERRIDE;

    // Called when victim is in line of sight
    bool CanAIAttack(const Unit* who) const OVERRIDE;

    // Used in scripts to share variables
    void DoAction(int32 param = 0) OVERRIDE;

    // Used in scripts to share variables
    uint32 GetData(uint32 id = 0) const OVERRIDE;

    // Used in scripts to share variables
    void SetData(uint32 id, uint32 value) OVERRIDE;

    // Used in scripts to share variables
    void SetGUID(uint64 guid, int32 id = 0) OVERRIDE;

    // Used in scripts to share variables
    uint64 GetGUID(int32 id = 0) const OVERRIDE;

    //core related
    static int Permissible(const Creature*);

    // Called at movepoint reached
    void MovepointReached(uint32 id);

    // Makes the creature run/walk
    void SetRun(bool run = true);

    void SetFly(bool fly = true);

    void SetSwim(bool swim = true);

    void SetInvincibilityHpLevel(uint32 level) { mInvincibilityHpLevel = level; }

    void sGossipHello(Player* player) OVERRIDE;
    void sGossipSelect(Player* player, uint32 sender, uint32 action) OVERRIDE;
    void sGossipSelectCode(Player* player, uint32 sender, uint32 action, const char* code) OVERRIDE;
    void sQuestAccept(Player* player, Quest const* quest) OVERRIDE;
    //void sQuestSelect(Player* player, Quest const* quest) OVERRIDE;
    //void sQuestComplete(Player* player, Quest const* quest) OVERRIDE;
    void sQuestReward(Player* player, Quest const* quest, uint32 opt) OVERRIDE;
    bool sOnDummyEffect(Unit* caster, uint32 spellId, SpellEffIndex effIndex) OVERRIDE;
    void sOnGameEvent(bool start, uint16 eventId) OVERRIDE;

    uint32 mEscortQuestID;

    void SetDespawnTime(uint32 t)
    {
        mDespawnTime = t;
        mDespawnState = t ? 1 : 0;
    }
    void StartDespawn() { mDespawnState = 2; }

    void RemoveAuras();

    void OnSpellClick(Unit* clicker, bool& result) OVERRIDE;

private:
    uint32 mFollowCreditType;
    uint32 mFollowArrivedTimer;
    uint32 mFollowCredit;
    uint32 mFollowArrivedEntry;
    uint64 mFollowGuid;
    float mFollowDist;
    float mFollowAngle;

    void ReturnToLastOOCPos();
    void UpdatePath(const uint32 diff);
    SmartScript mScript;
    WPPath* mWayPoints;
    uint32 mEscortState;
    uint32 mCurrentWPID;
    uint32 mLastWPIDReached;
    bool mWPReached;
    uint32 mWPPauseTimer;
    WayPoint* mLastWP;
    Position mLastOOCPos;//set on enter combat
    uint32 GetWPCount() { return mWayPoints ? mWayPoints->size() : 0; }
    bool mCanRepeatPath;
    bool mRun;
    bool mCanAutoAttack;
    bool mCanCombatMove;
    bool mForcedPaused;
    uint32 mInvincibilityHpLevel;
    bool AssistPlayerInCombat(Unit* who);

    uint32 mDespawnTime;
    uint32 mDespawnState;
    void UpdateDespawn(const uint32 diff);
    uint32 mEscortInvokerCheckTimer;
};

class SmartGameObjectAI : public GameObjectAI
{
public:
    SmartGameObjectAI(GameObject* g) : GameObjectAI(g), go(g) { }
    ~SmartGameObjectAI() { }

    void UpdateAI(uint32 diff) OVERRIDE;
    void InitializeAI() OVERRIDE;
    void Reset() OVERRIDE;
    SmartScript* GetScript() { return &mScript; }
    static int Permissible(const GameObject* g);

    bool GossipHello(Player* player) OVERRIDE;
    bool GossipSelect(Player* player, uint32 sender, uint32 action) OVERRIDE;
    bool GossipSelectCode(Player* /*player*/, uint32 /*sender*/, uint32 /*action*/, const char* /*code*/) OVERRIDE;
    bool QuestAccept(Player* player, Quest const* quest) OVERRIDE;
    bool QuestReward(Player* player, Quest const* quest, uint32 opt) OVERRIDE;
    uint32 GetDialogStatus(Player* /*player*/) OVERRIDE;
    void Destroyed(Player* player, uint32 eventId) OVERRIDE;
    void SetData(uint32 id, uint32 value) OVERRIDE;
    void SetScript9(SmartScriptHolder& e, uint32 entry, Unit* invoker);
    void OnGameEvent(bool start, uint16 eventId) OVERRIDE;
    void OnStateChanged(uint32 state, Unit* unit) OVERRIDE;
    void EventInform(uint32 eventId) OVERRIDE;

protected:
    GameObject* const go;
    SmartScript mScript;
};
#endif
