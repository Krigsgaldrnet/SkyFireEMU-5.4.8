/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackrock_spire.h"
#include "TemporarySummon.h"

enum Spells
{
    SPELL_FATAL_BITE                = 16495,
    SPELL_INFECTED_BITE             = 16128,
    SPELL_FRENZY                    = 8269
};

enum Paths
{
    GIZRUL_PATH                     = 402450
};

enum Events
{
    EVENT_FATAL_BITE                = 1,
    EVENT_INFECTED_BITE             = 2,
    EVENT_FRENZY                    = 3
};

class boss_gizrul_the_slavener : public CreatureScript
{
public:
    boss_gizrul_the_slavener() : CreatureScript("boss_gizrul_the_slavener") { }

    struct boss_gizrul_the_slavenerAI : public BossAI
    {
       boss_gizrul_the_slavenerAI(Creature* creature) : BossAI(creature, DATA_GIZRUL_THE_SLAVENER) { }

        void Reset() OVERRIDE
        {
            _Reset();
        }

        void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
        {
            me->GetMotionMaster()->MovePath(GIZRUL_PATH, false);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_FATAL_BITE, std::rand() % 20000 + 17000);
            events.ScheduleEvent(EVENT_INFECTED_BITE, std::rand() % 12000 + 10000);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            _JustDied();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FATAL_BITE:
                        DoCastVictim(SPELL_FATAL_BITE);
                        events.ScheduleEvent(EVENT_FATAL_BITE, std::rand() % 10000 + 8000);
                        break;
                    case EVENT_INFECTED_BITE:
                        DoCast(me, SPELL_INFECTED_BITE);
                        events.ScheduleEvent(EVENT_FATAL_BITE, std::rand() % 10000 + 8000);
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
        return new boss_gizrul_the_slavenerAI(creature);
    }
};

void AddSC_boss_gizrul_the_slavener()
{
    new boss_gizrul_the_slavener();
}
