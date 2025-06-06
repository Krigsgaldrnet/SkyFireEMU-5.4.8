/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Nethermancer_Sepethrea
SD%Complete: 90
SDComment: Need adjustments to initial summons
SDCategory: Tempest Keep, The Mechanar
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "mechanar.h"

enum Says
{
    SAY_AGGRO                      = 0,
    SAY_SUMMON                     = 1,
    SAY_DRAGONS_BREATH             = 2,
    SAY_SLAY                       = 3,
    SAY_DEATH                      = 4
};

enum Spells
{
    SPELL_SUMMON_RAGIN_FLAMES      = 35275, // Not scripted
    SPELL_FROST_ATTACK             = 35263,
    SPELL_ARCANE_BLAST             = 35314,
    SPELL_DRAGONS_BREATH           = 35250,
    SPELL_KNOCKBACK                = 37317,
    SPELL_SOLARBURN                = 35267,
    H_SPELL_SUMMON_RAGIN_FLAMES    = 39084, // Not scripted
    SPELL_INFERNO                  = 35268, // Not scripted
    H_SPELL_INFERNO                = 39346, // Not scripted
    SPELL_FIRE_TAIL                = 35278  // Not scripted
};

enum Events
{
    EVENT_FROST_ATTACK             = 1,
    EVENT_ARCANE_BLAST             = 2,
    EVENT_DRAGONS_BREATH           = 3,
    EVENT_KNOCKBACK                = 4,
    EVENT_SOLARBURN                = 5
};

class boss_nethermancer_sepethrea : public CreatureScript
{
    public:
        boss_nethermancer_sepethrea(): CreatureScript("boss_nethermancer_sepethrea") { }

        struct boss_nethermancer_sepethreaAI : public BossAI
        {
            boss_nethermancer_sepethreaAI(Creature* creature) : BossAI(creature, DATA_NETHERMANCER_SEPRETHREA) { }

            void EnterCombat(Unit* who) OVERRIDE
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_FROST_ATTACK, std::rand() % 10000 + 7000);
                events.ScheduleEvent(EVENT_ARCANE_BLAST, std::rand() % 18000 + 12000);
                events.ScheduleEvent(EVENT_DRAGONS_BREATH, std::rand() % 22000 + 18000);
                events.ScheduleEvent(EVENT_KNOCKBACK, std::rand() % 28000 + 22000);
                events.ScheduleEvent(EVENT_SOLARBURN, 30000);
                Talk(SAY_AGGRO);
                DoCast(who, SPELL_SUMMON_RAGIN_FLAMES);
                Talk(SAY_SUMMON);
            }

            void KilledUnit(Unit* /*victim*/) OVERRIDE
            {
                Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
                Talk(SAY_DEATH);
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
                        case EVENT_FROST_ATTACK:
                            DoCastVictim(SPELL_FROST_ATTACK, true);
                            events.ScheduleEvent(EVENT_FROST_ATTACK, std::rand() % 10000 + 7000);
                            break;
                        case EVENT_ARCANE_BLAST:
                            DoCastVictim(SPELL_ARCANE_BLAST, true);
                            events.ScheduleEvent(EVENT_ARCANE_BLAST, 15000);
                            break;
                        case EVENT_DRAGONS_BREATH:
                            DoCastVictim(SPELL_DRAGONS_BREATH, true);
                            events.ScheduleEvent(EVENT_DRAGONS_BREATH, std::rand() % 22000 + 12000);
                            if (roll_chance_i(50))
                                Talk(SAY_DRAGONS_BREATH);
                            break;
                        case EVENT_KNOCKBACK:
                            DoCastVictim(SPELL_KNOCKBACK, true);
                            events.ScheduleEvent(EVENT_KNOCKBACK, std::rand() % 25000 + 15000);
                            break;
                        case EVENT_SOLARBURN:
                            DoCastVictim(SPELL_SOLARBURN, true);
                            events.ScheduleEvent(EVENT_SOLARBURN, 30000);
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
            return new boss_nethermancer_sepethreaAI(creature);
        }
};

class npc_ragin_flames : public CreatureScript
{
    public:
        npc_ragin_flames() : CreatureScript("npc_ragin_flames") { }

            struct npc_ragin_flamesAI : public ScriptedAI
            {
                npc_ragin_flamesAI(Creature* creature) : ScriptedAI(creature)
                {
                    instance = creature->GetInstanceScript();
                }

                InstanceScript* instance;

                uint32 inferno_Timer;
                uint32 flame_timer;
                uint32 Check_Timer;

                bool onlyonce;

                void Reset() OVERRIDE
                {
                    inferno_Timer = 10000;
                    flame_timer = 500;
                    Check_Timer = 2000;
                    onlyonce = false;
                    me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_MAGIC, true);
                    me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, true);
                    me->SetSpeed(MOVE_RUN, DUNGEON_MODE(0.5f, 0.7f));
                }

                void EnterCombat(Unit* /*who*/) OVERRIDE
                {
                }

                void UpdateAI(uint32 diff) OVERRIDE
                {
                    //Check_Timer
                    if (Check_Timer <= diff)
                    {
                        if (instance)
                        {
                            if (instance->GetData(DATA_NETHERMANCER_SEPRETHREA) != IN_PROGRESS)
                            {
                                //remove
                                me->setDeathState(DeathState::JUST_DIED);
                                me->RemoveCorpse();
                            }
                        }
                        Check_Timer = 1000;
                    } else Check_Timer -= diff;

                    if (!UpdateVictim())
                        return;

                    if (!onlyonce)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            me->GetMotionMaster()->MoveChase(target);
                        onlyonce = true;
                    }

                    if (inferno_Timer <= diff)
                    {
                        DoCastVictim(SPELL_INFERNO);
                        me->TauntApply(me->GetVictim());
                        inferno_Timer = 10000;
                    } else inferno_Timer -= diff;

                    if (flame_timer <= diff)
                    {
                        DoCast(me, SPELL_FIRE_TAIL);
                        flame_timer = 500;
                    } else flame_timer -=diff;

                    DoMeleeAttackIfReady();
                }
            };

            CreatureAI* GetAI(Creature* creature) const OVERRIDE
            {
                return new npc_ragin_flamesAI(creature);
            }
};

void AddSC_boss_nethermancer_sepethrea()
{
    new boss_nethermancer_sepethrea();
    new npc_ragin_flames();
}
