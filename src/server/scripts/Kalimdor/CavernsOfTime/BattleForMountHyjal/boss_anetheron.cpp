/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "hyjal.h"
#include "hyjal_trash.h"

enum Spells
{
    SPELL_CARRION_SWARM     = 31306,
    SPELL_SLEEP             = 31298,
    SPELL_VAMPIRIC_AURA     = 38196,
    SPELL_INFERNO           = 31299,
    SPELL_IMMOLATION        = 31303,
    SPELL_INFERNO_EFFECT    = 31302,
};

enum Texts
{
    SAY_ONDEATH         = 0,
    SAY_ONSLAY          = 1,
    SAY_SWARM           = 2,
    SAY_SLEEP           = 3,
    SAY_INFERNO         = 4,
    SAY_ONAGGRO         = 5,
};

class boss_anetheron : public CreatureScript
{
public:
    boss_anetheron() : CreatureScript("boss_anetheron") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_anetheronAI(creature);
    }

    struct boss_anetheronAI : public hyjal_trashAI
    {
        boss_anetheronAI(Creature* creature) : hyjal_trashAI(creature)
        {
            instance = creature->GetInstanceScript();
            go = false;
        }

        uint32 SwarmTimer;
        uint32 SleepTimer;
        uint32 AuraTimer;
        uint32 InfernoTimer;
        bool go;

        void Reset() OVERRIDE
        {
            damageTaken = 0;
            SwarmTimer = 45000;
            SleepTimer = 60000;
            AuraTimer = 5000;
            InfernoTimer = 45000;

            if (instance && IsEvent)
                instance->SetData(DATA_ANETHERONEVENT, NOT_STARTED);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            if (instance && IsEvent)
                instance->SetData(DATA_ANETHERONEVENT, IN_PROGRESS);
            Talk(SAY_ONAGGRO);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_ONSLAY);
        }

        void WaypointReached(uint32 waypointId) OVERRIDE
        {
            if (waypointId == 7 && instance)
            {
                Unit* target = Unit::GetUnit(*me, instance->GetData64(DATA_JAINAPROUDMOORE));
                if (target && target->IsAlive())
                    me->AddThreat(target, 0.0f);
            }
        }

        void JustDied(Unit* killer) OVERRIDE
        {
            hyjal_trashAI::JustDied(killer);
            if (instance && IsEvent)
                instance->SetData(DATA_ANETHERONEVENT, DONE);
            Talk(SAY_ONDEATH);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (IsEvent)
            {
                //Must update npc_escortAI
                npc_escortAI::UpdateAI(diff);
                if (!go)
                {
                    go = true;
                    if (instance)
                    {
                        AddWaypoint(0, 4896.08f,    -1576.35f,    1333.65f);
                        AddWaypoint(1, 4898.68f,    -1615.02f,    1329.48f);
                        AddWaypoint(2, 4907.12f,    -1667.08f,    1321.00f);
                        AddWaypoint(3, 4963.18f,    -1699.35f,    1340.51f);
                        AddWaypoint(4, 4989.16f,    -1716.67f,    1335.74f);
                        AddWaypoint(5, 5026.27f,    -1736.89f,    1323.02f);
                        AddWaypoint(6, 5037.77f,    -1770.56f,    1324.36f);
                        AddWaypoint(7, 5067.23f,    -1789.95f,    1321.17f);
                        Start(false, true);
                        SetDespawnAtEnd(false);
                    }
                }
            }

            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (SwarmTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_CARRION_SWARM);

                SwarmTimer = std::rand() % 60000 + 45000;
                Talk(SAY_SWARM);
            } else SwarmTimer -= diff;

            if (SleepTimer <= diff)
            {
                for (uint8 i = 0; i < 3; ++i)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        target->CastSpell(target, SPELL_SLEEP, true);
                }
                SleepTimer = 60000;
                Talk(SAY_SLEEP);
            } else SleepTimer -= diff;
            if (AuraTimer <= diff)
            {
                DoCast(me, SPELL_VAMPIRIC_AURA, true);
                AuraTimer = std::rand() % 20000 + 10000;
            } else AuraTimer -= diff;
            if (InfernoTimer <= diff)
            {
                DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true), SPELL_INFERNO);
                InfernoTimer = 45000;
                Talk(SAY_INFERNO);
            } else InfernoTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_towering_infernal : public CreatureScript
{
public:
    npc_towering_infernal() : CreatureScript("npc_towering_infernal") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_towering_infernalAI(creature);
    }

    struct npc_towering_infernalAI : public ScriptedAI
    {
        npc_towering_infernalAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            if (instance)
                AnetheronGUID = instance->GetData64(DATA_ANETHERON);
        }

        uint32 ImmolationTimer;
        uint32 CheckTimer;
        uint64 AnetheronGUID;
        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            DoCast(me, SPELL_INFERNO_EFFECT);
            ImmolationTimer = 5000;
            CheckTimer = 5000;
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
        }

        void MoveInLineOfSight(Unit* who) OVERRIDE

        {
            if (me->IsWithinDist(who, 50) && !me->IsInCombat() && me->IsValidAttackTarget(who))
                AttackStart(who);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (CheckTimer <= diff)
            {
                if (AnetheronGUID)
                {
                    Creature* boss = Unit::GetCreature((*me), AnetheronGUID);
                    if (!boss || (boss && boss->isDead()))
                    {
                        me->setDeathState(DeathState::JUST_DIED);
                        me->RemoveCorpse();
                        return;
                    }
                }
                CheckTimer = 5000;
            } else CheckTimer -= diff;

            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (ImmolationTimer <= diff)
            {
                DoCast(me, SPELL_IMMOLATION);
                ImmolationTimer = 5000;
            } else ImmolationTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_anetheron()
{
    new boss_anetheron();
    new npc_towering_infernal();
}
