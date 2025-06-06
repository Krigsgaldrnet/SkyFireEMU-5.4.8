/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "the_botanica.h"
#include "SpellScript.h"

enum Says
{
    SAY_AGGRO                   = 0,
    SAY_KILL                    = 1,
    SAY_ARCANE_RESONANCE        = 2,
    SAY_ARCANE_DEVASTATION      = 3,
    EMOTE_SUMMON                = 4,
    SAY_SUMMON                  = 5,
    SAY_DEATH                   = 6
};

enum Spells
{
    SPELL_ARCANE_RESONANCE      = 34794,
    SPELL_ARCANE_DEVASTATION    = 34799,
    SPELL_SUMMON_REINFORCEMENTS = 34803
};

enum Events
{
    EVENT_ARCANE_RESONANCE      = 1,
    EVENT_ARCANE_DEVASTATION    = 2
};

class boss_commander_sarannis : public CreatureScript
{
    public:
        boss_commander_sarannis() : CreatureScript("boss_commander_sarannis") { }

        struct boss_commander_sarannisAI : public BossAI
        {
            boss_commander_sarannisAI(Creature* creature) : BossAI(creature, DATA_COMMANDER_SARANNIS) { }

            void Reset() OVERRIDE
            {
                _Reset();
                _phase = true;
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
                events.ScheduleEvent(EVENT_ARCANE_RESONANCE, 42700);
                events.ScheduleEvent(EVENT_ARCANE_DEVASTATION, 15200);
            }

            void KilledUnit(Unit* /*victim*/) OVERRIDE
            {
                Talk(SAY_KILL);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
                Talk(SAY_DEATH);
            }

            void DamageTaken(Unit* /*killer*/, uint32 &damage) OVERRIDE
            {
                if (me->HealthBelowPctDamaged(50, damage) && _phase)
                {
                    _phase = false;
                    Talk(EMOTE_SUMMON);
                    Talk(SAY_SUMMON);
                    DoCast(me, SPELL_SUMMON_REINFORCEMENTS);
                }
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
                BossAI::JustSummoned(summon);
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
                        case EVENT_ARCANE_RESONANCE:
                            Talk(SAY_ARCANE_RESONANCE);
                            DoCastVictim(SPELL_ARCANE_RESONANCE, true);
                            events.ScheduleEvent(EVENT_ARCANE_RESONANCE, 42700);
                            break;
                        case EVENT_ARCANE_DEVASTATION:
                            Talk(SAY_ARCANE_DEVASTATION);
                            DoCastVictim(SPELL_ARCANE_DEVASTATION, true);
                            events.ScheduleEvent(EVENT_ARCANE_DEVASTATION, std::rand() % 19200 + 11000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _phase;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_commander_sarannisAI(creature);
        }
};

Position const PosSummonReinforcements[4] =
{
    { 160.4483f, 287.6435f, -3.887904f, 2.3841f },
    { 153.4406f, 289.9929f, -4.736916f, 2.3841f },
    { 154.4137f, 292.8956f, -4.683603f, 2.3841f },
    { 157.1544f, 294.2599f, -4.726504f, 2.3841f }
};

enum Creatures
{
    NPC_SUMMONED_BLOODWARDER_MENDER     = 20083,
    NPC_SUMMONED_BLOODWARDER_RESERVIST  = 20078
};

class spell_commander_sarannis_summon_reinforcements : public SpellScriptLoader
{
    public:
        spell_commander_sarannis_summon_reinforcements() : SpellScriptLoader("spell_commander_sarannis_summon_reinforcements") { }

        class spell_commander_sarannis_summon_reinforcements_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_commander_sarannis_summon_reinforcements_SpellScript);

            void HandleCast(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->SummonCreature(NPC_SUMMONED_BLOODWARDER_MENDER, PosSummonReinforcements[0], TempSummonType::TEMPSUMMON_CORPSE_DESPAWN);
                GetCaster()->SummonCreature(NPC_SUMMONED_BLOODWARDER_RESERVIST, PosSummonReinforcements[1], TempSummonType::TEMPSUMMON_CORPSE_DESPAWN);
                GetCaster()->SummonCreature(NPC_SUMMONED_BLOODWARDER_RESERVIST, PosSummonReinforcements[2], TempSummonType::TEMPSUMMON_CORPSE_DESPAWN);
                if (GetCaster()->GetMap()->IsHeroic())
                    GetCaster()->SummonCreature(NPC_SUMMONED_BLOODWARDER_RESERVIST, PosSummonReinforcements[3], TempSummonType::TEMPSUMMON_CORPSE_DESPAWN);
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_commander_sarannis_summon_reinforcements_SpellScript::HandleCast, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_commander_sarannis_summon_reinforcements_SpellScript();
        }
};

void AddSC_boss_commander_sarannis()
{
    new boss_commander_sarannis();
    new spell_commander_sarannis_summon_reinforcements();
}
