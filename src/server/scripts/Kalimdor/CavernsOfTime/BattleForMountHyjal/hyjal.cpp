/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/
/* ScriptData
SDName: Hyjal
SD%Complete: 80
SDComment: gossip text id's unknown
SDCategory: Caverns of Time, Mount Hyjal
EndScriptData */

/* ContentData
npc_jaina_proudmoore
npc_thrall
npc_tyrande_whisperwind
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "hyjalAI.h"
#include "Player.h"

#define GOSSIP_ITEM_BEGIN_ALLY      "My companions and I are with you, Lady Proudmoore."
#define GOSSIP_ITEM_ANETHERON       "We are ready for whatever Archimonde might send our way, Lady Proudmoore."

#define GOSSIP_ITEM_BEGIN_HORDE     "I am with you, Thrall."
#define GOSSIP_ITEM_AZGALOR         "We have nothing to fear."

#define GOSSIP_ITEM_RETREAT         "We can't keep this up. Let's retreat!"

#define GOSSIP_ITEM_TYRANDE         "Aid us in defending Nordrassil"
#define ITEM_TEAR_OF_GODDESS        24494

#define GOSSIP_ITEM_GM1             "[GM] Toggle Debug Timers"

class npc_jaina_proudmoore : public CreatureScript
{
public:
    npc_jaina_proudmoore() : CreatureScript("npc_jaina_proudmoore") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        hyjalAI* ai = CAST_AI(hyjalAI, creature->AI());
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                ai->StartEvent(player);
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                ai->FirstBossDead = true;
                ai->WaveCount = 9;
                ai->StartEvent(player);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                ai->Retreat();
                break;
             case GOSSIP_ACTION_INFO_DEF:
                ai->Debug = !ai->Debug;
                SF_LOG_DEBUG("scripts", "HyjalAI - Debug mode has been toggled");
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        hyjalAI* ai = CAST_AI(hyjalAI, creature->AI());
        if (ai->EventBegun)
            return false;

        uint32 RageEncounter = ai->GetInstanceData(DATA_RAGEWINTERCHILLEVENT);
        uint32 AnetheronEncounter = ai->GetInstanceData(DATA_ANETHERONEVENT);
        if (RageEncounter == NOT_STARTED)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_BEGIN_ALLY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        else if (RageEncounter == DONE && AnetheronEncounter == NOT_STARTED)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ANETHERON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        else if (RageEncounter == DONE && AnetheronEncounter == DONE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_RETREAT, GOSSIP_SENDER_MAIN,    GOSSIP_ACTION_INFO_DEF + 3);

        if (player->IsGameMaster())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_ITEM_GM1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

        player->SEND_GOSSIP_MENU(907, creature->GetGUID());
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        hyjalAI* ai = new hyjalAI(creature);

        ai->Reset();
        ai->EnterEvadeMode();

        ai->Spells[0].SpellId = SPELL_BLIZZARD;
        ai->Spells[0].Cooldown = std::rand() % 35000 + 15000;
        ai->Spells[0].TargetType = TARGETTYPE_RANDOM;

        ai->Spells[1].SpellId = SPELL_PYROBLAST;
        ai->Spells[1].Cooldown = std::rand() % 9500 + 5500;
        ai->Spells[1].TargetType = TARGETTYPE_RANDOM;

        ai->Spells[2].SpellId = SPELL_SUMMON_ELEMENTALS;
        ai->Spells[2].Cooldown = std::rand() % 45000 + 15000;
        ai->Spells[2].TargetType = TARGETTYPE_SELF;

        return ai;
    }
};

class npc_thrall : public CreatureScript
{
public:
    npc_thrall() : CreatureScript("npc_thrall") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        hyjalAI* ai = CAST_AI(hyjalAI, creature->AI());
        ai->DeSpawnVeins();//despawn the alliance veins
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                ai->StartEvent(player);
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                ai->FirstBossDead = true;
                ai->WaveCount = 9;
                ai->StartEvent(player);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                ai->Retreat();
                break;
            case GOSSIP_ACTION_INFO_DEF:
                ai->Debug = !ai->Debug;
                SF_LOG_DEBUG("scripts", "HyjalAI - Debug mode has been toggled");
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        hyjalAI* ai = CAST_AI(hyjalAI, creature->AI());
        if (ai->EventBegun)
            return false;

        uint32 AnetheronEvent = ai->GetInstanceData(DATA_ANETHERONEVENT);
        // Only let them start the Horde phases if Anetheron is dead.
        if (AnetheronEvent == DONE && ai->GetInstanceData(DATA_ALLIANCE_RETREAT))
        {
            uint32 KazrogalEvent = ai->GetInstanceData(DATA_KAZROGALEVENT);
            uint32 AzgalorEvent  = ai->GetInstanceData(DATA_AZGALOREVENT);
            if (KazrogalEvent == NOT_STARTED)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_BEGIN_HORDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            else if (KazrogalEvent == DONE && AzgalorEvent == NOT_STARTED)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_AZGALOR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            else if (AzgalorEvent == DONE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_RETREAT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }

        if (player->IsGameMaster())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_ITEM_GM1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

        player->SEND_GOSSIP_MENU(907, creature->GetGUID());
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        hyjalAI* ai = new hyjalAI(creature);

        ai->Reset();
        ai->EnterEvadeMode();

        ai->Spells[0].SpellId = SPELL_CHAIN_LIGHTNING;
        ai->Spells[0].Cooldown = std::rand() % 8000 + 3000;
        ai->Spells[0].TargetType = TARGETTYPE_VICTIM;

        ai->Spells[1].SpellId = SPELL_SUMMON_DIRE_WOLF;
        ai->Spells[1].Cooldown = std::rand() % 41000 + 6000;
        ai->Spells[1].TargetType = TARGETTYPE_RANDOM;

        return ai;
    }
};

class npc_tyrande_whisperwind : public CreatureScript
{
public:
    npc_tyrande_whisperwind() : CreatureScript("npc_tyrande_whisperwind") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        hyjalAI* ai = new hyjalAI(creature);
        ai->Reset();
        ai->EnterEvadeMode();
        return ai;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF)
        {
                ItemPosCountVec dest;
                uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, ITEM_TEAR_OF_GODDESS, 1);
                if (msg == EQUIP_ERR_OK)
                     if (Item* item = player->StoreNewItem(dest, ITEM_TEAR_OF_GODDESS, true))
                         player->SendNewItem(item, 1, true, false, true);

                player->SEND_GOSSIP_MENU(907, creature->GetGUID());
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        hyjalAI* ai = CAST_AI(hyjalAI, creature->AI());
        uint32 AzgalorEvent = ai->GetInstanceData(DATA_AZGALOREVENT);

        // Only let them get item if Azgalor is dead.
        if (AzgalorEvent == DONE && !player->HasItemCount(ITEM_TEAR_OF_GODDESS))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        player->SEND_GOSSIP_MENU(907, creature->GetGUID());
        return true;
    }
};

void AddSC_hyjal()
{
    new npc_jaina_proudmoore();
    new npc_thrall();
    new npc_tyrande_whisperwind();
}
