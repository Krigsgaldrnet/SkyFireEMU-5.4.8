/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "temple_of_ahnqiraj.h"

enum Yells
{
    SAY_AGGRO                   = 0,
    SAY_SLAY                    = 1,
    SAY_SPLIT                   = 2,
    SAY_DEATH                   = 3
};

enum Spells
{
    SPELL_ARCANE_EXPLOSION      = 26192,
    SPELL_EARTH_SHOCK           = 26194,
    SPELL_TRUE_FULFILLMENT      = 785,
    SPELL_INITIALIZE_IMAGE      = 3730,
    SPELL_SUMMON_IMAGES         = 747
};

enum Events
{
    EVENT_ARCANE_EXPLOSION      = 1,
    EVENT_FULLFILMENT           = 2,
    EVENT_BLINK                 = 3,
    EVENT_EARTH_SHOCK           = 4
};

uint32 const BlinkSpells[3] = { 4801, 8195, 20449 };

class boss_skeram : public CreatureScript
{
    public:
        boss_skeram() : CreatureScript("boss_skeram") { }

        struct boss_skeramAI : public BossAI
        {
            boss_skeramAI(Creature* creature) : BossAI(creature, DATA_SKERAM) { }

            void Reset() OVERRIDE
            {
                _flag = 0;
                _hpct = 75.0f;
                me->SetVisible(true);
            }

            void KilledUnit(Unit* /*victim*/) OVERRIDE
            {
                Talk(SAY_SLAY);
            }

            void EnterEvadeMode() OVERRIDE
            {
                ScriptedAI::EnterEvadeMode();
                if (me->IsSummon())
                    ((TempSummon*)me)->UnSummon();
            }

            void JustSummoned(Creature* creature) OVERRIDE
            {
                // Shift the boss and images (Get it? *Shift*?)
                uint8 rand = 0;
                if (_flag != 0)
                {
                    while (_flag & (1 << rand))
                        rand = std::rand() % 2;
                    DoCast(me, BlinkSpells[rand]);
                    _flag |= (1 << rand);
                    _flag |= (1 << 7);
                }

                while (_flag & (1 << rand))
                    rand = std::rand() % 2;
                creature->CastSpell(creature, BlinkSpells[rand]);
                _flag |= (1 << rand);

                if (_flag & (1 << 7))
                    _flag = 0;

                if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM))
                    creature->AI()->AttackStart(Target);

                float ImageHealthPct;

                if (me->GetHealthPct() < 25.0f)
                    ImageHealthPct = 0.50f;
                else if (me->GetHealthPct() < 50.0f)
                    ImageHealthPct = 0.20f;
                else
                    ImageHealthPct = 0.10f;

                creature->SetMaxHealth(me->GetMaxHealth() * ImageHealthPct);
                creature->SetHealth(creature->GetMaxHealth() * (me->GetHealthPct() / 100.0f));
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                if (!me->IsSummon())
                    Talk(SAY_DEATH);
                else
                    me->RemoveCorpse();
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                _EnterCombat();
                events.Reset();

                events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, std::rand() % 12000 + 6000);
                events.ScheduleEvent(EVENT_FULLFILMENT, 15000);
                events.ScheduleEvent(EVENT_BLINK, std::rand() % 45000 + 30000);
                events.ScheduleEvent(EVENT_EARTH_SHOCK, 2000);

                Talk(SAY_AGGRO);
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ARCANE_EXPLOSION:
                            DoCastAOE(SPELL_ARCANE_EXPLOSION, true);
                            events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, std::rand() % 18000 + 8000);
                            break;
                        case EVENT_FULLFILMENT:
                            /// @todo For some weird reason boss does not cast this
                            // Spell actually works, tested in duel
                            DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true), SPELL_TRUE_FULFILLMENT, true);
                            events.ScheduleEvent(EVENT_FULLFILMENT, std::rand() % 30000 + 20000);
                            break;
                        case EVENT_BLINK:
                            DoCast(me, BlinkSpells[std::rand() % 2]);
                            DoResetThreat();
                            me->SetVisible(true);
                            events.ScheduleEvent(EVENT_BLINK, std::rand() % 30000 + 10000);
                            break;
                        case EVENT_EARTH_SHOCK:
                            DoCastVictim(SPELL_EARTH_SHOCK);
                            events.ScheduleEvent(EVENT_EARTH_SHOCK, 2000);
                            break;
                    }
                }

                if (!me->IsSummon() && me->GetHealthPct() < _hpct)
                {
                    DoCast(me, SPELL_SUMMON_IMAGES);
                    Talk(SAY_SPLIT);
                    _hpct -= 25.0f;
                    me->SetVisible(false);
                    events.RescheduleEvent(EVENT_BLINK, 2000);
                }

                if (me->IsWithinMeleeRange(me->GetVictim()))
                {
                    events.RescheduleEvent(EVENT_EARTH_SHOCK, 2000);
                    DoMeleeAttackIfReady();
                }
            }

        private:
            float _hpct;
            uint8 _flag;
        };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_skeramAI(creature);
    }
};

class PlayerOrPetCheck
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->GetTypeId() != TypeID::TYPEID_PLAYER)
                if (!object->ToCreature()->IsPet())
                    return true;

            return false;
        }
};

class spell_skeram_arcane_explosion : public SpellScriptLoader
{
    public:
        spell_skeram_arcane_explosion() : SpellScriptLoader("spell_skeram_arcane_explosion") { }

        class spell_skeram_arcane_explosion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_skeram_arcane_explosion_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(PlayerOrPetCheck());
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_skeram_arcane_explosion_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_skeram_arcane_explosion_SpellScript();
        }
};

void AddSC_boss_skeram()
{
    new boss_skeram();
    new spell_skeram_arcane_explosion();
}
