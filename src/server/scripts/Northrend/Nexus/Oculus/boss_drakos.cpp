/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "oculus.h"

enum Spells
{
    SPELL_MAGIC_PULL                              = 51336,
    SPELL_THUNDERING_STOMP                        = 50774,
    SPELL_UNSTABLE_SPHERE_PASSIVE                 = 50756,
    SPELL_UNSTABLE_SPHERE_PULSE                   = 50757,
    SPELL_UNSTABLE_SPHERE_TIMER                   = 50758,
    NPC_UNSTABLE_SPHERE                           = 28166,
};

enum Yells
{
    SAY_AGGRO                                     = 0,
    SAY_KILL                                      = 1,
    SAY_DEATH                                     = 2,
    SAY_PULL                                      = 3,
    SAY_STOMP                                     = 4
};

enum DrakosAchievement
{
    ACHIEV_TIMED_START_EVENT                      = 18153,
};

enum DrakosEvents
{
    EVENT_MAGIC_PULL = 1,
    EVENT_STOMP,
    EVENT_BOMB_SUMMON
};

class boss_drakos : public CreatureScript
{
public:
    boss_drakos() : CreatureScript("boss_drakos") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_drakosAI(creature);
    }

    struct boss_drakosAI : public BossAI
    {
        boss_drakosAI(Creature* creature) : BossAI(creature, DATA_DRAKOS_EVENT) { }

        void Reset() OVERRIDE
        {
            _Reset();

            events.ScheduleEvent(EVENT_MAGIC_PULL, 15000);
            events.ScheduleEvent(EVENT_STOMP, 17000);
            events.ScheduleEvent(EVENT_BOMB_SUMMON, 2000);

            postPull = false;
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            _EnterCombat();
            Talk(SAY_AGGRO);
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
                    case EVENT_BOMB_SUMMON:
                        {
                            Position pPosition;
                            me->GetPosition(&pPosition);

                            for (uint8 i = 0; i <= (postPull ? 3 : 0); i++)
                            {
                                me->GetRandomNearPosition(pPosition, float(std::rand() % 10));
                                me->SummonCreature(NPC_UNSTABLE_SPHERE, pPosition);
                            }
                        }
                        events.ScheduleEvent(EVENT_BOMB_SUMMON, 2000);
                        break;
                    case EVENT_MAGIC_PULL:
                        DoCast(SPELL_MAGIC_PULL);
                        postPull = true;
                        events.ScheduleEvent(EVENT_MAGIC_PULL, 15000);
                        break;
                    case EVENT_STOMP:
                        Talk(SAY_STOMP);
                        DoCast(SPELL_THUNDERING_STOMP);
                        events.ScheduleEvent(EVENT_STOMP, 17000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            _JustDied();

            Talk(SAY_DEATH);

            // start achievement timer (kill Eregos within 20 min)
            instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_KILL);
        }
    private:
        bool postPull;
    };
};

class npc_unstable_sphere : public CreatureScript
{
public:
    npc_unstable_sphere() : CreatureScript("npc_unstable_sphere") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_unstable_sphereAI(creature);
    }

    struct npc_unstable_sphereAI : public ScriptedAI
    {
        npc_unstable_sphereAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() OVERRIDE
        {
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MoveRandom(40.0f);

            me->AddAura(SPELL_UNSTABLE_SPHERE_PASSIVE, me);
            me->AddAura(SPELL_UNSTABLE_SPHERE_TIMER, me);

            pulseTimer = 3000;
            deathTimer = 19000;
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (pulseTimer <= diff)
            {
                DoCast(SPELL_UNSTABLE_SPHERE_PULSE);
                pulseTimer = 3*IN_MILLISECONDS;
            } else pulseTimer -= diff;

            if (deathTimer <= diff)
                me->DisappearAndDie();
            else deathTimer -= diff;
        }
    private:
        uint32 pulseTimer;
        uint32 deathTimer;
    };
};

void AddSC_boss_drakos()
{
    new boss_drakos();
    new npc_unstable_sphere();
}
