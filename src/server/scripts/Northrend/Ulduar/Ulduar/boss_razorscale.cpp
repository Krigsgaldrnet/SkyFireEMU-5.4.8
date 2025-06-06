/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/// @todo Harpoon chain from 62505 should not get removed when other chain is applied

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "ulduar.h"
#include "SpellInfo.h"
#include "Player.h"

enum Says
{
    // Expedition Commander
    SAY_INTRO                                    = 0,
    SAY_GROUND_PHASE                             = 1,
    SAY_AGGRO_2                                  = 2,

    // Expedition Engineer
    SAY_AGGRO_1                                  = 0,
    SAY_AGGRO_3                                  = 1,
    SAY_TURRETS                                  = 2, // unused

    // Razorscale Controller
    EMOTE_HARPOON                                = 0,

    // Razorscale
    EMOTE_PERMA                                  = 0,
    EMOTE_BREATH                                 = 1
};

enum Spells
{
    SPELL_FLAMEBUFFET                            = 64016,
    SPELL_FIREBALL                               = 62796,
    SPELL_FLAME_GROUND                           = 64734,
    SPELL_WINGBUFFET                             = 62666,
    SPELL_FLAMEBREATH                            = 63317,
    SPELL_FUSEARMOR                              = 64771,
    SPELL_FLAMED                                 = 62696,
    SPELL_STUN                                   = 9032,
    SPELL_BERSERK                                = 47008,
    // Additonal Spells
    // Devouring Flame Spells
    SPELL_DEVOURING_FLAME                        = 63308,
    SPELL_DEVOURING_FLAME_DAMAGE                 = 64704,
    SPELL_DEVOURING_FLAME_TRIGGER                = 64709,
    // HarpoonSpells
    SPELL_HARPOON_TRIGGER                        = 62505,
    SPELL_HARPOON_SHOT_1                         = 63658,
    SPELL_HARPOON_SHOT_2                         = 63657,
    SPELL_HARPOON_SHOT_3                         = 63659,
    SPELL_HARPOON_SHOT_4                         = 63524,
    // MoleMachine Spells
    SPELL_SUMMON_MOLE_MACHINE                    = 62899,
    SPELL_SUMMON_IRON_DWARVES                    = 63116,
    SPELL_SUMMON_IRON_DWARVES_2                  = 63114,
    SPELL_SUMMON_IRON_DWARVE_GUARDIAN            = 62926,
    SPELL_SUMMON_IRON_DWARVE_WATCHER             = 63135,
};

enum NPC
{
    NPC_DARK_RUNE_GUARDIAN                       = 33388,
    NPC_DARK_RUNE_SENTINEL                       = 33846,
    NPC_DARK_RUNE_WATCHER                        = 33453,
    MOLE_MACHINE_TRIGGER                         = 33245,
    NPC_COMMANDER                                = 33210,
    NPC_ENGINEER                                 = 33287,
    NPC_DEFENDER                                 = 33816,
};

enum DarkRuneSpells
{
    // Dark Rune Watcher
    SPELL_CHAIN_LIGHTNING                        = 64758,
    SPELL_LIGHTNING_BOLT                         = 63809,
    // Dark Rune Guardian
    SPELL_STORMSTRIKE                            = 64757,
    // Dark Rune Sentinel
    SPELL_BATTLE_SHOUT                           = 46763,
    SPELL_HEROIC_STRIKE                          = 45026,
    SPELL_WHIRLWIND                              = 63807,
};

enum Actions
{
    ACTION_EVENT_START                           = 1,
    ACTION_GROUND_PHASE                          = 2,
    ACTION_HARPOON_BUILD                         = 3,
    ACTION_PLACE_BROKEN_HARPOON                  = 4,
    ACTION_COMMANDER_RESET                       = 7,
};

enum Phases
{
    PHASE_PERMAGROUND                            = 1,
    PHASE_GROUND                                 = 2,
    PHASE_FLIGHT                                 = 3,
};

enum Events
{
    EVENT_BERSERK                                = 1,
    EVENT_BREATH                                 = 2,
    EVENT_BUFFET                                 = 3,
    EVENT_FIREBALL                               = 5,
    EVENT_FLIGHT                                 = 6,
    EVENT_DEVOURING                              = 7,
    EVENT_FLAME                                  = 8,
    EVENT_LAND                                   = 9,
    EVENT_GROUND                                 = 10,
    EVENT_FUSE                                   = 11,
    EVENT_SUMMON                                 = 12,
    // Razorscale Controller
    EVENT_BUILD_HARPOON_1                        = 13,
    EVENT_BUILD_HARPOON_2                        = 14,
    EVENT_BUILD_HARPOON_3                        = 15,
    EVENT_BUILD_HARPOON_4                        = 16,
};

#define GROUND_Z                                 391.517f
#define GOSSIP_ITEM_1                            "Activate Harpoons!"

enum Misc
{
    DATA_QUICK_SHAVE                             = 29192921, // 2919, 2921 are achievement IDs
    DATA_IRON_DWARF_MEDIUM_RARE                  = 29232924
};

const Position PosEngRepair[4] =
{
    { 590.442f, -130.550f, GROUND_Z, 4.789f },
    { 574.850f, -133.687f, GROUND_Z, 4.252f },
    { 606.567f, -143.369f, GROUND_Z, 4.434f },
    { 560.609f, -142.967f, GROUND_Z, 5.074f },
};

const Position PosDefSpawn[4] =
{
    { 600.75f, -104.850f, GROUND_Z, 0 },
    { 596.38f, -110.262f, GROUND_Z, 0 },
    { 566.47f, -103.633f, GROUND_Z, 0 },
    { 570.41f, -108.791f, GROUND_Z, 0 },
};

const Position PosDefCombat[4] =
{
    { 614.975f, -155.138f, GROUND_Z, 4.154f },
    { 609.814f, -204.968f, GROUND_Z, 5.385f },
    { 563.531f, -201.557f, GROUND_Z, 4.108f },
    { 560.231f, -153.677f, GROUND_Z, 5.403f },
};

const Position PosHarpoon[4] =
{
    { 571.901f, -136.554f, GROUND_Z, 0 },
    { 589.450f, -134.888f, GROUND_Z, 0 },
    { 559.119f, -140.505f, GROUND_Z, 0 },
    { 606.229f, -136.721f, GROUND_Z, 0 },
};

const Position RazorFlight = { 588.050f, -251.191f, 470.536f, 1.498f };
const Position RazorGround = { 586.966f, -175.534f, GROUND_Z, 4.682f };
const Position PosEngSpawn = { 591.951f, -95.9680f, GROUND_Z, 0.000f };

class boss_razorscale_controller : public CreatureScript
{
    public:
        boss_razorscale_controller() : CreatureScript("boss_razorscale_controller") { }

        struct boss_razorscale_controllerAI : public BossAI
        {
            boss_razorscale_controllerAI(Creature* creature) : BossAI(creature, DATA_RAZORSCALE_CONTROL)
            {
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
            }

            void Reset() OVERRIDE
            {
                _Reset();
                me->SetReactState(REACT_PASSIVE);
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell) OVERRIDE
            {
                switch (spell->Id)
                {
                    case SPELL_FLAMED:
                        if (GameObject* Harpoon1 = ObjectAccessor::GetGameObject(*me, instance->GetData64(GO_RAZOR_HARPOON_1)))
                            Harpoon1->RemoveFromWorld();
                        if (GameObject* Harpoon2 = ObjectAccessor::GetGameObject(*me, instance->GetData64(GO_RAZOR_HARPOON_2)))
                            Harpoon2->RemoveFromWorld();
                        if (GameObject* Harpoon3 = ObjectAccessor::GetGameObject(*me, instance->GetData64(GO_RAZOR_HARPOON_3)))
                            Harpoon3->RemoveFromWorld();
                        if (GameObject* Harpoon4 = ObjectAccessor::GetGameObject(*me, instance->GetData64(GO_RAZOR_HARPOON_4)))
                            Harpoon4->RemoveFromWorld();
                        me->AI()->DoAction(ACTION_HARPOON_BUILD);
                        me->AI()->DoAction(ACTION_PLACE_BROKEN_HARPOON);
                        break;
                    case SPELL_HARPOON_SHOT_1:
                    case SPELL_HARPOON_SHOT_2:
                    case SPELL_HARPOON_SHOT_3:
                    case SPELL_HARPOON_SHOT_4:
                        DoCast(SPELL_HARPOON_TRIGGER);
                        break;
                }
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
            }

            void DoAction(int32 action) OVERRIDE
            {
                if (instance->GetBossState(BOSS_RAZORSCALE) != IN_PROGRESS)
                    return;

                switch (action)
                {
                    case ACTION_HARPOON_BUILD:
                        events.ScheduleEvent(EVENT_BUILD_HARPOON_1, 50000);
                        if (me->GetMap()->GetSpawnMode() == DIFFICULTY_25MAN_NORMAL)
                            events.ScheduleEvent(EVENT_BUILD_HARPOON_3, 90000);
                        break;
                    case ACTION_PLACE_BROKEN_HARPOON:
                        for (uint8 n = 0; n < RAID_MODE(2, 4); n++)
                            me->SummonGameObject(GO_RAZOR_BROKEN_HARPOON, PosHarpoon[n].GetPositionX(), PosHarpoon[n].GetPositionY(), PosHarpoon[n].GetPositionZ(), 2.286f, 0, 0, 0, 0, 180000);
                        break;
                }
            }

            void UpdateAI(uint32 Diff) OVERRIDE
            {
                events.Update(Diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BUILD_HARPOON_1:
                            Talk(EMOTE_HARPOON);
                            if (GameObject* Harpoon = me->SummonGameObject(GO_RAZOR_HARPOON_1, PosHarpoon[0].GetPositionX(), PosHarpoon[0].GetPositionY(), PosHarpoon[0].GetPositionZ(), 4.790f, 0.0f, 0.0f, 0.0f, 0.0f, uint32(me->GetRespawnTime())))
                            {
                                if (GameObject* BrokenHarpoon = Harpoon->FindNearestGameObject(GO_RAZOR_BROKEN_HARPOON, 5.0f)) //only nearest broken harpoon
                                    BrokenHarpoon->RemoveFromWorld();
                                events.ScheduleEvent(EVENT_BUILD_HARPOON_2, 20000);
                                events.CancelEvent(EVENT_BUILD_HARPOON_1);
                            }
                            return;
                        case EVENT_BUILD_HARPOON_2:
                            Talk(EMOTE_HARPOON);
                            if (GameObject* Harpoon = me->SummonGameObject(GO_RAZOR_HARPOON_2, PosHarpoon[1].GetPositionX(), PosHarpoon[1].GetPositionY(), PosHarpoon[1].GetPositionZ(), 4.659f, 0, 0, 0, 0, uint32(me->GetRespawnTime())))
                            {
                                if (GameObject* BrokenHarpoon = Harpoon->FindNearestGameObject(GO_RAZOR_BROKEN_HARPOON, 5.0f))
                                    BrokenHarpoon->RemoveFromWorld();
                                events.CancelEvent(EVENT_BUILD_HARPOON_2);
                            }
                            return;
                        case EVENT_BUILD_HARPOON_3:
                            Talk(EMOTE_HARPOON);
                            if (GameObject* Harpoon = me->SummonGameObject(GO_RAZOR_HARPOON_3, PosHarpoon[2].GetPositionX(), PosHarpoon[2].GetPositionY(), PosHarpoon[2].GetPositionZ(), 5.382f, 0, 0, 0, 0, uint32(me->GetRespawnTime())))
                            {
                                if (GameObject* BrokenHarpoon = Harpoon->FindNearestGameObject(GO_RAZOR_BROKEN_HARPOON, 5.0f))
                                    BrokenHarpoon->RemoveFromWorld();
                                events.ScheduleEvent(EVENT_BUILD_HARPOON_4, 20000);
                                events.CancelEvent(EVENT_BUILD_HARPOON_3);
                            }
                            return;
                        case EVENT_BUILD_HARPOON_4:
                            Talk(EMOTE_HARPOON);
                            if (GameObject* Harpoon = me->SummonGameObject(GO_RAZOR_HARPOON_4, PosHarpoon[3].GetPositionX(), PosHarpoon[3].GetPositionY(), PosHarpoon[3].GetPositionZ(), 4.266f, 0, 0, 0, 0, uint32(me->GetRespawnTime())))
                            {
                                if (GameObject* BrokenHarpoon = Harpoon->FindNearestGameObject(GO_RAZOR_BROKEN_HARPOON, 5.0f))
                                    BrokenHarpoon->RemoveFromWorld();
                                events.CancelEvent(EVENT_BUILD_HARPOON_4);
                            }
                            return;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_razorscale_controllerAI(creature);
        }
};

class go_razorscale_harpoon : public GameObjectScript
{
    public:
        go_razorscale_harpoon() : GameObjectScript("go_razorscale_harpoon") { }

        bool OnGossipHello(Player* /*player*/, GameObject* go) OVERRIDE
        {
            InstanceScript* instance = go->GetInstanceScript();
            if (ObjectAccessor::GetCreature(*go, instance ? instance->GetData64(BOSS_RAZORSCALE) : 0))
                go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
            return false;
        }
};

class boss_razorscale : public CreatureScript
{
    public:
        boss_razorscale() : CreatureScript("boss_razorscale") { }

        struct boss_razorscaleAI : public BossAI
        {
            boss_razorscaleAI(Creature* creature) : BossAI(creature, BOSS_RAZORSCALE)
            {
                // Do not let Razorscale be affected by Battle Shout buff
                me->ApplySpellImmune(0, IMMUNITY_ID, (SPELL_BATTLE_SHOUT), true);
            }

            Phases phase;

            uint32 EnrageTimer;
            uint8 FlyCount;
            uint8 HarpoonCounter;
            bool PermaGround;
            bool Enraged;

            void Reset() OVERRIDE
            {
                _Reset();
                me->SetCanFly(true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_PASSIVE);
                PermaGround = false;
                HarpoonCounter = 0;
                if (Creature* commander = ObjectAccessor::GetCreature(*me, instance ? instance->GetData64(DATA_EXPEDITION_COMMANDER) : 0))
                    commander->AI()->DoAction(ACTION_COMMANDER_RESET);
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                _EnterCombat();
                if (Creature* controller = ObjectAccessor::GetCreature(*me, instance ? instance->GetData64(DATA_RAZORSCALE_CONTROL) : 0))
                    controller->AI()->DoAction(ACTION_HARPOON_BUILD);
                me->SetSpeed(MOVE_FLIGHT, 3.0f, true);
                me->SetReactState(REACT_PASSIVE);
                phase = PHASE_GROUND;
                events.SetPhase(PHASE_GROUND);
                FlyCount = 0;
                EnrageTimer = 600000;
                Enraged = false;
                events.ScheduleEvent(EVENT_FLIGHT, 0, 0, PHASE_GROUND);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
                if (Creature* controller = ObjectAccessor::GetCreature(*me, instance ? instance->GetData64(DATA_RAZORSCALE_CONTROL) : 0))
                    controller->AI()->Reset();
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell) OVERRIDE
            {
                if (spell->Id == SPELL_HARPOON_TRIGGER)
                    ++HarpoonCounter;
            }

            void MovementInform(uint32 type, uint32 id) OVERRIDE
            {
                if (type == EFFECT_MOTION_TYPE && id == 1)
                {
                    phase = PHASE_GROUND;
                    events.SetPhase(PHASE_GROUND);
                    events.ScheduleEvent(EVENT_LAND, 0, 0, PHASE_GROUND);
                }
            }

            uint32 GetData(uint32 type) const OVERRIDE
            {
                if (type == DATA_QUICK_SHAVE)
                    if (FlyCount <= 2)
                        return 1;

                return 0;
            }

            void UpdateAI(uint32 Diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(Diff);

                if (HealthBelowPct(50) && !PermaGround)
                    EnterPermaGround();

                if (EnrageTimer <= Diff && !Enraged)
                {
                    DoCast(me, SPELL_BERSERK);
                    Enraged = true;
                }
                else
                    EnrageTimer -= Diff;

                if (HarpoonCounter == RAID_MODE(2, 4))
                {
                    HarpoonCounter = 0;
                    me->GetMotionMaster()->MoveLand(1, RazorGround);
                }

                if (phase == PHASE_GROUND)
                {
                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_FLIGHT:
                                phase = PHASE_FLIGHT;
                                events.SetPhase(PHASE_FLIGHT);
                                me->SetCanFly(true);
                                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                me->SetReactState(REACT_PASSIVE);
                                me->AttackStop();
                                me->GetMotionMaster()->MoveTakeoff(0, RazorFlight);
                                events.ScheduleEvent(EVENT_FIREBALL, 7000, 0, PHASE_FLIGHT);
                                events.ScheduleEvent(EVENT_DEVOURING, 10000, 0, PHASE_FLIGHT);
                                events.ScheduleEvent(EVENT_SUMMON, 5000, 0, PHASE_FLIGHT);
                                ++FlyCount;
                                return;
                            case EVENT_LAND:
                                me->SetCanFly(false);
                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED | UNIT_FLAG_PACIFIED);
                                if (Creature* commander = ObjectAccessor::GetCreature(*me, instance ? instance->GetData64(DATA_EXPEDITION_COMMANDER) : 0))
                                    commander->AI()->DoAction(ACTION_GROUND_PHASE);
                                events.ScheduleEvent(EVENT_BREATH, 30000, 0, PHASE_GROUND);
                                events.ScheduleEvent(EVENT_BUFFET, 33000, 0, PHASE_GROUND);
                                events.ScheduleEvent(EVENT_FLIGHT, 35000, 0, PHASE_GROUND);
                                return;
                            case EVENT_BREATH:
                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED | UNIT_FLAG_PACIFIED);
                                me->RemoveAllAuras();
                                me->SetReactState(REACT_AGGRESSIVE);
                                Talk(EMOTE_BREATH);
                                DoCastAOE(SPELL_FLAMEBREATH);
                                events.CancelEvent(EVENT_BREATH);
                                return;
                            case EVENT_BUFFET:
                                DoCastAOE(SPELL_WINGBUFFET);
                                if (Creature* controller = ObjectAccessor::GetCreature(*me, instance ? instance->GetData64(DATA_RAZORSCALE_CONTROL) : 0))
                                    controller->CastSpell(controller, SPELL_FLAMED, true);
                                events.CancelEvent(EVENT_BUFFET);
                                return;
                        }
                    }
                }
                if (phase == PHASE_PERMAGROUND)
                {
                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_FLAME:
                                DoCastAOE(SPELL_FLAMEBUFFET);
                                events.ScheduleEvent(EVENT_FLAME, 10000, 0, PHASE_PERMAGROUND);
                                return;
                            case EVENT_BREATH:
                                Talk(EMOTE_BREATH);
                                DoCastVictim(SPELL_FLAMEBREATH);
                                events.ScheduleEvent(EVENT_BREATH, 20000, 0, PHASE_PERMAGROUND);
                                return;
                            case EVENT_FIREBALL:
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                                    DoCast(target, SPELL_FIREBALL);
                                events.ScheduleEvent(EVENT_FIREBALL, 3000, 0, PHASE_PERMAGROUND);
                                return;
                            case EVENT_DEVOURING:
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                                    DoCast(target, SPELL_DEVOURING_FLAME);
                                events.ScheduleEvent(EVENT_DEVOURING, 10000, 0, PHASE_PERMAGROUND);
                                return;
                            case EVENT_BUFFET:
                                DoCastAOE(SPELL_WINGBUFFET);
                                events.CancelEvent(EVENT_BUFFET);
                                return;
                            case EVENT_FUSE:
                                DoCastVictim(SPELL_FUSEARMOR);
                                events.ScheduleEvent(EVENT_FUSE, 10000, 0, PHASE_PERMAGROUND);
                                return;
                        }
                    }

                    DoMeleeAttackIfReady();
                }
                else
                {
                    if (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_FIREBALL:
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                                    DoCast(target, SPELL_FIREBALL);
                                events.ScheduleEvent(EVENT_FIREBALL, 3000, 0, PHASE_FLIGHT);
                                return;
                            case EVENT_DEVOURING:
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                                    me->CastSpell(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), SPELL_DEVOURING_FLAME, true);
                                events.ScheduleEvent(EVENT_DEVOURING, 10000, 0, PHASE_FLIGHT);
                                return;
                            case EVENT_SUMMON:
                                SummonMoleMachines();
                                events.ScheduleEvent(EVENT_SUMMON, 45000, 0, PHASE_FLIGHT);
                                return;
                        }
                    }
                }
            }

            void EnterPermaGround()
            {
                Talk(EMOTE_PERMA);
                phase = PHASE_PERMAGROUND;
                events.SetPhase(PHASE_PERMAGROUND);
                me->SetCanFly(false);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED | UNIT_FLAG_PACIFIED);
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveAurasDueToSpell(SPELL_HARPOON_TRIGGER);
                me->SetSpeed(MOVE_FLIGHT, 1.0f, true);
                PermaGround = true;
                DoCastAOE(SPELL_FLAMEBREATH);
                events.ScheduleEvent(EVENT_FLAME, 15000, 0, PHASE_PERMAGROUND);
                events.RescheduleEvent(EVENT_DEVOURING, 15000, 0, PHASE_PERMAGROUND);
                events.RescheduleEvent(EVENT_BREATH, 20000, 0, PHASE_PERMAGROUND);
                events.RescheduleEvent(EVENT_FIREBALL, 3000, 0, PHASE_PERMAGROUND);
                events.RescheduleEvent(EVENT_DEVOURING, 6000, 0, PHASE_PERMAGROUND);
                events.RescheduleEvent(EVENT_BUFFET, 2500, 0, PHASE_PERMAGROUND);
                events.RescheduleEvent(EVENT_FUSE, 5000, 0, PHASE_PERMAGROUND);
            }

            void SummonMoleMachines()
            {
                // Adds will come in waves from mole machines. One mole can spawn a Dark Rune Watcher
                // with 1-2 Guardians, or a lone Sentinel. Up to 4 mole machines can spawn adds at any given time.
                uint8 random = std::rand() % 4 + 2;
                for (uint8 n = 0; n < random; n++)
                {
                    float x = float(std::rand() % 640 + 540); // Safe range is between 500 and 650
                    float y = float(std::rand() % -195 + -230); // Safe range is between -235 and -145
                    float z = GROUND_Z;                     // Ground level
                    me->SummonCreature(MOLE_MACHINE_TRIGGER, x, y, z, 0, TempSummonType::TEMPSUMMON_TIMED_DESPAWN, 15000);
                }
            }

            void DoAction(int32 action) OVERRIDE
            {
                switch (action)
                {
                    case ACTION_EVENT_START:
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 150.0f);
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetUlduarAI<boss_razorscaleAI>(creature);
        }
};

class npc_expedition_commander : public CreatureScript
{
    public:
        npc_expedition_commander() : CreatureScript("npc_expedition_commander") { }

        struct npc_expedition_commanderAI : public ScriptedAI
        {
            npc_expedition_commanderAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = me->GetInstanceScript();
                Greet = false;
            }

            InstanceScript* instance;
            std::list<uint64> summons;

            bool Greet;
            uint32 AttackStartTimer;
            uint8  Phase;
            Creature* Engineer[4];
            Creature* Defender[4];

            void Reset() OVERRIDE
            {
                AttackStartTimer = 0;
                Phase = 0;
                Greet = false;
                summons.clear();
            }

            void MoveInLineOfSight(Unit* who) OVERRIDE

            {
                if (!Greet && me->IsWithinDistInMap(who, 10.0f) && who->GetTypeId() == TypeID::TYPEID_PLAYER)
                {
                    Talk(SAY_INTRO);
                    Greet = true;
                }
            }

            void JustSummoned(Creature* summoned) OVERRIDE
            {
                summons.push_back(summoned->GetGUID());
            }

            void DoAction(int32 action) OVERRIDE
            {
                switch (action)
                {
                    case ACTION_GROUND_PHASE:
                        Talk(SAY_GROUND_PHASE);
                        break;
                    case ACTION_COMMANDER_RESET:
                        summons.clear();
                        me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                }
            }

            void UpdateAI(uint32 Diff) OVERRIDE
            {
                if (AttackStartTimer <= Diff)
                {
                    switch (Phase)
                    {
                        case 1:
                            instance->SetBossState(BOSS_RAZORSCALE, IN_PROGRESS);
                            summons.clear();
                            AttackStartTimer = 1000;
                            Phase = 2;
                            break;
                        case 2:
                            for (uint8 n = 0; n < RAID_MODE(2, 4); n++)
                            {
                                Engineer[n] = me->SummonCreature(NPC_ENGINEER, PosEngSpawn, TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
                                Engineer[n]->SetWalk(false);
                                Engineer[n]->SetSpeed(MOVE_RUN, 0.5f);
                                Engineer[n]->SetHomePosition(PosEngRepair[n]);
                                Engineer[n]->GetMotionMaster()->MoveTargetedHome();
                            }
                            Engineer[0]->AI()->Talk(SAY_AGGRO_3);
                            Phase = 3;
                            AttackStartTimer = 14000;
                            break;
                        case 3:
                            for (uint8 n = 0; n < 4; n++)
                            {
                                Defender[n] = me->SummonCreature(NPC_DEFENDER, PosDefSpawn[n], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
                                Defender[n]->SetWalk(false);
                                Defender[n]->SetHomePosition(PosDefCombat[n]);
                                Defender[n]->GetMotionMaster()->MoveTargetedHome();
                            }
                            Phase = 4;
                            break;
                        case 4:
                            for (uint8 n = 0; n < RAID_MODE(2, 4); n++)
                                Engineer[n]->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_USE_STANDING);
                            for (uint8 n = 0; n < 4; ++n)
                                Defender[n]->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_READY2H);
                            Talk(SAY_AGGRO_2);
                            AttackStartTimer = 16000;
                            Phase = 5;
                            break;
                        case 5:
                            if (Creature* Razorscale = ObjectAccessor::GetCreature(*me, instance ? instance->GetData64(BOSS_RAZORSCALE) : 0))
                            {
                                Razorscale->AI()->DoAction(ACTION_EVENT_START);
                                me->SetInCombatWith(Razorscale);
                            }
                            Engineer[0]->AI()->Talk(SAY_AGGRO_1);
                            Phase = 6;
                            break;
                    }
                }
                else
                    AttackStartTimer -= Diff;
            }
        };

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
        {
            player->PlayerTalkClass->ClearMenus();
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF:
                    player->CLOSE_GOSSIP_MENU();
                    CAST_AI(npc_expedition_commanderAI, creature->AI())->Phase = 1;
                    break;
            }
            return true;
        }

        bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
        {
            InstanceScript* instance = creature->GetInstanceScript();
            if (instance && instance->GetBossState(BOSS_RAZORSCALE) == NOT_STARTED)
            {
                player->PrepareGossipMenu(creature);

                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(13853, creature->GetGUID());
            }
            else
                player->SEND_GOSSIP_MENU(13910, creature->GetGUID());

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_expedition_commanderAI(creature);
        }
};

class npc_mole_machine_trigger : public CreatureScript
{
    public:
        npc_mole_machine_trigger() : CreatureScript("npc_mole_machine_trigger") { }

        struct npc_mole_machine_triggerAI : public ScriptedAI
        {
            npc_mole_machine_triggerAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
            }

            uint32 SummonGobTimer;
            uint32 SummonNpcTimer;
            uint32 DissapearTimer;
            bool GobSummoned;
            bool NpcSummoned;

            void Reset() OVERRIDE
            {
                SummonGobTimer = 2000;
                SummonNpcTimer = 6000;
                DissapearTimer = 10000;
                GobSummoned = false;
                NpcSummoned = false;
            }

            void UpdateAI(uint32 Diff) OVERRIDE
            {
                if (!GobSummoned && SummonGobTimer <= Diff)
                {
                    DoCast(SPELL_SUMMON_MOLE_MACHINE);
                    GobSummoned = true;
                }
                else
                    SummonGobTimer -= Diff;

                if (!NpcSummoned && SummonNpcTimer <= Diff)
                {
                    switch (std::rand() % 1)
                    {
                        case 0:
                            DoCast(SPELL_SUMMON_IRON_DWARVES);
                            break;
                        case 1:
                            DoCast(SPELL_SUMMON_IRON_DWARVES_2);
                            break;
                    }

                    DoCast(SPELL_SUMMON_IRON_DWARVE_GUARDIAN);
                    DoCast(SPELL_SUMMON_IRON_DWARVE_WATCHER);
                    NpcSummoned = true;
                }
                else
                    SummonNpcTimer -= Diff;

                if (DissapearTimer <= Diff)
                {
                    if (GameObject* molemachine = me->FindNearestGameObject(GO_MOLE_MACHINE, 1))
                        molemachine->Delete();

                    me->DisappearAndDie();
                }
                else
                    DissapearTimer -= Diff;
            }

            void JustSummoned(Creature* summoned) OVERRIDE
            {
                summoned->AI()->DoZoneInCombat();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_mole_machine_triggerAI(creature);
        }
};

class npc_devouring_flame : public CreatureScript
{
    public:
        npc_devouring_flame() : CreatureScript("npc_devouring_flame") { }

        struct npc_devouring_flameAI : public ScriptedAI
        {
            npc_devouring_flameAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
            }

            void Reset() OVERRIDE
            {
                DoCast(SPELL_FLAME_GROUND);
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_devouring_flameAI(creature);
        }
};

class npc_darkrune_watcher : public CreatureScript
{
    public:
        npc_darkrune_watcher() : CreatureScript("npc_darkrune_watcher") { }

        struct npc_darkrune_watcherAI : public ScriptedAI
        {
            npc_darkrune_watcherAI(Creature* creature) : ScriptedAI(creature){ }

            uint32 ChainTimer;
            uint32 LightTimer;

            void Reset() OVERRIDE
            {
                ChainTimer = std::rand() % 15000 + 10000;
                LightTimer = std::rand() % 3000 + 1000;
            }

            void UpdateAI(uint32 Diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                if (ChainTimer <= Diff)
                {
                    DoCastVictim(SPELL_CHAIN_LIGHTNING);
                    ChainTimer = std::rand() % 15000 + 10000;
                }
                else
                    ChainTimer -= Diff;

                if (LightTimer <= Diff)
                {
                    DoCastVictim(SPELL_LIGHTNING_BOLT);
                    LightTimer = std::rand() % 7000 + 5000;
                }
                else
                    LightTimer -= Diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_darkrune_watcherAI(creature);
        }
};

class npc_darkrune_guardian : public CreatureScript
{
    public:
        npc_darkrune_guardian() : CreatureScript("npc_darkrune_guardian") { }

        struct npc_darkrune_guardianAI : public ScriptedAI
        {
            npc_darkrune_guardianAI(Creature* creature) : ScriptedAI(creature){ }

            uint32 StormTimer;

            void Reset() OVERRIDE
            {
                StormTimer = std::rand() % 6000 + 3000;
                killedByBreath = false;
            }

            uint32 GetData(uint32 type) const OVERRIDE
            {
                return type == DATA_IRON_DWARF_MEDIUM_RARE ? killedByBreath : 0;
            }

            void SetData(uint32 type, uint32 value) OVERRIDE
            {
                if (type == DATA_IRON_DWARF_MEDIUM_RARE)
                    killedByBreath = value;
            }


            void UpdateAI(uint32 Diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                if (StormTimer <= Diff)
                {
                    DoCastVictim(SPELL_STORMSTRIKE);
                    StormTimer = std::rand() % 8000 + 4000;
                }
                else
                    StormTimer -= Diff;

                DoMeleeAttackIfReady();
            }

        private:
            bool killedByBreath;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_darkrune_guardianAI(creature);
        }
};

class npc_darkrune_sentinel : public CreatureScript
{
    public:
        npc_darkrune_sentinel() : CreatureScript("npc_darkrune_sentinel") { }

        struct npc_darkrune_sentinelAI : public ScriptedAI
        {
            npc_darkrune_sentinelAI(Creature* creature) : ScriptedAI(creature){ }

            uint32 HeroicTimer;
            uint32 WhirlTimer;
            uint32 ShoutTimer;

            void Reset() OVERRIDE
            {
                HeroicTimer = std::rand() % 8000 + 4000;
                WhirlTimer = std::rand() % 25000 + 20000;
                ShoutTimer = std::rand() % 30000 + 15000;
            }

            void UpdateAI(uint32 Diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                if (HeroicTimer <= Diff)
                {
                    DoCastVictim(SPELL_HEROIC_STRIKE);
                    HeroicTimer = std::rand() % 6000 + 4000;
                }
                else
                    HeroicTimer -= Diff;

                if (WhirlTimer <= Diff)
                {
                    DoCastVictim(SPELL_WHIRLWIND);
                    WhirlTimer = std::rand() % 25000 + 20000;
                }
                else
                    WhirlTimer -= Diff;

                if (ShoutTimer <= Diff)
                {
                    DoCast(me, SPELL_BATTLE_SHOUT);
                    ShoutTimer = std::rand() % 40000 + 30000;
                }
                else
                    ShoutTimer -= Diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_darkrune_sentinelAI(creature);
        }
};

class spell_razorscale_devouring_flame : public SpellScriptLoader
{
    public:
        spell_razorscale_devouring_flame() : SpellScriptLoader("spell_razorscale_devouring_flame") { }

        class spell_razorscale_devouring_flame_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_razorscale_devouring_flame_SpellScript);

            void HandleSummon(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                Unit* caster = GetCaster();
                uint32 entry = uint32(GetSpellInfo()->Effects[effIndex].MiscValue);
                WorldLocation const* summonLocation = GetExplTargetDest();
                if (!caster || !summonLocation)
                    return;

                caster->SummonCreature(entry, summonLocation->GetPositionX(), summonLocation->GetPositionY(), GROUND_Z, 0.0f, TempSummonType::TEMPSUMMON_TIMED_DESPAWN, 20000);
            }

            void Register() OVERRIDE
            {
                OnEffectHit += SpellEffectFn(spell_razorscale_devouring_flame_SpellScript::HandleSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_razorscale_devouring_flame_SpellScript();
        }
};

class spell_razorscale_flame_breath : public SpellScriptLoader
{
    public:
        spell_razorscale_flame_breath() : SpellScriptLoader("spell_razorscale_flame_breath") { }

        class spell_razorscale_flame_breath_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_razorscale_flame_breath_SpellScript);

            void CheckDamage()
            {
                Creature* target = GetHitCreature();
                if (!target || target->GetEntry() != NPC_DARK_RUNE_GUARDIAN)
                    return;

                if (GetHitDamage() >= int32(target->GetHealth()))
                    target->AI()->SetData(DATA_IRON_DWARF_MEDIUM_RARE, 1);
            }

            void Register() OVERRIDE
            {
                OnHit += SpellHitFn(spell_razorscale_flame_breath_SpellScript::CheckDamage);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_razorscale_flame_breath_SpellScript();
        }
};

class achievement_iron_dwarf_medium_rare : public AchievementCriteriaScript
{
    public:
        achievement_iron_dwarf_medium_rare() : AchievementCriteriaScript("achievement_iron_dwarf_medium_rare")
        {
        }

        bool OnCheck(Player* /*player*/, Unit* target) OVERRIDE
        {
            return target && target->IsAIEnabled && target->GetAI()->GetData(DATA_IRON_DWARF_MEDIUM_RARE);
        }
};

class achievement_quick_shave : public AchievementCriteriaScript
{
    public:
        achievement_quick_shave() : AchievementCriteriaScript("achievement_quick_shave") { }

        bool OnCheck(Player* /*source*/, Unit* target) OVERRIDE
        {
            if (target)
                if (Creature* razorscale = target->ToCreature())
                    if (razorscale->AI()->GetData(DATA_QUICK_SHAVE))
                        return true;

            return false;
        }
};

void AddSC_boss_razorscale()
{
    new boss_razorscale_controller();
    new go_razorscale_harpoon();
    new boss_razorscale();
    new npc_expedition_commander();
    new npc_mole_machine_trigger();
    new npc_devouring_flame();
    new npc_darkrune_watcher();
    new npc_darkrune_guardian();
    new npc_darkrune_sentinel();
    new spell_razorscale_devouring_flame();
    new spell_razorscale_flame_breath();
    new achievement_iron_dwarf_medium_rare();
    new achievement_quick_shave();
}
