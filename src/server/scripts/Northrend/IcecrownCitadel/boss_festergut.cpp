/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "icecrown_citadel.h"

enum ScriptTexts
{
    SAY_STINKY_DEAD             = 0,
    SAY_AGGRO                   = 1,
    EMOTE_GAS_SPORE             = 2,
    EMOTE_WARN_GAS_SPORE        = 3,
    SAY_PUNGENT_BLIGHT          = 4,
    EMOTE_WARN_PUNGENT_BLIGHT   = 5,
    EMOTE_PUNGENT_BLIGHT        = 6,
    SAY_KILL                    = 7,
    SAY_BERSERK                 = 8,
    SAY_DEATH                   = 9,
};

enum Spells
{
    // Festergut
    SPELL_INHALE_BLIGHT         = 69165,
    SPELL_PUNGENT_BLIGHT        = 69195,
    SPELL_GASTRIC_BLOAT         = 72219, // 72214 is the proper way (with proc) but atm procs can't have cooldown for creatures
    SPELL_GASTRIC_EXPLOSION     = 72227,
    SPELL_GAS_SPORE             = 69278,
    SPELL_VILE_GAS              = 69240,
    SPELL_INOCULATED            = 69291,

    // Stinky
    SPELL_MORTAL_WOUND          = 71127,
    SPELL_DECIMATE              = 71123,
    SPELL_PLAGUE_STENCH         = 71805,
};

// Used for HasAura checks
#define PUNGENT_BLIGHT_HELPER RAID_MODE<uint32>(69195, 71219, 73031, 73032)
#define INOCULATED_HELPER     RAID_MODE<uint32>(69291, 72101, 72102, 72103)

uint32 const gaseousBlight[3]        = {69157, 69162, 69164};
uint32 const gaseousBlightVisual[3]  = {69126, 69152, 69154};

enum Events
{
    EVENT_BERSERK       = 1,
    EVENT_INHALE_BLIGHT = 2,
    EVENT_VILE_GAS      = 3,
    EVENT_GAS_SPORE     = 4,
    EVENT_GASTRIC_BLOAT = 5,

    EVENT_DECIMATE      = 6,
    EVENT_MORTAL_WOUND  = 7,
};

enum Misc
{
    DATA_INOCULATED_STACK       = 69291
};

class boss_festergut : public CreatureScript
{
    public:
        boss_festergut() : CreatureScript("boss_festergut") { }

        struct boss_festergutAI : public BossAI
        {
            boss_festergutAI(Creature* creature) : BossAI(creature, DATA_FESTERGUT)
            {
                _maxInoculatedStack = 0;
                _inhaleCounter = 0;
                _gasDummyGUID = 0;
            }

            void Reset() OVERRIDE
            {
                _Reset();
                me->SetReactState(REACT_DEFENSIVE);
                events.ScheduleEvent(EVENT_BERSERK, 300000);
                events.ScheduleEvent(EVENT_INHALE_BLIGHT, std::rand() % 30000 + 25000);
                events.ScheduleEvent(EVENT_GAS_SPORE, std::rand() % 25000 + 20000);
                events.ScheduleEvent(EVENT_GASTRIC_BLOAT, std::rand() % 15000 + 12500);
                _maxInoculatedStack = 0;
                _inhaleCounter = 0;
                me->RemoveAurasDueToSpell(SPELL_BERSERK2);
                if (Creature* gasDummy = me->FindNearestCreature(NPC_GAS_DUMMY, 100.0f, true))
                {
                    _gasDummyGUID = gasDummy->GetGUID();
                    for (uint8 i = 0; i < 3; ++i)
                    {
                        me->RemoveAurasDueToSpell(gaseousBlight[i]);
                        gasDummy->RemoveAurasDueToSpell(gaseousBlightVisual[i]);
                    }
                }
            }

            void EnterCombat(Unit* who) OVERRIDE
            {
                if (!instance->CheckRequiredBosses(DATA_FESTERGUT, who->ToPlayer()))
                {
                    EnterEvadeMode();
                    instance->DoCastSpellOnPlayers(LIGHT_S_HAMMER_TELEPORT);
                    return;
                }

                me->setActive(true);
                Talk(SAY_AGGRO);
                if (Creature* gasDummy = me->FindNearestCreature(NPC_GAS_DUMMY, 100.0f, true))
                    _gasDummyGUID = gasDummy->GetGUID();
                if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->DoAction(ACTION_FESTERGUT_COMBAT);
                DoZoneInCombat();
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
                Talk(SAY_DEATH);
                if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->DoAction(ACTION_FESTERGUT_DEATH);

                RemoveBlight();
            }

            void JustReachedHome() OVERRIDE
            {
                _JustReachedHome();
                instance->SetBossState(DATA_FESTERGUT, FAIL);
            }

            void EnterEvadeMode() OVERRIDE
            {
                ScriptedAI::EnterEvadeMode();
                if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->EnterEvadeMode();
            }

            void KilledUnit(Unit* victim) OVERRIDE
            {
                if (victim->GetTypeId() == TypeID::TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell) OVERRIDE
            {
                if (spell->Id == PUNGENT_BLIGHT_HELPER)
                    target->RemoveAurasDueToSpell(INOCULATED_HELPER);
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
                        case EVENT_INHALE_BLIGHT:
                        {
                            RemoveBlight();
                            if (_inhaleCounter == 3)
                            {
                                Talk(EMOTE_WARN_PUNGENT_BLIGHT);
                                Talk(SAY_PUNGENT_BLIGHT);
                                DoCast(me, SPELL_PUNGENT_BLIGHT);
                                _inhaleCounter = 0;
                                if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                                    professor->AI()->DoAction(ACTION_FESTERGUT_GAS);
                                events.RescheduleEvent(EVENT_GAS_SPORE, std::rand() % 25000 + 20000);
                            }
                            else
                            {
                                DoCast(me, SPELL_INHALE_BLIGHT);
                                // just cast and dont bother with target, conditions will handle it
                                ++_inhaleCounter;
                                if (_inhaleCounter < 3)
                                    me->CastSpell(me, gaseousBlight[_inhaleCounter], true, NULL, NULL, me->GetGUID());
                            }

                            events.ScheduleEvent(EVENT_INHALE_BLIGHT, std::rand() % 35000 + 33500);
                            break;
                        }
                        case EVENT_VILE_GAS:
                        {
                            std::list<Unit*> ranged, melee;
                            uint32 minTargets = RAID_MODE<uint32>(3, 8, 3, 8);
                            SelectTargetList(ranged, 25, SELECT_TARGET_RANDOM, -5.0f, true);
                            SelectTargetList(melee, 25, SELECT_TARGET_RANDOM, 5.0f, true);
                            while (ranged.size() < minTargets)
                            {
                                if (melee.empty())
                                    break;

                                Unit* target = Skyfire::Containers::SelectRandomContainerElement(melee);
                                ranged.push_back(target);
                                melee.remove(target);
                            }

                            if (!ranged.empty())
                            {
                                Skyfire::Containers::RandomResizeList(ranged, RAID_MODE<uint32>(1, 3, 1, 3));
                                for (std::list<Unit*>::iterator itr = ranged.begin(); itr != ranged.end(); ++itr)
                                    DoCast(*itr, SPELL_VILE_GAS);
                            }

                            events.ScheduleEvent(EVENT_VILE_GAS, std::rand() % 35000 + 28000);
                            break;
                        }
                        case EVENT_GAS_SPORE:
                            Talk(EMOTE_WARN_GAS_SPORE);
                            Talk(EMOTE_GAS_SPORE);
                            me->CastCustomSpell(SPELL_GAS_SPORE, SPELLVALUE_MAX_TARGETS, RAID_MODE<int32>(2, 3, 2, 3), me);
                            events.ScheduleEvent(EVENT_GAS_SPORE, std::rand() % 45000 + 40000);
                            events.RescheduleEvent(EVENT_VILE_GAS, std::rand() % 35000 + 28000);
                            break;
                        case EVENT_GASTRIC_BLOAT:
                            DoCastVictim(SPELL_GASTRIC_BLOAT);
                            events.ScheduleEvent(EVENT_GASTRIC_BLOAT, std::rand() % 17500 + 15000);
                            break;
                        case EVENT_BERSERK:
                            DoCast(me, SPELL_BERSERK2);
                            Talk(SAY_BERSERK);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void SetData(uint32 type, uint32 data) OVERRIDE
            {
                if (type == DATA_INOCULATED_STACK && data > _maxInoculatedStack)
                    _maxInoculatedStack = data;
            }

            uint32 GetData(uint32 type) const OVERRIDE
            {
                if (type == DATA_INOCULATED_STACK)
                    return uint32(_maxInoculatedStack);

                return 0;
            }

            void RemoveBlight()
            {
                if (Creature* gasDummy = ObjectAccessor::GetCreature(*me, _gasDummyGUID))
                    for (uint8 i = 0; i < 3; ++i)
                    {
                        me->RemoveAurasDueToSpell(gaseousBlight[i]);
                        gasDummy->RemoveAurasDueToSpell(gaseousBlightVisual[i]);
                    }
            }

        private:
            uint64 _gasDummyGUID;
            uint32 _maxInoculatedStack;
            uint32 _inhaleCounter;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetIcecrownCitadelAI<boss_festergutAI>(creature);
        }
};

class npc_stinky_icc : public CreatureScript
{
    public:
        npc_stinky_icc() : CreatureScript("npc_stinky_icc") { }

        struct npc_stinky_iccAI : public ScriptedAI
        {
            npc_stinky_iccAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() OVERRIDE
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_DECIMATE, std::rand() % 25000 + 20000);
                _events.ScheduleEvent(EVENT_MORTAL_WOUND, std::rand() % 7000 + 3000);
            }

            void EnterCombat(Unit* /*target*/) OVERRIDE
            {
                DoCast(me, SPELL_PLAGUE_STENCH);
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
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                if (Creature* festergut = me->GetCreature(*me, _instance->GetData64(DATA_FESTERGUT)))
                    if (festergut->IsAlive())
                        festergut->AI()->Talk(SAY_STINKY_DEAD);
            }

        private:
            EventMap _events;
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetIcecrownCitadelAI<npc_stinky_iccAI>(creature);
        }
};

class spell_festergut_pungent_blight : public SpellScriptLoader
{
    public:
        spell_festergut_pungent_blight() : SpellScriptLoader("spell_festergut_pungent_blight") { }

        class spell_festergut_pungent_blight_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_festergut_pungent_blight_SpellScript);

            bool Load() OVERRIDE
            {
                return GetCaster()->GetTypeId() == TypeID::TYPEID_UNIT;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                // Get Inhaled Blight id for our difficulty
                uint32 blightId = sSpellMgr->GetSpellIdForDifficulty(uint32(GetEffectValue()), GetCaster());

                // ...and remove it
                GetCaster()->RemoveAurasDueToSpell(blightId);
                GetCaster()->ToCreature()->AI()->Talk(EMOTE_PUNGENT_BLIGHT);
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_festergut_pungent_blight_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_festergut_pungent_blight_SpellScript();
        }
};

class spell_festergut_gastric_bloat : public SpellScriptLoader
{
    public:
        spell_festergut_gastric_bloat() : SpellScriptLoader("spell_festergut_gastric_bloat") { }

        class spell_festergut_gastric_bloat_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_festergut_gastric_bloat_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_GASTRIC_EXPLOSION))
                    return false;
                return true;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Aura const* aura = GetHitUnit()->GetAura(GetSpellInfo()->Id);
                if (!(aura && aura->GetStackAmount() == 10))
                    return;

                GetHitUnit()->RemoveAurasDueToSpell(GetSpellInfo()->Id);
                GetHitUnit()->CastSpell(GetHitUnit(), SPELL_GASTRIC_EXPLOSION, true);
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_festergut_gastric_bloat_SpellScript::HandleScript, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_festergut_gastric_bloat_SpellScript();
        }
};

class spell_festergut_blighted_spores : public SpellScriptLoader
{
    public:
        spell_festergut_blighted_spores() : SpellScriptLoader("spell_festergut_blighted_spores") { }

        class spell_festergut_blighted_spores_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_festergut_blighted_spores_AuraScript);

            bool Validate(SpellInfo const* /*spell*/) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_INOCULATED))
                    return false;
                return true;
            }

            void ExtraEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetTarget()->CastSpell(GetTarget(), SPELL_INOCULATED, true);
                if (InstanceScript* instance = GetTarget()->GetInstanceScript())
                    if (Creature* festergut = ObjectAccessor::GetCreature(*GetTarget(), instance->GetData64(DATA_FESTERGUT)))
                        festergut->AI()->SetData(DATA_INOCULATED_STACK, GetStackAmount());
            }

            void Register() OVERRIDE
            {
                AfterEffectApply += AuraEffectApplyFn(spell_festergut_blighted_spores_AuraScript::ExtraEffect, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_festergut_blighted_spores_AuraScript();
        }
};

class achievement_flu_shot_shortage : public AchievementCriteriaScript
{
    public:
        achievement_flu_shot_shortage() : AchievementCriteriaScript("achievement_flu_shot_shortage") { }

        bool OnCheck(Player* /*source*/, Unit* target) OVERRIDE
        {
            if (target && target->GetTypeId() == TypeID::TYPEID_UNIT)
                return target->ToCreature()->AI()->GetData(DATA_INOCULATED_STACK) < 3;

            return false;
        }
};

void AddSC_boss_festergut()
{
    new boss_festergut();
    new npc_stinky_icc();
    new spell_festergut_pungent_blight();
    new spell_festergut_gastric_bloat();
    new spell_festergut_blighted_spores();
    new achievement_flu_shot_shortage();
}
