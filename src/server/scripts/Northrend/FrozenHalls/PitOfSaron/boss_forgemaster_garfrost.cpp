/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "pit_of_saron.h"

enum Yells
{
    SAY_AGGRO                   = 0,
    SAY_PHASE2                  = 1,
    SAY_PHASE3                  = 2,
    SAY_DEATH                   = 3,
    SAY_SLAY                    = 4,
    SAY_THROW_SARONITE          = 5,
    SAY_CAST_DEEP_FREEZE        = 6,

    SAY_TYRANNUS_DEATH          = 0
};

enum Spells
{
    SPELL_PERMAFROST            = 70326,
    SPELL_THROW_SARONITE        = 68788,
    SPELL_THUNDERING_STOMP      = 68771,
    SPELL_CHILLING_WAVE         = 68778,
    SPELL_DEEP_FREEZE           = 70381,
    SPELL_FORGE_MACE            = 68785,
    SPELL_FORGE_BLADE           = 68774
};

#define SPELL_PERMAFROST_HELPER RAID_MODE<uint32>(68786, 70336)
#define SPELL_FORGE_BLADE_HELPER RAID_MODE<uint32>(68774, 70334)

enum Phases
{
    PHASE_ONE                   = 1,
    PHASE_TWO                   = 2,
    PHASE_THREE                 = 3
};

enum MiscData
{
    EQUIP_ID_SWORD              = 49345,
    EQUIP_ID_MACE               = 49344,
    ACHIEV_DOESNT_GO_TO_ELEVEN  = 0,
    POINT_FORGE                 = 0
};

enum Events
{
    EVENT_THROW_SARONITE        = 1,
    EVENT_CHILLING_WAVE         = 2,
    EVENT_DEEP_FREEZE           = 3,
    EVENT_FORGE_JUMP            = 4,
    EVENT_RESUME_ATTACK         = 5
};

Position const northForgePos = { 722.5643f, -234.1615f, 527.182f, 2.16421f };
Position const southForgePos = { 639.257f, -210.1198f, 529.015f, 0.523599f };

class boss_garfrost : public CreatureScript
{
    public:
        boss_garfrost() : CreatureScript("boss_garfrost") { }

        struct boss_garfrostAI : public BossAI
        {
            boss_garfrostAI(Creature* creature) : BossAI(creature, DATA_GARFROST) { }

            void Reset() OVERRIDE
            {
                _Reset();
                events.SetPhase(PHASE_ONE);
                SetEquipmentSlots(true);
                _permafrostStack = 0;
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
                DoCast(me, SPELL_PERMAFROST);
                me->CallForHelp(70.0f);
                events.ScheduleEvent(EVENT_THROW_SARONITE, 7000);
            }

            void KilledUnit(Unit* victim) OVERRIDE
            {
                if (victim->GetTypeId() == TypeID::TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
                Talk(SAY_DEATH);

                if (Creature* tyrannus = me->GetCreature(*me, instance->GetData64(DATA_TYRANNUS)))
                    tyrannus->AI()->Talk(SAY_TYRANNUS_DEATH);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*uiDamage*/) OVERRIDE
            {
                if (events.IsInPhase(PHASE_ONE) && !HealthAbovePct(66))
                {
                    events.SetPhase(PHASE_TWO);
                    Talk(SAY_PHASE2);
                    events.DelayEvents(8000);
                    DoCast(me, SPELL_THUNDERING_STOMP);
                    events.ScheduleEvent(EVENT_FORGE_JUMP, 1500);
                    return;
                }

                if (events.IsInPhase(PHASE_TWO) && !HealthAbovePct(33))
                {
                    events.SetPhase(PHASE_THREE);
                    Talk(SAY_PHASE3);
                    events.DelayEvents(8000);
                    DoCast(me, SPELL_THUNDERING_STOMP);
                    events.ScheduleEvent(EVENT_FORGE_JUMP, 1500);
                    return;
                }
            }

            void MovementInform(uint32 type, uint32 id) OVERRIDE
            {
                if (type != EFFECT_MOTION_TYPE || id != POINT_FORGE)
                    return;

                if (events.IsInPhase(PHASE_TWO))
                {
                    DoCast(me, SPELL_FORGE_BLADE);
                    SetEquipmentSlots(false, EQUIP_ID_SWORD);
                }
                if (events.IsInPhase(PHASE_THREE))
                {
                    me->RemoveAurasDueToSpell(SPELL_FORGE_BLADE_HELPER);
                    DoCast(me, SPELL_FORGE_MACE);
                    SetEquipmentSlots(false, EQUIP_ID_MACE);
                }
                events.ScheduleEvent(EVENT_RESUME_ATTACK, 5000);
            }

            void SpellHitTarget(Unit* target, const SpellInfo* spell) OVERRIDE
            {
                if (spell->Id == SPELL_PERMAFROST_HELPER)
                {
                    if (Aura* aura = target->GetAura(SPELL_PERMAFROST_HELPER))
                        _permafrostStack = std::max<uint32>(_permafrostStack, aura->GetStackAmount());
                }
            }

            uint32 GetData(uint32 /*type*/) const OVERRIDE
            {
                return _permafrostStack;
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
                        case EVENT_THROW_SARONITE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            {
                                Talk(SAY_THROW_SARONITE, target);
                                DoCast(target, SPELL_THROW_SARONITE);
                            }
                            events.ScheduleEvent(EVENT_THROW_SARONITE, std::rand() % 20000 + 12500);
                            break;
                        case EVENT_CHILLING_WAVE:
                            DoCast(me, SPELL_CHILLING_WAVE);
                            events.ScheduleEvent(EVENT_CHILLING_WAVE, 40000, 0, PHASE_TWO);
                            break;
                        case EVENT_DEEP_FREEZE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            {
                                Talk(SAY_CAST_DEEP_FREEZE, target);
                                DoCast(target, SPELL_DEEP_FREEZE);
                            }
                            events.ScheduleEvent(EVENT_DEEP_FREEZE, 35000, 0, PHASE_THREE);
                            break;
                        case EVENT_FORGE_JUMP:
                            me->AttackStop();
                            if (events.IsInPhase(PHASE_TWO))
                                me->GetMotionMaster()->MoveJump(northForgePos, 25.0f, 15.0f, POINT_FORGE);
                            else if (events.IsInPhase(PHASE_THREE))
                                me->GetMotionMaster()->MoveJump(southForgePos, 25.0f, 15.0f, POINT_FORGE);
                            break;
                        case EVENT_RESUME_ATTACK:
                            if (events.IsInPhase(PHASE_TWO))
                                events.ScheduleEvent(EVENT_CHILLING_WAVE, 5000, 0, PHASE_TWO);
                            else if (events.IsInPhase(PHASE_THREE))
                                events.ScheduleEvent(EVENT_DEEP_FREEZE, 10000, 0, PHASE_THREE);
                            AttackStart(me->GetVictim());
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint32 _permafrostStack;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetPitOfSaronAI<boss_garfrostAI>(creature);
        }
};

class spell_garfrost_permafrost : public SpellScriptLoader
{
    public:
        spell_garfrost_permafrost() : SpellScriptLoader("spell_garfrost_permafrost") { }

        class spell_garfrost_permafrost_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_garfrost_permafrost_SpellScript);

            bool Load() OVERRIDE
            {
                prevented = false;
                return true;
            }

            void PreventHitByLoS()
            {
                if (Unit* target = GetHitUnit())
                {
                    Unit* caster = GetCaster();
                    //Temporary Line of Sight Check
                    std::list<GameObject*> blockList;
                    caster->GetGameObjectListWithEntryInGrid(blockList, GO_SARONITE_ROCK, 100.0f);
                    if (!blockList.empty())
                    {
                        for (std::list<GameObject*>::const_iterator itr = blockList.begin(); itr != blockList.end(); ++itr)
                        {
                            if (!(*itr)->IsInvisibleDueToDespawn())
                            {
                                if ((*itr)->IsInBetween(caster, target, 4.0f))
                                {
                                    prevented = true;
                                    target->ApplySpellImmune(GetSpellInfo()->Id, IMMUNITY_ID, GetSpellInfo()->Id, true);
                                    PreventHitDefaultEffect(EFFECT_0);
                                    PreventHitDefaultEffect(EFFECT_1);
                                    PreventHitDefaultEffect(EFFECT_2);
                                    PreventHitDamage();
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            void RestoreImmunity()
            {
                if (Unit* target = GetHitUnit())
                {
                    target->ApplySpellImmune(GetSpellInfo()->Id, IMMUNITY_ID, GetSpellInfo()->Id, false);
                    if (prevented)
                        PreventHitAura();
                }
            }

            void Register() OVERRIDE
            {
                BeforeHit += SpellHitFn(spell_garfrost_permafrost_SpellScript::PreventHitByLoS);
                AfterHit += SpellHitFn(spell_garfrost_permafrost_SpellScript::RestoreImmunity);
            }

            bool prevented;
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_garfrost_permafrost_SpellScript();
        }
};

class achievement_doesnt_go_to_eleven : public AchievementCriteriaScript
{
    public:
        achievement_doesnt_go_to_eleven() : AchievementCriteriaScript("achievement_doesnt_go_to_eleven") { }

        bool OnCheck(Player* /*source*/, Unit* target) OVERRIDE
        {
            if (target)
                if (Creature* garfrost = target->ToCreature())
                    if (garfrost->AI()->GetData(ACHIEV_DOESNT_GO_TO_ELEVEN) <= 10)
                        return true;

            return false;
        }
};

void AddSC_boss_garfrost()
{
    new boss_garfrost();
    new spell_garfrost_permafrost();
    new achievement_doesnt_go_to_eleven();
}
