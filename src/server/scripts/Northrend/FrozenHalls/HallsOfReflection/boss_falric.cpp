/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "halls_of_reflection.h"

enum Texts
{
    SAY_AGGRO                                     = 0,
    SAY_SLAY                                      = 1,
    SAY_DEATH                                     = 2,
    SAY_IMPENDING_DESPAIR                         = 3,
    SAY_DEFILING_HORROR                           = 4,
};

enum Spells
{
    SPELL_QUIVERING_STRIKE                        = 72422,
    SPELL_IMPENDING_DESPAIR                       = 72426,
    SPELL_DEFILING_HORROR                         = 72435,
    SPELL_HOPELESSNESS                            = 72395,
    H_SPELL_HOPELESSNESS                          = 72390, /// @todo not in dbc. Add in DB.
};

enum Events
{
    EVENT_NONE,
    EVENT_QUIVERING_STRIKE,
    EVENT_IMPENDING_DESPAIR,
    EVENT_DEFILING_HORROR,
};

class boss_falric : public CreatureScript
{
public:
    boss_falric() : CreatureScript("boss_falric") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_falricAI(creature);
    }

    struct boss_falricAI : public boss_horAI
    {
        boss_falricAI(Creature* creature) : boss_horAI(creature) { }

        uint8 uiHopelessnessCount;

        void Reset() OVERRIDE
        {
            boss_horAI::Reset();

            uiHopelessnessCount = 0;

            if (instance)
                instance->SetBossState(DATA_FALRIC_EVENT, NOT_STARTED);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);
            if (instance)
                instance->SetBossState(DATA_FALRIC_EVENT, IN_PROGRESS);

            events.ScheduleEvent(EVENT_QUIVERING_STRIKE, 23000);
            events.ScheduleEvent(EVENT_IMPENDING_DESPAIR, 9000);
            events.ScheduleEvent(EVENT_DEFILING_HORROR, std::rand() % 45000 + 25000); /// @todo adjust timer.
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);

            if (instance)
                instance->SetBossState(DATA_FALRIC_EVENT, DONE);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_SLAY);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            // Return since we have no target
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_QUIVERING_STRIKE:
                    DoCast(SPELL_QUIVERING_STRIKE);
                    events.ScheduleEvent(EVENT_QUIVERING_STRIKE, 10000);
                    break;
                case EVENT_IMPENDING_DESPAIR:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                    {
                        Talk(SAY_IMPENDING_DESPAIR);
                        DoCast(target, SPELL_IMPENDING_DESPAIR);
                    }
                    events.ScheduleEvent(EVENT_IMPENDING_DESPAIR, 13000);
                    break;
                case EVENT_DEFILING_HORROR:
                    DoCast(SPELL_DEFILING_HORROR);
                    events.ScheduleEvent(EVENT_DEFILING_HORROR, std::rand() % 45000 + 25000); /// @todo adjust timer.
                    break;
            }

            if ((uiHopelessnessCount < 1 && HealthBelowPct(66))
                || (uiHopelessnessCount < 2 && HealthBelowPct(33))
                || (uiHopelessnessCount < 3 && HealthBelowPct(10)))
            {
                uiHopelessnessCount++;
                DoCast(DUNGEON_MODE(SPELL_HOPELESSNESS, H_SPELL_HOPELESSNESS));
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_falric()
{
    new boss_falric();
}
