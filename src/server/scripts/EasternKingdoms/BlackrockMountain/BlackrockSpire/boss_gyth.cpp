/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackrock_spire.h"

enum Spells
{
    SPELL_REND_MOUNTS               = 16167, // Change model
    SPELL_CORROSIVE_ACID            = 16359, // Combat (self cast)
    SPELL_FLAMEBREATH               = 16390, // Combat (Self cast)
    SPELL_FREEZE                    = 16350, // Combat (Self cast)
    SPELL_KNOCK_AWAY                = 10101, // Combat
    SPELL_SUMMON_REND               = 16328  // Summons Rend near death
};

enum Misc
{
    NEFARIUS_PATH_2                 = 1379671,
    NEFARIUS_PATH_3                 = 1379672,
    GYTH_PATH_1                     = 1379681,
};

enum Events
{
    EVENT_CORROSIVE_ACID            = 1,
    EVENT_FREEZE                    = 2,
    EVENT_FLAME_BREATH              = 3,
    EVENT_KNOCK_AWAY                = 4,
    EVENT_SUMMONED_1                = 5,
    EVENT_SUMMONED_2                = 6
};

class boss_gyth : public CreatureScript
{
public:
    boss_gyth() : CreatureScript("boss_gyth") { }

    struct boss_gythAI : public BossAI
    {
        boss_gythAI(Creature* creature) : BossAI(creature, DATA_GYTH) { }

        bool SummonedRend;

        void Reset() OVERRIDE
        {
            SummonedRend = false;
            if (instance->GetBossState(DATA_GYTH) == IN_PROGRESS)
            {
                instance->SetBossState(DATA_GYTH, DONE);
                me->DespawnOrUnsummon();
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            _EnterCombat();

            events.ScheduleEvent(EVENT_CORROSIVE_ACID, std::rand() % 16000 + 8000);
            events.ScheduleEvent(EVENT_FREEZE, std::rand() % 16000 + 8000);
            events.ScheduleEvent(EVENT_FLAME_BREATH, std::rand() % 16000 + 8000);
            events.ScheduleEvent(EVENT_KNOCK_AWAY, std::rand() % 18000 + 12000);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            instance->SetBossState(DATA_GYTH, DONE);
        }

        void SetData(uint32 /*type*/, uint32 data) OVERRIDE
        {
            switch (data)
            {
                case 1:
                    events.ScheduleEvent(EVENT_SUMMONED_1, 1000);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!SummonedRend && HealthBelowPct(5))
            {
                DoCast(me, SPELL_SUMMON_REND);
                me->RemoveAura(SPELL_REND_MOUNTS);
                SummonedRend = true;
            }

            if (!UpdateVictim())
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMONED_1:
                            me->AddAura(SPELL_REND_MOUNTS, me);
                            if (GameObject* portcullis = me->FindNearestGameObject(GO_DR_PORTCULLIS, 40.0f))
                                portcullis->UseDoorOrButton();
                            if (Creature* victor = me->FindNearestCreature(NPC_LORD_VICTOR_NEFARIUS, 75.0f, true))
                                victor->AI()->SetData(1, 1);
                            events.ScheduleEvent(EVENT_SUMMONED_2, 2000);
                            break;
                        case EVENT_SUMMONED_2:
                            me->GetMotionMaster()->MovePath(GYTH_PATH_1, false);
                            break;
                        default:
                            break;
                    }
                }
                return;
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CORROSIVE_ACID:
                        DoCast(me, SPELL_CORROSIVE_ACID);
                        events.ScheduleEvent(EVENT_CORROSIVE_ACID, std::rand() % 16000 + 10000);
                        break;
                    case EVENT_FREEZE:
                        DoCast(me, SPELL_FREEZE);
                        events.ScheduleEvent(EVENT_FREEZE, std::rand() % 16000 + 10000);
                        break;
                    case EVENT_FLAME_BREATH:
                        DoCast(me, SPELL_FLAMEBREATH);
                        events.ScheduleEvent(EVENT_FLAME_BREATH, std::rand() % 16000 + 10000);
                        break;
                    case EVENT_KNOCK_AWAY:
                        DoCastVictim(SPELL_KNOCK_AWAY);
                        events.ScheduleEvent(EVENT_KNOCK_AWAY, std::rand() % 20000 + 14000);
                        break;
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_gythAI(creature);
    }
};

void AddSC_boss_gyth()
{
    new boss_gyth();
}
