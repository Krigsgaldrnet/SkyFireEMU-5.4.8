/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "AccountMgr.h"
#include "Common.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "SocialMgr.h"
#include "Spell.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::SendTradeStatus(TradeStatus status)
{
    WorldPacket data;

    data.Initialize(SMSG_TRADE_STATUS, 1 + 4 + 4);
    data.WriteBit(0); // unk bit, usually 0
    data.WriteBits(status, 5);

    switch (status)
    {
        case TRADE_STATUS_PROPOSED:
            data.WriteBits(0, 8); // zero guid
            data.FlushBits();
            break;
        case TRADE_STATUS_INITIATED:
            data.FlushBits();
            data << uint32(0); // unk
            break;
        case TRADE_STATUS_FAILED:
            data.WriteBit(0); // unk
            data.FlushBits();
            data << uint32(0); // unk
            data << uint32(0); // unk
            break;
        case TRADE_STATUS_WRONG_REALM:
        case TRADE_STATUS_NOT_ON_TAPLIST:
            data.FlushBits();
            data << uint8(0); // unk
            break;
        case TRADE_STATUS_NOT_ENOUGH_CURRENCY: // Not implemented
        case TRADE_STATUS_CURRENCY_NOT_TRADABLE: // Not implemented
            data.FlushBits();
            data << uint32(0); // unk
            data << uint32(0); // unk
        default:
            data.FlushBits();
            break;
    }

    SendPacket(&data);
}

void WorldSession::HandleIgnoreTradeOpcode(WorldPacket& /*recvPacket*/)
{
    SF_LOG_DEBUG("network", "WORLD: Ignore Trade %u", _player->GetGUIDLow());
}

void WorldSession::HandleBusyTradeOpcode(WorldPacket& /*recvPacket*/)
{
    SF_LOG_DEBUG("network", "WORLD: Busy Trade %u", _player->GetGUIDLow());
}

void WorldSession::SendUpdateTrade(bool trader_data /*= true*/)
{
    TradeData* view_trade = trader_data ? _player->GetTradeData()->GetTraderData() : _player->GetTradeData();

    ByteBuffer itemData(7 * 2 + 7 * 4 + 3 * 4 + 3 * 4 + 1);

    uint8 count = 0;
    for (uint8 i = 0; i < TRADE_SLOT_COUNT; ++i)
        if (view_trade->GetItem(TradeSlots(i)))
            ++count;

    WorldPacket data(SMSG_TRADE_STATUS_EXTENDED, 4 * 6 + 8 + 1 + 3 + count * 70);
    data << uint32(0);                                      // this value must be equal to value from TRADE_STATUS_INITIATED status packet (different value for different players to block multiple trades?)
    data << uint32(0);                                      // unk 2
    data << uint32(view_trade->GetSpell());                 // spell casted on lowest slot item
    data << uint8(trader_data);                             // 1 means traders data, 0 means own
    data << uint64(view_trade->GetMoney());                 // trader gold
    data << uint32(TRADE_SLOT_COUNT);                       // trade slots count/number?, = next field in most cases
    data << uint32(0);                                      // unk 5
    data << uint32(TRADE_SLOT_COUNT);                       // trade slots count/number?, = prev field in most cases
    data.WriteBits(count, 20);

    for (uint8 i = 0; i < TRADE_SLOT_COUNT; ++i)
    {
        Item* item = view_trade->GetItem(TradeSlots(i));
        if (!item)
            continue;

        ObjectGuid giftCreatorGuid = item->GetUInt64Value(ITEM_FIELD_GIFT_CREATOR);
        ObjectGuid creatorGuid = item->GetUInt64Value(ITEM_FIELD_CREATOR);

        bool notWrapped = data.WriteBit(!item->HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_WRAPPED));
        data.WriteBit(giftCreatorGuid[2]);

        if (notWrapped)
        {
            data.WriteBit(creatorGuid[3]);
            data.WriteBit(creatorGuid[5]);
            data.WriteBit(creatorGuid[1]);
            data.WriteBit(creatorGuid[6]);
            data.WriteBit(creatorGuid[0]);
            data.WriteBit(item->GetTemplate()->LockID != 0);
            data.WriteBit(creatorGuid[4]);
            data.WriteBit(creatorGuid[7]);
            data.WriteBit(creatorGuid[2]);

            itemData.WriteByteSeq(creatorGuid[3]);

            itemData << uint32(item->GetUInt32Value(ITEM_FIELD_MAX_DURABILITY));
            itemData << uint32(0);
            itemData << uint32(item->GetDynamicUInt32Value(ITEM_DYNAMIC_MODIFIERS, 0));

            itemData.WriteByteSeq(creatorGuid[1]);
            itemData.WriteByteSeq(creatorGuid[5]);
            itemData.WriteByteSeq(creatorGuid[7]);
            itemData.WriteByteSeq(creatorGuid[6]);
            itemData.WriteByteSeq(creatorGuid[0]);

            itemData << uint32(item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
            itemData << uint32(item->GetUInt32Value(ITEM_FIELD_DURABILITY)); // ok

            itemData.WriteByteSeq(creatorGuid[2]);

            for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS /*3*/; ++enchant_slot)
                itemData << uint32(item->GetEnchantmentId(EnchantmentSlot(enchant_slot)));

            itemData << uint32(item->GetItemRandomPropertyId());
            itemData << uint32(item->GetSpellCharges());
            itemData << uint32(item->GetItemSuffixFactor());

            itemData.WriteByteSeq(creatorGuid[4]);
        }

        data.WriteBit(giftCreatorGuid[0]);
        data.WriteBit(giftCreatorGuid[4]);
        data.WriteBit(giftCreatorGuid[7]);
        data.WriteBit(giftCreatorGuid[3]);
        data.WriteBit(giftCreatorGuid[6]);
        data.WriteBit(giftCreatorGuid[1]);
        data.WriteBit(giftCreatorGuid[5]);

        itemData.WriteByteSeq(giftCreatorGuid[4]);

        itemData << uint8(i);

        itemData.WriteByteSeq(giftCreatorGuid[5]);
        itemData.WriteByteSeq(giftCreatorGuid[1]);
        itemData.WriteByteSeq(giftCreatorGuid[2]);
        itemData.WriteByteSeq(giftCreatorGuid[3]);

        itemData << uint32(item->GetTemplate()->ItemId);

        itemData.WriteByteSeq(giftCreatorGuid[7]);
        itemData.WriteByteSeq(giftCreatorGuid[0]);

        itemData << uint32(item->GetCount());

        itemData.WriteByteSeq(giftCreatorGuid[6]);
    }

    data.FlushBits();
    data.append(itemData);

    SendPacket(&data);
}

//==============================================================
// transfer the items to the players

void WorldSession::moveItems(Item* myItems[], Item* hisItems[])
{
    Player* trader = _player->GetTrader();
    if (!trader)
        return;

    for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
    {
        ItemPosCountVec traderDst;
        ItemPosCountVec playerDst;
        bool traderCanTrade = (myItems[i] == NULL || trader->CanStoreItem(NULL_BAG, NULL_SLOT, traderDst, myItems[i], false) == EQUIP_ERR_OK);
        bool playerCanTrade = (hisItems[i] == NULL || _player->CanStoreItem(NULL_BAG, NULL_SLOT, playerDst, hisItems[i], false) == EQUIP_ERR_OK);
        if (traderCanTrade && playerCanTrade)
        {
            // Ok, if trade item exists and can be stored
            // If we trade in both directions we had to check, if the trade will work before we actually do it
            // A roll back is not possible after we stored it
            if (myItems[i])
            {
                // logging
                SF_LOG_DEBUG("network", "partner storing: %u", myItems[i]->GetGUIDLow());
                if (HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
                {
                    sLog->outCommand(_player->GetSession()->GetAccountId(), "GM %s (Account: %u) trade: %s (Entry: %d Count: %u) to player: %s (Account: %u)",
                        _player->GetName().c_str(), _player->GetSession()->GetAccountId(),
                        myItems[i]->GetTemplate()->Name1.c_str(), myItems[i]->GetEntry(), myItems[i]->GetCount(),
                        trader->GetName().c_str(), trader->GetSession()->GetAccountId());
                }

                // adjust time (depends on /played)
                if (myItems[i]->HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_BOP_TRADEABLE))
                    myItems[i]->SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, trader->GetTotalPlayedTime() - (_player->GetTotalPlayedTime() - myItems[i]->GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME)));
                // store
                trader->MoveItemToInventory(traderDst, myItems[i], true, true);
            }
            if (hisItems[i])
            {
                // logging
                SF_LOG_DEBUG("network", "player storing: %u", hisItems[i]->GetGUIDLow());
                if (HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
                {
                    sLog->outCommand(trader->GetSession()->GetAccountId(), "GM %s (Account: %u) trade: %s (Entry: %d Count: %u) to player: %s (Account: %u)",
                        trader->GetName().c_str(), trader->GetSession()->GetAccountId(),
                        hisItems[i]->GetTemplate()->Name1.c_str(), hisItems[i]->GetEntry(), hisItems[i]->GetCount(),
                        _player->GetName().c_str(), _player->GetSession()->GetAccountId());
                }

                // adjust time (depends on /played)
                if (hisItems[i]->HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_BOP_TRADEABLE))
                    hisItems[i]->SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, _player->GetTotalPlayedTime() - (trader->GetTotalPlayedTime() - hisItems[i]->GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME)));
                // store
                _player->MoveItemToInventory(playerDst, hisItems[i], true, true);
            }
        }
        else
        {
            // in case of fatal error log error message
            // return the already removed items to the original owner
            if (myItems[i])
            {
                if (!traderCanTrade)
                    SF_LOG_ERROR("network", "trader can't store item: %u", myItems[i]->GetGUIDLow());
                if (_player->CanStoreItem(NULL_BAG, NULL_SLOT, playerDst, myItems[i], false) == EQUIP_ERR_OK)
                    _player->MoveItemToInventory(playerDst, myItems[i], true, true);
                else
                    SF_LOG_ERROR("network", "player can't take item back: %u", myItems[i]->GetGUIDLow());
            }
            // return the already removed items to the original owner
            if (hisItems[i])
            {
                if (!playerCanTrade)
                    SF_LOG_ERROR("network", "player can't store item: %u", hisItems[i]->GetGUIDLow());
                if (trader->CanStoreItem(NULL_BAG, NULL_SLOT, traderDst, hisItems[i], false) == EQUIP_ERR_OK)
                    trader->MoveItemToInventory(traderDst, hisItems[i], true, true);
                else
                    SF_LOG_ERROR("network", "trader can't take item back: %u", hisItems[i]->GetGUIDLow());
            }
        }
    }
}

//==============================================================

static void setAcceptTradeMode(TradeData* myTrade, TradeData* hisTrade, Item** myItems, Item** hisItems)
{
    myTrade->SetInAcceptProcess(true);
    hisTrade->SetInAcceptProcess(true);

    // store items in local list and set 'in-trade' flag
    for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
    {
        if (Item* item = myTrade->GetItem(TradeSlots(i)))
        {
            SF_LOG_DEBUG("network", "player trade item %u bag: %u slot: %u", item->GetGUIDLow(), item->GetBagSlot(), item->GetSlot());
            //Can return NULL
            myItems[i] = item;
            myItems[i]->SetInTrade();
        }

        if (Item* item = hisTrade->GetItem(TradeSlots(i)))
        {
            SF_LOG_DEBUG("network", "partner trade item %u bag: %u slot: %u", item->GetGUIDLow(), item->GetBagSlot(), item->GetSlot());
            hisItems[i] = item;
            hisItems[i]->SetInTrade();
        }
    }
}

static void clearAcceptTradeMode(TradeData* myTrade, TradeData* hisTrade)
{
    myTrade->SetInAcceptProcess(false);
    hisTrade->SetInAcceptProcess(false);
}

static void clearAcceptTradeMode(Item** myItems, Item** hisItems)
{
    // clear 'in-trade' flag
    for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
    {
        if (myItems[i])
            myItems[i]->SetInTrade(false);
        if (hisItems[i])
            hisItems[i]->SetInTrade(false);
    }
}

void WorldSession::HandleAcceptTradeOpcode(WorldPacket& /*recvPacket*/)
{
    TradeData* my_trade = _player->m_trade;
    if (!my_trade)
        return;

    Player* trader = my_trade->GetTrader();

    TradeData* his_trade = trader->m_trade;
    if (!his_trade)
        return;

    Item* myItems[TRADE_SLOT_TRADED_COUNT] = { NULL, NULL, NULL, NULL, NULL, NULL };
    Item* hisItems[TRADE_SLOT_TRADED_COUNT] = { NULL, NULL, NULL, NULL, NULL, NULL };

    // set before checks for propertly undo at problems (it already set in to client)
    my_trade->SetAccepted(true);

    // not accept case incorrect money amount
    if (!_player->HasEnoughMoney(my_trade->GetMoney()))
    {
        SendNotification(LANG_NOT_ENOUGH_GOLD);
        my_trade->SetAccepted(false, true);
        return;
    }

    // not accept case incorrect money amount
    if (!trader->HasEnoughMoney(his_trade->GetMoney()))
    {
        trader->GetSession()->SendNotification(LANG_NOT_ENOUGH_GOLD);
        his_trade->SetAccepted(false, true);
        return;
    }

    if (_player->GetMoney() >= uint64(MAX_MONEY_AMOUNT) - his_trade->GetMoney())
    {
        _player->SendEquipError(EQUIP_ERR_TOO_MUCH_GOLD, NULL, NULL);
        my_trade->SetAccepted(false, true);
        return;
    }

    if (trader->GetMoney() >= uint64(MAX_MONEY_AMOUNT) - my_trade->GetMoney())
    {
        trader->SendEquipError(EQUIP_ERR_TOO_MUCH_GOLD, NULL, NULL);
        his_trade->SetAccepted(false, true);
        return;
    }

    // not accept if some items now can't be trade (cheating)
    for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
    {
        if (Item* item = my_trade->GetItem(TradeSlots(i)))
        {
            if (!item->CanBeTraded(false, true))
            {
                SendTradeStatus(TRADE_STATUS_CANCELLED);
                return;
            }

            if (item->IsBindedNotWith(trader))
            {
                SendTradeStatus(TRADE_STATUS_NOT_ON_TAPLIST);
                SendTradeStatus(TRADE_STATUS_FAILED/*TRADE_STATUS_CANCELLED*/);
                return;
            }
        }

        if (Item* item = his_trade->GetItem(TradeSlots(i)))
        {
            if (!item->CanBeTraded(false, true))
            {
                SendTradeStatus(TRADE_STATUS_CANCELLED);
                return;
            }
            //if (item->IsBindedNotWith(_player))   // dont mark as invalid when his item isnt good (not exploitable because if item is invalid trade will fail anyway later on the same check)
            //{
            //    SendTradeStatus(TRADE_STATUS_NOT_ON_TAPLIST);
            //    his_trade->SetAccepted(false, true);
            //    return;
            //}
        }
    }

    if (his_trade->IsAccepted())
    {
        setAcceptTradeMode(my_trade, his_trade, myItems, hisItems);

        Spell* my_spell = NULL;
        SpellCastTargets my_targets;

        Spell* his_spell = NULL;
        SpellCastTargets his_targets;

        // not accept if spell can't be casted now (cheating)
        if (uint32 my_spell_id = my_trade->GetSpell())
        {
            SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(my_spell_id);
            Item* castItem = my_trade->GetSpellCastItem();

            if (!spellEntry || !his_trade->GetItem(TRADE_SLOT_NONTRADED) ||
                (my_trade->HasSpellCastItem() && !castItem))
            {
                clearAcceptTradeMode(my_trade, his_trade);
                clearAcceptTradeMode(myItems, hisItems);

                my_trade->SetSpell(0);
                return;
            }

            my_spell = new Spell(_player, spellEntry, TRIGGERED_FULL_MASK);
            my_spell->m_CastItem = castItem;
            my_targets.SetTradeItemTarget(_player);
            my_spell->m_targets = my_targets;

            SpellCastResult res = my_spell->CheckCast(true);
            if (res != SpellCastResult::SPELL_CAST_OK)
            {
                my_spell->SendCastResult(res);

                clearAcceptTradeMode(my_trade, his_trade);
                clearAcceptTradeMode(myItems, hisItems);

                delete my_spell;
                my_trade->SetSpell(0);
                return;
            }
        }

        // not accept if spell can't be casted now (cheating)
        if (uint32 his_spell_id = his_trade->GetSpell())
        {
            SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(his_spell_id);
            Item* castItem = his_trade->GetSpellCastItem();

            if (!spellEntry || !my_trade->GetItem(TRADE_SLOT_NONTRADED) || (his_trade->HasSpellCastItem() && !castItem))
            {
                delete my_spell;
                his_trade->SetSpell(0);

                clearAcceptTradeMode(my_trade, his_trade);
                clearAcceptTradeMode(myItems, hisItems);
                return;
            }

            his_spell = new Spell(trader, spellEntry, TRIGGERED_FULL_MASK);
            his_spell->m_CastItem = castItem;
            his_targets.SetTradeItemTarget(trader);
            his_spell->m_targets = his_targets;

            SpellCastResult res = his_spell->CheckCast(true);
            if (res != SpellCastResult::SPELL_CAST_OK)
            {
                his_spell->SendCastResult(res);

                clearAcceptTradeMode(my_trade, his_trade);
                clearAcceptTradeMode(myItems, hisItems);

                delete my_spell;
                delete his_spell;

                his_trade->SetSpell(0);
                return;
            }
        }

        // inform partner client
        trader->GetSession()->SendTradeStatus(TRADE_STATUS_ACCEPTED);

        // test if item will fit in each inventory
        bool hisCanCompleteTrade = (trader->CanStoreItems(myItems, TRADE_SLOT_TRADED_COUNT) == EQUIP_ERR_OK);
        bool myCanCompleteTrade = (_player->CanStoreItems(hisItems, TRADE_SLOT_TRADED_COUNT) == EQUIP_ERR_OK);

        clearAcceptTradeMode(myItems, hisItems);

        // in case of missing space report error
        if (!myCanCompleteTrade)
        {
            clearAcceptTradeMode(my_trade, his_trade);

            SendNotification(LANG_NOT_FREE_TRADE_SLOTS);
            trader->GetSession()->SendNotification(LANG_NOT_PARTNER_FREE_TRADE_SLOTS);
            my_trade->SetAccepted(false);
            his_trade->SetAccepted(false);
            return;
        }
        else if (!hisCanCompleteTrade)
        {
            clearAcceptTradeMode(my_trade, his_trade);

            SendNotification(LANG_NOT_PARTNER_FREE_TRADE_SLOTS);
            trader->GetSession()->SendNotification(LANG_NOT_FREE_TRADE_SLOTS);
            my_trade->SetAccepted(false);
            his_trade->SetAccepted(false);
            return;
        }

        // execute trade: 1. remove
        for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
        {
            if (myItems[i])
            {
                myItems[i]->SetUInt64Value(ITEM_FIELD_GIFT_CREATOR, _player->GetGUID());
                _player->MoveItemFromInventory(myItems[i]->GetBagSlot(), myItems[i]->GetSlot(), true);
            }
            if (hisItems[i])
            {
                hisItems[i]->SetUInt64Value(ITEM_FIELD_GIFT_CREATOR, trader->GetGUID());
                trader->MoveItemFromInventory(hisItems[i]->GetBagSlot(), hisItems[i]->GetSlot(), true);
            }
        }

        // execute trade: 2. store
        moveItems(myItems, hisItems);

        // logging money
        if (HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
        {
            if (my_trade->GetMoney() > 0)
            {
                sLog->outCommand(_player->GetSession()->GetAccountId(), "GM %s (Account: %u) give money (Amount: " UI64FMTD ") to player: %s (Account: %u)",
                    _player->GetName().c_str(), _player->GetSession()->GetAccountId(),
                    my_trade->GetMoney(),
                    trader->GetName().c_str(), trader->GetSession()->GetAccountId());
            }

            if (his_trade->GetMoney() > 0)
            {
                sLog->outCommand(trader->GetSession()->GetAccountId(), "GM %s (Account: %u) give money (Amount: " UI64FMTD ") to player: %s (Account: %u)",
                    trader->GetName().c_str(), trader->GetSession()->GetAccountId(),
                    his_trade->GetMoney(),
                    _player->GetName().c_str(), _player->GetSession()->GetAccountId());
            }
        }

        // update money
        _player->ModifyMoney(-int64(my_trade->GetMoney()));
        _player->ModifyMoney(his_trade->GetMoney());
        trader->ModifyMoney(-int64(his_trade->GetMoney()));
        trader->ModifyMoney(my_trade->GetMoney());

        if (my_spell)
            my_spell->prepare(&my_targets);

        if (his_spell)
            his_spell->prepare(&his_targets);

        // cleanup
        clearAcceptTradeMode(my_trade, his_trade);
        delete _player->m_trade;
        _player->m_trade = NULL;
        delete trader->m_trade;
        trader->m_trade = NULL;

        // desynchronized with the other saves here (SaveInventoryAndGoldToDB() not have own transaction guards)
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        _player->SaveInventoryAndGoldToDB(trans);
        trader->SaveInventoryAndGoldToDB(trans);
        CharacterDatabase.CommitTransaction(trans);

        trader->GetSession()->SendTradeStatus(TRADE_STATUS_COMPLETE);
        SendTradeStatus(TRADE_STATUS_COMPLETE);
    }
    else
    {
        trader->GetSession()->SendTradeStatus(TRADE_STATUS_ACCEPTED);
    }
}

void WorldSession::HandleUnacceptTradeOpcode(WorldPacket& /*recvPacket*/)
{
    TradeData* my_trade = _player->GetTradeData();
    if (!my_trade)
        return;

    my_trade->SetAccepted(false, true);
}

void WorldSession::HandleBeginTradeOpcode(WorldPacket& /*recvPacket*/)
{
    TradeData* my_trade = _player->m_trade;
    if (!my_trade)
        return;

    my_trade->GetTrader()->GetSession()->SendTradeStatus(TRADE_STATUS_INITIATED);
    SendTradeStatus(TRADE_STATUS_INITIATED);
}

void WorldSession::SendCancelTrade()
{
    if (PlayerRecentlyLoggedOut() || PlayerLogout())
        return;

    SendTradeStatus(TRADE_STATUS_CANCELLED);
}

void WorldSession::HandleCancelTradeOpcode(WorldPacket& /*recvPacket*/)
{
    // sent also after LOGOUT COMPLETE
    if (_player)                                             // needed because STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT
        _player->TradeCancel(true);
}

void WorldSession::HandleInitiateTradeOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guid;

    guid[5] = recvPacket.ReadBit();
    guid[1] = recvPacket.ReadBit();
    guid[4] = recvPacket.ReadBit();
    guid[2] = recvPacket.ReadBit();
    guid[3] = recvPacket.ReadBit();
    guid[7] = recvPacket.ReadBit();
    guid[0] = recvPacket.ReadBit();
    guid[6] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guid[4]);
    recvPacket.ReadByteSeq(guid[6]);
    recvPacket.ReadByteSeq(guid[2]);
    recvPacket.ReadByteSeq(guid[0]);
    recvPacket.ReadByteSeq(guid[3]);
    recvPacket.ReadByteSeq(guid[7]);
    recvPacket.ReadByteSeq(guid[5]);
    recvPacket.ReadByteSeq(guid[1]);

    if (GetPlayer()->m_trade)
        return;

    if (!GetPlayer()->IsAlive())
    {
        SendTradeStatus(TRADE_STATUS_DEAD);
        return;
    }

    if (GetPlayer()->HasUnitState(UNIT_STATE_STUNNED))
    {
        SendTradeStatus(TRADE_STATUS_STUNNED);
        return;
    }

    if (isLogingOut())
    {
        SendTradeStatus(TRADE_STATUS_LOGGING_OUT);
        return;
    }

    if (GetPlayer()->IsInFlight())
    {
        SendTradeStatus(TRADE_STATUS_TOO_FAR_AWAY);
        return;
    }

    if (GetPlayer()->getLevel() < sWorld->getIntConfig(WorldIntConfigs::CONFIG_TRADE_LEVEL_REQ))
    {
        SendNotification(GetSkyFireString(LANG_TRADE_REQ), sWorld->getIntConfig(WorldIntConfigs::CONFIG_TRADE_LEVEL_REQ));
        return;
    }

    Player* pOther = ObjectAccessor::FindPlayer(guid);

    if (!pOther)
    {
        SendTradeStatus(TRADE_STATUS_PLAYER_NOT_FOUND);
        return;
    }

    if (pOther == GetPlayer() || pOther->m_trade)
    {
        SendTradeStatus(TRADE_STATUS_ALREADY_TRADING);
        return;
    }

    if (!pOther->IsAlive())
    {
        SendTradeStatus(TRADE_STATUS_TARGET_DEAD);
        return;
    }

    if (pOther->IsInFlight())
    {
        SendTradeStatus(TRADE_STATUS_TOO_FAR_AWAY);
        return;
    }

    if (pOther->HasUnitState(UNIT_STATE_STUNNED))
    {
        SendTradeStatus(TRADE_STATUS_TARGET_STUNNED);
        return;
    }

    if (pOther->GetSession()->isLogingOut())
    {
        SendTradeStatus(TRADE_STATUS_TARGET_LOGGING_OUT);
        return;
    }

    if (pOther->GetSocial()->HasIgnore(GetPlayer()->GetGUIDLow()))
    {
        SendTradeStatus(TRADE_STATUS_PLAYER_IGNORED);
        return;
    }

    if (!sWorld->GetBoolConfig(WorldBoolConfigs::CONFIG_ALLOW_TWO_SIDE_TRADE) && pOther->GetTeam() != _player->GetTeam())
    {
        SendTradeStatus(TRADE_STATUS_WRONG_FACTION);
        return;
    }

    if (!pOther->IsWithinDistInMap(_player, 10.0f, false))
    {
        SendTradeStatus(TRADE_STATUS_TOO_FAR_AWAY);
        return;
    }

    if (pOther->getLevel() < sWorld->getIntConfig(WorldIntConfigs::CONFIG_TRADE_LEVEL_REQ))
    {
        SendNotification(GetSkyFireString(LANG_TRADE_OTHER_REQ), sWorld->getIntConfig(WorldIntConfigs::CONFIG_TRADE_LEVEL_REQ));
        return;
    }

    // OK start trade
    _player->m_trade = new TradeData(_player, pOther);
    pOther->m_trade = new TradeData(pOther, _player);

    WorldPacket data(SMSG_TRADE_STATUS, 2 + 7);
    data.WriteBit(0); // unk bit, usually 0
    data.WriteBits(TRADE_STATUS_PROPOSED, 5);

    ObjectGuid playerGuid = _player->GetGUID();
    // WTB StartBitStream...
    data.WriteBit(playerGuid[6]);
    data.WriteBit(playerGuid[2]);
    data.WriteBit(playerGuid[1]);
    data.WriteBit(playerGuid[4]);
    data.WriteBit(playerGuid[7]);
    data.WriteBit(playerGuid[3]);
    data.WriteBit(playerGuid[0]);
    data.WriteBit(playerGuid[5]);

    data.WriteByteSeq(playerGuid[6]);
    data.WriteByteSeq(playerGuid[2]);
    data.WriteByteSeq(playerGuid[1]);
    data.WriteByteSeq(playerGuid[7]);
    data.WriteByteSeq(playerGuid[5]);
    data.WriteByteSeq(playerGuid[4]);
    data.WriteByteSeq(playerGuid[0]);
    data.WriteByteSeq(playerGuid[3]);

    pOther->GetSession()->SendPacket(&data);
}

void WorldSession::HandleSetTradeGoldOpcode(WorldPacket& recvPacket)
{
    uint64 gold;
    recvPacket >> gold;

    TradeData* my_trade = _player->GetTradeData();
    if (!my_trade)
        return;

    Player* trader = my_trade->GetTrader();

    if (trader->getVirtualRealm() != _player->getVirtualRealm())
    {
        SendTradeStatus(TRADE_STATUS_WRONG_REALM);
        return;
    }

    // gold can be incorrect, but this is checked at trade finished.
    my_trade->SetMoney(gold);
}

void WorldSession::HandleSetTradeItemOpcode(WorldPacket& recvPacket)
{
    // send update
    uint8 tradeSlot;
    uint8 bag;
    uint8 slot;

    recvPacket >> tradeSlot;
    recvPacket >> slot;
    recvPacket >> bag;

    TradeData* my_trade = _player->GetTradeData();
    if (!my_trade)
        return;

    // invalid slot number
    if (tradeSlot >= TRADE_SLOT_COUNT)
    {
        SendTradeStatus(TRADE_STATUS_CANCELLED);
        return;
    }

    // check cheating, can't fail with correct client operations
    Item* item = _player->GetItemByPos(bag, slot);
    if (!item || (tradeSlot != TRADE_SLOT_NONTRADED && !item->CanBeTraded(false, true)))
    {
        SendTradeStatus(TRADE_STATUS_CANCELLED);
        return;
    }

    Player* trader = my_trade->GetTrader();

    if (trader->getVirtualRealm() != _player->getVirtualRealm())
    {
        if (item->GetTemplate()->Class != ITEM_CLASS_CONSUMABLE)
        {
            SendTradeStatus(TRADE_STATUS_WRONG_REALM);
            return;
        }
    }

    uint64 iGUID = item->GetGUID();

    // prevent place single item into many trade slots using cheating and client bugs
    if (my_trade->HasItem(iGUID))
    {
        // cheating attempt
        SendTradeStatus(TRADE_STATUS_CANCELLED);
        return;
    }

    my_trade->SetItem(TradeSlots(tradeSlot), item);
}

void WorldSession::HandleClearTradeItemOpcode(WorldPacket& recvPacket)
{
    uint8 tradeSlot;
    recvPacket >> tradeSlot;

    TradeData* my_trade = _player->m_trade;
    if (!my_trade)
        return;

    // invalid slot number
    if (tradeSlot >= TRADE_SLOT_COUNT)
        return;

    my_trade->SetItem(TradeSlots(tradeSlot), NULL);
}
