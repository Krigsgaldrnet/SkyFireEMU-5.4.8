/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Razorfen_Downs
SD%Complete: 100
SDComment: Support for Henry Stern(2 recipes)
SDCategory: Razorfen Downs
EndScriptData */

/* ContentData
npc_henry_stern
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "razorfen_downs.h"
#include "Player.h"

/*###
# npc_henry_stern
####*/

enum Spells
{
    SPELL_TEACHING_GOLDTHORN_TEA                = 13029,
    SPELL_TEACHING_MIGHTY_TROLLS_BLOOD_POTION   = 13030
};

enum Gossips
{
    GOSSIP_COOKING_SKILL_HIGH                   = 1444,
    GOSSIP_COOKING_SKILL_LOW                    = 1501,
    GOSSIP_ALCHEMY_SKILL_HIGH                   = 1442,
    GOSSIP_ALCHEMY_SKILL_LOW                    = 1502
};

class npc_henry_stern : public CreatureScript
{
public:
    npc_henry_stern() : CreatureScript("npc_henry_stern") { }

    struct npc_henry_sternAI : public ScriptedAI
    {
        npc_henry_sternAI(Creature* creature) : ScriptedAI(creature) { }

        void sGossipSelect(Player* player, uint32 /*sender*/, uint32 action) OVERRIDE
        {
            if (action == 0)
            {
                if (player->GetBaseSkillValue(SKILL_COOKING) >= 175)
                {
                    player->PrepareGossipMenu(me, GOSSIP_COOKING_SKILL_HIGH);
                    player->SendPreparedGossip(me);
                    DoCast(player, SPELL_TEACHING_GOLDTHORN_TEA);
                }
                else
                {
                    player->PrepareGossipMenu(me, GOSSIP_COOKING_SKILL_LOW);
                    player->SendPreparedGossip(me);
                }
            }

            if (action == 1)
            {
                if (player->GetBaseSkillValue(SKILL_ALCHEMY) >= 180)
                {
                    player->PrepareGossipMenu(me, GOSSIP_ALCHEMY_SKILL_HIGH);
                    player->SendPreparedGossip(me);
                    DoCast(player, SPELL_TEACHING_MIGHTY_TROLLS_BLOOD_POTION);
                }
                else
                {
                    player->PrepareGossipMenu(me, GOSSIP_ALCHEMY_SKILL_LOW);
                    player->SendPreparedGossip(me);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_henry_sternAI(creature);
    }
};

/*######
## go_gong
######*/

class go_gong : public GameObjectScript
{
public:
    go_gong() : GameObjectScript("go_gong") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go) OVERRIDE
    {
        //basic support, not blizzlike data is missing...
        InstanceScript* instance = go->GetInstanceScript();

        if (instance)
        {
            instance->SetData(DATA_GONG_WAVES, instance->GetData(DATA_GONG_WAVES)+1);
            return true;
        }

        return false;
    }
};

enum TombCreature
{
    SPELL_WEB                   = 745
};

class npc_tomb_creature : public CreatureScript
{
public:
    npc_tomb_creature() : CreatureScript("npc_tomb_creature") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_tomb_creatureAI(creature);
    }

    struct npc_tomb_creatureAI : public ScriptedAI
    {
        npc_tomb_creatureAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        uint32 uiWebTimer;

        void Reset() OVERRIDE
        {
            uiWebTimer = std::rand() % 8000 + 5000;
        }

        void UpdateAI(uint32 uiDiff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            //from acid
            if (me->GetEntry() == NPC_TOMB_REAVER)
            {
                if (uiWebTimer <= uiDiff)
                {
                    DoCastVictim(SPELL_WEB);
                    uiWebTimer = std::rand() % 16000 + 7000;
                } else uiWebTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (instance)
                instance->SetData(DATA_GONG_WAVES, instance->GetData(DATA_GONG_WAVES)+1);
        }
    };
};

void AddSC_razorfen_downs()
{
    new npc_henry_stern();
    new go_gong();
    new npc_tomb_creature();
}
