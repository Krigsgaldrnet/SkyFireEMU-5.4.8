/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/*
 * Comment: MAYBE need more improve the "Raptor Call".
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "drak_tharon_keep.h"

enum Spells
{
    SPELL_BELLOWING_ROAR                          = 22686, // fears the group, can be resisted/dispelled
    SPELL_GRIEVOUS_BITE                           = 48920,
    SPELL_MANGLING_SLASH                          = 48873, // casted on the current tank, adds debuf
    SPELL_FEARSOME_ROAR                           = 48849,
    SPELL_PIERCING_SLASH                          = 48878, // debuff --> Armor reduced by 75%
    SPELL_RAPTOR_CALL                             = 59416, // dummy
    SPELL_GUT_RIP                                 = 49710,
    SPELL_REND                                    = 13738
};

enum Misc
{
    ACTION_RAPTOR_KILLED                          = 1,
    DATA_RAPTORS_KILLED                           = 2
};

enum Events
{
    EVENT_BELLOWING_ROAR                          = 1,
    EVENT_GRIEVOUS_BITE,
    EVENT_MANGLING_SLASH,
    EVENT_FEARSOME_ROAR,
    EVENT_PIERCING_SLASH,
    EVENT_RAPTOR_CALL
};

class boss_king_dred : public CreatureScript
{
public:
    boss_king_dred() : CreatureScript("boss_king_dred") { }

    struct boss_king_dredAI : public BossAI
    {
        boss_king_dredAI(Creature* creature) : BossAI(creature, DATA_KING_DRED)
        {
            raptorsKilled = 0;
        }

        void Reset() OVERRIDE
        {
            _Reset();
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            _EnterCombat();

            events.ScheduleEvent(EVENT_BELLOWING_ROAR, 33000);
            events.ScheduleEvent(EVENT_GRIEVOUS_BITE, 20000);
            events.ScheduleEvent(EVENT_MANGLING_SLASH, 18500);
            events.ScheduleEvent(EVENT_FEARSOME_ROAR, std::rand() % 20000 + 10000);
            events.ScheduleEvent(EVENT_PIERCING_SLASH, 17000);
            events.ScheduleEvent(EVENT_RAPTOR_CALL, std::rand() % 25000 + 20000);
        }

        void DoAction(int32 action) OVERRIDE
        {
            if (action == ACTION_RAPTOR_KILLED)
                ++raptorsKilled;
        }

        uint32 GetData(uint32 type) const OVERRIDE
        {
            if (type == DATA_RAPTORS_KILLED)
                return raptorsKilled;

            return 0;
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            _JustDied();
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
                    case EVENT_BELLOWING_ROAR:
                    {
                        DoCastAOE(SPELL_BELLOWING_ROAR);
                        events.ScheduleEvent(EVENT_BELLOWING_ROAR, 33000);
                        break;
                    }
                    case EVENT_GRIEVOUS_BITE:
                    {
                        DoCastVictim(SPELL_GRIEVOUS_BITE);
                        events.ScheduleEvent(EVENT_GRIEVOUS_BITE, 20000);
                        break;
                    }
                    case EVENT_MANGLING_SLASH:
                    {
                        DoCastVictim(SPELL_MANGLING_SLASH);
                        events.ScheduleEvent(EVENT_MANGLING_SLASH, 18500);
                        break;
                    }
                    case EVENT_FEARSOME_ROAR:
                    {
                        DoCastAOE(SPELL_FEARSOME_ROAR);
                        events.ScheduleEvent(EVENT_FEARSOME_ROAR, std::rand() % 20000 + 10000);
                        break;
                    }
                    case EVENT_PIERCING_SLASH:
                    {
                        DoCastVictim(SPELL_PIERCING_SLASH);
                        events.ScheduleEvent(EVENT_PIERCING_SLASH, 17000);
                        break;
                    }
                    case EVENT_RAPTOR_CALL:
                    {
                        DoCastVictim(SPELL_RAPTOR_CALL);

                        float x, y, z;

                        me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 10.0f);
                        me->SummonCreature(RAND(NPC_DRAKKARI_GUTRIPPER, NPC_DRAKKARI_SCYTHECLAW), x, y, z, 0, TempSummonType::TEMPSUMMON_DEAD_DESPAWN, 1000);
                        events.ScheduleEvent(EVENT_RAPTOR_CALL, std::rand() % 25000 + 20000);
                        break;
                    }
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        uint8 raptorsKilled;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetDrakTharonKeepAI<boss_king_dredAI>(creature);
    }
};

class npc_drakkari_gutripper : public CreatureScript
{
public:
    npc_drakkari_gutripper() : CreatureScript("npc_drakkari_gutripper") { }

    struct npc_drakkari_gutripperAI : public ScriptedAI
    {
        npc_drakkari_gutripperAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
            GutRipTimer = 0;
        }

        void Reset() OVERRIDE
        {
            GutRipTimer = std::rand() % 15000 + 10000;
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            if (GutRipTimer <= diff)
            {
                DoCastVictim(SPELL_GUT_RIP, false);
                GutRipTimer = std::rand() % 15000 + 10000;
            }
            else
                GutRipTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (Creature* Dred = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_KING_DRED)))
                Dred->AI()->DoAction(ACTION_RAPTOR_KILLED);
        }

    private:
        InstanceScript* instance;
        uint32 GutRipTimer;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetDrakTharonKeepAI<npc_drakkari_gutripperAI>(creature);
    }
};

class npc_drakkari_scytheclaw : public CreatureScript
{
public:
    npc_drakkari_scytheclaw() : CreatureScript("npc_drakkari_scytheclaw") { }

    struct npc_drakkari_scytheclawAI : public ScriptedAI
    {
        npc_drakkari_scytheclawAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
            uiRendTimer = 0;
        }

        void Reset() OVERRIDE
        {
            uiRendTimer = std::rand() % 15000 + 10000;
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            if (uiRendTimer <= diff)
            {
                DoCastVictim(SPELL_REND, false);
                uiRendTimer = std::rand() % 15000 + 10000;
            }
            else
                uiRendTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (Creature* Dred = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_KING_DRED)))
                Dred->AI()->DoAction(ACTION_RAPTOR_KILLED);
        }

    private:
        InstanceScript* instance;
        uint32 uiRendTimer;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetDrakTharonKeepAI<npc_drakkari_scytheclawAI>(creature);
    }
};

class achievement_king_dred : public AchievementCriteriaScript
{
public:
    achievement_king_dred() : AchievementCriteriaScript("achievement_king_dred") { }

    bool OnCheck(Player* /*player*/, Unit* target) OVERRIDE
    {
        if (!target)
            return false;

        if (Creature* Dred = target->ToCreature())
            if (Dred->AI()->GetData(DATA_RAPTORS_KILLED) >= 6)
                return true;

        return false;
    }
};

void AddSC_boss_king_dred()
{
    new boss_king_dred();
    new npc_drakkari_gutripper();
    new npc_drakkari_scytheclaw();
    new achievement_king_dred();
}
