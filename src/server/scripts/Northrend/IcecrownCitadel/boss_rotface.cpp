/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "GridNotifiers.h"
#include "icecrown_citadel.h"

// KNOWN BUGS:
// ~ No Slime Spray animation directly at target spot

enum Texts
{
    SAY_PRECIOUS_DIES           = 0,
    SAY_AGGRO                   = 1,
    EMOTE_SLIME_SPRAY           = 2,
    SAY_SLIME_SPRAY             = 3,
    EMOTE_UNSTABLE_EXPLOSION    = 4,
    SAY_UNSTABLE_EXPLOSION      = 5,
    SAY_KILL                    = 6,
    SAY_BERSERK                 = 7,
    SAY_DEATH                   = 8,
    EMOTE_MUTATED_INFECTION     = 9,

    EMOTE_PRECIOUS_ZOMBIES      = 0,
};

enum Spells
{
    // Rotface
    SPELL_SLIME_SPRAY                       = 69508,    // every 20 seconds
    SPELL_MUTATED_INFECTION                 = 69674,    // hastens every 1:30
    SPELL_VILE_GAS_TRIGGER_SUMMON           = 72287,

    // Oozes
    SPELL_LITTLE_OOZE_COMBINE               = 69537,    // combine 2 Small Oozes
    SPELL_LARGE_OOZE_COMBINE                = 69552,    // combine 2 Large Oozes
    SPELL_LARGE_OOZE_BUFF_COMBINE           = 69611,    // combine Large and Small Ooze
    SPELL_OOZE_MERGE                        = 69889,    // 2 Small Oozes summon a Large Ooze
    SPELL_WEAK_RADIATING_OOZE               = 69750,    // passive damage aura - small
    SPELL_RADIATING_OOZE                    = 69760,    // passive damage aura - large
    SPELL_UNSTABLE_OOZE                     = 69558,    // damage boost and counter for explosion
    SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC = 70001,    // prevents getting hit by infection
    SPELL_UNSTABLE_OOZE_EXPLOSION           = 69839,
    SPELL_STICKY_OOZE                       = 69774,
    SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER   = 69832,

    // Precious
    SPELL_MORTAL_WOUND                      = 71127,
    SPELL_DECIMATE                          = 71123,
    SPELL_AWAKEN_PLAGUED_ZOMBIES            = 71159,

    // Professor Putricide
    SPELL_VILE_GAS_H                        = 72272,
    SPELL_VILE_GAS_TRIGGER                  = 72285,
};

#define MUTATED_INFECTION RAID_MODE<int32>(69674, 71224, 73022, 73023)

enum Events
{
    // Rotface
    EVENT_SLIME_SPRAY       = 1,
    EVENT_HASTEN_INFECTIONS = 2,
    EVENT_MUTATED_INFECTION = 3,
    EVENT_VILE_GAS          = 4,

    // Precious
    EVENT_DECIMATE          = 5,
    EVENT_MORTAL_WOUND      = 6,
    EVENT_SUMMON_ZOMBIES    = 7,

    EVENT_STICKY_OOZE       = 8,
};

class boss_rotface : public CreatureScript
{
    public:
        boss_rotface() : CreatureScript("boss_rotface") { }

        struct boss_rotfaceAI : public BossAI
        {
            boss_rotfaceAI(Creature* creature) : BossAI(creature, DATA_ROTFACE)
            {
                infectionStage = 0;
                infectionCooldown = 14000;
            }

            void Reset() OVERRIDE
            {
                _Reset();
                events.ScheduleEvent(EVENT_SLIME_SPRAY, 20000);
                events.ScheduleEvent(EVENT_HASTEN_INFECTIONS, 90000);
                events.ScheduleEvent(EVENT_MUTATED_INFECTION, 14000);
                if (IsHeroic())
                    events.ScheduleEvent(EVENT_VILE_GAS, std::rand() % 27000 + 22000);

                infectionStage = 0;
                infectionCooldown = 14000;
            }

            void EnterCombat(Unit* who) OVERRIDE
            {
                if (!instance->CheckRequiredBosses(DATA_ROTFACE, who->ToPlayer()))
                {
                    EnterEvadeMode();
                    instance->DoCastSpellOnPlayers(LIGHT_S_HAMMER_TELEPORT);
                    return;
                }

                me->setActive(true);
                Talk(SAY_AGGRO);
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->DoAction(ACTION_ROTFACE_COMBAT);

                DoZoneInCombat();
                DoCast(me, SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                instance->DoRemoveAurasDueToSpellOnPlayers(MUTATED_INFECTION);
                _JustDied();
                Talk(SAY_DEATH);
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->DoAction(ACTION_ROTFACE_DEATH);
            }

            void JustReachedHome() OVERRIDE
            {
                _JustReachedHome();
                instance->SetBossState(DATA_ROTFACE, FAIL);
                instance->SetData(DATA_OOZE_DANCE_ACHIEVEMENT, uint32(true));   // reset
            }

            void KilledUnit(Unit* victim) OVERRIDE
            {
                if (victim->GetTypeId() == TypeID::TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void EnterEvadeMode() OVERRIDE
            {
                ScriptedAI::EnterEvadeMode();
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->EnterEvadeMode();
            }

            void SpellHitTarget(Unit* /*target*/, SpellInfo const* spell) OVERRIDE
            {
                if (spell->Id == SPELL_SLIME_SPRAY)
                    Talk(SAY_SLIME_SPRAY);
            }

            void MoveInLineOfSight(Unit* /*who*/) OVERRIDE
            {
                // don't enter combat
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
                if (summon->GetEntry() == NPC_VILE_GAS_STALKER)
                    if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                        professor->CastSpell(summon, SPELL_VILE_GAS_H, true);
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim() || !CheckInRoom())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SLIME_SPRAY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            {
                                DoSummon(NPC_OOZE_SPRAY_STALKER, *target, 8000, TempSummonType::TEMPSUMMON_TIMED_DESPAWN);
                                Talk(EMOTE_SLIME_SPRAY);
                                DoCast(me, SPELL_SLIME_SPRAY);
                            }
                            events.ScheduleEvent(EVENT_SLIME_SPRAY, 20000);
                            break;
                        case EVENT_HASTEN_INFECTIONS:
                            if (infectionStage++ < 4)
                            {
                                infectionCooldown -= 2000;
                                events.ScheduleEvent(EVENT_HASTEN_INFECTIONS, 90000);
                            }
                            break;
                        case EVENT_MUTATED_INFECTION:
                            me->CastCustomSpell(SPELL_MUTATED_INFECTION, SPELLVALUE_MAX_TARGETS, 1, NULL, false);
                            events.ScheduleEvent(EVENT_MUTATED_INFECTION, infectionCooldown);
                            break;
                        case EVENT_VILE_GAS:
                            DoCastAOE(SPELL_VILE_GAS_TRIGGER);
                            events.ScheduleEvent(EVENT_VILE_GAS, std::rand() % 35000 + 30000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint32 infectionCooldown;
            uint32 infectionStage;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetIcecrownCitadelAI<boss_rotfaceAI>(creature);
        }
};

class npc_little_ooze : public CreatureScript
{
    public:
        npc_little_ooze() : CreatureScript("npc_little_ooze") { }

        struct npc_little_oozeAI : public ScriptedAI
        {
            npc_little_oozeAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void IsSummonedBy(Unit* summoner) OVERRIDE
            {
                DoCast(me, SPELL_LITTLE_OOZE_COMBINE, true);
                DoCast(me, SPELL_WEAK_RADIATING_OOZE, true);
                DoCast(me, SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);
                events.ScheduleEvent(EVENT_STICKY_OOZE, 5000);
                me->AddThreat(summoner, 500000.0f);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                me->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (events.ExecuteEvent() == EVENT_STICKY_OOZE)
                {
                    DoCastVictim(SPELL_STICKY_OOZE);
                    events.ScheduleEvent(EVENT_STICKY_OOZE, 15000);
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetIcecrownCitadelAI<npc_little_oozeAI>(creature);
        }
};

class npc_big_ooze : public CreatureScript
{
    public:
        npc_big_ooze() : CreatureScript("npc_big_ooze") { }

        struct npc_big_oozeAI : public ScriptedAI
        {
            npc_big_oozeAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript())
            {
            }

            void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
            {
                DoCast(me, SPELL_LARGE_OOZE_COMBINE, true);
                DoCast(me, SPELL_LARGE_OOZE_BUFF_COMBINE, true);
                DoCast(me, SPELL_RADIATING_OOZE, true);
                DoCast(me, SPELL_UNSTABLE_OOZE, true);
                DoCast(me, SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);
                events.ScheduleEvent(EVENT_STICKY_OOZE, 5000);
                // register in Rotface's summons - not summoned with Rotface as owner
                if (Creature* rotface = Unit::GetCreature(*me, instance->GetData64(DATA_ROTFACE)))
                    rotface->AI()->JustSummoned(me);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                if (Creature* rotface = Unit::GetCreature(*me, instance->GetData64(DATA_ROTFACE)))
                    rotface->AI()->SummonedCreatureDespawn(me);
                me->DespawnOrUnsummon();
            }

            void DoAction(int32 action) OVERRIDE
            {
                if (action == EVENT_STICKY_OOZE)
                    events.CancelEvent(EVENT_STICKY_OOZE);
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
                        case EVENT_STICKY_OOZE:
                            DoCastVictim(SPELL_STICKY_OOZE);
                            events.ScheduleEvent(EVENT_STICKY_OOZE, 15000);
                        default:
                            break;
                    }
                }

                if (me->IsVisible())
                    DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetIcecrownCitadelAI<npc_big_oozeAI>(creature);
        }
};

class npc_precious_icc : public CreatureScript
{
    public:
        npc_precious_icc() : CreatureScript("npc_precious_icc") { }

        struct npc_precious_iccAI : public ScriptedAI
        {
            npc_precious_iccAI(Creature* creature) : ScriptedAI(creature), _summons(me)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() OVERRIDE
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_DECIMATE, std::rand() % 25000 + 20000);
                _events.ScheduleEvent(EVENT_MORTAL_WOUND, std::rand() % 7000 + 3000);
                _events.ScheduleEvent(EVENT_SUMMON_ZOMBIES, std::rand() % 22000 + 20000);
                _summons.DespawnAll();
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
                _summons.Summon(summon);
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    summon->AI()->AttackStart(target);
            }

            void SummonedCreatureDespawn(Creature* summon) OVERRIDE
            {
                _summons.Despawn(summon);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _summons.DespawnAll();
                if (Creature* rotface = Unit::GetCreature(*me, _instance->GetData64(DATA_ROTFACE)))
                    if (rotface->IsAlive())
                        rotface->AI()->Talk(SAY_PRECIOUS_DIES);
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DECIMATE:
                            DoCastVictim(SPELL_DECIMATE);
                            _events.ScheduleEvent(EVENT_DECIMATE, std::rand() % 25000 + 20000);
                            break;
                        case EVENT_MORTAL_WOUND:
                            DoCastVictim(SPELL_MORTAL_WOUND);
                            _events.ScheduleEvent(EVENT_MORTAL_WOUND, std::rand() % 12500 + 10000);
                            break;
                        case EVENT_SUMMON_ZOMBIES:
                            Talk(EMOTE_PRECIOUS_ZOMBIES);
                            for (uint32 i = 0; i < 11; ++i)
                                DoCast(me, SPELL_AWAKEN_PLAGUED_ZOMBIES, false);
                            _events.ScheduleEvent(EVENT_SUMMON_ZOMBIES, std::rand() % 22000 + 20000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            SummonList _summons;
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetIcecrownCitadelAI<npc_precious_iccAI>(creature);
        }
};

class spell_rotface_ooze_flood : public SpellScriptLoader
{
    public:
        spell_rotface_ooze_flood() : SpellScriptLoader("spell_rotface_ooze_flood") { }

        class spell_rotface_ooze_flood_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_ooze_flood_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!GetHitUnit())
                    return;

                std::list<Creature*> triggers;
                GetHitUnit()->GetCreatureListWithEntryInGrid(triggers, GetHitUnit()->GetEntry(), 12.5f);

                if (triggers.empty())
                    return;

                triggers.sort(Skyfire::ObjectDistanceOrderPred(GetHitUnit()));
                GetHitUnit()->CastSpell(triggers.back(), uint32(GetEffectValue()), false, NULL, NULL, GetOriginalCaster() ? GetOriginalCaster()->GetGUID() : 0);
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // get 2 targets except 2 nearest
                targets.sort(Skyfire::ObjectDistanceOrderPred(GetCaster()));

                // .resize() runs pop_back();
                if (targets.size() > 4)
                    targets.resize(4);

                while (targets.size() > 2)
                    targets.pop_front();
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_ooze_flood_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_ooze_flood_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_rotface_ooze_flood_SpellScript();
        }
};

class spell_rotface_mutated_infection : public SpellScriptLoader
{
    public:
        spell_rotface_mutated_infection() : SpellScriptLoader("spell_rotface_mutated_infection") { }

        class spell_rotface_mutated_infection_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_mutated_infection_SpellScript);

            bool Load() OVERRIDE
            {
                _target = NULL;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // remove targets with this aura already
                // tank is not on this list
                targets.remove_if(Skyfire::UnitAuraCheck(true, GetSpellInfo()->Id));
                if (targets.empty())
                    return;

                WorldObject* target = Skyfire::Containers::SelectRandomContainerElement(targets);
                targets.clear();
                targets.push_back(target);
                _target = target;
            }

            void ReplaceTargets(std::list<WorldObject*>& targets)
            {
                targets.clear();
                if (_target)
                    targets.push_back(_target);
            }

            void NotifyTargets()
            {
                if (Creature* caster = GetCaster()->ToCreature())
                    if (Unit* target = GetHitUnit())
                        caster->AI()->Talk(EMOTE_MUTATED_INFECTION, target);
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_mutated_infection_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_mutated_infection_SpellScript::ReplaceTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_mutated_infection_SpellScript::ReplaceTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
                AfterHit += SpellHitFn(spell_rotface_mutated_infection_SpellScript::NotifyTargets);
            }

            WorldObject* _target;
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_rotface_mutated_infection_SpellScript();
        }
};

class spell_rotface_little_ooze_combine : public SpellScriptLoader
{
    public:
        spell_rotface_little_ooze_combine() : SpellScriptLoader("spell_rotface_little_ooze_combine") { }

        class spell_rotface_little_ooze_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_little_ooze_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitCreature() && GetHitUnit()->IsAlive()))
                    return;

                GetCaster()->RemoveAurasDueToSpell(SPELL_LITTLE_OOZE_COMBINE);
                GetHitCreature()->RemoveAurasDueToSpell(SPELL_LITTLE_OOZE_COMBINE);
                GetHitCreature()->CastSpell(GetCaster(), SPELL_OOZE_MERGE, true);
                GetHitCreature()->DespawnOrUnsummon();
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_little_ooze_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_rotface_little_ooze_combine_SpellScript();
        }
};

class spell_rotface_large_ooze_combine : public SpellScriptLoader
{
    public:
        spell_rotface_large_ooze_combine() : SpellScriptLoader("spell_rotface_large_ooze_combine") { }

        class spell_rotface_large_ooze_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_large_ooze_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitCreature() && GetHitCreature()->IsAlive()))
                    return;

                if (Aura* unstable = GetCaster()->GetAura(SPELL_UNSTABLE_OOZE))
                {
                    if (Aura* targetAura = GetHitCreature()->GetAura(SPELL_UNSTABLE_OOZE))
                        unstable->ModStackAmount(targetAura->GetStackAmount());
                    else
                        unstable->ModStackAmount(1);

                    // no idea why, but this does not trigger explosion on retail (only small+large do)
                }

                // just for safety
                GetHitCreature()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
                GetHitCreature()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);
                GetHitCreature()->DespawnOrUnsummon();
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_large_ooze_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_rotface_large_ooze_combine_SpellScript();
        }
};

class spell_rotface_large_ooze_buff_combine : public SpellScriptLoader
{
    public:
        spell_rotface_large_ooze_buff_combine() : SpellScriptLoader("spell_rotface_large_ooze_buff_combine") { }

        class spell_rotface_large_ooze_buff_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_large_ooze_buff_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitCreature() && GetHitCreature()->IsAlive()))
                    return;

                if (Aura* unstable = GetCaster()->GetAura(SPELL_UNSTABLE_OOZE))
                {
                    uint8 newStack = uint8(unstable->GetStackAmount()+1);
                    unstable->SetStackAmount(newStack);

                    // explode!
                    if (newStack >= 5)
                    {
                        GetCaster()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
                        GetCaster()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);
                        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                            if (Creature* rotface = Unit::GetCreature(*GetCaster(), instance->GetData64(DATA_ROTFACE)))
                                if (rotface->IsAlive())
                                {
                                    rotface->AI()->Talk(EMOTE_UNSTABLE_EXPLOSION);
                                    rotface->AI()->Talk(SAY_UNSTABLE_EXPLOSION);
                                }

                        if (Creature* cre = GetCaster()->ToCreature())
                            cre->AI()->DoAction(EVENT_STICKY_OOZE);
                        GetCaster()->CastSpell(GetCaster(), SPELL_UNSTABLE_OOZE_EXPLOSION, false, NULL, NULL, GetCaster()->GetGUID());
                        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                            instance->SetData(DATA_OOZE_DANCE_ACHIEVEMENT, uint32(false));
                    }
                }

                GetHitCreature()->DespawnOrUnsummon();
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_large_ooze_buff_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_rotface_large_ooze_buff_combine_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion_init : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion_init() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion_init") { }

        class spell_rotface_unstable_ooze_explosion_init_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_unstable_ooze_explosion_init_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER))
                    return false;
                return true;
            }

            void HandleCast(SpellEffIndex effIndex)
            {
                PreventHitEffect(effIndex);
                if (!GetHitUnit())
                    return;

                float x, y, z;
                GetHitUnit()->GetPosition(x, y, z);
                Creature* dummy = GetCaster()->SummonCreature(NPC_UNSTABLE_EXPLOSION_STALKER, x, y, z, 0.0f, TempSummonType::TEMPSUMMON_TIMED_DESPAWN, 60000);
                GetCaster()->CastSpell(dummy, SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER, true);
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_unstable_ooze_explosion_init_SpellScript::HandleCast, EFFECT_0, SPELL_EFFECT_FORCE_CAST);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_rotface_unstable_ooze_explosion_init_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion") { }

        class spell_rotface_unstable_ooze_explosion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_unstable_ooze_explosion_SpellScript);

            void CheckTarget(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(EFFECT_0);
                if (!GetExplTargetDest())
                    return;

                uint32 triggered_spell_id = GetSpellInfo()->Effects[effIndex].TriggerSpell;

                float x, y, z;
                GetExplTargetDest()->GetPosition(x, y, z);
                // let Rotface handle the cast - caster dies before this executes
                if (InstanceScript* script = GetCaster()->GetInstanceScript())
                    if (Creature* rotface = script->instance->GetCreature(script->GetData64(DATA_ROTFACE)))
                        rotface->CastSpell(x, y, z, triggered_spell_id, true, NULL, NULL, GetCaster()->GetGUID());
            }

            void Register() OVERRIDE
            {
                OnEffectHit += SpellEffectFn(spell_rotface_unstable_ooze_explosion_SpellScript::CheckTarget, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_rotface_unstable_ooze_explosion_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion_suicide : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion_suicide() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion_suicide") { }

        class spell_rotface_unstable_ooze_explosion_suicide_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rotface_unstable_ooze_explosion_suicide_AuraScript);

            void DespawnSelf(AuraEffect const* /*aurEff*/)
            {
                PreventDefaultAction();
                Unit* target = GetTarget();
                if (target->GetTypeId() != TypeID::TYPEID_UNIT)
                    return;

                target->RemoveAllAuras();
                target->SetVisible(false);
                target->ToCreature()->DespawnOrUnsummon(60000);
            }

            void Register() OVERRIDE
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rotface_unstable_ooze_explosion_suicide_AuraScript::DespawnSelf, EFFECT_2, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_rotface_unstable_ooze_explosion_suicide_AuraScript();
        }
};

class spell_rotface_vile_gas_trigger : public SpellScriptLoader
{
    public:
        spell_rotface_vile_gas_trigger() : SpellScriptLoader("spell_rotface_vile_gas_trigger") { }

        class spell_rotface_vile_gas_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_vile_gas_trigger_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.sort(Skyfire::ObjectDistanceOrderPred(GetCaster()));
                if (targets.empty())
                    return;

                std::list<WorldObject*> ranged, melee;
                std::list<WorldObject*>::iterator itr = targets.begin();
                while (itr != targets.end() && (*itr)->GetDistance(GetCaster()) < 5.0f)
                {
                    melee.push_back((*itr)->ToUnit());
                    ++itr;
                }

                while (itr != targets.end())
                {
                    ranged.push_back((*itr)->ToUnit());
                    ++itr;
                }

                uint32 minTargets = GetCaster()->GetMap()->Is25ManRaid() ? 8 : 3;
                while (ranged.size() < minTargets)
                {
                    if (melee.empty())
                        break;

                    WorldObject* target = Skyfire::Containers::SelectRandomContainerElement(melee);
                    ranged.push_back(target);
                    melee.remove(target);
                }

                if (!ranged.empty())
                    Skyfire::Containers::RandomResizeList(ranged, GetCaster()->GetMap()->Is25ManRaid() ? 3 : 1);

                targets.swap(ranged);
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetCaster()->CastSpell(GetHitUnit(), SPELL_VILE_GAS_TRIGGER_SUMMON);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_vile_gas_trigger_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_rotface_vile_gas_trigger_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rotface_vile_gas_trigger_SpellScript();
        }
};

void AddSC_boss_rotface()
{
    new boss_rotface();
    new npc_little_ooze();
    new npc_big_ooze();
    new npc_precious_icc();
    new spell_rotface_ooze_flood();
    new spell_rotface_mutated_infection();
    new spell_rotface_little_ooze_combine();
    new spell_rotface_large_ooze_combine();
    new spell_rotface_large_ooze_buff_combine();
    new spell_rotface_unstable_ooze_explosion_init();
    new spell_rotface_unstable_ooze_explosion();
    new spell_rotface_unstable_ooze_explosion_suicide();
    new spell_rotface_vile_gas_trigger();
}
