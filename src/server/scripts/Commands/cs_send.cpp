/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Chat.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"

class send_commandscript : public CommandScript
{
public:
    send_commandscript() : CommandScript("send_commandscript") { }

    std::vector<ChatCommand> GetCommands() const OVERRIDE
    {
        static std::vector<ChatCommand> sendCommandTable =
        {
            { "items",   rbac::RBAC_PERM_COMMAND_SEND_ITEMS,   true, &HandleSendItemsCommand,   "", },
            { "mail",    rbac::RBAC_PERM_COMMAND_SEND_MAIL,    true, &HandleSendMailCommand,    "", },
            { "message", rbac::RBAC_PERM_COMMAND_SEND_MESSAGE, true, &HandleSendMessageCommand, "", },
            { "money",   rbac::RBAC_PERM_COMMAND_SEND_MONEY,   true, &HandleSendMoneyCommand,   "", },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "send", rbac::RBAC_PERM_COMMAND_SEND, false, NULL, "", sendCommandTable },
        };
        return commandTable;
    }

    // Send mail by command
    static bool HandleSendMailCommand(ChatHandler* handler, char const* args)
    {
        // format: name "subject text" "mail text"
        Player* target;
        uint64 targetGuid;
        std::string targetName;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, &targetName))
            return false;

        char* tail1 = strtok(NULL, "");
        if (!tail1)
            return false;

        char const* msgSubject = handler->extractQuotedArg(tail1);
        if (!msgSubject)
            return false;

        char* tail2 = strtok(NULL, "");
        if (!tail2)
            return false;

        char const* msgText = handler->extractQuotedArg(tail2);
        if (!msgText)
            return false;

        // msgSubject, msgText isn't NUL after prev. check
        std::string subject = msgSubject;
        std::string text = msgText;

        // from console show not existed sender
        MailSender sender(MAIL_NORMAL, handler->GetSession() ? handler->GetSession()->GetPlayer()->GetGUIDLow() : 0, MAIL_STATIONERY_GM);

        /// @todo Fix poor design
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        MailDraft(subject, text)
            .SendMailTo(trans, MailReceiver(target, GUID_LOPART(targetGuid)), sender);

        CharacterDatabase.CommitTransaction(trans);

        std::string nameLink = handler->playerLink(targetName);
        handler->PSendSysMessage(LANG_MAIL_SENT, nameLink.c_str());
        return true;
    }

    // Send items by mail
    static bool HandleSendItemsCommand(ChatHandler* handler, char const* args)
    {
        // format: name "subject text" "mail text" item1[:count1] item2[:count2] ... item12[:count12]
        Player* receiver;
        uint64 receiverGuid;
        std::string receiverName;
        if (!handler->extractPlayerTarget((char*)args, &receiver, &receiverGuid, &receiverName))
            return false;

        char* tail1 = strtok(NULL, "");
        if (!tail1)
            return false;

        char const* msgSubject = handler->extractQuotedArg(tail1);
        if (!msgSubject)
            return false;

        char* tail2 = strtok(NULL, "");
        if (!tail2)
            return false;

        char const* msgText = handler->extractQuotedArg(tail2);
        if (!msgText)
            return false;

        // msgSubject, msgText isn't NUL after prev. check
        std::string subject = msgSubject;
        std::string text = msgText;

        // extract items
        typedef std::pair<uint32, uint32> ItemPair;
        typedef std::list< ItemPair > ItemPairs;
        ItemPairs items;

        // get all tail string
        char* tail = strtok(NULL, "");

        // get from tail next item str
        while (char* itemStr = strtok(tail, " "))
        {
            // and get new tail
            tail = strtok(NULL, "");

            // parse item str
            char const* itemIdStr = strtok(itemStr, ":");
            char const* itemCountStr = strtok(NULL, " ");

            uint32 itemId = atoi(itemIdStr);
            if (!itemId)
                return false;

            ItemTemplate const* item_proto = sObjectMgr->GetItemTemplate(itemId);
            if (!item_proto)
            {
                handler->PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, itemId);
                handler->SetSentErrorMessage(true);
                return false;
            }

            uint32 itemCount = itemCountStr ? atoi(itemCountStr) : 1;
            if (itemCount < 1 || (item_proto->MaxCount > 0 && itemCount > uint32(item_proto->MaxCount)))
            {
                handler->PSendSysMessage(LANG_COMMAND_INVALID_ITEM_COUNT, itemCount, itemId);
                handler->SetSentErrorMessage(true);
                return false;
            }

            while (itemCount > item_proto->GetMaxStackSize())
            {
                items.push_back(ItemPair(itemId, item_proto->GetMaxStackSize()));
                itemCount -= item_proto->GetMaxStackSize();
            }

            items.push_back(ItemPair(itemId, itemCount));

            if (items.size() > MAX_MAIL_ITEMS)
            {
                handler->PSendSysMessage(LANG_COMMAND_MAIL_ITEMS_LIMIT, MAX_MAIL_ITEMS);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // from console show not existed sender
        MailSender sender(MAIL_NORMAL, handler->GetSession() ? handler->GetSession()->GetPlayer()->GetGUIDLow() : 0, MAIL_STATIONERY_GM);

        // fill mail
        MailDraft draft(subject, text);

        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        for (ItemPairs::const_iterator itr = items.begin(); itr != items.end(); ++itr)
        {
            if (Item* item = Item::CreateItem(itr->first, itr->second, handler->GetSession() ? handler->GetSession()->GetPlayer() : 0))
            {
                item->SaveToDB(trans);                               // save for prevent lost at next mail load, if send fail then item will deleted
                draft.AddItem(item);
            }
        }

        draft.SendMailTo(trans, MailReceiver(receiver, GUID_LOPART(receiverGuid)), sender);
        CharacterDatabase.CommitTransaction(trans);

        std::string nameLink = handler->playerLink(receiverName);
        handler->PSendSysMessage(LANG_MAIL_SENT, nameLink.c_str());
        return true;
    }
    /// Send money by mail
    static bool HandleSendMoneyCommand(ChatHandler* handler, char const* args)
    {
        /// format: name "subject text" "mail text" money

        Player* receiver;
        uint64 receiverGuid;
        std::string receiverName;
        if (!handler->extractPlayerTarget((char*)args, &receiver, &receiverGuid, &receiverName))
            return false;

        char* tail1 = strtok(NULL, "");
        if (!tail1)
            return false;

        char* msgSubject = handler->extractQuotedArg(tail1);
        if (!msgSubject)
            return false;

        char* tail2 = strtok(NULL, "");
        if (!tail2)
            return false;

        char* msgText = handler->extractQuotedArg(tail2);
        if (!msgText)
            return false;

        char* moneyStr = strtok(NULL, "");
        int32 money = moneyStr ? atoi(moneyStr) : 0;
        if (money <= 0)
            return false;

        // msgSubject, msgText isn't NUL after prev. check
        std::string subject = msgSubject;
        std::string text = msgText;

        // from console show not existed sender
        MailSender sender(MAIL_NORMAL, handler->GetSession() ? handler->GetSession()->GetPlayer()->GetGUIDLow() : 0, MAIL_STATIONERY_GM);

        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        MailDraft(subject, text)
            .AddMoney(money)
            .SendMailTo(trans, MailReceiver(receiver, GUID_LOPART(receiverGuid)), sender);

        CharacterDatabase.CommitTransaction(trans);

        std::string nameLink = handler->playerLink(receiverName);
        handler->PSendSysMessage(LANG_MAIL_SENT, nameLink.c_str());
        return true;
    }
    /// Send a message to a player in game
    static bool HandleSendMessageCommand(ChatHandler* handler, char const* args)
    {
        /// - Find the player
        Player* player;
        if (!handler->extractPlayerTarget((char*)args, &player))
            return false;

        char* msgStr = strtok(NULL, "");
        if (!msgStr)
            return false;

        ///- Check that he is not logging out.
        if (player->GetSession()->isLogingOut())
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// - Send the message
        // Use SendAreaTriggerMessage for fastest delivery.
        player->GetSession()->SendNotification("%s", msgStr);
        player->GetSession()->SendNotification("|cffff0000[Message from administrator]:|r");

        // Confirmation message
        std::string nameLink = handler->GetNameLink(player);
        handler->PSendSysMessage(LANG_SENDMESSAGE, nameLink.c_str(), msgStr);

        return true;
    }
};

void AddSC_send_commandscript()
{
    new send_commandscript();
}
