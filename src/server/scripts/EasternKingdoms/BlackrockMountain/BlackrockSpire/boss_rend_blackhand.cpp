/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "blackrock_spire.h"

enum Spells
{
    SPELL_WHIRLWIND                 = 13736, // sniffed
    SPELL_CLEAVE                    = 15284,
    SPELL_MORTAL_STRIKE             = 16856,
    SPELL_FRENZY                    = 8269,
    SPELL_KNOCKDOWN                 = 13360  // On spawn during Gyth fight
};

enum Says
{
    // Rend Blackhand
    SAY_BLACKHAND_1                 = 0,
    SAY_BLACKHAND_2                 = 1,
    EMOTE_BLACKHAND_DISMOUNT        = 2,
    // Victor Nefarius
    SAY_NEFARIUS_0                  = 0,
    SAY_NEFARIUS_1                  = 1,
    SAY_NEFARIUS_2                  = 2,
    SAY_NEFARIUS_3                  = 3,
    SAY_NEFARIUS_4                  = 4,
    SAY_NEFARIUS_5                  = 5,
    SAY_NEFARIUS_6                  = 6,
    SAY_NEFARIUS_7                  = 7,
    SAY_NEFARIUS_8                  = 8,
    SAY_NEFARIUS_9                  = 9,
};

enum Adds
{
    NPC_CHROMATIC_WHELP             = 10442,
    NPC_CHROMATIC_DRAGONSPAWN       = 10447,
    NPC_BLACKHAND_DRAGON_HANDLER    = 10742
};

enum Misc
{
    NEFARIUS_PATH_1                 = 1379670,
    NEFARIUS_PATH_2                 = 1379671,
    NEFARIUS_PATH_3                 = 1379672,
    REND_PATH_1                     = 1379680,
    REND_PATH_2                     = 1379681,
};

/*
struct Wave
{
    uint32 entry;
    float  x_pos;
    float  y_pos;
    float  z_pos;
    float  o_pos;
};

static Wave Wave2[]= // 22 sec
{
    { 10447, 209.8637f, -428.2729f, 110.9877f, 0.6632251f },
    { 10442, 209.3122f, -430.8724f, 110.9814f, 2.9147f    },
    { 10442, 211.3309f, -425.9111f, 111.0006f, 1.727876f  }
};

static Wave Wave3[]= // 60 sec
{
    { 10742, 208.6493f, -424.5787f, 110.9872f, 5.8294f    },
    { 10447, 203.9482f, -428.9446f, 110.982f,  4.677482f  },
    { 10442, 203.3441f, -426.8668f, 110.9772f, 4.712389f  },
    { 10442, 206.3079f, -424.7509f, 110.9943f, 4.08407f   }
};

static Wave Wave4[]= // 49 sec
{
    { 10742, 212.3541f, -412.6826f, 111.0352f, 5.88176f   },
    { 10447, 212.5754f, -410.2841f, 111.0296f, 2.740167f  },
    { 10442, 212.3449f, -414.8659f, 111.0348f, 2.356194f  },
    { 10442, 210.6568f, -412.1552f, 111.0124f, 0.9773844f }
};

static Wave Wave5[]= // 60 sec
{
    { 10742, 210.2188f, -410.6686f, 111.0211f, 5.8294f    },
    { 10447, 209.4078f, -414.13f,   111.0264f, 4.677482f  },
    { 10442, 208.0858f, -409.3145f, 111.0118f, 4.642576f  },
    { 10442, 207.9811f, -413.0728f, 111.0098f, 5.288348f  },
    { 10442, 208.0854f, -412.1505f, 111.0057f, 4.08407f   }
};

static Wave Wave6[]= // 27 sec
{
    { 10742, 213.9138f, -426.512f,  111.0013f, 3.316126f  },
    { 10447, 213.7121f, -429.8102f, 110.9888f, 1.413717f  },
    { 10447, 213.7157f, -424.4268f, 111.009f,  3.001966f  },
    { 10442, 210.8935f, -423.913f,  111.0125f, 5.969026f  },
    { 10442, 212.2642f, -430.7648f, 110.9807f, 5.934119f  }
};
*/

Position const GythLoc =      { 211.762f,  -397.5885f, 111.1817f,  4.747295f   };
Position const Teleport1Loc = { 194.2993f, -474.0814f, 121.4505f, -0.01225555f };
Position const Teleport2Loc = { 216.485f,  -434.93f,   110.888f,  -0.01225555f };

enum Events
{
    EVENT_START_1                   = 1,
    EVENT_START_2                   = 2,
    EVENT_START_3                   = 3,
    EVENT_START_4                   = 4,
    EVENT_TURN_TO_REND              = 5,
    EVENT_TURN_TO_PLAYER            = 6,
    EVENT_TURN_TO_FACING_1          = 7,
    EVENT_TURN_TO_FACING_2          = 8,
    EVENT_TURN_TO_FACING_3          = 9,
    EVENT_WAVE_1                    = 10,
    EVENT_WAVE_2                    = 11,
    EVENT_WAVE_3                    = 12,
    EVENT_WAVE_4                    = 13,
    EVENT_WAVE_5                    = 14,
    EVENT_WAVE_6                    = 15,
    EVENT_WAVES_TEXT_1              = 16,
    EVENT_WAVES_TEXT_2              = 17,
    EVENT_WAVES_TEXT_3              = 18,
    EVENT_WAVES_TEXT_4              = 19,
    EVENT_WAVES_TEXT_5              = 20,
    EVENT_WAVES_COMPLETE_TEXT_1     = 21,
    EVENT_WAVES_COMPLETE_TEXT_2     = 22,
    EVENT_WAVES_COMPLETE_TEXT_3     = 23,
    EVENT_WAVES_EMOTE_1             = 24,
    EVENT_WAVES_EMOTE_2             = 25,
    EVENT_PATH_REND                 = 26,
    EVENT_PATH_NEFARIUS             = 27,
    EVENT_TELEPORT_1                = 28,
    EVENT_TELEPORT_2                = 29,
    EVENT_WHIRLWIND                 = 30,
    EVENT_CLEAVE                    = 31,
    EVENT_MORTAL_STRIKE             = 32,
};

class boss_rend_blackhand : public CreatureScript
{
public:
    boss_rend_blackhand() : CreatureScript("boss_rend_blackhand") { }

    struct boss_rend_blackhandAI : public BossAI
    {
        boss_rend_blackhandAI(Creature* creature) : BossAI(creature, DATA_WARCHIEF_REND_BLACKHAND) { }

        void Reset() OVERRIDE
        {
            _Reset();
            gythEvent = false;
            victorGUID = 0;
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_WHIRLWIND, std::rand() % 15000 + 13000);
            events.ScheduleEvent(EVENT_CLEAVE, std::rand() % 17000 + 15000);
            events.ScheduleEvent(EVENT_MORTAL_STRIKE, std::rand() % 19000 + 17000);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            _JustDied();
            if (Creature* victor = me->FindNearestCreature(NPC_LORD_VICTOR_NEFARIUS, 75.0f, true))
                victor->AI()->SetData(1, 2);
        }

        void SetData(uint32 type, uint32 data) OVERRIDE
        {
            if (instance && type == AREATRIGGER && data == AREATRIGGER_BLACKROCK_STADIUM)
            {
                if (!gythEvent)
                {
                    gythEvent = true;

                    if (Creature* victor = me->FindNearestCreature(NPC_LORD_VICTOR_NEFARIUS, 5.0f, true))
                        victorGUID = victor->GetGUID();

                    if (GameObject* portcullis = me->FindNearestGameObject(GO_DR_PORTCULLIS, 50.0f))
                        portcullisGUID = portcullis->GetGUID();

                    events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                    events.ScheduleEvent(EVENT_START_1, 1000);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) OVERRIDE
        {
            if (type == WAYPOINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case 5:
                        events.ScheduleEvent(EVENT_TELEPORT_1, 2000);
                        break;
                    case 11:
                        if (Creature* gyth = me->FindNearestCreature(NPC_GYTH, 10.0f, true))
                            gyth->AI()->SetData(1, 1);
                        me->DespawnOrUnsummon(1000);
                        break;
                }
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (gythEvent)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_START_1:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_0);
                            events.ScheduleEvent(EVENT_START_2, 4000);
                            break;
                        case EVENT_START_2:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                            events.ScheduleEvent(EVENT_START_3, 4000);
                            break;
                        case EVENT_START_3:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_1);
                            events.ScheduleEvent(EVENT_WAVE_1, 2000);
                            events.ScheduleEvent(EVENT_TURN_TO_REND, 4000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_1, 20000);
                            break;
                        case EVENT_TURN_TO_REND:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                            {
                                victor->SetFacingToObject(me);
                                victor->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            }
                            break;
                        case EVENT_TURN_TO_PLAYER:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                if (Unit* player = victor->SelectNearestPlayer(60.0f))
                                    victor->SetFacingToObject(player);
                            break;
                        case EVENT_TURN_TO_FACING_1:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->SetFacingTo(1.518436f);
                            break;
                        case EVENT_TURN_TO_FACING_2:
                            me->SetFacingTo(1.658063f);
                            break;
                        case EVENT_TURN_TO_FACING_3:
                            me->SetFacingTo(1.500983f);
                            break;
                        case EVENT_WAVES_EMOTE_1:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                            break;
                        case EVENT_WAVES_EMOTE_2:
                                me->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
                            break;
                        case EVENT_WAVES_TEXT_1:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                    victor->AI()->Talk(SAY_NEFARIUS_2);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVES_EMOTE_1, 5000);
                            events.ScheduleEvent(EVENT_WAVE_2, 2000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_2, 20000);
                            break;
                        case EVENT_WAVES_TEXT_2:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_3);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVE_3, 2000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_3, 20000);
                            break;
                        case EVENT_WAVES_TEXT_3:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_4);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVE_4, 2000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_4, 20000);
                            break;
                        case EVENT_WAVES_TEXT_4:
                            Talk(SAY_BLACKHAND_1);
                            events.ScheduleEvent(EVENT_WAVES_EMOTE_2, 4000);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_3, 8000);
                            events.ScheduleEvent(EVENT_WAVE_5, 2000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_5, 20000);
                            break;
                        case EVENT_WAVES_TEXT_5:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_5);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVE_6, 2000);
                            events.ScheduleEvent(EVENT_WAVES_COMPLETE_TEXT_1, 20000);
                            break;
                        case EVENT_WAVES_COMPLETE_TEXT_1:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_6);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVES_COMPLETE_TEXT_2, 13000);
                            break;
                        case EVENT_WAVES_COMPLETE_TEXT_2:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_7);
                            Talk(SAY_BLACKHAND_2);
                            events.ScheduleEvent(EVENT_PATH_REND, 1000);
                            events.ScheduleEvent(EVENT_WAVES_COMPLETE_TEXT_3, 4000);
                            break;
                        case EVENT_WAVES_COMPLETE_TEXT_3:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_8);
                            events.ScheduleEvent(EVENT_PATH_NEFARIUS, 1000);
                            events.ScheduleEvent(EVENT_PATH_REND, 1000);
                            break;
                        case EVENT_PATH_NEFARIUS:
                            if (Creature* victor = me->GetCreature(*me, victorGUID))
                                victor->GetMotionMaster()->MovePath(NEFARIUS_PATH_1, true);
                            break;
                        case EVENT_PATH_REND:
                            me->GetMotionMaster()->MovePath(REND_PATH_1, false);
                            break;
                        case EVENT_TELEPORT_1:
                            me->NearTeleportTo(194.2993f, -474.0814f, 121.4505f, -0.01225555f);
                            events.ScheduleEvent(EVENT_TELEPORT_2, 50000);
                            break;
                        case EVENT_TELEPORT_2:
                            me->NearTeleportTo(216.485f, -434.93f, 110.888f, -0.01225555f);
                            me->SummonCreature(NPC_GYTH, 211.762f, -397.5885f, 111.1817f, 4.747295f);
                            break;
                        case EVENT_WAVE_1:
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            // move wave
                            break;
                        case EVENT_WAVE_2:
                            // spawn wave
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            // move wave
                            break;
                        case EVENT_WAVE_3:
                            // spawn wave
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            // move wave
                            break;
                        case EVENT_WAVE_4:
                            // spawn wave
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            // move wave
                            break;
                        case EVENT_WAVE_5:
                            // spawn wave
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            // move wave
                            break;
                        case EVENT_WAVE_6:
                            // spawn wave
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            // move wave
                            break;
                        default:
                            break;
                    }
                }
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_WHIRLWIND:
                        DoCast(SPELL_WHIRLWIND);
                        events.ScheduleEvent(EVENT_WHIRLWIND, std::rand() % 18000 + 13000);
                        break;
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, std::rand() % 14000 + 10000);
                        break;
                    case EVENT_MORTAL_STRIKE:
                        DoCastVictim(SPELL_MORTAL_STRIKE);
                        events.ScheduleEvent(EVENT_MORTAL_STRIKE, std::rand() % 16000 + 14000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

        private:
            bool   gythEvent;
            uint64 victorGUID;
            uint64 portcullisGUID;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_rend_blackhandAI(creature);
    }
};

void AddSC_boss_rend_blackhand()
{
    new boss_rend_blackhand();
}
