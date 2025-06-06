/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/* ScriptData
Name: account_commandscript
%Complete: 100
Comment: All account related commands
Category: commandscripts
EndScriptData */

#include "AccountMgr.h"
#include "Chat.h"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"

class account_commandscript : public CommandScript
{
public:
    account_commandscript() : CommandScript("account_commandscript") { }

    std::vector<ChatCommand> GetCommands() const OVERRIDE
    {
        static std::vector<ChatCommand> accountSetSecTable =
        {
            { "regmail",        rbac::RBAC_PERM_COMMAND_ACCOUNT_SET_SEC_REGMAIL, true,  &HandleAccountSetRegEmailCommand,  "",      },
            { "email",          rbac::RBAC_PERM_COMMAND_ACCOUNT_SET_SEC_EMAIL,   true,  &HandleAccountSetEmailCommand,     "",      },
        };
        static std::vector<ChatCommand> accountSetCommandTable =
        {
            { "addon",          rbac::RBAC_PERM_COMMAND_ACCOUNT_SET_ADDON,       true,  &HandleAccountSetAddonCommand,     "",      },
            { "sec",            rbac::RBAC_PERM_COMMAND_ACCOUNT_SET_SEC,         true,  NULL,                "", accountSetSecTable },
            { "password",       rbac::RBAC_PERM_COMMAND_ACCOUNT_SET_PASSWORD,    true,  &HandleAccountSetPasswordCommand,  "",      },
        };
        static std::vector<ChatCommand> accountLockCommandTable =
        {
            { "country",        rbac::RBAC_PERM_COMMAND_ACCOUNT_LOCK_COUNTRY,    true,  &HandleAccountLockCountryCommand,  "",      },
            { "ip",             rbac::RBAC_PERM_COMMAND_ACCOUNT_LOCK_IP,         true,  &HandleAccountLockIpCommand,       "",      },
        };
        static std::vector<ChatCommand> accountBoostCommandTable =
        {
            { "add",            rbac::RBAC_PERM_COMMAND_ACCOUNT_BOOST_ADD,       true,  &HandleAccountBoostAddCommand,     "",      },
            { "delete",         rbac::RBAC_PERM_COMMAND_ACCOUNT_BOOST_DEL,       true,  &HandleAccountBoostDelCommand,     "",      },
        };
        static std::vector<ChatCommand> accountCommandTable =
        {
            { "addon",          rbac::RBAC_PERM_COMMAND_ACCOUNT_ADDON,           false, &HandleAccountAddonCommand,        "",      },
            { "boost",          rbac::RBAC_PERM_COMMAND_ACCOUNT_BOOST,           false, NULL,          "", accountBoostCommandTable },
            { "create",         rbac::RBAC_PERM_COMMAND_ACCOUNT_CREATE,          true,  &HandleAccountCreateCommand,       "",      },
            { "delete",         rbac::RBAC_PERM_COMMAND_ACCOUNT_DELETE,          true,  &HandleAccountDeleteCommand,       "",      },
            { "email",          rbac::RBAC_PERM_COMMAND_ACCOUNT_EMAIL,           false, &HandleAccountEmailCommand,        "",      },
            { "onlinelist",     rbac::RBAC_PERM_COMMAND_ACCOUNT_ONLINE_LIST,     true,  &HandleAccountOnlineListCommand,   "",      },
            { "lock",           rbac::RBAC_PERM_COMMAND_ACCOUNT_LOCK,            false, NULL,           "", accountLockCommandTable },
            { "set",            rbac::RBAC_PERM_COMMAND_ACCOUNT_SET,             true,  NULL,            "", accountSetCommandTable },
            { "password",       rbac::RBAC_PERM_COMMAND_ACCOUNT_PASSWORD,        false, &HandleAccountPasswordCommand,     "",      },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "account",        rbac::RBAC_PERM_COMMAND_ACCOUNT,                 true,  NULL,              "",  accountCommandTable },
        };
        return commandTable;
    }

    static bool HandleAccountAddonCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* exp = strtok((char*)args, " ");

        uint32 accountId = handler->GetSession()->GetAccountId();

        int expansion = atoi(exp); //get int anyway (0 if error)
        if (expansion < 0 || uint8(expansion) > sWorld->getIntConfig(WorldIntConfigs::CONFIG_EXPANSION))
        {
            handler->SendSysMessage(LANG_IMPROPER_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

        stmt->setUInt8(0, uint8(expansion));
        stmt->setUInt32(1, accountId);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_ACCOUNT_ADDON, expansion);
        return true;
    }

    /// Create an account
    static bool HandleAccountCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string email;

        ///- %Parse the command line arguments
        char* accountName = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");
        char* possibleEmail = strtok(NULL, " ' ");
        if (possibleEmail)
            email = possibleEmail;

        if (!accountName || !password)
            return false;

        AccountOpResult result = sAccountMgr->CreateAccount(std::string(accountName), std::string(password), email);
        switch (result)
        {
            case AccountOpResult::AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_CREATED, accountName);
                if (handler->GetSession())
                {
                    SF_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Character:[%s] (GUID: %u) created Account %s (Email: '%s')",
                        handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                        handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow(),
                        accountName, email.c_str());
                }
                break;
            case AccountOpResult::AOR_NAME_TOO_LONG:
                handler->SendSysMessage(LANG_ACCOUNT_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            case AccountOpResult::AOR_NAME_ALREADY_EXIST:
                handler->SendSysMessage(LANG_ACCOUNT_ALREADY_EXIST);
                handler->SetSentErrorMessage(true);
                return false;
            case AccountOpResult::AOR_DB_INTERNAL_ERROR:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED_SQL_ERROR, accountName);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED, accountName);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    /// Delete a user account and all associated characters in this realm
    /// @todo This function has to be enhanced to respect the login/realm split (delete char, delete account chars in realm then delete account)
    static bool HandleAccountDeleteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the account name from the command line
        char* account = strtok((char*)args, " ");
        if (!account)
            return false;

        std::string accountName = account;
        if (!AccountMgr::normalizeString(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 accountId = AccountMgr::GetId(accountName);
        if (!accountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// Commands not recommended call from chat, but support anyway
        /// can delete only for account with less security
        /// This is also reject self apply in fact
        if (handler->HasLowerSecurityAccount(NULL, accountId, true))
            return false;

        AccountOpResult result = AccountMgr::DeleteAccount(accountId);
        switch (result)
        {
            case AccountOpResult::AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_DELETED, accountName.c_str());
                break;
            case AccountOpResult::AOR_NAME_NOT_EXIST:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            case AccountOpResult::AOR_DB_INTERNAL_ERROR:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_DELETED_SQL_ERROR, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_DELETED, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    /// Display info on users currently in the realm
    static bool HandleAccountOnlineListCommand(ChatHandler* handler, char const* /*args*/)
    {
        ///- Get the list of accounts ID logged to the realm
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ONLINE);

        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        if (!result)
        {
            handler->SendSysMessage(LANG_ACCOUNT_LIST_EMPTY);
            return true;
        }

        ///- Display the list of account/characters online
        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR_HEADER);
        handler->SendSysMessage(LANG_ACCOUNT_LIST_HEADER);
        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR);

        ///- Cycle through accounts
        do
        {
            Field* fieldsDB = result->Fetch();
            std::string name = fieldsDB[0].GetString();
            uint32 account = fieldsDB[1].GetUInt32();

            ///- Get the username, last IP and GM level of each account
            // No SQL injection. account is uint32.
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO);
            stmt->setUInt32(0, account);
            PreparedQueryResult resultLogin = LoginDatabase.Query(stmt);

            if (resultLogin)
            {
                Field* fieldsLogin = resultLogin->Fetch();
                handler->PSendSysMessage(LANG_ACCOUNT_LIST_LINE,
                    fieldsLogin[0].GetCString(), name.c_str(), fieldsLogin[1].GetCString(),
                    fieldsDB[2].GetUInt16(), fieldsDB[3].GetUInt16(), fieldsLogin[3].GetUInt8(),
                    fieldsLogin[2].GetUInt8());
            }
            else
                handler->PSendSysMessage(LANG_ACCOUNT_LIST_ERROR, name.c_str());
        } while (result->NextRow());

        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR);
        return true;
    }

    static bool HandleAccountLockCountryCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_USE_BOL);
            handler->SetSentErrorMessage(true);
            return false;
        }
        std::string param = (char*)args;

        if (!param.empty())
        {
            if (param == "on")
            {
                PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_LOGON_COUNTRY);
                uint32 ip = inet_addr(handler->GetSession()->GetRemoteAddress().c_str());
                EndianConvertReverse(ip);
                stmt->setUInt32(0, ip);
                PreparedQueryResult result = LoginDatabase.Query(stmt);
                if (result)
                {
                    Field* fields = result->Fetch();
                    std::string country = fields[0].GetString();
                    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK_CONTRY);
                    stmt->setString(0, country);
                    stmt->setUInt32(1, handler->GetSession()->GetAccountId());
                    LoginDatabase.Execute(stmt);
                    handler->PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
                }
                else
                {
                    handler->PSendSysMessage("[IP2NATION] Table empty");
                    SF_LOG_DEBUG("server.authserver", "[IP2NATION] Table empty");
                }
            }
            else if (param == "off")
            {
                PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK_CONTRY);
                stmt->setString(0, "00");
                stmt->setUInt32(1, handler->GetSession()->GetAccountId());
                LoginDatabase.Execute(stmt);
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
            }
            return true;
        }
        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleAccountLockIpCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_USE_BOL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string param = (char*)args;

        if (!param.empty())
        {
            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK);

            if (param == "on")
            {
                stmt->setBool(0, true);                                     // locked
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
            }
            else if (param == "off")
            {
                stmt->setBool(0, false);                                    // unlocked
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
            }

            stmt->setUInt32(1, handler->GetSession()->GetAccountId());

            LoginDatabase.Execute(stmt);
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleAccountEmailCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* oldEmail = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");
        char* email = strtok(NULL, " ");
        char* emailConfirmation = strtok(NULL, " ");

        if (!oldEmail || !password || !email || !emailConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!AccountMgr::CheckEmail(handler->GetSession()->GetAccountId(), std::string(oldEmail)))
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGEMAIL);
            handler->SetSentErrorMessage(true);
            SF_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change email, but the provided email [%s] is not equal to registration email [%s].",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow(),
                email, oldEmail);
            return false;
        }

        if (!AccountMgr::CheckPassword(handler->GetSession()->GetAccountId(), std::string(password)))
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGOLDPASSWORD);
            handler->SetSentErrorMessage(true);
            SF_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change email, but the provided password is wrong.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow());
            return false;
        }

        if (strcmp(email, oldEmail) == 0)
        {
            handler->SendSysMessage(LANG_OLD_EMAIL_IS_NEW_EMAIL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (strcmp(email, emailConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_EMAILS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            SF_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change email, but the provided password is wrong.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow());
            return false;
        }


        AccountOpResult result = AccountMgr::ChangeEmail(handler->GetSession()->GetAccountId(), std::string(email));
        switch (result)
        {
            case AccountOpResult::AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_EMAIL);
                SF_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Changed Email from [%s] to [%s].",
                    handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                    handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow(),
                    oldEmail, email);
                break;
            case AccountOpResult::AOR_EMAIL_TOO_LONG:
                handler->SendSysMessage(LANG_EMAIL_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEEMAIL);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    static bool HandleAccountPasswordCommand(ChatHandler* handler, char const* args)
    {
        // If no args are given at all, we can return false right away.
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // First, we check config. What security type (sec type) is it ? Depending on it, the command branches out
        uint32 pwConfig = sWorld->getIntConfig(WorldIntConfigs::CONFIG_ACC_PASSCHANGESEC); // 0 - PW_NONE, 1 - PW_EMAIL, 2 - PW_RBAC

        // Command is supposed to be: .account password [$oldpassword] [$newpassword] [$newpasswordconfirmation] [$emailconfirmation]
        char* oldPassword = strtok((char*)args, " ");       // This extracts [$oldpassword]
        char* newPassword = strtok(NULL, " ");              // This extracts [$newpassword]
        char* passwordConfirmation = strtok(NULL, " ");     // This extracts [$newpasswordconfirmation]
        char const* emailConfirmation = strtok(NULL, " ");  // This defines the emailConfirmation variable, which is optional depending on sec type.
        if (!emailConfirmation)                             // This extracts [$emailconfirmation]. If it doesn't exist, however...
            emailConfirmation = "";                         // ... it's simply "" for emailConfirmation.

        //Is any of those variables missing for any reason ? We return false.
        if (!oldPassword || !newPassword || !passwordConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // We compare the old, saved password to the entered old password - no chance for the unauthorized.
        if (!AccountMgr::CheckPassword(handler->GetSession()->GetAccountId(), std::string(oldPassword)))
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGOLDPASSWORD);
            handler->SetSentErrorMessage(true);
            SF_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change password, but the provided old password is wrong.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow());
            return false;
        }

        // This compares the old, current email to the entered email - however, only...
        if ((pwConfig == PW_EMAIL || (pwConfig == PW_RBAC && handler->HasPermission(rbac::RBAC_PERM_EMAIL_CONFIRM_FOR_PASS_CHANGE))) // ...if either PW_EMAIL or PW_RBAC with the Permission is active...
            && !AccountMgr::CheckEmail(handler->GetSession()->GetAccountId(), std::string(emailConfirmation))) // ... and returns false if the comparison fails.
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGEMAIL);
            handler->SetSentErrorMessage(true);
            SF_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change password, but the entered email [%s] is wrong.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow(),
                emailConfirmation);
            return false;
        }

        // Making sure that newly entered password is correctly entered.
        if (strcmp(newPassword, passwordConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Changes password and prints result.
        AccountOpResult result = AccountMgr::ChangePassword(handler->GetSession()->GetAccountId(), std::string(newPassword));
        switch (result)
        {
            case AccountOpResult::AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_PASSWORD);
                SF_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Changed Password.",
                    handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                    handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow());
                break;
            case AccountOpResult::AOR_PASS_TOO_LONG:
                handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    static bool HandleAccountCommand(ChatHandler* handler, char const* /*args*/)
    {
        // GM Level
        AccountTypes gmLevel = handler->GetSession()->GetSecurity();
        handler->PSendSysMessage(LANG_ACCOUNT_LEVEL, uint32(gmLevel));

        // Security level required
        bool hasRBAC = (handler->HasPermission(rbac::RBAC_PERM_EMAIL_CONFIRM_FOR_PASS_CHANGE) ? true : false);
        uint32 pwConfig = sWorld->getIntConfig(WorldIntConfigs::CONFIG_ACC_PASSCHANGESEC); // 0 - PW_NONE, 1 - PW_EMAIL, 2 - PW_RBAC

        handler->PSendSysMessage(LANG_ACCOUNT_SEC_TYPE, (pwConfig == PW_NONE ? "Lowest level: No Email input required." :
            pwConfig == PW_EMAIL ? "Highest level: Email input required." :
            pwConfig == PW_RBAC ? "Special level: Your account may require email input depending on settings. That is the case if another lien is printed." :
            "Unknown security level: Notify technician for details."));

        // RBAC required display - is not displayed for console
        if (pwConfig == PW_RBAC && handler->GetSession() && hasRBAC)
            handler->PSendSysMessage(LANG_RBAC_EMAIL_REQUIRED);

        // Email display if sufficient rights
        if (handler->HasPermission(rbac::RBAC_PERM_MAY_CHECK_OWN_EMAIL))
        {
            std::string emailoutput;
            uint32 accountId = handler->GetSession()->GetAccountId();

            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_EMAIL_BY_ID);
            stmt->setUInt32(0, accountId);
            PreparedQueryResult result = LoginDatabase.Query(stmt);

            if (result)
            {
                emailoutput = (*result)[0].GetString();
                handler->PSendSysMessage(LANG_COMMAND_EMAIL_OUTPUT, emailoutput.c_str());
            }
        }

        return true;
    }

    /// Set/Unset the expansion level for an account
    static bool HandleAccountSetAddonCommand(ChatHandler* handler, char const* args)
    {
        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* exp = strtok(NULL, " ");

        if (!account)
            return false;

        std::string accountName;
        uint32 accountId;

        if (!exp)
        {
            Player* player = handler->getSelectedPlayer();
            if (!player)
                return false;

            accountId = player->GetSession()->GetAccountId();
            AccountMgr::GetName(accountId, accountName);
            exp = account;
        }
        else
        {
            ///- Convert Account name to Upper Format
            accountName = account;
            if (!AccountMgr::normalizeString(accountName))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }

            accountId = AccountMgr::GetId(accountName);
            if (!accountId)
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Let set addon state only for lesser (strong) security level
        // or to self account
        if (handler->GetSession() && handler->GetSession()->GetAccountId() != accountId &&
            handler->HasLowerSecurityAccount(NULL, accountId, true))
            return false;

        int expansion = atoi(exp); //get int anyway (0 if error)
        if (expansion < 0 || uint8(expansion) > sWorld->getIntConfig(WorldIntConfigs::CONFIG_EXPANSION))
            return false;

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

        stmt->setUInt8(0, expansion);
        stmt->setUInt32(1, accountId);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_ACCOUNT_SETADDON, accountName.c_str(), accountId, expansion);
        return true;
    }

    /// Set password for account
    static bool HandleAccountSetPasswordCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");
        char* passwordConfirmation = strtok(NULL, " ");

        if (!account || !password || !passwordConfirmation)
            return false;

        std::string accountName = account;
        if (!AccountMgr::normalizeString(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// can set password only for target with less security
        /// This also restricts setting handler's own password
        if (handler->HasLowerSecurityAccount(NULL, targetAccountId, true))
            return false;

        if (strcmp(password, passwordConfirmation))
        {
            handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangePassword(targetAccountId, password);

        switch (result)
        {
            case AccountOpResult::AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_PASSWORD);
                break;
            case AccountOpResult::AOR_NAME_NOT_EXIST:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            case AccountOpResult::AOR_PASS_TOO_LONG:
                handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
                handler->SetSentErrorMessage(true);
                return false;
        }
        return true;
    }

    /// Set normal email for account
    static bool HandleAccountSetEmailCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* email = strtok(NULL, " ");
        char* emailConfirmation = strtok(NULL, " ");

        if (!account || !email || !emailConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string accountName = account;
        if (!AccountMgr::normalizeString(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// can set email only for target with less security
        /// This also restricts setting handler's own email.
        if (handler->HasLowerSecurityAccount(NULL, targetAccountId, true))
            return false;

        if (strcmp(email, emailConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_EMAILS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangeEmail(targetAccountId, email);
        switch (result)
        {
            case AccountOpResult::AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_EMAIL);
                SF_LOG_INFO("entities.player.character", "ChangeEmail: Account %s [Id: %u] had it's email changed to %s.",
                    accountName.c_str(), targetAccountId, email);
                break;
            case AccountOpResult::AOR_NAME_NOT_EXIST:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            case AccountOpResult::AOR_EMAIL_TOO_LONG:
                handler->SendSysMessage(LANG_EMAIL_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEEMAIL);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    /// Change registration email for account
    static bool HandleAccountSetRegEmailCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        //- We do not want anything short of console to use this by default.
        //- So we force that.
        if (handler->GetSession())
            return false;

        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* email = strtok(NULL, " ");
        char* emailConfirmation = strtok(NULL, " ");

        if (!account || !email || !emailConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string accountName = account;
        if (!AccountMgr::normalizeString(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// can set email only for target with less security
        /// This also restricts setting handler's own email.
        if (handler->HasLowerSecurityAccount(NULL, targetAccountId, true))
            return false;

        if (strcmp(email, emailConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_EMAILS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangeRegEmail(targetAccountId, email);
        switch (result)
        {
            case AccountOpResult::AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_EMAIL);
                SF_LOG_INFO("entities.player.character", "ChangeRegEmail: Account %s [Id: %u] had it's Registration Email changed to %s.",
                    accountName.c_str(), targetAccountId, email);
                break;
            case AccountOpResult::AOR_NAME_NOT_EXIST:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            case AccountOpResult::AOR_EMAIL_TOO_LONG:
                handler->SendSysMessage(LANG_EMAIL_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEEMAIL);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    static bool HandleAccountBoostAddCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the account name from the command line
        char* account = strtok((char*)args, " ");
        if (!account)
            return false;

        std::string accountName = account;
        if (!AccountMgr::normalizeString(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 accountId = AccountMgr::GetId(accountName);
        if (!accountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Check if accounts exists
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BY_ID);
        stmt->setUInt32(0, accountId);
        PreparedQueryResult result = LoginDatabase.Query(stmt);

        if (!result)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        SQLTransaction trans = LoginDatabase.BeginTransaction();

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_BOOST);
        stmt->setBool(0, true);
        stmt->setUInt32(1, accountId);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_COMMAND_BOOST_ADD, accountName.c_str());

        return true;
    }

    static bool HandleAccountBoostDelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the account name from the command line
        char* account = strtok((char*)args, " ");
        if (!account)
            return false;

        std::string accountName = account;
        if (!AccountMgr::normalizeString(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 accountId = AccountMgr::GetId(accountName);
        if (!accountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Check if accounts exists
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BY_ID);
        stmt->setUInt32(0, accountId);
        PreparedQueryResult result = LoginDatabase.Query(stmt);

        if (!result)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        SQLTransaction trans = LoginDatabase.BeginTransaction();

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_BOOST);
        stmt->setBool(0, false);
        stmt->setUInt32(1, accountId);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_COMMAND_BOOST_DEL, accountName.c_str());

        return true;
    }
};

void AddSC_account_commandscript()
{
    new account_commandscript();
}
