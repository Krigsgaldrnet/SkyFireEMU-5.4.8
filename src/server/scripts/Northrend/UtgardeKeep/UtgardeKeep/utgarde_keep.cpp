/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "utgarde_keep.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum Spells
{
    SPELL_UK_SECOUND_WIND_TRIGGER    = 42771
};

uint32 ForgeSearch[3] =
{
    GO_GLOWING_ANVIL_1,
    GO_GLOWING_ANVIL_2,
    GO_GLOWING_ANVIL_3
};

class npc_dragonflayer_forge_master : public CreatureScript
{
    public:
        npc_dragonflayer_forge_master() : CreatureScript("npc_dragonflayer_forge_master") { }

        struct npc_dragonflayer_forge_masterAI : public ScriptedAI
        {
            npc_dragonflayer_forge_masterAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
                _forgeId = 0;
            }

            void Reset() OVERRIDE
            {
                if (!_forgeId)
                    _forgeId = GetForgeMasterType();

                if (!me->IsAlive())
                    return;

                if (_forgeId)
                    _instance->SetData(DATA_FORGE_1 + _forgeId - 1, NOT_STARTED);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                if (!_forgeId)
                    _forgeId = GetForgeMasterType();

                if (_forgeId)
                    _instance->SetData(DATA_FORGE_1 + _forgeId - 1, DONE);
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                if (!_forgeId)
                    _forgeId = GetForgeMasterType();

                if (_forgeId)
                    _instance->SetData(DATA_FORGE_1 + _forgeId - 1, IN_PROGRESS);

                me->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
            }

            void UpdateAI(uint32 /*diff*/) OVERRIDE
            {
                if (!_forgeId)
                    _forgeId = GetForgeMasterType();

                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }

        private:
            uint8 GetForgeMasterType()
            {
                float diff = 30.0f;
                uint8 id = 0;

                for (uint8 i = 0; i < 3; ++i)
                {
                    if (GameObject* go = me->FindNearestGameObject(ForgeSearch[i], 30))
                    {
                        if (me->IsWithinDist(go, diff, false))
                        {
                            id = i + 1;
                            diff = me->GetDistance2d(go);
                        }
                    }
                }
                return id > 0 && id < 4 ? id : 0;
            }

            InstanceScript* _instance;
            uint8 _forgeId;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetUtgardeKeepAI<npc_dragonflayer_forge_masterAI>(creature);
        }
};

enum TickingTimeBomb
{
    SPELL_TICKING_TIME_BOMB_EXPLODE = 59687
};

class spell_ticking_time_bomb : public SpellScriptLoader
{
    public:
        spell_ticking_time_bomb() : SpellScriptLoader("spell_ticking_time_bomb") { }

        class spell_ticking_time_bomb_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ticking_time_bomb_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_TICKING_TIME_BOMB_EXPLODE))
                    return false;
                return true;
            }

            void HandleOnEffectRemove(AuraEffect const* /* aurEff */, AuraEffectHandleModes /* mode */)
            {
                if (GetCaster() == GetTarget())
                    GetTarget()->CastSpell(GetTarget(), SPELL_TICKING_TIME_BOMB_EXPLODE, true);
            }

            void Register() OVERRIDE
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_ticking_time_bomb_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_ticking_time_bomb_AuraScript();
        }
};

enum Fixate
{
    SPELL_FIXATE_TRIGGER = 40415
};

class spell_fixate : public SpellScriptLoader
{
    public:
        spell_fixate() : SpellScriptLoader("spell_fixate") { }

        class spell_fixate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_fixate_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_FIXATE_TRIGGER))
                    return false;
                return true;
            }

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                GetHitUnit()->CastSpell(GetCaster(), SPELL_FIXATE_TRIGGER, true);
            }

            void Register() OVERRIDE
            {
                OnEffectHitTarget += SpellEffectFn(spell_fixate_SpellScript::HandleScriptEffect, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_fixate_SpellScript();
        }
};

enum EnslavedProtoDrake
{
    TYPE_PROTODRAKE_AT      = 28,
    DATA_PROTODRAKE_MOVE    = 6,

    PATH_PROTODRAKE         = 125946,

    EVENT_REND              = 1,
    EVENT_FLAME_BREATH      = 2,
    EVENT_KNOCKAWAY         = 3,

    SPELL_REND              = 43931,
    SPELL_FLAME_BREATH      = 50653,
    SPELL_KNOCK_AWAY        = 49722,

    POINT_LAST              = 5,
};

const Position protodrakeCheckPos = {206.24f, -190.28f, 200.11f, 0.f};

class npc_enslaved_proto_drake : public CreatureScript
{
    public:
        npc_enslaved_proto_drake() : CreatureScript("npc_enslaved_proto_drake") { }

        struct npc_enslaved_proto_drakeAI : public ScriptedAI
        {
            npc_enslaved_proto_drakeAI(Creature* creature) : ScriptedAI(creature)
            {
                _setData = false;
            }

            void Reset() OVERRIDE
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_REND, std::rand() % 3000 + 2000);
                _events.ScheduleEvent(EVENT_FLAME_BREATH, std::rand() % 7000 + 5500);
                _events.ScheduleEvent(EVENT_KNOCKAWAY, std::rand() % 6000 + 3500);
            }

            void MovementInform(uint32 type, uint32 id) OVERRIDE
            {
                if (type == WAYPOINT_MOTION_TYPE && id == POINT_LAST)
                {
                    me->RemoveByteFlag(UNIT_FIELD_ANIM_TIER, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                }
            }

            void SetData(uint32 type, uint32 data) OVERRIDE
            {
                if (type == TYPE_PROTODRAKE_AT && data == DATA_PROTODRAKE_MOVE && !_setData && me->GetDistance(protodrakeCheckPos) < 5.0f)
                {
                    _setData = true;
                    me->SetByteFlag(UNIT_FIELD_ANIM_TIER, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->GetMotionMaster()->MovePath(PATH_PROTODRAKE, false);
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventid = _events.ExecuteEvent())
                {
                    switch (eventid)
                    {
                        case EVENT_REND:
                            DoCast(SPELL_REND);
                            _events.ScheduleEvent(EVENT_REND, std::rand() % 20000 + 15000);
                            break;
                        case EVENT_FLAME_BREATH:
                            DoCast(SPELL_FLAME_BREATH);
                            _events.ScheduleEvent(EVENT_FLAME_BREATH, std::rand() % 12000 + 11000);
                            break;
                        case EVENT_KNOCKAWAY:
                            DoCast(SPELL_KNOCK_AWAY);
                            _events.ScheduleEvent(EVENT_KNOCKAWAY, std::rand() % 8500 + 7000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _setData;
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_enslaved_proto_drakeAI(creature);
        }
};

class spell_uk_second_wind_proc : public SpellScriptLoader
{
    public:
        spell_uk_second_wind_proc() : SpellScriptLoader("spell_uk_second_wind_proc") { }

        class spell_uk_second_wind_proc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_uk_second_wind_proc_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_UK_SECOUND_WIND_TRIGGER))
                    return false;
                return true;
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (eventInfo.GetProcTarget() == GetTarget())
                    return false;
                if (!(eventInfo.GetDamageInfo() || eventInfo.GetDamageInfo()->GetSpellInfo()->GetAllEffectsMechanicMask() & ((1 << MECHANIC_ROOT) | (1 << MECHANIC_STUN))))
                    return false;
                return true;
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();
                GetTarget()->CastCustomSpell(SPELL_UK_SECOUND_WIND_TRIGGER, SPELLVALUE_BASE_POINT0, 5, GetTarget(), true, NULL, aurEff);
            }

            void Register() OVERRIDE
            {
                DoCheckProc += AuraCheckProcFn(spell_uk_second_wind_proc_AuraScript::CheckProc);
                OnEffectProc += AuraEffectProcFn(spell_uk_second_wind_proc_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_uk_second_wind_proc_AuraScript();
        }
};

void AddSC_utgarde_keep()
{
    new npc_dragonflayer_forge_master();
    new npc_enslaved_proto_drake();
    new spell_ticking_time_bomb();
    new spell_fixate();
    new spell_uk_second_wind_proc();
}
