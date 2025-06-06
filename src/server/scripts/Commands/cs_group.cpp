/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Chat.h"
#include "GroupMgr.h"
#include "Language.h"
#include "LFG.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

class group_commandscript : public CommandScript
{
public:
    group_commandscript() : CommandScript("group_commandscript") { }

    std::vector<ChatCommand> GetCommands() const OVERRIDE
    {
        static std::vector<ChatCommand> groupCommandTable =
        {
            { "leader",  rbac::RBAC_PERM_COMMAND_GROUP_LEADER,  false, &HandleGroupLeaderCommand,  "", },
            { "disband", rbac::RBAC_PERM_COMMAND_GROUP_DISBAND, false, &HandleGroupDisbandCommand, "", },
            { "remove",  rbac::RBAC_PERM_COMMAND_GROUP_REMOVE,  false, &HandleGroupRemoveCommand,  "", },
            { "join",    rbac::RBAC_PERM_COMMAND_GROUP_JOIN,    false, &HandleGroupJoinCommand,    "", },
            { "list",    rbac::RBAC_PERM_COMMAND_GROUP_LIST,    false, &HandleGroupListCommand,    "", },
            { "summon",  rbac::RBAC_PERM_COMMAND_GROUP_SUMMON,  false, &HandleGroupSummonCommand,  "", },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "group", rbac::RBAC_PERM_COMMAND_GROUP, false, NULL, "", groupCommandTable },
        };
        return commandTable;
    }

    // Summon group of player
    static bool HandleGroupSummonCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        // check online security
        if (handler->HasLowerSecurity(target, 0))
            return false;

        Group* group = target->GetGroup();

        std::string nameLink = handler->GetNameLink(target);

        if (!group)
        {
            handler->PSendSysMessage(LANG_NOT_IN_GROUP, nameLink.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* gmPlayer = handler->GetSession()->GetPlayer();
        Group* gmGroup = gmPlayer->GetGroup();
        Map* gmMap = gmPlayer->GetMap();
        bool toInstance = gmMap->Instanceable();

        // we are in instance, and can summon only player in our group with us as lead
        if (toInstance && (
            !gmGroup || group->GetLeaderGUID() != gmPlayer->GetGUID() ||
            gmGroup->GetLeaderGUID() != gmPlayer->GetGUID()))
            // the last check is a bit excessive, but let it be, just in case
        {
            handler->SendSysMessage(LANG_CANNOT_SUMMON_TO_INST);
            handler->SetSentErrorMessage(true);
            return false;
        }

        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* player = itr->GetSource();

            if (!player || player == gmPlayer || !player->GetSession())
                continue;

            // check online security
            if (handler->HasLowerSecurity(player, 0))
                return false;

            std::string plNameLink = handler->GetNameLink(player);

            if (player->IsBeingTeleported())
            {
                handler->PSendSysMessage(LANG_IS_TELEPORTED, plNameLink.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }

            if (toInstance)
            {
                Map* playerMap = player->GetMap();

                if (playerMap->Instanceable() && playerMap->GetInstanceId() != gmMap->GetInstanceId())
                {
                    // cannot summon from instance to instance
                    handler->PSendSysMessage(LANG_CANNOT_SUMMON_TO_INST, plNameLink.c_str());
                    handler->SetSentErrorMessage(true);
                    return false;
                }
            }

            handler->PSendSysMessage(LANG_SUMMONING, plNameLink.c_str(), "");
            if (handler->needReportToTarget(player))
                ChatHandler(player->GetSession()).PSendSysMessage(LANG_SUMMONED_BY, handler->GetNameLink().c_str());

            // stop flight if need
            if (player->IsInFlight())
            {
                player->GetMotionMaster()->MovementExpired();
                player->CleanupAfterTaxiFlight();
            }
            // save only in non-flight case
            else
                player->SaveRecallPosition();

            // before GM
            float x, y, z;
            gmPlayer->GetClosePoint(x, y, z, player->GetObjectSize());
            player->TeleportTo(gmPlayer->GetMapId(), x, y, z, player->GetOrientation());
        }

        return true;
    }

    static bool HandleGroupLeaderCommand(ChatHandler* handler, char const* args)
    {
        Player* player = NULL;
        Group* group = NULL;
        uint64 guid = 0;
        char* nameStr = strtok((char*)args, " ");

        if (!handler->GetPlayerGroupAndGUIDByName(nameStr, player, group, guid))
            return false;

        if (!group)
        {
            handler->PSendSysMessage(LANG_GROUP_NOT_IN_GROUP, player->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (group->GetLeaderGUID() != guid)
        {
            group->ChangeLeader(guid);
            group->SendUpdate();
        }

        return true;
    }

    static bool HandleGroupDisbandCommand(ChatHandler* handler, char const* args)
    {
        Player* player = NULL;
        Group* group = NULL;
        uint64 guid = 0;
        char* nameStr = strtok((char*)args, " ");

        if (!handler->GetPlayerGroupAndGUIDByName(nameStr, player, group, guid))
            return false;

        if (!group)
        {
            handler->PSendSysMessage(LANG_GROUP_NOT_IN_GROUP, player->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        group->Disband();
        return true;
    }

    static bool HandleGroupRemoveCommand(ChatHandler* handler, char const* args)
    {
        Player* player = NULL;
        Group* group = NULL;
        uint64 guid = 0;
        char* nameStr = strtok((char*)args, " ");

        if (!handler->GetPlayerGroupAndGUIDByName(nameStr, player, group, guid))
            return false;

        if (!group)
        {
            handler->PSendSysMessage(LANG_GROUP_NOT_IN_GROUP, player->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        group->RemoveMember(guid);
        return true;
    }

    static bool HandleGroupJoinCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* playerSource = NULL;
        Player* playerTarget = NULL;
        Group* groupSource = NULL;
        Group* groupTarget = NULL;
        uint64 guidSource = 0;
        uint64 guidTarget = 0;
        char* nameplgrStr = strtok((char*)args, " ");
        char* nameplStr = strtok(NULL, " ");

        if (!handler->GetPlayerGroupAndGUIDByName(nameplgrStr, playerSource, groupSource, guidSource, true))
            return false;

        if (!groupSource)
        {
            handler->PSendSysMessage(LANG_GROUP_NOT_IN_GROUP, playerSource->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!handler->GetPlayerGroupAndGUIDByName(nameplStr, playerTarget, groupTarget, guidTarget, true))
            return false;

        if (groupTarget || playerTarget->GetGroup() == groupSource)
        {
            handler->PSendSysMessage(LANG_GROUP_ALREADY_IN_GROUP, playerTarget->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!groupSource->IsFull())
        {
            handler->PSendSysMessage(LANG_GROUP_FULL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        groupSource->AddMember(playerTarget);
        groupSource->BroadcastGroupUpdate();
        handler->PSendSysMessage(LANG_GROUP_PLAYER_JOINED, playerTarget->GetName().c_str(), playerSource->GetName().c_str());
        return true;
    }

    static bool HandleGroupListCommand(ChatHandler* handler, char const* args)
    {
        // Get ALL the variables!
        Player* playerTarget;
        uint32 phase = 0;
        uint64 guidTarget;
        std::string nameTarget;
        std::string zoneName;
        const char* onlineState = "";

        // Parse the guid to uint32...
        uint32 parseGUID = MAKE_NEW_GUID(atol((char*)args), 0, HIGHGUID_PLAYER);

        // ... and try to extract a player out of it.
        if (sObjectMgr->GetPlayerNameByGUID(parseGUID, nameTarget))
        {
            playerTarget = sObjectMgr->GetPlayerByLowGUID(parseGUID);
            guidTarget = parseGUID;
        }
        // If not, we return false and end right away.
        else if (!handler->extractPlayerTarget((char*)args, &playerTarget, &guidTarget, &nameTarget))
            return false;

        // Next, we need a group. So we define a group variable.
        Group* groupTarget = NULL;

        // We try to extract a group from an online player.
        if (playerTarget)
            groupTarget = playerTarget->GetGroup();

        // If not, we extract it from the SQL.
        if (!groupTarget)
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);
            stmt->setUInt32(0, guidTarget);
            PreparedQueryResult resultGroup = CharacterDatabase.Query(stmt);
            if (resultGroup)
                groupTarget = sGroupMgr->GetGroupByDbStoreId((*resultGroup)[0].GetUInt32());
        }

        // If both fails, players simply has no party. Return false.
        if (!groupTarget)
        {
            handler->PSendSysMessage(LANG_GROUP_NOT_IN_GROUP, nameTarget.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        // We get the group members after successfully detecting a group.
        Group::MemberSlotList const& members = groupTarget->GetMemberSlots();

        // To avoid a cluster fuck, namely trying multiple queries to simply get a group member count...
        handler->PSendSysMessage(LANG_GROUP_TYPE, (groupTarget->isRaidGroup() ? "raid" : "party"), members.size());
        // ... we simply move the group type and member count print after retrieving the slots and simply output it's size.

        // While rather dirty codestyle-wise, it saves space (if only a little). For each member, we look several informations up.
        for (Group::MemberSlotList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
        {
            // Define temporary variable slot to iterator.
            Group::MemberSlot const& slot = *itr;

            // Check for given flag and assign it to that iterator
            std::string flags;
            if (slot.flags & uint8(GroupMemberFlags::MEMBER_FLAG_ASSISTANT))
                flags = "Assistant";

            if (slot.flags & uint8(GroupMemberFlags::MEMBER_FLAG_MAINTANK))
            {
                if (!flags.empty())
                    flags.append(", ");
                flags.append("MainTank");
            }

            if (slot.flags & uint8(GroupMemberFlags::MEMBER_FLAG_MAINASSIST))
            {
                if (!flags.empty())
                    flags.append(", ");
                flags.append("MainAssist");
            }

            if (flags.empty())
                flags = "None";

            // Check if iterator is online. If is...
            Player* p = ObjectAccessor::FindPlayer((*itr).guid);
            if (p && p->IsInWorld())
            {
                // ... than, it prints information like "is online", where he is, etc...
                onlineState = "online";
                phase = (!p->IsGameMaster() ? p->GetPhaseMask() : -1);
                uint32 locale = handler->GetSessionDbcLocale();

                AreaTableEntry const* area = GetAreaEntryByAreaID(p->GetAreaId());
                if (area)
                {
                    AreaTableEntry const* zone = GetAreaEntryByAreaID(area->m_ParentAreaID);
                    if (zone)
                        zoneName = zone->m_AreaName[locale];
                }
            }
            else
            {
                // ... else, everything is set to offline or neutral values.
                zoneName = "<ERROR>";
                onlineState = "Offline";
                phase = 0;
            }

            // Now we can print those informations for every single member of each group!
            handler->PSendSysMessage(LANG_GROUP_PLAYER_NAME_GUID, slot.name.c_str(), onlineState,
                zoneName.c_str(), phase, GUID_LOPART(slot.guid), flags.c_str(),
                lfg::GetRolesString(slot.roles).c_str());
        }

        // And finish after every iterator is done.
        return true;
    }
};

void AddSC_group_commandscript()
{
    new group_commandscript();
}
