/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/* ScriptData
Name: server_commandscript
%Complete: 100
Comment: All server related commands
Category: commandscripts
EndScriptData */

#include "Chat.h"
#include "Config.h"
#include "Language.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SystemConfig.h"

class server_commandscript : public CommandScript
{
public:
    server_commandscript() : CommandScript("server_commandscript") { }

    std::vector<ChatCommand> GetCommands() const OVERRIDE
    {
        static std::vector<ChatCommand> serverIdleRestartCommandTable =
        {
            { "cancel", rbac::RBAC_PERM_COMMAND_SERVER_IDLERESTART_CANCEL, true, &HandleServerShutDownCancelCommand, "", },
            { ""   ,    rbac::RBAC_PERM_COMMAND_SERVER_IDLERESTART,        true, &HandleServerIdleRestartCommand,    "", },
        };

        static std::vector<ChatCommand> serverIdleShutdownCommandTable =
        {
            { "cancel", rbac::RBAC_PERM_COMMAND_SERVER_IDLESHUTDOWN_CANCEL, true, &HandleServerShutDownCancelCommand, "", },
            { ""   ,    rbac::RBAC_PERM_COMMAND_SERVER_IDLESHUTDOWN,        true, &HandleServerIdleShutDownCommand,   "", },
        };

        static std::vector<ChatCommand> serverRestartCommandTable =
        {
            { "cancel", rbac::RBAC_PERM_COMMAND_SERVER_RESTART_CANCEL, true, &HandleServerShutDownCancelCommand, "", },
            { ""   ,    rbac::RBAC_PERM_COMMAND_SERVER_RESTART,        true, &HandleServerRestartCommand,        "", },
        };

        static std::vector<ChatCommand> serverShutdownCommandTable =
        {
            { "cancel", rbac::RBAC_PERM_COMMAND_SERVER_SHUTDOWN_CANCEL, true, &HandleServerShutDownCancelCommand, "", },
            { ""   ,    rbac::RBAC_PERM_COMMAND_SERVER_SHUTDOWN,        true, &HandleServerShutDownCommand,       "", },
        };

        static std::vector<ChatCommand> serverSetCommandTable =
        {
            { "difftime", rbac::RBAC_PERM_COMMAND_SERVER_SET_DIFFTIME, true, &HandleServerSetDiffTimeCommand, "", },
            { "loglevel", rbac::RBAC_PERM_COMMAND_SERVER_SET_LOGLEVEL, true, &HandleServerSetLogLevelCommand, "", },
            { "motd",     rbac::RBAC_PERM_COMMAND_SERVER_SET_MOTD,     true, &HandleServerSetMotdCommand,     "", },
            { "closed",   rbac::RBAC_PERM_COMMAND_SERVER_SET_CLOSED,   true, &HandleServerSetClosedCommand,   "", },
        };

        static std::vector<ChatCommand> serverCommandTable =
        {
            { "corpses",      rbac::RBAC_PERM_COMMAND_SERVER_CORPSES,      true, &HandleServerCorpsesCommand, "", },
            { "exit",         rbac::RBAC_PERM_COMMAND_SERVER_EXIT,         true, &HandleServerExitCommand,    "", },
            { "idlerestart",  rbac::RBAC_PERM_COMMAND_SERVER_IDLERESTART,  true, NULL,                        "", serverIdleRestartCommandTable },
            { "idleshutdown", rbac::RBAC_PERM_COMMAND_SERVER_IDLESHUTDOWN, true, NULL,                        "", serverIdleShutdownCommandTable },
            { "info",         rbac::RBAC_PERM_COMMAND_SERVER_INFO,         true, &HandleServerInfoCommand,    "", },
            { "motd",         rbac::RBAC_PERM_COMMAND_SERVER_MOTD,         true, &HandleServerMotdCommand,    "", },
            { "plimit",       rbac::RBAC_PERM_COMMAND_SERVER_PLIMIT,       true, &HandleServerPLimitCommand,  "", },
            { "restart",      rbac::RBAC_PERM_COMMAND_SERVER_RESTART,      true, NULL,                        "", serverRestartCommandTable },
            { "shutdown",     rbac::RBAC_PERM_COMMAND_SERVER_SHUTDOWN,     true, NULL,                        "", serverShutdownCommandTable },
            { "set",          rbac::RBAC_PERM_COMMAND_SERVER_SET,          true, NULL,                        "", serverSetCommandTable },
            { "uptime",       rbac::RBAC_PERM_COMMAND_SERVER_UPTIME,       true, &HandleServerUptimeCommand,  "", },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "server", rbac::RBAC_PERM_COMMAND_SERVER, true, NULL, "", serverCommandTable },
        };
        return commandTable;
    }

    // Triggering corpses expire check in world
    static bool HandleServerCorpsesCommand(ChatHandler* /*handler*/, char const* /*args*/)
    {
        sObjectAccessor->RemoveOldCorpses();
        return true;
    }

    static bool HandleServerInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        uint32 playersNum = sWorld->GetPlayerCount();
        uint32 maxPlayersNum = sWorld->GetMaxPlayerCount();
        uint32 activeClientsNum = sWorld->GetActiveSessionCount();
        uint32 queuedClientsNum = sWorld->GetQueuedSessionCount();
        uint32 maxActiveClientsNum = sWorld->GetMaxActiveSessionCount();
        uint32 maxQueuedClientsNum = sWorld->GetMaxQueuedSessionCount();
        std::string uptime = secsToTimeString(sWorld->GetUptime());
        uint32 updateTime = sWorld->GetUpdateTime();

        // TODO: FIX ME revision
        //handler->SendSysMessage(_FULLVERSION);
        handler->PSendSysMessage(LANG_CONNECTED_PLAYERS, playersNum, maxPlayersNum);
        handler->PSendSysMessage(LANG_CONNECTED_USERS, activeClientsNum, maxActiveClientsNum, queuedClientsNum, maxQueuedClientsNum);
        handler->PSendSysMessage(LANG_UPTIME, uptime.c_str());
        handler->PSendSysMessage(LANG_UPDATE_DIFF, updateTime);

        // Can't use sWorld->ShutdownMsg here in case of console command
        if (sWorld->IsShuttingDown())
            handler->PSendSysMessage(LANG_SHUTDOWN_TIMELEFT, secsToTimeString(sWorld->GetShutDownTimeLeft()).c_str());

        return true;
    }

    static bool HandleServerUptimeCommand(ChatHandler* handler, char const* /*args*/)
    {
        std::string uptime = secsToTimeString(sWorld->GetUptime());
        handler->PSendSysMessage(LANG_UPTIME, uptime.c_str());
        return true;
    }

    // Display the 'Message of the day' for the realm
    static bool HandleServerMotdCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage(LANG_MOTD_CURRENT, sWorld->GetMotd());
        return true;
    }

    static bool HandleServerPLimitCommand(ChatHandler* handler, char const* args)
    {
        if (*args)
        {
            char* paramStr = strtok((char*)args, " ");
            if (!paramStr)
                return false;

            int32 limit = strlen(paramStr);

            if (strncmp(paramStr, "player", limit) == 0)
                sWorld->SetPlayerSecurityLimit(AccountTypes::SEC_PLAYER);
            else if (strncmp(paramStr, "moderator", limit) == 0)
                sWorld->SetPlayerSecurityLimit(AccountTypes::SEC_MODERATOR);
            else if (strncmp(paramStr, "gamemaster", limit) == 0)
                sWorld->SetPlayerSecurityLimit(AccountTypes::SEC_GAMEMASTER);
            else if (strncmp(paramStr, "administrator", limit) == 0)
                sWorld->SetPlayerSecurityLimit(AccountTypes::SEC_ADMINISTRATOR);
            else if (strncmp(paramStr, "reset", limit) == 0)
            {
                sWorld->SetPlayerAmountLimit(sConfigMgr->GetIntDefault("PlayerLimit", 100));
                sWorld->LoadDBAllowedSecurityLevel();
            }
            else
            {
                int32 value = atoi(paramStr);
                if (value < 0)
                    sWorld->SetPlayerSecurityLimit(AccountTypes(-value));
                else
                    sWorld->SetPlayerAmountLimit(uint32(value));
            }
        }

        uint32 playerAmountLimit = sWorld->GetPlayerAmountLimit();
        AccountTypes allowedAccountType = sWorld->GetPlayerSecurityLimit();
        char const* secName = "";
        switch (allowedAccountType)
        {
            case AccountTypes::SEC_PLAYER:
                secName = "Player";
                break;
            case AccountTypes::SEC_MODERATOR:
                secName = "Moderator";
                break;
            case AccountTypes::SEC_GAMEMASTER:
                secName = "Gamemaster";
                break;
            case AccountTypes::SEC_ADMINISTRATOR:
                secName = "Administrator";
                break;
            default:
                secName = "<unknown>";
                break;
        }
        handler->PSendSysMessage("Player limits: amount %u, min. security level %s.", playerAmountLimit, secName);

        return true;
    }

    static bool HandleServerShutDownCancelCommand(ChatHandler* /*handler*/, char const* /*args*/)
    {
        sWorld->ShutdownCancel();

        return true;
    }

    static bool HandleServerShutDownCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* timeStr = strtok((char*)args, " ");
        char* exitCodeStr = strtok(NULL, "");

        int32 time = atoi(timeStr);

        // Prevent interpret wrong arg value as 0 secs shutdown time
        if ((time == 0 && (timeStr[0] != '0' || timeStr[1] != '\0')) || time < 0)
            return false;

        if (exitCodeStr)
        {
            int32 exitCode = atoi(exitCodeStr);

            // Handle atoi() errors
            if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
                return false;

            // Exit code should be in range of 0-125, 126-255 is used
            // in many shells for their own return codes and code > 255
            // is not supported in many others
            if (exitCode < 0 || exitCode > 125)
                return false;

            sWorld->ShutdownServ(time, 0, exitCode);
        }
        else
            sWorld->ShutdownServ(time, 0, SHUTDOWN_EXIT_CODE);

        return true;
    }

    static bool HandleServerRestartCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* timeStr = strtok((char*)args, " ");
        char* exitCodeStr = strtok(NULL, "");

        int32 time = atoi(timeStr);

        //  Prevent interpret wrong arg value as 0 secs shutdown time
        if ((time == 0 && (timeStr[0] != '0' || timeStr[1] != '\0')) || time < 0)
            return false;

        if (exitCodeStr)
        {
            int32 exitCode = atoi(exitCodeStr);

            // Handle atoi() errors
            if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
                return false;

            // Exit code should be in range of 0-125, 126-255 is used
            // in many shells for their own return codes and code > 255
            // is not supported in many others
            if (exitCode < 0 || exitCode > 125)
                return false;

            sWorld->ShutdownServ(time, SHUTDOWN_MASK_RESTART, exitCode);
        }
        else
            sWorld->ShutdownServ(time, SHUTDOWN_MASK_RESTART, RESTART_EXIT_CODE);

        return true;
    }

    static bool HandleServerIdleRestartCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* timeStr = strtok((char*)args, " ");
        char* exitCodeStr = strtok(NULL, "");

        int32 time = atoi(timeStr);

        // Prevent interpret wrong arg value as 0 secs shutdown time
        if ((time == 0 && (timeStr[0] != '0' || timeStr[1] != '\0')) || time < 0)
            return false;

        if (exitCodeStr)
        {
            int32 exitCode = atoi(exitCodeStr);

            // Handle atoi() errors
            if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
                return false;

            // Exit code should be in range of 0-125, 126-255 is used
            // in many shells for their own return codes and code > 255
            // is not supported in many others
            if (exitCode < 0 || exitCode > 125)
                return false;

            sWorld->ShutdownServ(time, SHUTDOWN_MASK_RESTART | SHUTDOWN_MASK_IDLE, exitCode);
        }
        else
            sWorld->ShutdownServ(time, SHUTDOWN_MASK_RESTART | SHUTDOWN_MASK_IDLE, RESTART_EXIT_CODE);
        return true;
    }

    static bool HandleServerIdleShutDownCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* timeStr = strtok((char*)args, " ");
        char* exitCodeStr = strtok(NULL, "");

        int32 time = atoi(timeStr);

        // Prevent interpret wrong arg value as 0 secs shutdown time
        if ((time == 0 && (timeStr[0] != '0' || timeStr[1] != '\0')) || time < 0)
            return false;

        if (exitCodeStr)
        {
            int32 exitCode = atoi(exitCodeStr);

            // Handle atoi() errors
            if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
                return false;

            // Exit code should be in range of 0-125, 126-255 is used
            // in many shells for their own return codes and code > 255
            // is not supported in many others
            if (exitCode < 0 || exitCode > 125)
                return false;

            sWorld->ShutdownServ(time, SHUTDOWN_MASK_IDLE, exitCode);
        }
        else
            sWorld->ShutdownServ(time, SHUTDOWN_MASK_IDLE, SHUTDOWN_EXIT_CODE);

        return true;
    }

    // Exit the realm
    static bool HandleServerExitCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage(LANG_COMMAND_EXIT);
        World::StopNow(SHUTDOWN_EXIT_CODE);
        return true;
    }

    // Define the 'Message of the day' for the realm
    static bool HandleServerSetMotdCommand(ChatHandler* handler, char const* args)
    {
        sWorld->SetMotd(args);
        handler->PSendSysMessage(LANG_MOTD_NEW, args);
        return true;
    }

    // Set whether we accept new clients
    static bool HandleServerSetClosedCommand(ChatHandler* handler, char const* args)
    {
        if (strncmp(args, "on", 3) == 0)
        {
            handler->SendSysMessage(LANG_WORLD_CLOSED);
            sWorld->SetClosed(true);
            return true;
        }
        else if (strncmp(args, "off", 4) == 0)
        {
            handler->SendSysMessage(LANG_WORLD_OPENED);
            sWorld->SetClosed(false);
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    // Set the level of logging
    static bool HandleServerSetLogLevelCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* type = strtok((char*)args, " ");
        char* name = strtok(NULL, " ");
        char* level = strtok(NULL, " ");

        if (!type || !name || !level || *name == '\0' || *level == '\0' || (*type != 'a' && *type != 'l'))
            return false;

        sLog->SetLogLevel(name, level, *type == 'l');
        return true;
    }

    // set diff time record interval
    static bool HandleServerSetDiffTimeCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* newTimeStr = strtok((char*)args, " ");
        if (!newTimeStr)
            return false;

        int32 newTime = atoi(newTimeStr);
        if (newTime < 0)
            return false;

        sWorld->SetRecordDiffInterval(newTime);
        printf("Record diff every %u ms\n", newTime);

        return true;
    }
};

void AddSC_server_commandscript()
{
    new server_commandscript();
}
