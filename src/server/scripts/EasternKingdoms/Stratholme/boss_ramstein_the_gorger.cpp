/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Ramstein_The_Gorger
SD%Complete: 70
SDComment:
SDCategory: Stratholme
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "stratholme.h"

enum Spells
{
    SPELL_TRAMPLE           = 5568,
    SPELL_KNOCKOUT          = 17307
};

enum CreatureId
{
    NPC_MINDLESS_UNDEAD     = 11030
};

class boss_ramstein_the_gorger : public CreatureScript
{
public:
    boss_ramstein_the_gorger() : CreatureScript("boss_ramstein_the_gorger") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_ramstein_the_gorgerAI(creature);
    }

    struct boss_ramstein_the_gorgerAI : public ScriptedAI
    {
        boss_ramstein_the_gorgerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        InstanceScript* instance;

        uint32 Trample_Timer;
        uint32 Knockout_Timer;

        void Reset() OVERRIDE
        {
            Trample_Timer = 3000;
            Knockout_Timer = 12000;
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            for (uint8 i = 0; i < 30; ++i)
            {
                if (Creature* mob = me->SummonCreature(NPC_MINDLESS_UNDEAD, 3969.35f+(std::rand() % 10 + -10), -3391.87f+(std::rand() % 10 + -10), 119.11f, 5.91f, TempSummonType::TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 1800000))
                    mob->AI()->AttackStart(me->SelectNearestTarget(100.0f));
            }

            if (instance)
                instance->SetData(TYPE_RAMSTEIN, DONE);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            //Trample
            if (Trample_Timer <= diff)
            {
                DoCast(me, SPELL_TRAMPLE);
                Trample_Timer = 7000;
            } else Trample_Timer -= diff;

            //Knockout
            if (Knockout_Timer <= diff)
            {
                DoCastVictim(SPELL_KNOCKOUT);
                Knockout_Timer = 10000;
            } else Knockout_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_ramstein_the_gorger()
{
    new boss_ramstein_the_gorger();
}
