/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/* ScriptData
Name: reset_commandscript
%Complete: 100
Comment: All reset related commands
Category: commandscripts
EndScriptData */

#include "AchievementMgr.h"
#include "Chat.h"
#include "Language.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"

class reset_commandscript : public CommandScript
{
public:
    reset_commandscript() : CommandScript("reset_commandscript") { }

    std::vector<ChatCommand> GetCommands() const OVERRIDE
    {
        static std::vector<ChatCommand> resetCommandTable =
        {
            { "achievements", rbac::RBAC_PERM_COMMAND_RESET_ACHIEVEMENTS, true, &HandleResetAchievementsCommand, "", },
            { "honor",        rbac::RBAC_PERM_COMMAND_RESET_HONOR,        true, &HandleResetHonorCommand,        "", },
            { "level",        rbac::RBAC_PERM_COMMAND_RESET_LEVEL,        true, &HandleResetLevelCommand,        "", },
            { "spells",       rbac::RBAC_PERM_COMMAND_RESET_SPELLS,       true, &HandleResetSpellsCommand,       "", },
            { "stats",        rbac::RBAC_PERM_COMMAND_RESET_STATS,        true, &HandleResetStatsCommand,        "", },
            { "talents",      rbac::RBAC_PERM_COMMAND_RESET_TALENTS,      true, &HandleResetTalentsCommand,      "", },
            { "all",          rbac::RBAC_PERM_COMMAND_RESET_ALL,          true, &HandleResetAllCommand,          "", },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "reset", rbac::RBAC_PERM_COMMAND_RESET, true, NULL, "", resetCommandTable },
        };
        return commandTable;
    }

    static bool HandleResetAchievementsCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        uint64 targetGuid;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid))
            return false;

        if (target)
            target->ResetAchievements();
        else
            AchievementMgr<Player>::DeleteFromDB(GUID_LOPART(targetGuid));

        return true;
    }

    static bool HandleResetHonorCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        target->SetUInt32Value(PLAYER_FIELD_YESTERDAY_HONORABLE_KILLS, 0);
        target->SetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, 0);
        target->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL);

        return true;
    }

    static bool HandleResetStatsOrLevelHelper(Player* player)
    {
        ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(player->getClass());
        if (!classEntry)
        {
            SF_LOG_ERROR("misc", "Class %u not found in DBC (Wrong DBC files?)", player->getClass());
            return false;
        }

        uint8 powerType = classEntry->powerType;

        // reset m_form if no aura
        if (!player->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
            player->SetShapeshiftForm(FORM_NONE);

        player->setFactionForRace(player->getRace());

        player->SetRace(player->getRace());
        player->SetClass(player->getClass());
        player->SetGender(player->getGender());

        player->SetFieldPowerType(powerType);

        // reset only if player not in some form;
        if (player->GetShapeshiftForm() == FORM_NONE)
            player->InitDisplayIds();

        player->SetByteValue(UNIT_FIELD_SHAPESHIFT_FORM, 1, UNIT_BYTE2_FLAG_PVP);

        player->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);

        //-1 is default value
        player->SetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, uint32(-1));
        return true;
    }

    static bool HandleResetLevelCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        if (!HandleResetStatsOrLevelHelper(target))
            return false;

        uint8 oldLevel = target->getLevel();

        // set starting level
        uint32 startLevel = target->getClass() != CLASS_DEATH_KNIGHT
            ? sWorld->getIntConfig(WorldIntConfigs::CONFIG_START_PLAYER_LEVEL)
            : sWorld->getIntConfig(WorldIntConfigs::CONFIG_START_HEROIC_PLAYER_LEVEL);

        target->_ApplyAllLevelScaleItemMods(false);
        target->SetLevel(startLevel);
        target->InitRunes();
        target->InitStatsForLevel(true);
        target->InitTaxiNodesForLevel();
        target->InitGlyphsForLevel();
        target->InitTalentForLevel();
        target->SetUInt32Value(PLAYER_FIELD_XP, 0);

        target->_ApplyAllLevelScaleItemMods(true);

        // reset level for pet
        if (Pet* pet = target->GetPet())
            pet->SynchronizeLevelWithOwner();

        sScriptMgr->OnPlayerLevelChanged(target, oldLevel);

        return true;
    }

    static bool HandleResetSpellsCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        uint64 targetGuid;
        std::string targetName;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, &targetName))
            return false;

        if (target)
        {
            target->resetSpells(/* bool myClassOnly */);

            ChatHandler(target->GetSession()).SendSysMessage(LANG_RESET_SPELLS);
            if (!handler->GetSession() || handler->GetSession()->GetPlayer() != target)
                handler->PSendSysMessage(LANG_RESET_SPELLS_ONLINE, handler->GetNameLink(target).c_str());
        }
        else
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
            stmt->setUInt16(0, uint16(AT_LOGIN_RESET_SPELLS));
            stmt->setUInt32(1, GUID_LOPART(targetGuid));
            CharacterDatabase.Execute(stmt);

            handler->PSendSysMessage(LANG_RESET_SPELLS_OFFLINE, targetName.c_str());
        }

        return true;
    }

    static bool HandleResetStatsCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        if (!HandleResetStatsOrLevelHelper(target))
            return false;

        target->InitRunes();
        target->InitStatsForLevel(true);
        target->InitTaxiNodesForLevel();
        target->InitGlyphsForLevel();
        target->InitTalentForLevel();

        return true;
    }

    static bool HandleResetTalentsCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        uint64 targetGuid;
        std::string targetName;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, &targetName))
        {
            // Try reset talents as Hunter Pet
            Creature* creature = handler->getSelectedCreature();
            if (!*args && creature && creature->IsPet())
            {
                Unit* owner = creature->GetOwner();
                if (owner && owner->GetTypeId() == TypeID::TYPEID_PLAYER && creature->ToPet()->IsPermanentPetFor(owner->ToPlayer()))
                {
                    creature->ToPet()->resetTalents();

                    ChatHandler(owner->ToPlayer()->GetSession()).SendSysMessage(LANG_RESET_PET_TALENTS);
                    if (!handler->GetSession() || handler->GetSession()->GetPlayer() != owner->ToPlayer())
                        handler->PSendSysMessage(LANG_RESET_PET_TALENTS_ONLINE, handler->GetNameLink(owner->ToPlayer()).c_str());
                }
                return true;
            }

            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (target)
        {
            target->ResetTalents(true);
            target->SendTalentsInfoData();
            ChatHandler(target->GetSession()).SendSysMessage(LANG_RESET_TALENTS);
            if (!handler->GetSession() || handler->GetSession()->GetPlayer() != target)
                handler->PSendSysMessage(LANG_RESET_TALENTS_ONLINE, handler->GetNameLink(target).c_str());

            return true;
        }
        else if (targetGuid)
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
            stmt->setUInt16(0, uint16(AT_LOGIN_NONE | AT_LOGIN_RESET_PET_TALENTS));
            stmt->setUInt32(1, GUID_LOPART(targetGuid));
            CharacterDatabase.Execute(stmt);

            std::string nameLink = handler->playerLink(targetName);
            handler->PSendSysMessage(LANG_RESET_TALENTS_OFFLINE, nameLink.c_str());
            return true;
        }

        handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleResetAllCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string caseName = args;

        AtLoginFlags atLogin;

        // Command specially created as single command to prevent using short case names
        if (caseName == "spells")
        {
            atLogin = AT_LOGIN_RESET_SPELLS;
            sWorld->SendWorldText(LANG_RESETALL_SPELLS);
            if (!handler->GetSession())
                handler->SendSysMessage(LANG_RESETALL_SPELLS);
        }
        else if (caseName == "talents")
        {
            atLogin = AtLoginFlags(AT_LOGIN_RESET_TALENTS | AT_LOGIN_RESET_PET_TALENTS);
            sWorld->SendWorldText(LANG_RESETALL_TALENTS);
            if (!handler->GetSession())
                handler->SendSysMessage(LANG_RESETALL_TALENTS);
        }
        else
        {
            handler->PSendSysMessage(LANG_RESETALL_UNKNOWN_CASE, args);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ALL_AT_LOGIN_FLAGS);
        stmt->setUInt16(0, uint16(atLogin));
        CharacterDatabase.Execute(stmt);

        SF_SHARED_GUARD readGuard(*HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType const& plist = sObjectAccessor->GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = plist.begin(); itr != plist.end(); ++itr)
            itr->second->SetAtLoginFlag(atLogin);

        return true;
    }
};

void AddSC_reset_commandscript()
{
    new reset_commandscript();
}
