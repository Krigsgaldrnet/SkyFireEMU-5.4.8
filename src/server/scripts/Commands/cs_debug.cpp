/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/* ScriptData
Name: debug_commandscript
%Complete: 100
Comment: All debug related commands
Category: commandscripts
EndScriptData */

#include "BattlegroundMgr.h"
#include "Cell.h"
#include "CellImpl.h"
#include "Chat.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "Transport.h"

class debug_commandscript : public CommandScript
{
public:
    debug_commandscript() : CommandScript("debug_commandscript") { }

    std::vector<ChatCommand> GetCommands() const OVERRIDE
    {
        static std::vector<ChatCommand> debugPlayCommandTable =
        {
            { "cinematic",     rbac::RBAC_PERM_COMMAND_DEBUG_PLAY_CINEMATIC, false, &HandleDebugPlayCinematicCommand,   "", },
            { "movie",         rbac::RBAC_PERM_COMMAND_DEBUG_PLAY_MOVIE,     false, &HandleDebugPlayMovieCommand,       "", },
            { "sound",         rbac::RBAC_PERM_COMMAND_DEBUG_PLAY_SOUND,     false, &HandleDebugPlaySoundCommand,       "", },
        };
        static std::vector<ChatCommand> debugSendCommandTable =
        {
            { "buyerror",      rbac::RBAC_PERM_COMMAND_DEBUG_SEND_BUYERROR,      false, &HandleDebugSendBuyErrorCommand,        "", },
            { "channelnotify", rbac::RBAC_PERM_COMMAND_DEBUG_SEND_CHANNELNOTIFY, false, &HandleDebugSendChannelNotifyCommand,   "", },
            { "chatmessage",   rbac::RBAC_PERM_COMMAND_DEBUG_SEND_CHATMESSAGE,   false, &HandleDebugSendChatMsgCommand,         "", },
            { "equiperror",    rbac::RBAC_PERM_COMMAND_DEBUG_SEND_EQUIPERROR,    false, &HandleDebugSendEquipErrorCommand,      "", },
            { "largepacket",   rbac::RBAC_PERM_COMMAND_DEBUG_SEND_LARGEPACKET,   false, &HandleDebugSendLargePacketCommand,     "", },
            { "qpartymsg",     rbac::RBAC_PERM_COMMAND_DEBUG_SEND_QPARTYMSG,     false, &HandleDebugSendQuestPartyMsgCommand,   "", },
            { "qinvalidmsg",   rbac::RBAC_PERM_COMMAND_DEBUG_SEND_QINVALIDMSG,   false, &HandleDebugSendQuestInvalidMsgCommand, "", },
            { "sellerror",     rbac::RBAC_PERM_COMMAND_DEBUG_SEND_SELLERROR,     false, &HandleDebugSendSellErrorCommand,       "", },
            { "setphaseshift", rbac::RBAC_PERM_COMMAND_DEBUG_SEND_SETPHASESHIFT, false, &HandleDebugSendSetPhaseShiftCommand,   "", },
            { "spellfail",     rbac::RBAC_PERM_COMMAND_DEBUG_SEND_SPELLFAIL,     false, &HandleDebugSendSpellFailCommand,       "", },
        };
        static std::vector<ChatCommand> debugCommandTable =
        {
            { "setbit",        rbac::RBAC_PERM_COMMAND_DEBUG_SETBIT,        false, &HandleDebugSet32BitCommand,         "", },
            { "threat",        rbac::RBAC_PERM_COMMAND_DEBUG_THREAT,        false, &HandleDebugThreatListCommand,       "", },
            { "hostil",        rbac::RBAC_PERM_COMMAND_DEBUG_HOSTIL,        false, &HandleDebugHostileRefListCommand,   "", },
            { "anim",          rbac::RBAC_PERM_COMMAND_DEBUG_ANIM,          false, &HandleDebugAnimCommand,             "", },
            { "arena",         rbac::RBAC_PERM_COMMAND_DEBUG_ARENA,         false, &HandleDebugArenaCommand,            "", },
            { "bg",            rbac::RBAC_PERM_COMMAND_DEBUG_BG,            false, &HandleDebugBattlegroundCommand,     "", },
            { "getitemstate",  rbac::RBAC_PERM_COMMAND_DEBUG_GETITEMSTATE,  false, &HandleDebugGetItemStateCommand,     "", },
            { "lootrecipient", rbac::RBAC_PERM_COMMAND_DEBUG_LOOTRECIPIENT, false, &HandleDebugGetLootRecipientCommand, "", },
            { "getvalue",      rbac::RBAC_PERM_COMMAND_DEBUG_GETVALUE,      false, &HandleDebugGetValueCommand,         "", },
            { "getitemvalue",  rbac::RBAC_PERM_COMMAND_DEBUG_GETITEMVALUE,  false, &HandleDebugGetItemValueCommand,     "", },
            { "Mod32Value",    rbac::RBAC_PERM_COMMAND_DEBUG_MOD32VALUE,    false, &HandleDebugMod32ValueCommand,       "", },
            { "play",          rbac::RBAC_PERM_COMMAND_DEBUG_PLAY,          false, NULL,              "", debugPlayCommandTable },
            { "send",          rbac::RBAC_PERM_COMMAND_DEBUG_SEND,          false, NULL,              "", debugSendCommandTable },
            { "setaurastate",  rbac::RBAC_PERM_COMMAND_DEBUG_SETAURASTATE,  false, &HandleDebugSetAuraStateCommand,     "", },
            { "setitemvalue",  rbac::RBAC_PERM_COMMAND_DEBUG_SETITEMVALUE,  false, &HandleDebugSetItemValueCommand,     "", },
            { "setvalue",      rbac::RBAC_PERM_COMMAND_DEBUG_SETVALUE,      false, &HandleDebugSetValueCommand,         "", },
            { "spawnvehicle",  rbac::RBAC_PERM_COMMAND_DEBUG_SPAWNVEHICLE,  false, &HandleDebugSpawnVehicleCommand,     "", },
            { "setvid",        rbac::RBAC_PERM_COMMAND_DEBUG_SETVID,        false, &HandleDebugSetVehicleIdCommand,     "", },
            { "entervehicle",  rbac::RBAC_PERM_COMMAND_DEBUG_ENTERVEHICLE,  false, &HandleDebugEnterVehicleCommand,     "", },
            { "uws",           rbac::RBAC_PERM_COMMAND_DEBUG_UWS,           false, &HandleDebugUpdateWorldStateCommand, "", },
            { "update",        rbac::RBAC_PERM_COMMAND_DEBUG_UPDATE,        false, &HandleDebugUpdateCommand,           "", },
            { "itemexpire",    rbac::RBAC_PERM_COMMAND_DEBUG_ITEMEXPIRE,    false, &HandleDebugItemExpireCommand,       "", },
            { "areatriggers",  rbac::RBAC_PERM_COMMAND_DEBUG_AREATRIGGERS,  false, &HandleDebugAreaTriggersCommand,     "", },
            { "los",           rbac::RBAC_PERM_COMMAND_DEBUG_LOS,           false, &HandleDebugLoSCommand,              "", },
            { "moveflags",     rbac::RBAC_PERM_COMMAND_DEBUG_MOVEFLAGS,     false, &HandleDebugMoveflagsCommand,        "", },
            { "transport",     rbac::RBAC_PERM_COMMAND_DEBUG_TRANSPORT,     false, &HandleDebugTransportCommand,        "", },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "debug",         rbac::RBAC_PERM_COMMAND_DEBUG,   true,  NULL,               "", debugCommandTable },
            { "wpgps",         rbac::RBAC_PERM_COMMAND_WPGPS,  false, &HandleWPGPSCommand, "", },
        };
        return commandTable;
    }

    static bool HandleDebugPlayCinematicCommand(ChatHandler* handler, char const* args)
    {
        // USAGE: .debug play cinematic #cinematicid
        // #cinematicid - ID decimal number from CinemaicSequences.dbc (1st column)
        if (!*args)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 id = atoi((char*)args);

        if (!sCinematicSequencesStore.LookupEntry(id))
        {
            handler->PSendSysMessage(LANG_CINEMATIC_NOT_EXIST, id);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->GetSession()->GetPlayer()->SendCinematicStart(id);
        return true;
    }

    static bool HandleDebugPlayMovieCommand(ChatHandler* handler, char const* args)
    {
        // USAGE: .debug play movie #movieid
        // #movieid - ID decimal number from Movie.dbc (1st column)
        if (!*args)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 id = atoi((char*)args);

        if (!sMovieStore.LookupEntry(id))
        {
            handler->PSendSysMessage(LANG_MOVIE_NOT_EXIST, id);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->GetSession()->GetPlayer()->SendMovieStart(id);
        return true;
    }

    //Play sound
    static bool HandleDebugPlaySoundCommand(ChatHandler* handler, char const* args)
    {
        // USAGE: .debug playsound #soundid
        // #soundid - ID decimal number from SoundEntries.dbc (1st column)
        if (!*args)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 soundId = atoi((char*)args);

        if (!sSoundEntriesStore.LookupEntry(soundId))
        {
            handler->PSendSysMessage(LANG_SOUND_NOT_EXIST, soundId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Unit* unit = handler->getSelectedUnit();
        if (!unit)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->GetSession()->GetPlayer()->GetTarget())
            unit->PlayDistanceSound(soundId, handler->GetSession()->GetPlayer());
        else
            unit->PlayDirectSound(soundId, handler->GetSession()->GetPlayer());

        handler->PSendSysMessage(LANG_YOU_HEAR_SOUND, soundId);
        return true;
    }

    static bool HandleDebugSendSpellFailCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* result = strtok((char*)args, " ");
        if (!result)
            return false;

        uint8 failNum = (uint8)atoi(result);
        if (failNum == 0 && *result != '0')
            return false;

        char* fail1 = strtok(NULL, " ");
        uint8 failArg1 = fail1 ? (uint8)atoi(fail1) : 0;

        char* fail2 = strtok(NULL, " ");
        uint8 failArg2 = fail2 ? (uint8)atoi(fail2) : 0;

        WorldPacket data(SMSG_CAST_FAILED, 5);
        data << uint8(0);
        data << uint32(133);
        data << uint8(failNum);
        if (fail1 || fail2)
            data << uint32(failArg1);
        if (fail2)
            data << uint32(failArg2);

        handler->GetSession()->SendPacket(&data);

        return true;
    }

    static bool HandleDebugSendEquipErrorCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        InventoryResult msg = InventoryResult(atoi(args));
        handler->GetSession()->GetPlayer()->SendEquipError(msg, NULL, NULL);
        return true;
    }

    static bool HandleDebugSendSellErrorCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        SellResult msg = SellResult(atoi(args));
        handler->GetSession()->GetPlayer()->SendSellResponse(msg, 0, 0);
        return true;
    }

    static bool HandleDebugSendBuyErrorCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        BuyResult msg = BuyResult(atoi(args));
        handler->GetSession()->GetPlayer()->SendBuyFailed(msg, 0, 0);
        return true;
    }

    static bool HandleDebugUpdateWorldStateCommand(ChatHandler* handler, char const* args)
    {
        char* w = strtok((char*)args, " ");
        char* s = strtok(NULL, " ");

        if (!w || !s)
            return false;

        uint32 world = (uint32)atoi(w);
        uint32 state = (uint32)atoi(s);
        handler->GetSession()->GetPlayer()->SendUpdateWorldState(world, state);
        return true;
    }

    static bool HandleDebugAreaTriggersCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player->isDebugAreaTriggers)
        {
            handler->PSendSysMessage(LANG_DEBUG_AREATRIGGER_ON);
            player->isDebugAreaTriggers = true;
        }
        else
        {
            handler->PSendSysMessage(LANG_DEBUG_AREATRIGGER_OFF);
            player->isDebugAreaTriggers = false;
        }
        return true;
    }

    //Send notification in channel
    static bool HandleDebugSendChannelNotifyCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char const* name = "test";
        uint8 code = atoi(args);

        WorldPacket data(SMSG_CHANNEL_NOTIFY, (1 + 10));
        data << code;                                           // notify type
        data << name;                                           // channel name
        data << uint32(0);
        data << uint32(0);
        handler->GetSession()->SendPacket(&data);
        return true;
    }

    //Send notification in chat
    static bool HandleDebugSendChatMsgCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char const* msg = "testtest";
        uint8 type = atoi(args);
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, ChatMsg(type), Language::LANG_UNIVERSAL, handler->GetSession()->GetPlayer(), handler->GetSession()->GetPlayer(), msg, 0, "chan");
        handler->GetSession()->SendPacket(&data);
        return true;
    }

    static bool HandleDebugSendQuestPartyMsgCommand(ChatHandler* handler, char const* args)
    {
        uint32 msg = atol((char*)args);
        handler->GetSession()->GetPlayer()->SendPushToPartyResponse(handler->GetSession()->GetPlayer(), msg);
        return true;
    }

    static bool HandleDebugGetLootRecipientCommand(ChatHandler* handler, char const* /*args*/)
    {
        Creature* target = handler->getSelectedCreature();
        if (!target)
            return false;

        handler->PSendSysMessage("Loot recipient for creature %s (GUID %u, DB GUID %u) is %s",
            target->GetName().c_str(), target->GetGUIDLow(), target->GetDBTableGUIDLow(),
            target->hasLootRecipient() ? (target->GetLootRecipient() ? target->GetLootRecipient()->GetName().c_str() : "offline") : "no loot recipient");
        return true;
    }

    static bool HandleDebugSendQuestInvalidMsgCommand(ChatHandler* handler, char const* args)
    {
        QuestFailedReason msg = static_cast<QuestFailedReason>(atol((char*)args));
        handler->GetSession()->GetPlayer()->SendCanTakeQuestResponse(msg);
        return true;
    }

    static bool HandleDebugGetItemStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string itemState = args;

        ItemUpdateState state = ITEM_UNCHANGED;
        bool listQueue = false;
        bool checkAll = false;

        if (itemState == "unchanged")
            state = ITEM_UNCHANGED;
        else if (itemState == "changed")
            state = ITEM_CHANGED;
        else if (itemState == "new")
            state = ITEM_NEW;
        else if (itemState == "removed")
            state = ITEM_REMOVED;
        else if (itemState == "queue")
            listQueue = true;
        else if (itemState == "check_all")
            checkAll = true;
        else
            return false;

        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();

        if (!listQueue && !checkAll)
        {
            itemState = "The player has the following " + itemState + " items: ";
            handler->SendSysMessage(itemState.c_str());
            for (uint8 i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
            {
                if (i >= BUYBACK_SLOT_START && i < BUYBACK_SLOT_END)
                    continue;

                if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                {
                    if (Bag* bag = item->ToBag())
                    {
                        for (uint8 j = 0; j < bag->GetBagSize(); ++j)
                            if (Item* item2 = bag->GetItemByPos(j))
                                if (item2->GetState() == state)
                                    handler->PSendSysMessage("bag: 255 slot: %d guid: %d owner: %d", item2->GetSlot(), item2->GetGUIDLow(), GUID_LOPART(item2->GetOwnerGUID()));
                    }
                    else if (item->GetState() == state)
                        handler->PSendSysMessage("bag: 255 slot: %d guid: %d owner: %d", item->GetSlot(), item->GetGUIDLow(), GUID_LOPART(item->GetOwnerGUID()));
                }
            }
        }

        if (listQueue)
        {
            std::vector<Item*>& updateQueue = player->GetItemUpdateQueue();
            for (size_t i = 0; i < updateQueue.size(); ++i)
            {
                Item* item = updateQueue[i];
                if (!item)
                    continue;

                Bag* container = item->GetContainer();
                uint8 bagSlot = container ? container->GetSlot() : uint8(INVENTORY_SLOT_BAG_0);

                std::string st;
                switch (item->GetState())
                {
                    case ITEM_UNCHANGED:
                        st = "unchanged";
                        break;
                    case ITEM_CHANGED:
                        st = "changed";
                        break;
                    case ITEM_NEW:
                        st = "new";
                        break;
                    case ITEM_REMOVED:
                        st = "removed";
                        break;
                }

                handler->PSendSysMessage("bag: %d slot: %d guid: %d - state: %s", bagSlot, item->GetSlot(), item->GetGUIDLow(), st.c_str());
            }
            if (updateQueue.empty())
                handler->PSendSysMessage("The player's updatequeue is empty");
        }

        if (checkAll)
        {
            bool error = false;
            std::vector<Item*>& updateQueue = player->GetItemUpdateQueue();
            for (uint8 i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
            {
                if (i >= BUYBACK_SLOT_START && i < BUYBACK_SLOT_END)
                    continue;

                Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
                if (!item)
                    continue;

                if (item->GetSlot() != i)
                {
                    handler->PSendSysMessage("Item with slot %d and guid %d has an incorrect slot value: %d", i, item->GetGUIDLow(), item->GetSlot());
                    error = true;
                    continue;
                }

                if (item->GetOwnerGUID() != player->GetGUID())
                {
                    handler->PSendSysMessage("The item with slot %d and itemguid %d does have non-matching owner guid (%d) and player guid (%d) !", item->GetSlot(), item->GetGUIDLow(), GUID_LOPART(item->GetOwnerGUID()), player->GetGUIDLow());
                    error = true;
                    continue;
                }

                if (Bag* container = item->GetContainer())
                {
                    handler->PSendSysMessage("The item with slot %d and guid %d has a container (slot: %d, guid: %d) but shouldn't!", item->GetSlot(), item->GetGUIDLow(), container->GetSlot(), container->GetGUIDLow());
                    error = true;
                    continue;
                }

                if (item->IsInUpdateQueue())
                {
                    uint16 qp = item->GetQueuePos();
                    if (qp > updateQueue.size())
                    {
                        handler->PSendSysMessage("The item with slot %d and guid %d has its queuepos (%d) larger than the update queue size! ", item->GetSlot(), item->GetGUIDLow(), qp);
                        error = true;
                        continue;
                    }

                    if (updateQueue[qp] == NULL)
                    {
                        handler->PSendSysMessage("The item with slot %d and guid %d has its queuepos (%d) pointing to NULL in the queue!", item->GetSlot(), item->GetGUIDLow(), qp);
                        error = true;
                        continue;
                    }

                    if (updateQueue[qp] != item)
                    {
                        handler->PSendSysMessage("The item with slot %d and guid %d has a queuepos (%d) that points to another item in the queue (bag: %d, slot: %d, guid: %d)", item->GetSlot(), item->GetGUIDLow(), qp, updateQueue[qp]->GetBagSlot(), updateQueue[qp]->GetSlot(), updateQueue[qp]->GetGUIDLow());
                        error = true;
                        continue;
                    }
                }
                else if (item->GetState() != ITEM_UNCHANGED)
                {
                    handler->PSendSysMessage("The item with slot %d and guid %d is not in queue but should be (state: %d)!", item->GetSlot(), item->GetGUIDLow(), item->GetState());
                    error = true;
                    continue;
                }

                if (Bag* bag = item->ToBag())
                {
                    for (uint8 j = 0; j < bag->GetBagSize(); ++j)
                    {
                        Item* item2 = bag->GetItemByPos(j);
                        if (!item2)
                            continue;

                        if (item2->GetSlot() != j)
                        {
                            handler->PSendSysMessage("The item in bag %d and slot %d (guid: %d) has an incorrect slot value: %d", bag->GetSlot(), j, item2->GetGUIDLow(), item2->GetSlot());
                            error = true;
                            continue;
                        }

                        if (item2->GetOwnerGUID() != player->GetGUID())
                        {
                            handler->PSendSysMessage("The item in bag %d at slot %d and with itemguid %d, the owner's guid (%d) and the player's guid (%d) don't match!", bag->GetSlot(), item2->GetSlot(), item2->GetGUIDLow(), GUID_LOPART(item2->GetOwnerGUID()), player->GetGUIDLow());
                            error = true;
                            continue;
                        }

                        Bag* container = item2->GetContainer();
                        if (!container)
                        {
                            handler->PSendSysMessage("The item in bag %d at slot %d with guid %d has no container!", bag->GetSlot(), item2->GetSlot(), item2->GetGUIDLow());
                            error = true;
                            continue;
                        }

                        if (container != bag)
                        {
                            handler->PSendSysMessage("The item in bag %d at slot %d with guid %d has a different container(slot %d guid %d)!", bag->GetSlot(), item2->GetSlot(), item2->GetGUIDLow(), container->GetSlot(), container->GetGUIDLow());
                            error = true;
                            continue;
                        }

                        if (item2->IsInUpdateQueue())
                        {
                            uint16 qp = item2->GetQueuePos();
                            if (qp > updateQueue.size())
                            {
                                handler->PSendSysMessage("The item in bag %d at slot %d having guid %d has a queuepos (%d) larger than the update queue size! ", bag->GetSlot(), item2->GetSlot(), item2->GetGUIDLow(), qp);
                                error = true;
                                continue;
                            }

                            if (updateQueue[qp] == NULL)
                            {
                                handler->PSendSysMessage("The item in bag %d at slot %d having guid %d has a queuepos (%d) that points to NULL in the queue!", bag->GetSlot(), item2->GetSlot(), item2->GetGUIDLow(), qp);
                                error = true;
                                continue;
                            }

                            if (updateQueue[qp] != item2)
                            {
                                handler->PSendSysMessage("The item in bag %d at slot %d having guid %d has a queuepos (%d) that points to another item in the queue (bag: %d, slot: %d, guid: %d)", bag->GetSlot(), item2->GetSlot(), item2->GetGUIDLow(), qp, updateQueue[qp]->GetBagSlot(), updateQueue[qp]->GetSlot(), updateQueue[qp]->GetGUIDLow());
                                error = true;
                                continue;
                            }
                        }
                        else if (item2->GetState() != ITEM_UNCHANGED)
                        {
                            handler->PSendSysMessage("The item in bag %d at slot %d having guid %d is not in queue but should be (state: %d)!", bag->GetSlot(), item2->GetSlot(), item2->GetGUIDLow(), item2->GetState());
                            error = true;
                            continue;
                        }
                    }
                }
            }

            for (size_t i = 0; i < updateQueue.size(); ++i)
            {
                Item* item = updateQueue[i];
                if (!item)
                    continue;

                if (item->GetOwnerGUID() != player->GetGUID())
                {
                    handler->PSendSysMessage("queue(" SIZEFMTD "): For the item with guid %d, the owner's guid (%d) and the player's guid (%d) don't match!", i, item->GetGUIDLow(), GUID_LOPART(item->GetOwnerGUID()), player->GetGUIDLow());
                    error = true;
                    continue;
                }

                if (item->GetQueuePos() != i)
                {
                    handler->PSendSysMessage("queue(" SIZEFMTD "): For the item with guid %d, the queuepos doesn't match it's position in the queue!", i, item->GetGUIDLow());
                    error = true;
                    continue;
                }

                if (item->GetState() == ITEM_REMOVED)
                    continue;

                Item* test = player->GetItemByPos(item->GetBagSlot(), item->GetSlot());

                if (test == NULL)
                {
                    handler->PSendSysMessage("queue(" SIZEFMTD "): The bag(%d) and slot(%d) values for the item with guid %d are incorrect, the player doesn't have any item at that position!", i, item->GetBagSlot(), item->GetSlot(), item->GetGUIDLow());
                    error = true;
                    continue;
                }

                if (test != item)
                {
                    handler->PSendSysMessage("queue(" SIZEFMTD "): The bag(%d) and slot(%d) values for the item with guid %d are incorrect, an item which guid is %d is there instead!", i, item->GetBagSlot(), item->GetSlot(), item->GetGUIDLow(), test->GetGUIDLow());
                    error = true;
                    continue;
                }
            }
            if (!error)
                handler->SendSysMessage("All OK!");
        }

        return true;
    }

    static bool HandleDebugBattlegroundCommand(ChatHandler* /*handler*/, char const* /*args*/)
    {
        sBattlegroundMgr->ToggleTesting();
        return true;
    }

    static bool HandleDebugArenaCommand(ChatHandler* /*handler*/, char const* /*args*/)
    {
        sBattlegroundMgr->ToggleArenaTesting();
        return true;
    }

    static bool HandleDebugThreatListCommand(ChatHandler* handler, char const* /*args*/)
    {
        Creature* target = handler->getSelectedCreature();
        if (!target || target->IsTotem() || target->IsPet())
            return false;

        ThreatContainer::StorageType const& threatList = target->getThreatManager().getThreatList();
        ThreatContainer::StorageType::const_iterator itr;
        uint32 count = 0;
        handler->PSendSysMessage("Threat list of %s (guid %u)", target->GetName().c_str(), target->GetGUIDLow());
        for (itr = threatList.begin(); itr != threatList.end(); ++itr)
        {
            Unit* unit = (*itr)->getTarget();
            if (!unit)
                continue;
            ++count;
            handler->PSendSysMessage("   %u.   %s   (guid %u)  - threat %f", count, unit->GetName().c_str(), unit->GetGUIDLow(), (*itr)->getThreat());
        }
        handler->SendSysMessage("End of threat list.");
        return true;
    }

    static bool HandleDebugHostileRefListCommand(ChatHandler* handler, char const* /*args*/)
    {
        Unit* target = handler->getSelectedUnit();
        if (!target)
            target = handler->GetSession()->GetPlayer();
        HostileReference* ref = target->getHostileRefManager().getFirst();
        uint32 count = 0;
        handler->PSendSysMessage("Hostil reference list of %s (guid %u)", target->GetName().c_str(), target->GetGUIDLow());
        while (ref)
        {
            if (Unit* unit = ref->GetSource()->GetOwner())
            {
                ++count;
                handler->PSendSysMessage("   %u.   %s   (guid %u)  - threat %f", count, unit->GetName().c_str(), unit->GetGUIDLow(), ref->getThreat());
            }
            ref = ref->next();
        }
        handler->SendSysMessage("End of hostil reference list.");
        return true;
    }

    static bool HandleDebugSetVehicleIdCommand(ChatHandler* handler, char const* args)
    {
        Unit* target = handler->getSelectedUnit();
        if (!target || target->IsVehicle())
            return false;

        if (!args)
            return false;

        char* i = strtok((char*)args, " ");
        if (!i)
            return false;

        uint32 id = (uint32)atoi(i);
        //target->SetVehicleId(id);
        handler->PSendSysMessage("Vehicle id set to %u", id);
        return true;
    }

    static bool HandleDebugEnterVehicleCommand(ChatHandler* handler, char const* args)
    {
        Unit* target = handler->getSelectedUnit();
        if (!target || !target->IsVehicle())
            return false;

        if (!args)
            return false;

        char* i = strtok((char*)args, " ");
        if (!i)
            return false;

        char* j = strtok(NULL, " ");

        uint32 entry = (uint32)atoi(i);
        int8 seatId = j ? (int8)atoi(j) : -1;

        if (!entry)
            handler->GetSession()->GetPlayer()->EnterVehicle(target, seatId);
        else
        {
            Creature* passenger = NULL;
            Skyfire::AllCreaturesOfEntryInRange check(handler->GetSession()->GetPlayer(), entry, 20.0f);
            Skyfire::CreatureSearcher<Skyfire::AllCreaturesOfEntryInRange> searcher(handler->GetSession()->GetPlayer(), passenger, check);
            handler->GetSession()->GetPlayer()->VisitNearbyObject(30.0f, searcher);
            if (!passenger || passenger == target)
                return false;
            passenger->EnterVehicle(target, seatId);
        }

        handler->PSendSysMessage("Unit %u entered vehicle %d", entry, (int32)seatId);
        return true;
    }

    static bool HandleDebugSpawnVehicleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* e = strtok((char*)args, " ");
        char* i = strtok(NULL, " ");

        if (!e)
            return false;

        uint32 entry = (uint32)atoi(e);

        float x, y, z, o = handler->GetSession()->GetPlayer()->GetOrientation();
        handler->GetSession()->GetPlayer()->GetClosePoint(x, y, z, handler->GetSession()->GetPlayer()->GetObjectSize());

        if (!i)
            return handler->GetSession()->GetPlayer()->SummonCreature(entry, x, y, z, o);

        uint32 id = (uint32)atoi(i);

        CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(entry);

        if (!ci)
            return false;

        VehicleEntry const* ve = sVehicleStore.LookupEntry(id);

        if (!ve)
            return false;

        Creature* v = new Creature;

        Map* map = handler->GetSession()->GetPlayer()->GetMap();

        if (!v->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_VEHICLE), map, entry, id, handler->GetSession()->GetPlayer()->GetTeam(), x, y, z, o))
        {
            delete v;
            return false;
        }

        for (auto phase : handler->GetSession()->GetPlayer()->GetPhases())
            v->SetPhased(phase, false, true);

        map->AddToMap(v->ToCreature());

        return true;
    }

    static bool HandleDebugSendLargePacketCommand(ChatHandler* handler, char const* /*args*/)
    {
        const char* stuffingString = "This is a dummy string to push the packet's size beyond 128000 bytes. ";
        std::ostringstream ss;
        while (ss.str().size() < 128000)
            ss << stuffingString;
        handler->SendSysMessage(ss.str().c_str());
        return true;
    }

    static bool HandleDebugSendSetPhaseShiftCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* t = strtok((char*)args, " ");
        char* p = strtok(NULL, " ");

        if (!t)
            return false;

        std::set<uint32> terrainswap;
        std::set<uint32> phaseId;
        std::set<uint32> worldMapArea;

        terrainswap.insert((uint32)atoi(t));

        if (p)
            phaseId.insert((uint32)atoi(p));

        handler->GetSession()->SendSetPhaseShift(phaseId, terrainswap, worldMapArea);
        return true;
    }

    static bool HandleDebugGetItemValueCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* e = strtok((char*)args, " ");
        char* f = strtok(NULL, " ");

        if (!e || !f)
            return false;

        uint32 guid = (uint32)atoi(e);
        uint32 index = (uint32)atoi(f);

        Item* i = handler->GetSession()->GetPlayer()->GetItemByGuid(MAKE_NEW_GUID(guid, 0, HIGHGUID_ITEM));

        if (!i)
            return false;

        if (index >= i->GetValuesCount())
            return false;

        uint32 value = i->GetUInt32Value(index);

        handler->PSendSysMessage("Item %u: value at %u is %u", guid, index, value);

        return true;
    }

    static bool HandleDebugSetItemValueCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* e = strtok((char*)args, " ");
        char* f = strtok(NULL, " ");
        char* g = strtok(NULL, " ");

        if (!e || !f || !g)
            return false;

        uint32 guid = (uint32)atoi(e);
        uint32 index = (uint32)atoi(f);
        uint32 value = (uint32)atoi(g);

        Item* i = handler->GetSession()->GetPlayer()->GetItemByGuid(MAKE_NEW_GUID(guid, 0, HIGHGUID_ITEM));

        if (!i)
            return false;

        if (index >= i->GetValuesCount())
            return false;

        i->SetUInt32Value(index, value);

        return true;
    }

    static bool HandleDebugItemExpireCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* e = strtok((char*)args, " ");
        if (!e)
            return false;

        uint32 guid = (uint32)atoi(e);

        Item* i = handler->GetSession()->GetPlayer()->GetItemByGuid(MAKE_NEW_GUID(guid, 0, HIGHGUID_ITEM));

        if (!i)
            return false;

        handler->GetSession()->GetPlayer()->DestroyItem(i->GetBagSlot(), i->GetSlot(), true);
        sScriptMgr->OnItemExpire(handler->GetSession()->GetPlayer(), i->GetTemplate());

        return true;
    }

    //show animation
    static bool HandleDebugAnimCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 animId = atoi((char*)args);
        handler->GetSession()->GetPlayer()->HandleEmoteCommand(animId);
        return true;
    }

    static bool HandleDebugLoSCommand(ChatHandler* handler, char const* /*args*/)
    {
        if (Unit* unit = handler->getSelectedUnit())
            handler->PSendSysMessage("Unit %s (GuidLow: %u) is %sin LoS", unit->GetName().c_str(), unit->GetGUIDLow(), handler->GetSession()->GetPlayer()->IsWithinLOSInMap(unit) ? "" : "not ");
        return true;
    }

    static bool HandleDebugSetAuraStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Unit* unit = handler->getSelectedUnit();
        if (!unit)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        int32 state = atoi((char*)args);
        if (!state)
        {
            // reset all states
            for (int i = 1; i <= 32; ++i)
                unit->ModifyAuraState(AuraStateType(i), false);
            return true;
        }

        unit->ModifyAuraState(AuraStateType(abs(state)), state > 0);
        return true;
    }

    static bool HandleDebugSetValueCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* x = strtok((char*)args, " ");
        char* y = strtok(NULL, " ");
        char* z = strtok(NULL, " ");

        if (!x || !y)
            return false;

        WorldObject* target = handler->getSelectedObject();
        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint64 guid = target->GetGUID();

        uint32 opcode = (uint32)atoi(x);
        if (opcode >= target->GetValuesCount())
        {
            handler->PSendSysMessage(LANG_TOO_BIG_INDEX, opcode, GUID_LOPART(guid), target->GetValuesCount());
            return false;
        }

        bool isInt32 = true;
        if (z)
            isInt32 = (bool)atoi(z);

        if (isInt32)
        {
            uint32 value = (uint32)atoi(y);
            target->SetUInt32Value(opcode, value);
            handler->PSendSysMessage(LANG_SET_UINT_FIELD, GUID_LOPART(guid), opcode, value);
        }
        else
        {
            float value = (float)atof(y);
            target->SetFloatValue(opcode, value);
            handler->PSendSysMessage(LANG_SET_FLOAT_FIELD, GUID_LOPART(guid), opcode, value);
        }

        return true;
    }

    static bool HandleDebugGetValueCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* x = strtok((char*)args, " ");
        char* z = strtok(NULL, " ");

        if (!x)
            return false;

        Unit* target = handler->getSelectedUnit();
        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint64 guid = target->GetGUID();

        uint32 opcode = (uint32)atoi(x);
        if (opcode >= target->GetValuesCount())
        {
            handler->PSendSysMessage(LANG_TOO_BIG_INDEX, opcode, GUID_LOPART(guid), target->GetValuesCount());
            return false;
        }

        bool isInt32 = true;
        if (z)
            isInt32 = (bool)atoi(z);

        if (isInt32)
        {
            uint32 value = target->GetUInt32Value(opcode);
            handler->PSendSysMessage(LANG_GET_UINT_FIELD, GUID_LOPART(guid), opcode, value);
        }
        else
        {
            float value = target->GetFloatValue(opcode);
            handler->PSendSysMessage(LANG_GET_FLOAT_FIELD, GUID_LOPART(guid), opcode, value);
        }

        return true;
    }

    static bool HandleDebugMod32ValueCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* x = strtok((char*)args, " ");
        char* y = strtok(NULL, " ");

        if (!x || !y)
            return false;

        uint32 opcode = (uint32)atoi(x);
        int value = atoi(y);

        if (opcode >= handler->GetSession()->GetPlayer()->GetValuesCount())
        {
            handler->PSendSysMessage(LANG_TOO_BIG_INDEX, opcode, handler->GetSession()->GetPlayer()->GetGUIDLow(), handler->GetSession()->GetPlayer()->GetValuesCount());
            return false;
        }

        int currentValue = (int)handler->GetSession()->GetPlayer()->GetUInt32Value(opcode);

        currentValue += value;
        handler->GetSession()->GetPlayer()->SetUInt32Value(opcode, (uint32)currentValue);

        handler->PSendSysMessage(LANG_CHANGE_32BIT_FIELD, opcode, currentValue);

        return true;
    }

    static bool HandleDebugUpdateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 updateIndex;
        uint32 value;

        char* index = strtok((char*)args, " ");

        Unit* unit = handler->getSelectedUnit();
        if (!unit)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!index)
            return true;

        updateIndex = atoi(index);
        //check updateIndex
        if (unit->GetTypeId() == TypeID::TYPEID_PLAYER)
        {
            if (updateIndex >= PLAYER_END)
                return true;
        }
        else if (updateIndex >= UNIT_END)
            return true;

        char* val = strtok(NULL, " ");
        if (!val)
        {
            value = unit->GetUInt32Value(updateIndex);

            handler->PSendSysMessage(LANG_UPDATE, unit->GetGUIDLow(), updateIndex, value);
            return true;
        }

        value = atoi(val);

        handler->PSendSysMessage(LANG_UPDATE_CHANGE, unit->GetGUIDLow(), updateIndex, value);

        unit->SetUInt32Value(updateIndex, value);

        return true;
    }

    static bool HandleDebugSet32BitCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        WorldObject* target = handler->getSelectedObject();
        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* x = strtok((char*)args, " ");
        char* y = strtok(NULL, " ");

        if (!x || !y)
            return false;

        uint32 opcode = (uint32)atoi(x);
        uint32 val = (uint32)atoi(y);
        if (val > 32)                                         //uint32 = 32 bits
            return false;

        uint32 value = val ? 1 << (val - 1) : 0;
        target->SetUInt32Value(opcode, value);

        handler->PSendSysMessage(LANG_SET_32BIT_FIELD, opcode, value);
        return true;
    }

    static bool HandleDebugMoveflagsCommand(ChatHandler* handler, char const* args)
    {
        Unit* target = handler->getSelectedUnit();
        if (!target)
            target = handler->GetSession()->GetPlayer();

        if (!*args)
        {
            //! Display case
            handler->PSendSysMessage(LANG_MOVEFLAGS_GET, target->GetUnitMovementFlags(), target->GetExtraUnitMovementFlags());
        }
        else
        {
            char* mask1 = strtok((char*)args, " ");
            if (!mask1)
                return false;

            char* mask2 = strtok(NULL, " \n");

            uint32 moveFlags = (uint32)atoi(mask1);
            target->SetUnitMovementFlags(moveFlags);

            /// @fixme: port master's HandleDebugMoveflagsCommand; flags need different handling

            if (mask2)
            {
                uint32 moveFlagsExtra = uint32(atoi(mask2));
                target->SetExtraUnitMovementFlags(moveFlagsExtra);
            }

            if (target->GetTypeId() != TypeID::TYPEID_PLAYER)
                target->DestroyForNearbyPlayers();  // Force new SMSG_UPDATE_OBJECT:CreateObject
            else
            {
                WorldPacket data(SMSG_PLAYER_MOVE);
                target->WriteMovementInfo(data);
                target->SendMessageToSet(&data, true);
            }

            handler->PSendSysMessage(LANG_MOVEFLAGS_SET, target->GetUnitMovementFlags(), target->GetExtraUnitMovementFlags());
        }

        return true;
    }

    static bool HandleWPGPSCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        SF_LOG_INFO("sql.dev", "(@PATH, XX, %.3f, %.3f, %.5f, 0, 0, 0, 100, 0),", player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());

        handler->PSendSysMessage("Waypoint SQL written to SQL Developer log");
        return true;
    }

    static bool HandleDebugTransportCommand(ChatHandler* handler, char const* args)
    {
        Transport* transport = handler->GetSession()->GetPlayer()->GetTransport();
        if (!transport)
            return false;

        bool start = false;
        if (!stricmp(args, "stop"))
            transport->EnableMovement(false);
        else if (!stricmp(args, "start"))
        {
            transport->EnableMovement(true);
            start = true;
        }
        else
        {
            handler->PSendSysMessage("Transport %s is %s", transport->GetName().c_str(), transport->GetGoState() == GOState::GO_STATE_READY ? "stopped" : "moving");
            return true;
        }

        handler->PSendSysMessage("Transport %s %s", transport->GetName().c_str(), start ? "started" : "stopped");
        return true;
    }
};

void AddSC_debug_commandscript()
{
    new debug_commandscript();
}
