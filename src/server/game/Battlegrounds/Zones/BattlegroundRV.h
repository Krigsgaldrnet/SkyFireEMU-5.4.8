/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SF_BATTLEGROUNDRV_H
#define SF_BATTLEGROUNDRV_H

#include "Battleground.h"

enum BattlegroundRVObjectTypes
{
    BG_RV_OBJECT_BUFF_1,
    BG_RV_OBJECT_BUFF_2,
    BG_RV_OBJECT_FIRE_1,
    BG_RV_OBJECT_FIRE_2,
    BG_RV_OBJECT_FIREDOOR_1,
    BG_RV_OBJECT_FIREDOOR_2,

    BG_RV_OBJECT_PILAR_1,
    BG_RV_OBJECT_PILAR_3,
    BG_RV_OBJECT_GEAR_1,
    BG_RV_OBJECT_GEAR_2,

    BG_RV_OBJECT_PILAR_2,
    BG_RV_OBJECT_PILAR_4,
    BG_RV_OBJECT_PULLEY_1,
    BG_RV_OBJECT_PULLEY_2,

    BG_RV_OBJECT_PILAR_COLLISION_1,
    BG_RV_OBJECT_PILAR_COLLISION_2,
    BG_RV_OBJECT_PILAR_COLLISION_3,
    BG_RV_OBJECT_PILAR_COLLISION_4,

    BG_RV_OBJECT_ELEVATOR_1,
    BG_RV_OBJECT_ELEVATOR_2,
    BG_RV_OBJECT_MAX
};

enum BattlegroundRVObjects
{
    BG_RV_OBJECT_TYPE_BUFF_1 = 184663,
    BG_RV_OBJECT_TYPE_BUFF_2 = 184664,
    BG_RV_OBJECT_TYPE_FIRE_1 = 192704,
    BG_RV_OBJECT_TYPE_FIRE_2 = 192705,

    BG_RV_OBJECT_TYPE_FIREDOOR_2 = 192387,
    BG_RV_OBJECT_TYPE_FIREDOOR_1 = 192388,
    BG_RV_OBJECT_TYPE_PULLEY_1 = 192389,
    BG_RV_OBJECT_TYPE_PULLEY_2 = 192390,
    BG_RV_OBJECT_TYPE_GEAR_1 = 192393,
    BG_RV_OBJECT_TYPE_GEAR_2 = 192394,
    BG_RV_OBJECT_TYPE_ELEVATOR_1 = 194582,
    BG_RV_OBJECT_TYPE_ELEVATOR_2 = 194586,

    BG_RV_OBJECT_TYPE_PILAR_COLLISION_1 = 194580, // axe
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_2 = 194579, // arena
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_3 = 194581, // lightning
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_4 = 194578, // ivory

    BG_RV_OBJECT_TYPE_PILAR_1 = 194583, // axe
    BG_RV_OBJECT_TYPE_PILAR_2 = 194584, // arena
    BG_RV_OBJECT_TYPE_PILAR_3 = 194585, // lightning
    BG_RV_OBJECT_TYPE_PILAR_4 = 194587  // ivory
};

enum BattlegroundRVData
{
    BG_RV_STATE_OPEN_FENCES,
    BG_RV_STATE_SWITCH_PILLARS,
    BG_RV_STATE_CLOSE_FIRE,

    BG_RV_PILLAR_SWITCH_TIMER = 25000,
    BG_RV_FIRE_TO_PILLAR_TIMER = 20000,
    BG_RV_CLOSE_FIRE_TIMER = 5000,
    BG_RV_FIRST_TIMER = 20133,
    BG_RV_WORLD_STATE_A = 0xe10,
    BG_RV_WORLD_STATE_H = 0xe11,
    BG_RV_WORLD_STATE = 0xe1a
};

class BattlegroundRV : public Battleground
{
public:
    BattlegroundRV();
    ~BattlegroundRV();

    /* inherited from BattlegroundClass */
    void AddPlayer(Player* player) OVERRIDE;
    void StartingEventCloseDoors() OVERRIDE;
    void StartingEventOpenDoors() OVERRIDE;
    void Reset() OVERRIDE;
    void FillInitialWorldStates(WorldStateBuilder& builder) OVERRIDE;

    void RemovePlayer(Player* player, uint64 guid, uint32 team) OVERRIDE;
    void HandleAreaTrigger(Player* Source, uint32 Trigger) OVERRIDE;
    bool SetupBattleground() OVERRIDE;
    void HandleKillPlayer(Player* player, Player* killer) OVERRIDE;

private:
    uint32 Timer;
    uint32 State;
    bool   PillarCollision;

    void PostUpdateImpl(uint32 diff) OVERRIDE;

protected:
    uint32 getTimer() const { return Timer; }
    void setTimer(uint32 timer) { Timer = timer; }

    uint32 getState() const { return State; }
    void setState(uint32 state) { State = state; }
    void TogglePillarCollision();
    bool GetPillarCollision() const { return PillarCollision; }
    void SetPillarCollision(bool apply) { PillarCollision = apply; }
};
#endif
