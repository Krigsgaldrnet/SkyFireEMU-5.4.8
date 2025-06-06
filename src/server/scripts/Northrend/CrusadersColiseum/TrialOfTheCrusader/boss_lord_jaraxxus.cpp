/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "trial_of_the_crusader.h"

enum Yells
{
    SAY_INTRO               = 0,
    SAY_AGGRO               = 1,
    EMOTE_LEGION_FLAME      = 2,
    EMOTE_NETHER_PORTAL     = 3,
    SAY_MISTRESS_OF_PAIN    = 4,
    EMOTE_INCINERATE        = 5,
    SAY_INCINERATE          = 6,
    EMOTE_INFERNAL_ERUPTION = 7,
    SAY_INFERNAL_ERUPTION   = 8,
    SAY_KILL_PLAYER         = 9,
    SAY_DEATH               = 10,
    SAY_BERSERK             = 11
};

enum Summons
{
    NPC_LEGION_FLAME     = 34784,
    NPC_INFERNAL_VOLCANO = 34813,
    NPC_FEL_INFERNAL     = 34815, // immune to all CC on Heroic (stuns, banish, interrupt, etc)
    NPC_NETHER_PORTAL    = 34825,
    NPC_MISTRESS_OF_PAIN = 34826
};

enum BossSpells
{
    SPELL_LEGION_FLAME                = 66197, // player should run away from raid because he triggers Legion Flame
    SPELL_LEGION_FLAME_EFFECT         = 66201, // used by trigger npc
    SPELL_NETHER_POWER                = 66228, // +20% of spell damage per stack, stackable up to 5/10 times, must be dispelled/stealed
    SPELL_FEL_LIGHTING                = 66528, // jumps to nearby targets
    SPELL_FEL_FIREBALL                = 66532, // does heavy damage to the tank, interruptable
    SPELL_INCINERATE_FLESH            = 66237, // target must be healed or will trigger Burning Inferno
    SPELL_BURNING_INFERNO             = 66242, // triggered by Incinerate Flesh
    SPELL_INFERNAL_ERUPTION           = 66258, // summons Infernal Volcano
    SPELL_INFERNAL_ERUPTION_EFFECT    = 66252, // summons Felflame Infernal (3 at Normal and inifinity at Heroic)
    SPELL_NETHER_PORTAL               = 66269, // summons Nether Portal
    SPELL_NETHER_PORTAL_EFFECT        = 66263, // summons Mistress of Pain (1 at Normal and infinity at Heroic)

    SPELL_BERSERK                     = 64238, // unused

    // Mistress of Pain spells
    SPELL_SHIVAN_SLASH                  = 67098,
    SPELL_SPINNING_STRIKE               = 66283,
    SPELL_MISTRESS_KISS                 = 66336,
    SPELL_FEL_INFERNO                   = 67047,
    SPELL_FEL_STREAK                    = 66494,
    SPELL_LORD_HITTIN                   = 66326,   // special effect preventing more specific spells be cast on the same player within 10 seconds
    SPELL_MISTRESS_KISS_DAMAGE_SILENCE  = 66359
};

enum Events
{
    // Lord Jaraxxus
    EVENT_FEL_FIREBALL              = 1,
    EVENT_FEL_LIGHTNING             = 2,
    EVENT_INCINERATE_FLESH          = 3,
    EVENT_NETHER_POWER              = 4,
    EVENT_LEGION_FLAME              = 5,
    EVENT_SUMMONO_NETHER_PORTAL     = 6,
    EVENT_SUMMON_INFERNAL_ERUPTION  = 7,

    // Mistress of Pain
    EVENT_SHIVAN_SLASH              = 8,
    EVENT_SPINNING_STRIKE           = 9,
    EVENT_MISTRESS_KISS             = 10
};

class boss_jaraxxus : public CreatureScript
{
    public:
        boss_jaraxxus() : CreatureScript("boss_jaraxxus") { }

        struct boss_jaraxxusAI : public BossAI
        {
            boss_jaraxxusAI(Creature* creature) : BossAI(creature, BOSS_JARAXXUS)
            {
            }

            void Reset() OVERRIDE
            {
                _Reset();
                events.ScheduleEvent(EVENT_FEL_FIREBALL, 5*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FEL_LIGHTNING, std::rand() % (15 * IN_MILLISECONDS) + (10*IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_INCINERATE_FLESH, std::rand() % (25 * IN_MILLISECONDS) + (20 * IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_NETHER_POWER, 40*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LEGION_FLAME, 30*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SUMMONO_NETHER_PORTAL, 20*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SUMMON_INFERNAL_ERUPTION, 80*IN_MILLISECONDS);
            }

            void JustReachedHome() OVERRIDE
            {
                _JustReachedHome();
                if (instance)
                    instance->SetBossState(BOSS_JARAXXUS, FAIL);
                DoCast(me, SPELL_JARAXXUS_CHAINS);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void KilledUnit(Unit* who) OVERRIDE
            {
                if (who->GetTypeId() == TypeID::TYPEID_PLAYER)
                {
                    Talk(SAY_KILL_PLAYER);
                    if (instance)
                        instance->SetData(DATA_TRIBUTE_TO_IMMORTALITY_ELIGIBLE, 0);
                }
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
                Talk(SAY_DEATH);
            }

            void JustSummoned(Creature* summoned) OVERRIDE
            {
                summons.Summon(summoned);
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
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
                        case EVENT_FEL_FIREBALL:
                            DoCastVictim(SPELL_FEL_FIREBALL);
                            events.ScheduleEvent(EVENT_FEL_FIREBALL, std::rand() % (15 * IN_MILLISECONDS) + (10*IN_MILLISECONDS));
                            return;
                        case EVENT_FEL_LIGHTNING:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, -SPELL_LORD_HITTIN))
                                DoCast(target, SPELL_FEL_LIGHTING);
                            events.ScheduleEvent(EVENT_FEL_LIGHTNING, std::rand() % (15 * IN_MILLISECONDS) + (10*IN_MILLISECONDS));
                            return;
                        case EVENT_INCINERATE_FLESH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true, -SPELL_LORD_HITTIN))
                            {
                                Talk(EMOTE_INCINERATE, target);
                                Talk(SAY_INCINERATE);
                                DoCast(target, SPELL_INCINERATE_FLESH);
                            }
                            events.ScheduleEvent(EVENT_INCINERATE_FLESH, std::rand() % (25 * IN_MILLISECONDS) + (20 * IN_MILLISECONDS));
                            return;
                        case EVENT_NETHER_POWER:
                            me->CastCustomSpell(SPELL_NETHER_POWER, SPELLVALUE_AURA_STACK, RAID_MODE<uint32>(5, 10, 5, 10), me, true);
                            events.ScheduleEvent(EVENT_NETHER_POWER, 40*IN_MILLISECONDS);
                            return;
                        case EVENT_LEGION_FLAME:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true, -SPELL_LORD_HITTIN))
                            {
                                Talk(EMOTE_LEGION_FLAME, target);
                                DoCast(target, SPELL_LEGION_FLAME);
                            }
                            events.ScheduleEvent(EVENT_LEGION_FLAME, 30*IN_MILLISECONDS);
                            return;
                        case EVENT_SUMMONO_NETHER_PORTAL:
                            Talk(EMOTE_NETHER_PORTAL);
                            Talk(SAY_MISTRESS_OF_PAIN);
                            DoCast(SPELL_NETHER_PORTAL);
                            events.ScheduleEvent(EVENT_SUMMONO_NETHER_PORTAL, 2*MINUTE*IN_MILLISECONDS);
                            return;
                        case EVENT_SUMMON_INFERNAL_ERUPTION:
                            Talk(EMOTE_INFERNAL_ERUPTION);
                            Talk(SAY_INFERNAL_ERUPTION);
                            DoCast(SPELL_INFERNAL_ERUPTION);
                            events.ScheduleEvent(EVENT_SUMMON_INFERNAL_ERUPTION, 2*MINUTE*IN_MILLISECONDS);
                            return;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_jaraxxusAI(creature);
        }
};

class npc_legion_flame : public CreatureScript
{
    public:
        npc_legion_flame() : CreatureScript("npc_legion_flame") { }

        struct npc_legion_flameAI : public ScriptedAI
        {
            npc_legion_flameAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                _instance = creature->GetInstanceScript();
            }

            void Reset() OVERRIDE
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetInCombatWithZone();
                DoCast(SPELL_LEGION_FLAME_EFFECT);
            }

            void UpdateAI(uint32 /*diff*/) OVERRIDE
            {
                UpdateVictim();
                if (_instance && _instance->GetBossState(BOSS_JARAXXUS) != IN_PROGRESS)
                    me->DespawnOrUnsummon();
            }
            private:
                InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_legion_flameAI(creature);
        }
};

class npc_infernal_volcano : public CreatureScript
{
    public:
        npc_infernal_volcano() : CreatureScript("npc_infernal_volcano") { }

        struct npc_infernal_volcanoAI : public ScriptedAI
        {
            npc_infernal_volcanoAI(Creature* creature) : ScriptedAI(creature), _summons(me)
            {
                SetCombatMovement(false);
            }

            void Reset() OVERRIDE
            {
                me->SetReactState(REACT_PASSIVE);

                if (!IsHeroic())
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PACIFIED);
                else
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PACIFIED);

                _summons.DespawnAll();
            }

            void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
            {
                DoCast(SPELL_INFERNAL_ERUPTION_EFFECT);
            }

            void JustSummoned(Creature* summoned) OVERRIDE
            {
                _summons.Summon(summoned);
                // makes immediate corpse despawn of summoned Felflame Infernals
                summoned->SetCorpseDelay(0);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                // used to despawn corpse immediately
                me->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 /*diff*/) OVERRIDE { }

            private:
                SummonList _summons;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_infernal_volcanoAI(creature);
        }
};

class npc_fel_infernal : public CreatureScript
{
    public:
        npc_fel_infernal() : CreatureScript("npc_fel_infernal") { }

        struct npc_fel_infernalAI : public ScriptedAI
        {
            npc_fel_infernalAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() OVERRIDE
            {
                _felStreakTimer = 30*IN_MILLISECONDS;
                me->SetInCombatWithZone();
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (_instance && _instance->GetBossState(BOSS_JARAXXUS) != IN_PROGRESS)
                {
                    me->DespawnOrUnsummon();
                    return;
                }

                if (!UpdateVictim())
                    return;

                if (_felStreakTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_FEL_STREAK);
                    _felStreakTimer = 30*IN_MILLISECONDS;
                }
                else
                    _felStreakTimer -= diff;

                DoMeleeAttackIfReady();
            }
            private:
                uint32 _felStreakTimer;
                InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_fel_infernalAI(creature);
        }
};

class npc_nether_portal : public CreatureScript
{
    public:
        npc_nether_portal() : CreatureScript("npc_nether_portal") { }

        struct npc_nether_portalAI : public ScriptedAI
        {
            npc_nether_portalAI(Creature* creature) : ScriptedAI(creature), _summons(me)
            {
            }

            void Reset() OVERRIDE
            {
                me->SetReactState(REACT_PASSIVE);

                if (!IsHeroic())
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PACIFIED);
                else
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PACIFIED);

                _summons.DespawnAll();
            }

            void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
            {
                DoCast(SPELL_NETHER_PORTAL_EFFECT);
            }

            void JustSummoned(Creature* summoned) OVERRIDE
            {
                _summons.Summon(summoned);
                // makes immediate corpse despawn of summoned Mistress of Pain
                summoned->SetCorpseDelay(0);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                // used to despawn corpse immediately
                me->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 /*diff*/) OVERRIDE { }

            private:
                SummonList _summons;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_nether_portalAI(creature);
        }
};

class npc_mistress_of_pain : public CreatureScript
{
    public:
        npc_mistress_of_pain() : CreatureScript("npc_mistress_of_pain") { }

        struct npc_mistress_of_painAI : public ScriptedAI
        {
            npc_mistress_of_painAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
                if (_instance)
                    _instance->SetData(DATA_MISTRESS_OF_PAIN_COUNT, INCREASE);
            }

            void Reset() OVERRIDE
            {
                _events.ScheduleEvent(EVENT_SHIVAN_SLASH, 30*IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_SPINNING_STRIKE, 30*IN_MILLISECONDS);
                if (IsHeroic())
                    _events.ScheduleEvent(EVENT_MISTRESS_KISS, 15*IN_MILLISECONDS);
                me->SetInCombatWithZone();
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                if (_instance)
                    _instance->SetData(DATA_MISTRESS_OF_PAIN_COUNT, DECREASE);
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (_instance && _instance->GetBossState(BOSS_JARAXXUS) != IN_PROGRESS)
                {
                    me->DespawnOrUnsummon();
                    return;
                }

                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHIVAN_SLASH:
                            DoCastVictim(SPELL_SHIVAN_SLASH);
                            _events.ScheduleEvent(EVENT_SHIVAN_SLASH, 30*IN_MILLISECONDS);
                            return;
                        case EVENT_SPINNING_STRIKE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_SPINNING_STRIKE);
                            _events.ScheduleEvent(EVENT_SPINNING_STRIKE, 30*IN_MILLISECONDS);
                            return;
                        case EVENT_MISTRESS_KISS:
                            DoCast(me, SPELL_MISTRESS_KISS);
                            _events.ScheduleEvent(EVENT_MISTRESS_KISS, 30*IN_MILLISECONDS);
                            return;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
            private:
                InstanceScript* _instance;
                EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_mistress_of_painAI(creature);
        }
};

class spell_mistress_kiss : public SpellScriptLoader
{
    public:
        spell_mistress_kiss() : SpellScriptLoader("spell_mistress_kiss") { }

        class spell_mistress_kiss_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mistress_kiss_AuraScript);

            bool Load() OVERRIDE
            {
                if (GetCaster())
                    if (sSpellMgr->GetSpellIdForDifficulty(SPELL_MISTRESS_KISS_DAMAGE_SILENCE, GetCaster()))
                        return true;
                return false;
            }

            void HandleDummyTick(AuraEffect const* /*aurEff*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();
                if (caster && target)
                {
                    if (target->HasUnitState(UNIT_STATE_CASTING))
                    {
                        caster->CastSpell(target, SPELL_MISTRESS_KISS_DAMAGE_SILENCE, true);
                        target->RemoveAurasDueToSpell(GetSpellInfo()->Id);
                    }
                }
            }

            void Register() OVERRIDE
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_mistress_kiss_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_mistress_kiss_AuraScript();
        }
};

class MistressKissTargetSelector
{
    public:
        MistressKissTargetSelector() { }

        bool operator()(WorldObject* unit) const
        {
            if (unit->GetTypeId() == TypeID::TYPEID_PLAYER)
                if (unit->ToPlayer()->getPowerType() == POWER_MANA)
                    return false;

            return true;
        }
};

class spell_mistress_kiss_area : public SpellScriptLoader
{
    public:
        spell_mistress_kiss_area() : SpellScriptLoader("spell_mistress_kiss_area") { }

        class spell_mistress_kiss_area_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mistress_kiss_area_SpellScript)

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // get a list of players with mana
                targets.remove_if(MistressKissTargetSelector());
                if (targets.empty())
                    return;

                WorldObject* target = Skyfire::Containers::SelectRandomContainerElement(targets);
                targets.clear();
                targets.push_back(target);
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), true);
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mistress_kiss_area_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_mistress_kiss_area_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_mistress_kiss_area_SpellScript();
        }
};

void AddSC_boss_jaraxxus()
{
    new boss_jaraxxus();
    new npc_legion_flame();
    new npc_infernal_volcano();
    new npc_fel_infernal();
    new npc_nether_portal();
    new npc_mistress_of_pain();

    new spell_mistress_kiss();
    new spell_mistress_kiss_area();
}
