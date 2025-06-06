/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "oculus.h"

//Types of drake mounts: Ruby(Tank),  Amber(DPS),  Emerald(Healer)
//Two Repeating phases

enum Events
{
    EVENT_ARCANE_BARRAGE = 1,
    EVENT_ARCANE_VOLLEY,
    EVENT_ENRAGED_ASSAULT,
    EVENT_SUMMON_LEY_WHELP
};

enum Says
{
    SAY_SPAWN           = 0,
    SAY_AGGRO           = 1,
    SAY_ENRAGE          = 2,
    SAY_KILL            = 3,
    SAY_DEATH           = 4,
    SAY_SHIELD          = 5,
};

enum Spells
{
    SPELL_ARCANE_BARRAGE                          = 50804,
    SPELL_ARCANE_VOLLEY                           = 51153,
    SPELL_ENRAGED_ASSAULT                         = 51170,
    SPELL_PLANAR_ANOMALIES                        = 57959,
    SPELL_PLANAR_SHIFT                            = 51162,
    SPELL_SUMMON_LEY_WHELP                        = 51175,
    SPELL_SUMMON_PLANAR_ANOMALIES                 = 57963,
    SPELL_PLANAR_BLAST                            = 57976
};

enum Npcs
{
    NPC_PLANAR_ANOMALY = 30879
};

enum Phases
{
    PHASE_NORMAL = 1,
    PHASE_FIRST_PLANAR = 2,
    PHASE_SECOND_PLANAR = 3
};

enum Actions
{
    ACTION_SET_NORMAL_EVENTS = 1
};

enum EregosData
{
    DATA_RUBY_VOID          = 0,      // http://www.wowhead.com/achievement=2044
    DATA_EMERALD_VOID       = 1,      // http://www.wowhead.com/achievement=2045
    DATA_AMBER_VOID         = 2       // http://www.wowhead.com/achievement=2046
};

class boss_eregos : public CreatureScript
{
public:
    boss_eregos() : CreatureScript("boss_eregos") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_eregosAI(creature);
    }

    struct boss_eregosAI : public BossAI
    {
        boss_eregosAI(Creature* creature) : BossAI(creature, DATA_EREGOS_EVENT) { }

        void Reset() OVERRIDE
        {
            _Reset();
            _phase = PHASE_NORMAL;

            _rubyVoid = true;
            _emeraldVoid = true;
            _amberVoid = true;

            DoAction(ACTION_SET_NORMAL_EVENTS);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_KILL);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            _EnterCombat();

            Talk(SAY_AGGRO);
            /* Checks for present drakes vehicles from each type and deactivate achievement that corresponds to each found
               The checks are so big in case some party try weird things like pulling boss down or hiding out of check range, the only thing player need is to get the boss kill credit after the check /even if he or his drake die/
               Drakes mechanic would despawn all after unmount and also drakes should be auto mounted after item use, item use after Eregos is engaged leads to his despawn - based on retail data. */
            if (me->FindNearestCreature(NPC_RUBY_DRAKE_VEHICLE, 500.0f, true))
                _rubyVoid = false;
            if (me->FindNearestCreature(NPC_EMERALD_DRAKE_VEHICLE, 500.0f, true))
                _emeraldVoid = false;
            if (me->FindNearestCreature(NPC_AMBER_DRAKE_VEHICLE, 500.0f, true))
                _amberVoid = false;
        }

        uint32 GetData(uint32 type) const OVERRIDE
        {
           switch (type)
           {
               case DATA_RUBY_VOID:
                    return _rubyVoid;
               case DATA_EMERALD_VOID:
                    return _emeraldVoid;
               case DATA_AMBER_VOID:
                    return _amberVoid;
                default:
                    break;
            }
            return 0;
        }

        void DoAction(int32 action) OVERRIDE
        {
            if (action != ACTION_SET_NORMAL_EVENTS)
                return;

            events.ScheduleEvent(EVENT_ARCANE_BARRAGE, std::rand() % (10 * IN_MILLISECONDS) + (3 * IN_MILLISECONDS), 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_ARCANE_VOLLEY, std::rand() % (25 * IN_MILLISECONDS) + (10 * IN_MILLISECONDS), 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_ENRAGED_ASSAULT, std::rand() % (50 * IN_MILLISECONDS) + (35 * IN_MILLISECONDS), 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_SUMMON_LEY_WHELP, std::rand() % (30* IN_MILLISECONDS) + (15 * IN_MILLISECONDS), 0, PHASE_NORMAL);
        }

        void JustSummoned(Creature* summon) OVERRIDE
        {
            BossAI::JustSummoned(summon);

            if (summon->GetEntry() != NPC_PLANAR_ANOMALY)
                return;

            summon->CombatStop(true);
            summon->SetReactState(REACT_PASSIVE);
            summon->GetMotionMaster()->MoveRandom(100.0f);
        }

        void SummonedCreatureDespawn(Creature* summon) OVERRIDE
        {
            if (summon->GetEntry() != NPC_PLANAR_ANOMALY)
                return;

            // TO-DO: See why the spell is not casted
            summon->CastSpell(summon, SPELL_PLANAR_BLAST, true);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) OVERRIDE
        {
            if (!me->GetMap()->IsHeroic())
                return;

            if ( (me->GetHealthPct() < 60.0f  && me->GetHealthPct() > 20.0f && _phase < PHASE_FIRST_PLANAR)
                || (me->GetHealthPct() < 20.0f && _phase < PHASE_SECOND_PLANAR) )
            {
                events.Reset();
                _phase = (me->GetHealthPct() < 60.0f  && me->GetHealthPct() > 20.0f) ? PHASE_FIRST_PLANAR : PHASE_SECOND_PLANAR;

                Talk(SAY_SHIELD);
                DoCast(SPELL_PLANAR_SHIFT);

                // not sure about the amount, and if we should despawn previous spawns (dragon trashs)
                summons.DespawnAll();
                for (uint8 i = 0; i < 6; i++)
                    DoCast(SPELL_PLANAR_ANOMALIES);
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_ARCANE_BARRAGE:
                        DoCastVictim(SPELL_ARCANE_BARRAGE);
                        events.ScheduleEvent(EVENT_ARCANE_BARRAGE, std::rand() % (10 * IN_MILLISECONDS) + (3 * IN_MILLISECONDS), 0, PHASE_NORMAL);
                        break;
                    case EVENT_ARCANE_VOLLEY:
                        DoCastAOE(SPELL_ARCANE_VOLLEY);
                        events.ScheduleEvent(EVENT_ARCANE_VOLLEY, std::rand() % (25 * IN_MILLISECONDS) + (10 * IN_MILLISECONDS), 0, PHASE_NORMAL);
                        break;
                    case EVENT_ENRAGED_ASSAULT:
                        Talk(SAY_ENRAGE);
                        DoCast(SPELL_ENRAGED_ASSAULT);
                        events.ScheduleEvent(EVENT_ENRAGED_ASSAULT, std::rand() % (50 * IN_MILLISECONDS) + (35 * IN_MILLISECONDS), 0, PHASE_NORMAL);
                        break;
                    case EVENT_SUMMON_LEY_WHELP:
                        for (uint8 i = 0; i < 3; i++)
                            DoCast(SPELL_SUMMON_LEY_WHELP);
                        events.ScheduleEvent(EVENT_SUMMON_LEY_WHELP, std::rand() % (30 * IN_MILLISECONDS) + (15 * IN_MILLISECONDS), 0, PHASE_NORMAL);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);

            _JustDied();
        }

     private:
         uint8 _phase;
         bool _rubyVoid;
         bool _emeraldVoid;
         bool _amberVoid;
    };
};

class spell_eregos_planar_shift : public SpellScriptLoader
{
    public:
        spell_eregos_planar_shift() : SpellScriptLoader("spell_eregos_planar_shift") { }

        class spell_eregos_planar_shift_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_eregos_planar_shift_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (Creature* creatureCaster = caster->ToCreature())
                        creatureCaster->AI()->DoAction(ACTION_SET_NORMAL_EVENTS);
            }

            void Register() OVERRIDE
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_eregos_planar_shift_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_SCHOOL_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_eregos_planar_shift_AuraScript();
        }
};

class achievement_gen_eregos_void : public AchievementCriteriaScript
{
    public:
        achievement_gen_eregos_void(char const* name, uint32 data) : AchievementCriteriaScript(name), _data(data) { }

        bool OnCheck(Player* /*player*/, Unit* target) OVERRIDE
        {
            return target && target->GetAI()->GetData(_data);
        }

    private:
        uint32 _data;
};

 void AddSC_boss_eregos()
 {
    new boss_eregos();
    new spell_eregos_planar_shift();
    new achievement_gen_eregos_void("achievement_ruby_void", DATA_RUBY_VOID);
    new achievement_gen_eregos_void("achievement_emerald_void", DATA_EMERALD_VOID);
    new achievement_gen_eregos_void("achievement_amber_void", DATA_AMBER_VOID);
 }
