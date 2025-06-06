/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Huhuran
SD%Complete: 100
SDComment:
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Huhuran
{
    EMOTE_FRENZY_KILL           = 0,
    EMOTE_BERSERK               = 1,

    SPELL_FRENZY                = 26051,
    SPELL_BERSERK               = 26068,
    SPELL_POISONBOLT            = 26052,
    SPELL_NOXIOUSPOISON         = 26053,
    SPELL_WYVERNSTING           = 26180,
    SPELL_ACIDSPIT              = 26050
};

class boss_huhuran : public CreatureScript
{
public:
    boss_huhuran() : CreatureScript("boss_huhuran") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_huhuranAI(creature);
    }

    struct boss_huhuranAI : public ScriptedAI
    {
        boss_huhuranAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 Frenzy_Timer;
        uint32 Wyvern_Timer;
        uint32 Spit_Timer;
        uint32 PoisonBolt_Timer;
        uint32 NoxiousPoison_Timer;
        uint32 FrenzyBack_Timer;

        bool Frenzy;
        bool Berserk;

        void Reset() OVERRIDE
        {
            Frenzy_Timer = std::rand() % 35000 + 25000;
            Wyvern_Timer = std::rand() % 28000 + 18000;
            Spit_Timer = 8000;
            PoisonBolt_Timer = 4000;
            NoxiousPoison_Timer = std::rand() % 20000 + 10000;
            FrenzyBack_Timer = 15000;

            Frenzy = false;
            Berserk = false;
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            //Frenzy_Timer
            if (!Frenzy && Frenzy_Timer <= diff)
            {
                DoCast(me, SPELL_FRENZY);
                Talk(EMOTE_FRENZY_KILL);
                Frenzy = true;
                PoisonBolt_Timer = 3000;
                Frenzy_Timer = std::rand() % 35000 + 25000;
            } else Frenzy_Timer -= diff;

            // Wyvern Timer
            if (Wyvern_Timer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_WYVERNSTING);
                Wyvern_Timer = std::rand() % 32000 + 15000;
            } else Wyvern_Timer -= diff;

            //Spit Timer
            if (Spit_Timer <= diff)
            {
                DoCastVictim(SPELL_ACIDSPIT);
                Spit_Timer = std::rand() % 10000 + 5000;
            } else Spit_Timer -= diff;

            //NoxiousPoison_Timer
            if (NoxiousPoison_Timer <= diff)
            {
                DoCastVictim(SPELL_NOXIOUSPOISON);
                NoxiousPoison_Timer = std::rand() % 24000 + 12000;
            } else NoxiousPoison_Timer -= diff;

            //PoisonBolt only if frenzy or berserk
            if (Frenzy || Berserk)
            {
                if (PoisonBolt_Timer <= diff)
                {
                    DoCastVictim(SPELL_POISONBOLT);
                    PoisonBolt_Timer = 3000;
                } else PoisonBolt_Timer -= diff;
            }

            //FrenzyBack_Timer
            if (Frenzy && FrenzyBack_Timer <= diff)
            {
                me->InterruptNonMeleeSpells(false);
                Frenzy = false;
                FrenzyBack_Timer = 15000;
            } else FrenzyBack_Timer -= diff;

            if (!Berserk && HealthBelowPct(31))
            {
                me->InterruptNonMeleeSpells(false);
                Talk(EMOTE_BERSERK);
                DoCast(me, SPELL_BERSERK);
                Berserk = true;
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_huhuran()
{
    new boss_huhuran();
}
