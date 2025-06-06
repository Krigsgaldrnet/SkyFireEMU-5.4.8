/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "Player.h"
#include "halls_of_origination.h"

enum Texts
{
    SAY_AGGRO                    = 0,
    SAY_SHIELD                   = 1,
    EMOTE_SHIELD                 = 2,
    EMOTE_UNSHIELD               = 3,
    SAY_KILL                     = 4,
    SAY_DEATH                    = 5
};

enum Events
{
    EVENT_DIVINE_RECKONING       = 1,
    EVENT_BURNING_LIGHT          = 2,
    EVENT_SEAR                   = 3,
};

enum Spells
{
    SPELL_DIVINE_RECKONING       = 75592,
    SPELL_BURNING_LIGHT          = 75115,
    SPELL_REVERBERATING_HYMN     = 75322,
    SPELL_SHIELD_OF_LIGHT        = 74938,

    SPELL_ACTIVATE_BEACONS       = 76599,
    SPELL_TELEPORT               = 74969,

    SPELL_SHIELD_VISUAL_RIGHT    = 83698,
    SPELL_BEAM_OF_LIGHT_RIGHT    = 76573,

    SPELL_SHIELD_VISUAL_LEFT     = 83697,
    SPELL_BEAM_OF_LIGHT_LEFT     = 74930,

    SPELL_SEARING_LIGHT          = 75194,
};

enum Phases
{
    PHASE_SHIELDED               = 0,
    PHASE_FIRST_SHIELD           = 1, // Ready to be shielded for the first time
    PHASE_SECOND_SHIELD          = 2, // First shield already happened, ready to be shielded a second time
    PHASE_FINAL                  = 3  // Already shielded twice, ready to finish the encounter normally.
};

enum Actions
{
    ACTION_DISABLE_BEACON,
};

class boss_temple_guardian_anhuur : public CreatureScript
{
public:
    boss_temple_guardian_anhuur() : CreatureScript("boss_temple_guardian_anhuur") { }

    struct boss_temple_guardian_anhuurAI : public BossAI
    {
        boss_temple_guardian_anhuurAI(Creature* creature) : BossAI(creature, DATA_TEMPLE_GUARDIAN_ANHUUR) { }

        void CleanStalkers()
        {
            std::list<Creature*> stalkers;
            GetCreatureListWithEntryInGrid(stalkers, me, NPC_CAVE_IN_STALKER, 100.0f);
            for (std::list<Creature*>::iterator itr = stalkers.begin(); itr != stalkers.end(); ++itr)
            {
                (*itr)->RemoveAurasDueToSpell(SPELL_BEAM_OF_LIGHT_RIGHT);
                (*itr)->RemoveAurasDueToSpell(SPELL_BEAM_OF_LIGHT_LEFT);
            }
        }

        void Reset() OVERRIDE
        {
            _phase = PHASE_FIRST_SHIELD;
            _oldPhase = PHASE_FIRST_SHIELD;
            _beacons = 0;
            _Reset();
            CleanStalkers();
            me->RemoveAurasDueToSpell(SPELL_SHIELD_OF_LIGHT);
            events.ScheduleEvent(EVENT_DIVINE_RECKONING, std::rand() % 12000 + 10000);
            events.ScheduleEvent(EVENT_BURNING_LIGHT, 12000);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage) OVERRIDE
        {
            if ((me->HealthBelowPctDamaged(66, damage) && _phase == PHASE_FIRST_SHIELD) ||
                (me->HealthBelowPctDamaged(33, damage) && _phase == PHASE_SECOND_SHIELD))
            {
                _beacons = 2;
                _phase++; // Increase the phase
                _oldPhase = _phase;

                _phase = PHASE_SHIELDED;

                me->InterruptNonMeleeSpells(true);
                me->AttackStop();
                DoCast(me, SPELL_TELEPORT);

                DoCast(me, SPELL_SHIELD_OF_LIGHT);
                me->SetFlag(UNIT_FIELD_FLAGS, uint32(UNIT_FLAG_UNK_31));

                DoCastAOE(SPELL_ACTIVATE_BEACONS);

                std::list<Creature*> stalkers;
                GameObject* door = ObjectAccessor::GetGameObject(*me, instance->GetData64(DATA_ANHUUR_DOOR));
                GetCreatureListWithEntryInGrid(stalkers, me, NPC_CAVE_IN_STALKER, 100.0f);

                stalkers.remove_if(Skyfire::HeightDifferenceCheck(door, 0.0f, false)); // Target only the bottom ones
                for (std::list<Creature*>::iterator itr = stalkers.begin(); itr != stalkers.end(); ++itr)
                {
                    if ((*itr)->GetPositionX() > door->GetPositionX())
                    {
                        (*itr)->CastSpell((*itr), SPELL_SHIELD_VISUAL_LEFT, true);
                        (*itr)->CastSpell((*itr), SPELL_BEAM_OF_LIGHT_LEFT, true);
                    }
                    else
                    {
                        (*itr)->CastSpell((*itr), SPELL_SHIELD_VISUAL_RIGHT, true);
                        (*itr)->CastSpell((*itr), SPELL_BEAM_OF_LIGHT_RIGHT, true);
                    }
                }

                DoCast(me, SPELL_REVERBERATING_HYMN);

                Talk(EMOTE_SHIELD);
                Talk(SAY_SHIELD);
            }
        }

        void DoAction(int32 action) OVERRIDE
        {
            if (action == ACTION_DISABLE_BEACON)
            {
                --_beacons;
                if (!_beacons)
                {
                    me->RemoveAurasDueToSpell(SPELL_SHIELD_OF_LIGHT);
                    Talk(EMOTE_UNSHIELD);
                    _phase = _oldPhase;
                }
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
            Talk(SAY_AGGRO);
            _EnterCombat();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            Talk(SAY_DEATH);
            _JustDied();
        }

        void KilledUnit(Unit* victim) OVERRIDE
        {
            if (victim->GetTypeId() == TypeID::TYPEID_PLAYER)
                Talk(SAY_KILL);
        }

        void JustReachedHome() OVERRIDE
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            _JustReachedHome();
            instance->SetBossState(DATA_TEMPLE_GUARDIAN_ANHUUR, FAIL);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || !CheckInRoom() || me->GetCurrentSpell(CURRENT_CHANNELED_SPELL) || _phase == PHASE_SHIELDED)
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_DIVINE_RECKONING:
                        DoCastVictim(SPELL_DIVINE_RECKONING);
                        events.ScheduleEvent(EVENT_DIVINE_RECKONING, std::rand() % 12000 + 10000);
                        break;
                    case EVENT_BURNING_LIGHT:
                    {
                        Unit* unit = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me));
                        if (!unit)
                            unit = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true);
                        DoCast(unit, SPELL_BURNING_LIGHT);
                        events.ScheduleEvent(EVENT_SEAR, 2000);
                        events.ScheduleEvent(EVENT_BURNING_LIGHT, 12000);
                        break;
                    }
                    case EVENT_SEAR:
                    {
                        Unit* target = me->FindNearestCreature(NPC_SEARING_LIGHT, 100.0f);
                        if (!target)
                            break;

                        std::list<Creature*> stalkers;
                        GetCreatureListWithEntryInGrid(stalkers, me, NPC_CAVE_IN_STALKER, 100.0f);
                        stalkers.remove_if(Skyfire::HeightDifferenceCheck(ObjectAccessor::GetGameObject(*me, instance->GetData64(DATA_ANHUUR_DOOR)), 5.0f, true));

                        if (stalkers.empty())
                            break;

                        stalkers.sort(Skyfire::ObjectDistanceOrderPred(target));

                        // Get the closest statue face (any of its eyes)
                        Creature* eye1 = stalkers.front();
                        stalkers.remove(eye1); // Remove the eye.
                        stalkers.sort(Skyfire::ObjectDistanceOrderPred(eye1)); // Find the second eye.
                        Creature* eye2 = stalkers.front();

                        eye1->CastSpell(eye1, SPELL_SEARING_LIGHT, true);
                        eye2->CastSpell(eye2, SPELL_SEARING_LIGHT, true);
                        break;
                    }
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        uint8 _phase;
        uint8 _oldPhase;
        uint8 _beacons;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetHallsOfOriginationAI<boss_temple_guardian_anhuurAI>(creature);
    }
};

class spell_anhuur_shield_of_light : public SpellScriptLoader
{
    public:
        spell_anhuur_shield_of_light() : SpellScriptLoader("spell_anhuur_shield_of_light") { }

        class spell_anhuur_shield_of_light_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_anhuur_shield_of_light_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (InstanceMap* instance = GetCaster()->GetMap()->ToInstanceMap())
                {
                    if (InstanceScript* const script = instance->GetInstanceScript())
                    {
                        if (GameObject* go = ObjectAccessor::GetGameObject(*GetCaster(), script->GetData64(DATA_ANHUUR_DOOR)))
                        {
                            targets.remove_if(Skyfire::HeightDifferenceCheck(go, 5.0f, false));
                            targets.remove(GetCaster());
                            targets.sort(Skyfire::ObjectDistanceOrderPred(GetCaster()));
                            targets.resize(2);
                        }
                    }
                }
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_anhuur_shield_of_light_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_anhuur_shield_of_light_SpellScript();
        }
};

class spell_anhuur_disable_beacon_beams : public SpellScriptLoader
{
    public:
        spell_anhuur_disable_beacon_beams() : SpellScriptLoader("spell_anhuur_disable_beacon_beams") { }

        class spell_anhuur_disable_beacon_beams_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_anhuur_disable_beacon_beams_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                GetHitUnit()->RemoveAurasDueToSpell(GetEffectValue());
            }

            void Notify(SpellEffIndex /*index*/)
            {
                if (InstanceMap* instance = GetCaster()->GetMap()->ToInstanceMap())
                    if (InstanceScript* const script = instance->GetInstanceScript())
                        if (Creature* anhuur = instance->GetCreature(script->GetData64(DATA_ANHUUR_GUID)))
                            anhuur->AI()->DoAction(ACTION_DISABLE_BEACON);
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_anhuur_disable_beacon_beams_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnEffectHit += SpellEffectFn(spell_anhuur_disable_beacon_beams_SpellScript::Notify, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_anhuur_disable_beacon_beams_SpellScript();
        }
};

class spell_anhuur_activate_beacons : public SpellScriptLoader
{
    public:
        spell_anhuur_activate_beacons() : SpellScriptLoader("spell_anhuur_activate_beacons") { }

        class spell_anhuur_activate_beacons_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_anhuur_activate_beacons_SpellScript);

            void Activate(SpellEffIndex index)
            {
                PreventHitDefaultEffect(index);
                GetHitGObj()->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_anhuur_activate_beacons_SpellScript::Activate, EFFECT_0, SPELL_EFFECT_ACTIVATE_OBJECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_anhuur_activate_beacons_SpellScript();
        }
};

class spell_anhuur_divine_reckoning : public SpellScriptLoader
{
public:
    spell_anhuur_divine_reckoning() : SpellScriptLoader("spell_anhuur_divine_reckoning") { }

    class spell_anhuur_divine_reckoning_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_anhuur_divine_reckoning_AuraScript);

        void OnPeriodic(AuraEffect const* aurEff)
        {
            if (Unit* caster = GetCaster())
            {
                CustomSpellValues values;
                values.AddSpellMod(SPELLVALUE_BASE_POINT0, aurEff->GetAmount());
                caster->CastCustomSpell(GetSpellInfo()->Effects[EFFECT_0].TriggerSpell, values, GetTarget());
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_anhuur_divine_reckoning_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_anhuur_divine_reckoning_AuraScript();
    }
};

void AddSC_boss_temple_guardian_anhuur()
{
    new boss_temple_guardian_anhuur();
    new spell_anhuur_shield_of_light();
    new spell_anhuur_disable_beacon_beams();
    new spell_anhuur_activate_beacons();
    new spell_anhuur_divine_reckoning();
}
