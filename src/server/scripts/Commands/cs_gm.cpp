/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/* ScriptData
Name: gm_commandscript
%Complete: 100
Comment: All gm related commands
Category: commandscripts
EndScriptData */

#include "AccountMgr.h"
#include "Chat.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "World.h"

class gm_commandscript : public CommandScript
{
public:
    gm_commandscript() : CommandScript("gm_commandscript") { }

    std::vector<ChatCommand> GetCommands() const OVERRIDE
    {
        static std::vector<ChatCommand> gmCommandTable =
        {
            { "chat",    rbac::RBAC_PERM_COMMAND_GM_CHAT,    false, &HandleGMChatCommand,       "", },
            { "fly",     rbac::RBAC_PERM_COMMAND_GM_FLY,     false, &HandleGMFlyCommand,        "", },
            { "ingame",  rbac::RBAC_PERM_COMMAND_GM_INGAME,   true, &HandleGMListIngameCommand, "", },
            { "visible", rbac::RBAC_PERM_COMMAND_GM_VISIBLE, false, &HandleGMVisibleCommand,    "", },
            { "",        rbac::RBAC_PERM_COMMAND_GM,         false, &HandleGMCommand,           "", },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "gm", rbac::RBAC_PERM_COMMAND_GM, false, NULL, "", gmCommandTable },
        };
        return commandTable;
    }

    // Enables or disables hiding of the staff badge
    static bool HandleGMChatCommand(ChatHandler* handler, char const* args)
    {
        if (WorldSession* session = handler->GetSession())
        {
            if (!*args)
            {
                if (session->HasPermission(rbac::RBAC_PERM_CHAT_USE_STAFF_BADGE) && session->GetPlayer()->isGMChat())
                    session->SendNotification(LANG_GM_CHAT_ON);
                else
                    session->SendNotification(LANG_GM_CHAT_OFF);
                return true;
            }

            std::string param = (char*)args;

            if (param == "on")
            {
                session->GetPlayer()->SetGMChat(true);
                session->SendNotification(LANG_GM_CHAT_ON);
                return true;
            }

            if (param == "off")
            {
                session->GetPlayer()->SetGMChat(false);
                session->SendNotification(LANG_GM_CHAT_OFF);
                return true;
            }
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleGMFlyCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* target = handler->getSelectedPlayer();
        if (!target)
            target = handler->GetSession()->GetPlayer();

        WorldPacket data;
        if (strncmp(args, "on", 3) == 0)
            target->SetCanFly(true);
        else if (strncmp(args, "off", 4) == 0)
            target->SetCanFly(false);
        else
        {
            handler->SendSysMessage(LANG_USE_BOL);
            return false;
        }
        handler->PSendSysMessage(LANG_COMMAND_FLYMODE_STATUS, handler->GetNameLink(target).c_str(), args);
        return true;
    }

    static bool HandleGMListIngameCommand(ChatHandler* handler, char const* /*args*/)
    {
        bool first = true;
        bool footer = false;


        SF_SHARED_GUARD readGuard(*HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType const& m = sObjectAccessor->GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            AccountTypes itrSec = itr->second->GetSession()->GetSecurity();
            if ((itr->second->IsGameMaster() ||
                (itr->second->GetSession()->HasPermission(rbac::RBAC_PERM_COMMANDS_APPEAR_IN_GM_LIST) &&
                    itrSec <= AccountTypes(sWorld->getIntConfig(WorldIntConfigs::CONFIG_GM_LEVEL_IN_GM_LIST)))) &&
                (!handler->GetSession() || itr->second->IsVisibleGloballyFor(handler->GetSession()->GetPlayer())))
            {
                if (first)
                {
                    first = false;
                    footer = true;
                    handler->SendSysMessage(LANG_GMS_ON_SRV);
                    handler->SendSysMessage("========================");
                }
                std::string const& name = itr->second->GetName();
                uint8 size = name.size();
                AccountTypes security = itrSec;
                uint8 max = ((16 - size) / 2);
                uint8 max2 = max;
                if ((max + max2 + size) == 16)
                    max2 = max - 1;
                if (handler->GetSession())
                    handler->PSendSysMessage("|    %s GMLevel %u", name.c_str(), uint8(security));
                else
                    handler->PSendSysMessage("|%*s%s%*s|   %u  |", max, " ", name.c_str(), max2, " ", uint8(security));
            }
        }
        if (footer)
            handler->SendSysMessage("========================");
        if (first)
            handler->SendSysMessage(LANG_GMS_NOT_LOGGED);
        return true;
    }

    //Enable\Disable Invisible mode
    static bool HandleGMVisibleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(LANG_YOU_ARE, handler->GetSession()->GetPlayer()->isGMVisible() ? handler->GetSkyFireString(LANG_VISIBLE) : handler->GetSkyFireString(LANG_INVISIBLE));
            return true;
        }

        const uint32 VISUAL_AURA = 37800;
        std::string param = (char*)args;
        Player* player = handler->GetSession()->GetPlayer();

        if (param == "on")
        {
            if (player->HasAura(VISUAL_AURA, 0))
                player->RemoveAurasDueToSpell(VISUAL_AURA);

            player->SetGMVisible(true);
            handler->GetSession()->SendNotification(LANG_INVISIBLE_VISIBLE);
            return true;
        }

        if (param == "off")
        {
            handler->GetSession()->SendNotification(LANG_INVISIBLE_INVISIBLE);
            player->SetGMVisible(false);

            player->AddAura(VISUAL_AURA, player);

            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    //Enable\Disable GM Mode
    static bool HandleGMCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            if (handler->GetSession()->GetPlayer()->IsGameMaster())
                handler->GetSession()->SendNotification(LANG_GM_ON);
            else
                handler->GetSession()->SendNotification(LANG_GM_OFF);
            return true;
        }

        std::string param = (char*)args;

        if (param == "on")
        {
            handler->GetSession()->GetPlayer()->SetGameMaster(true);
            handler->GetSession()->SendNotification(LANG_GM_ON);
            handler->GetSession()->GetPlayer()->UpdateTriggerVisibility();
#ifdef _DEBUG_VMAPS
            VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
            vMapManager->processCommand("stoplog");
#endif
            return true;
        }

        if (param == "off")
        {
            handler->GetSession()->GetPlayer()->SetGameMaster(false);
            handler->GetSession()->SendNotification(LANG_GM_OFF);
            handler->GetSession()->GetPlayer()->UpdateTriggerVisibility();
#ifdef _DEBUG_VMAPS
            VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
            vMapManager->processCommand("startlog");
#endif
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }
};

void AddSC_gm_commandscript()
{
    new gm_commandscript();
}
