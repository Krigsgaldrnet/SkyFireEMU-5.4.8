/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: boss_archaedas
SD%Complete: 100
SDComment: Archaedas is activated when 1 person (was 3, changed in 3.0.8) clicks on his altar.
Every 10 seconds he will awaken one of his minions along the wall.
At 66%, he will awaken the 6 Guardians.
At 33%, he will awaken the Vault Walkers
On his death the vault door opens.
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "uldaman.h"
#include "Player.h"

enum Says
{
    SAY_AGGRO                   = 0,
    SAY_SUMMON_GUARDIANS        = 1,
    SAY_SUMMON_VAULT_WALKERS    = 2,
    SAY_KILL                    = 3
};

enum Spells
{
    SPELL_GROUND_TREMOR              = 6524,
    SPELL_ARCHAEDAS_AWAKEN           = 10347,
    SPELL_BOSS_OBJECT_VISUAL         = 11206,
    SPELL_BOSS_AGGRO                 = 10340,
    SPELL_SUB_BOSS_AGGRO             = 11568,
    SPELL_AWAKEN_VAULT_WALKER        = 10258,
    SPELL_AWAKEN_EARTHEN_GUARDIAN    = 10252,
    SPELL_SELF_DESTRUCT              = 9874
};

class boss_archaedas : public CreatureScript
{
public:
    boss_archaedas() : CreatureScript("boss_archaedas") { }

    struct boss_archaedasAI : public ScriptedAI
    {
        boss_archaedasAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        uint32 uiTremorTimer;
        int32  iAwakenTimer;
        uint32 uiWallMinionTimer;
        bool bWakingUp;

        bool bGuardiansAwake;
        bool bVaultWalkersAwake;
        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            uiTremorTimer = 60000;
            iAwakenTimer = 0;
            uiWallMinionTimer = 10000;

            bWakingUp = false;
            bGuardiansAwake = false;
            bVaultWalkersAwake = false;

            if (instance)
                instance->SetData(0, 5);    // respawn any dead minions
            me->setFaction(35);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        void ActivateMinion(uint64 uiGuid, bool flag)
        {
            Unit* minion = Unit::GetUnit(*me, uiGuid);

            if (minion && minion->IsAlive())
            {
                DoCast(minion, SPELL_AWAKEN_VAULT_WALKER, flag);
                minion->CastSpell(minion, SPELL_ARCHAEDAS_AWAKEN, true);
                minion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                minion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                minion->setFaction(14);
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            me->setFaction(14);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        void SpellHit(Unit* /*caster*/, const SpellInfo* spell) OVERRIDE
        {
            // Being woken up from the altar, start the awaken sequence
            if (spell == sSpellMgr->GetSpellInfo(SPELL_ARCHAEDAS_AWAKEN))
            {
                Talk(SAY_AGGRO);
                iAwakenTimer = 4000;
                bWakingUp = true;
            }
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_KILL);
        }

        void UpdateAI(uint32 uiDiff) OVERRIDE
        {
            if (!instance)
                return;
            // we're still doing awaken animation
            if (bWakingUp && iAwakenTimer >= 0)
            {
                iAwakenTimer -= uiDiff;
                return;        // dont do anything until we are done
            } else if (bWakingUp && iAwakenTimer <= 0)
            {
                bWakingUp = false;
                AttackStart(Unit::GetUnit(*me, instance->GetData64(0)));
                return;     // dont want to continue until we finish the AttackStart method
            }

            //Return since we have no target
            if (!UpdateVictim())
                return;

            // wake a wall minion
            if (uiWallMinionTimer <= uiDiff)
            {
                instance->SetData(DATA_MINIONS, IN_PROGRESS);

                uiWallMinionTimer = 10000;
            } else uiWallMinionTimer -= uiDiff;

            //If we are <66 summon the guardians
            if (!bGuardiansAwake && !HealthAbovePct(66))
            {
                ActivateMinion(instance->GetData64(5), true);   // EarthenGuardian1
                ActivateMinion(instance->GetData64(6), true);   // EarthenGuardian2
                ActivateMinion(instance->GetData64(7), true);   // EarthenGuardian3
                ActivateMinion(instance->GetData64(8), true);   // EarthenGuardian4
                ActivateMinion(instance->GetData64(9), true);   // EarthenGuardian5
                ActivateMinion(instance->GetData64(10), false); // EarthenGuardian6
                Talk(SAY_SUMMON_GUARDIANS);
                bGuardiansAwake = true;
            }

            //If we are <33 summon the vault walkers
            if (!bVaultWalkersAwake && !HealthAbovePct(33))
            {
                ActivateMinion(instance->GetData64(1), true);    // VaultWalker1
                ActivateMinion(instance->GetData64(2), true);    // VaultWalker2
                ActivateMinion(instance->GetData64(3), true);    // VaultWalker3
                ActivateMinion(instance->GetData64(4), false);    // VaultWalker4
                Talk(SAY_SUMMON_VAULT_WALKERS);
                bVaultWalkersAwake = true;
            }

            if (uiTremorTimer <= uiDiff)
            {
                //Cast
                DoCastVictim(SPELL_GROUND_TREMOR);

                //45 seconds until we should cast this agian
                uiTremorTimer  = 45000;
            } else uiTremorTimer  -= uiDiff;

            DoMeleeAttackIfReady();
        }

        void JustDied (Unit* /*killer*/) OVERRIDE
        {
            if (instance)
            {
                instance->SetData(DATA_ANCIENT_DOOR, DONE);      // open the vault door
                instance->SetData(DATA_MINIONS, SPECIAL);        // deactivate his minions
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_archaedasAI(creature);
    }
};

/* ScriptData
SDName: npc_archaedas_minions
SD%Complete: 100
SDComment: These mobs are initially frozen until Archaedas awakens them
one at a time.
EndScriptData */

class npc_archaedas_minions : public CreatureScript
{
public:
    npc_archaedas_minions() : CreatureScript("npc_archaedas_minions") { }

    struct npc_archaedas_minionsAI : public ScriptedAI
    {
        npc_archaedas_minionsAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        uint32 uiArcing_Timer;
        int32 iAwakenTimer;
        bool bWakingUp;

        bool bAmIAwake;
        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            uiArcing_Timer = 3000;
            iAwakenTimer = 0;

            bWakingUp = false;
            bAmIAwake = false;

            me->setFaction(35);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->RemoveAllAuras();
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            me->setFaction (14);
            me->RemoveAllAuras();
            me->RemoveFlag (UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveFlag (UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            bAmIAwake = true;
        }

        void SpellHit(Unit * /*caster*/, const SpellInfo* spell) OVERRIDE
        {
            // time to wake up, start animation
            if (spell == sSpellMgr->GetSpellInfo(SPELL_ARCHAEDAS_AWAKEN))
            {
                iAwakenTimer = 5000;
                bWakingUp = true;
            }
        }

        void MoveInLineOfSight(Unit* who) OVERRIDE

        {
            if (bAmIAwake)
                ScriptedAI::MoveInLineOfSight(who);
        }

        void UpdateAI(uint32 uiDiff) OVERRIDE
        {
            // we're still in the awaken animation
            if (bWakingUp && iAwakenTimer >= 0)
            {
                iAwakenTimer -= uiDiff;
                return;        // dont do anything until we are done
            } else if (bWakingUp && iAwakenTimer <= 0)
            {
                bWakingUp = false;
                bAmIAwake = true;
                // AttackStart(Unit::GetUnit(*me, instance->GetData64(0))); // whoWokeArchaedasGUID
                return;     // dont want to continue until we finish the AttackStart method
            }

            //Return since we have no target
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_archaedas_minionsAI(creature);
    }
};

/* ScriptData
SDName: npc_stonekeepers
SD%Complete: 100
SDComment: After activating the altar of the keepers, the stone keepers will
wake up one by one.
EndScriptData */

class npc_stonekeepers : public CreatureScript
{
public:
    npc_stonekeepers() : CreatureScript("npc_stonekeepers") { }

    struct npc_stonekeepersAI : public ScriptedAI
    {
        npc_stonekeepersAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            me->setFaction(35);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->RemoveAllAuras();
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            me->setFaction(14);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        void UpdateAI(uint32 /*diff*/) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*attacker*/) OVERRIDE
        {
            DoCast (me, SPELL_SELF_DESTRUCT, true);
            if (instance)
                instance->SetData(DATA_STONE_KEEPERS, IN_PROGRESS);    // activate next stonekeeper
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_stonekeepersAI(creature);
    }
};

/* ScriptData
SDName: go_altar_archaedas
SD%Complete: 100
SDComment: Needs 1 person to activate the Archaedas script
SDCategory: Uldaman
EndScriptData */

class go_altar_of_archaedas : public GameObjectScript
{
public:
    go_altar_of_archaedas() : GameObjectScript("go_altar_of_archaedas") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) OVERRIDE
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (!instance)
            return false;

        player->CastSpell (player, SPELL_BOSS_OBJECT_VISUAL, false);

        instance->SetData64(0, player->GetGUID());     // activate archaedas
        return false;
    }
};

//This is the actual function called only once durring InitScripts()
//It must define all handled functions that are to be run in this script
void AddSC_boss_archaedas()
{
    new boss_archaedas();
    new npc_archaedas_minions();
    new npc_stonekeepers();
    new go_altar_of_archaedas();
}

