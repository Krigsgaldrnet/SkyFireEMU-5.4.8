/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Common.h"
#include "DBCStores.h"
#include "DisableMgr.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "GroupMgr.h"
#include "InstanceSaveMgr.h"
#include "LFGGroupData.h"
#include "LFGMgr.h"
#include "LFGPlayerData.h"
#include "LFGQueue.h"
#include "LFGScripts.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "WorldSession.h"

namespace lfg
{

    LFGMgr::LFGMgr() : m_QueueTimer(0), m_lfgProposalId(1),
        m_options(sWorld->getIntConfig(WorldIntConfigs::CONFIG_LFG_OPTIONSMASK))
    {
        new LFGPlayerScript();
        new LFGGroupScript();
    }

    LFGMgr::~LFGMgr()
    {
        for (LfgRewardContainer::iterator itr = RewardMapStore.begin(); itr != RewardMapStore.end(); ++itr)
            delete itr->second;
    }

    void LFGMgr::_LoadFromDB(Field* fields, uint64 guid)
    {
        if (!fields)
            return;

        if (!IS_GROUP_GUID(guid))
            return;

        SetLeader(guid, MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER));

        uint32 dungeon = fields[16].GetUInt32();
        uint8 state = fields[17].GetUInt8();

        if (!dungeon || !state)
            return;

        SetDungeon(guid, dungeon);

        switch (state)
        {
            case LFG_STATE_DUNGEON:
            case LFG_STATE_FINISHED_DUNGEON:
                //case LFG_STATE_BOOT:
                SetState(guid, (LfgState)state);
                break;
            default:
                break;
        }
    }

    void LFGMgr::_SaveToDB(uint64 guid, uint32 db_guid)
    {
        if (!IS_GROUP_GUID(guid))
            return;
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_LFG_DATA);

        stmt->setUInt32(0, db_guid);

        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_LFG_DATA);
        stmt->setUInt32(0, db_guid);

        stmt->setUInt32(1, GetDungeon(guid));
        stmt->setUInt32(2, GetState(guid));
        trans->Append(stmt);
        CharacterDatabase.CommitTransaction(trans);
    }

    /// Load rewards for completing dungeons
    void LFGMgr::LoadRewards()
    {
        uint32 oldMSTime = getMSTime();

        for (LfgRewardContainer::iterator itr = RewardMapStore.begin(); itr != RewardMapStore.end(); ++itr)
            delete itr->second;
        RewardMapStore.clear();

        // ORDER BY is very important for GetRandomDungeonReward!
        QueryResult result = WorldDatabase.Query("SELECT dungeonId, maxLevel, firstQuestId, otherQuestId FROM lfg_dungeon_rewards ORDER BY dungeonId, maxLevel ASC");

        if (!result)
        {
            SF_LOG_ERROR("server.loading", ">> Loaded 0 lfg dungeon rewards. DB table `lfg_dungeon_rewards` is empty!");
            return;
        }

        uint32 count = 0;

        Field* fields = NULL;
        do
        {
            fields = result->Fetch();
            uint32 dungeonId = fields[0].GetUInt32();
            uint32 maxLevel = fields[1].GetUInt8();
            uint32 firstQuestId = fields[2].GetUInt32();
            uint32 otherQuestId = fields[3].GetUInt32();

            if (!GetLFGDungeonEntry(dungeonId))
            {
                SF_LOG_ERROR("sql.sql", "Dungeon %u specified in table `lfg_dungeon_rewards` does not exist!", dungeonId);
                continue;
            }

            if (!maxLevel || maxLevel > sWorld->getIntConfig(WorldIntConfigs::CONFIG_MAX_PLAYER_LEVEL))
            {
                SF_LOG_ERROR("sql.sql", "Level %u specified for dungeon %u in table `lfg_dungeon_rewards` can never be reached!", maxLevel, dungeonId);
                maxLevel = sWorld->getIntConfig(WorldIntConfigs::CONFIG_MAX_PLAYER_LEVEL);
            }

            if (!firstQuestId || !sObjectMgr->GetQuestTemplate(firstQuestId))
            {
                SF_LOG_ERROR("sql.sql", "First quest %u specified for dungeon %u in table `lfg_dungeon_rewards` does not exist!", firstQuestId, dungeonId);
                continue;
            }

            if (otherQuestId && !sObjectMgr->GetQuestTemplate(otherQuestId))
            {
                SF_LOG_ERROR("sql.sql", "Other quest %u specified for dungeon %u in table `lfg_dungeon_rewards` does not exist!", otherQuestId, dungeonId);
                otherQuestId = 0;
            }

            RewardMapStore.insert(LfgRewardContainer::value_type(dungeonId, new LfgReward(maxLevel, firstQuestId, otherQuestId)));
            ++count;
        } while (result->NextRow());

        SF_LOG_INFO("server.loading", ">> Loaded %u lfg dungeon rewards in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    LFGDungeonData const* LFGMgr::GetLFGDungeon(uint32 id)
    {
        LFGDungeonContainer::const_iterator itr = LfgDungeonStore.find(id);
        if (itr != LfgDungeonStore.end())
            return &(itr->second);

        return NULL;
    }

    void LFGMgr::LoadLFGDungeons(bool reload /* = false */)
    {
        uint32 oldMSTime = getMSTime();

        LfgDungeonStore.clear();

        // Initialize Dungeon map with data from dbcs
        for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
        {
            LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i);
            if (!dungeon)
                continue;

            switch (dungeon->m_Type)
            {
                case LFG_TYPE_DUNGEON:
                case LFG_TYPE_RAID:
                case LFG_TYPE_RANDOM:
                    LfgDungeonStore[dungeon->m_ID] = LFGDungeonData(dungeon);
                    break;
            }
        }

        // Fill teleport locations from DB
        QueryResult result = WorldDatabase.Query("SELECT dungeonId, position_x, position_y, position_z, orientation FROM lfg_entrances");
        if (!result)
        {
            SF_LOG_ERROR("server.loading", ">> Loaded 0 lfg entrance positions. DB table `lfg_entrances` is empty!");
            return;
        }

        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();
            uint32 dungeonId = fields[0].GetUInt32();
            LFGDungeonContainer::iterator dungeonItr = LfgDungeonStore.find(dungeonId);
            if (dungeonItr == LfgDungeonStore.end())
            {
                SF_LOG_ERROR("sql.sql", "table `lfg_entrances` contains coordinates for wrong dungeon %u", dungeonId);
                continue;
            }

            LFGDungeonData& data = dungeonItr->second;
            data.x = fields[1].GetFloat();
            data.y = fields[2].GetFloat();
            data.z = fields[3].GetFloat();
            data.o = fields[4].GetFloat();

            ++count;
        } while (result->NextRow());

        SF_LOG_INFO("server.loading", ">> Loaded %u lfg entrance positions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

        // Fill all other teleport coords from areatriggers
        for (LFGDungeonContainer::iterator itr = LfgDungeonStore.begin(); itr != LfgDungeonStore.end(); ++itr)
        {
            LFGDungeonData& dungeon = itr->second;

            // No teleport coords in database, load from areatriggers
            if (dungeon.type != LFG_TYPE_RANDOM && dungeon.x == 0.0f && dungeon.y == 0.0f && dungeon.z == 0.0f)
            {
                AreaTriggerStruct const* at = sObjectMgr->GetMapEntranceTrigger(dungeon.map);
                if (!at)
                {
                    SF_LOG_ERROR("sql.sql", "Failed to load dungeon %s, cant find areatrigger for map %u", dungeon.name.c_str(), dungeon.map);
                    continue;
                }

                dungeon.map = at->target_mapId;
                dungeon.x = at->target_X;
                dungeon.y = at->target_Y;
                dungeon.z = at->target_Z;
                dungeon.o = at->target_Orientation;
            }

            if (dungeon.type != LFG_TYPE_RANDOM)
                CachedDungeonMapStore[dungeon.group].insert(dungeon.id);
            CachedDungeonMapStore[0].insert(dungeon.id);
        }

        if (reload)
            CachedDungeonMapStore.clear();
    }

    void LFGMgr::Update(uint32 diff)
    {
        if (!isOptionEnabled(LFG_OPTION_ENABLE_DUNGEON_FINDER | LFG_OPTION_ENABLE_RAID_BROWSER))
            return;

        time_t currTime = time(NULL);

        // Remove obsolete role checks
        for (LfgRoleCheckContainer::iterator it = RoleChecksStore.begin(); it != RoleChecksStore.end();)
        {
            LfgRoleCheckContainer::iterator itRoleCheck = it++;
            LfgRoleCheck& roleCheck = itRoleCheck->second;
            if (currTime < roleCheck.cancelTime)
                continue;
            roleCheck.state = LFG_ROLECHECK_MISSING_ROLE;

            for (LfgRolesMap::const_iterator itRoles = roleCheck.roles.begin(); itRoles != roleCheck.roles.end(); ++itRoles)
            {
                uint64 guid = itRoles->first;
                RestoreState(guid, "Remove Obsolete RoleCheck");
                SendLfgRoleCheckUpdate(guid, roleCheck);
                if (guid == roleCheck.leader)
                    SendLfgJoinResult(guid, LfgJoinResultData(LFG_JOIN_ROLE_CHECK_FAILED, LFG_ROLECHECK_MISSING_ROLE));
            }

            RestoreState(itRoleCheck->first, "Remove Obsolete RoleCheck");
            RoleChecksStore.erase(itRoleCheck);
        }

        // Remove obsolete proposals
        for (LfgProposalContainer::iterator it = ProposalsStore.begin(); it != ProposalsStore.end();)
        {
            LfgProposalContainer::iterator itRemove = it++;
            if (itRemove->second.cancelTime < currTime)
                RemoveProposal(itRemove, LFG_UPDATETYPE_PROPOSAL_FAILED);
        }

        // Remove obsolete kicks
        for (LfgPlayerBootContainer::iterator it = BootsStore.begin(); it != BootsStore.end();)
        {
            LfgPlayerBootContainer::iterator itBoot = it++;
            LfgPlayerBoot& boot = itBoot->second;
            if (boot.cancelTime < currTime)
            {
                boot.inProgress = false;
                for (LfgAnswerContainer::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
                {
                    uint64 pguid = itVotes->first;
                    if (pguid != boot.victim)
                        SendLfgBootProposalUpdate(pguid, boot);
                    SetState(pguid, LFG_STATE_DUNGEON);
                }
                SetVoteKick(itBoot->first, false);
                BootsStore.erase(itBoot);
            }
        }

        uint32 lastProposalId = m_lfgProposalId;
        // Check if a proposal can be formed with the new groups being added
        for (LfgQueueContainer::iterator it = QueuesStore.begin(); it != QueuesStore.end(); ++it)
            if (uint8 newProposals = it->second.FindGroups())
                SF_LOG_DEBUG("lfg.update", "Found %u new groups in queue %u", newProposals, it->first);

        if (lastProposalId != m_lfgProposalId)
        {
            // FIXME lastProposalId ? lastProposalId +1 ?
            for (LfgProposalContainer::const_iterator itProposal = ProposalsStore.find(m_lfgProposalId); itProposal != ProposalsStore.end(); ++itProposal)
            {
                uint32 proposalId = itProposal->first;
                LfgProposal& proposal = ProposalsStore[proposalId];

                uint64 guid = 0;
                for (LfgProposalPlayerContainer::const_iterator itPlayers = proposal.players.begin(); itPlayers != proposal.players.end(); ++itPlayers)
                {
                    guid = itPlayers->first;
                    SetState(guid, LFG_STATE_PROPOSAL);
                    if (uint64 gguid = GetGroup(guid))
                    {
                        SetState(gguid, LFG_STATE_PROPOSAL);
                        SendLfgUpdateStatus(guid, LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)), true);
                    }
                    else
                        SendLfgUpdateStatus(guid, LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)), false);
                    SendLfgUpdateProposal(guid, proposal);
                }

                if (proposal.state == LFG_PROPOSAL_SUCCESS)
                    UpdateProposal(proposalId, guid, true);
            }
        }

        // Update all players status queue info
        if (m_QueueTimer > LFG_QUEUEUPDATE_INTERVAL)
        {
            m_QueueTimer = 0;
            time_t currTime = time(NULL);
            for (LfgQueueContainer::iterator it = QueuesStore.begin(); it != QueuesStore.end(); ++it)
                it->second.UpdateQueueTimers(it->first, currTime);
        }
        else
            m_QueueTimer += diff;
    }

    /**
        Adds the player/group to lfg queue. If player is in a group then it is the leader
        of the group tying to join the group. Join conditions are checked before adding
        to the new queue.

       @param[in]     player Player trying to join (or leader of group trying to join)
       @param[in]     roles Player selected roles
       @param[in]     dungeons Dungeons the player/group is applying for
       @param[in]     comment Player selected comment
    */
    void LFGMgr::JoinLfg(Player* player, uint8 roles, LfgDungeonSet& dungeons, const std::string& comment)
    {
        if (!player || !player->GetSession() || dungeons.empty())
            return;

        Group* grp = player->GetGroup();
        uint64 guid = player->GetGUID();
        uint64 gguid = grp ? grp->GetGUID() : guid;
        LfgJoinResultData joinData;
        LfgGuidSet players;
        uint32 rDungeonId = 0;
        bool isContinue = grp && grp->isLFGGroup() && GetState(gguid) != LFG_STATE_FINISHED_DUNGEON;

        // Do not allow to change dungeon in the middle of a current dungeon
        if (isContinue)
        {
            dungeons.clear();
            dungeons.insert(GetDungeon(gguid));
        }

        // Already in queue?
        LfgState state = GetState(gguid);
        if (state == LFG_STATE_QUEUED)
        {
            LFGQueue& queue = GetQueue(gguid);
            queue.RemoveFromQueue(gguid);
        }

        // Check player or group member restrictions
        if (!player->GetSession()->HasPermission(rbac::RBAC_PERM_JOIN_DUNGEON_FINDER))
            joinData.result = LFG_JOIN_NOT_MEET_REQS;
        else if (player->InBattleground() || player->InArena() || player->InBattlegroundQueue())
            joinData.result = LFG_JOIN_USING_BG_SYSTEM;
        else if (player->HasAura(LFG_SPELL_DUNGEON_DESERTER))
            joinData.result = LFG_JOIN_DESERTER;
        else if (player->HasAura(LFG_SPELL_DUNGEON_COOLDOWN))
            joinData.result = LFG_JOIN_RANDOM_COOLDOWN;
        else if (dungeons.empty())
            joinData.result = LFG_JOIN_NOT_MEET_REQS;
        else if (grp)
        {
            if (grp->GetMembersCount() > MAXGROUPSIZE)
                joinData.result = LFG_JOIN_TOO_MUCH_MEMBERS;
            else
            {
                uint8 memberCount = 0;
                for (GroupReference* itr = grp->GetFirstMember(); itr != NULL && joinData.result == LFG_JOIN_OK; itr = itr->next())
                {
                    if (Player* plrg = itr->GetSource())
                    {
                        if (!plrg->GetSession()->HasPermission(rbac::RBAC_PERM_JOIN_DUNGEON_FINDER))
                            joinData.result = LFG_JOIN_INTERNAL_ERROR;
                        if (plrg->HasAura(LFG_SPELL_DUNGEON_DESERTER))
                            joinData.result = LFG_JOIN_PARTY_DESERTER;
                        else if (plrg->HasAura(LFG_SPELL_DUNGEON_COOLDOWN))
                            joinData.result = LFG_JOIN_PARTY_RANDOM_COOLDOWN;
                        else if (plrg->InBattleground() || plrg->InArena() || plrg->InBattlegroundQueue())
                            joinData.result = LFG_JOIN_USING_BG_SYSTEM;
                        ++memberCount;
                        players.insert(plrg->GetGUID());
                    }
                }

                if (joinData.result == LFG_JOIN_OK && memberCount != grp->GetMembersCount())
                    joinData.result = LFG_JOIN_DISCONNECTED;
            }
        }
        else
            players.insert(player->GetGUID());

        // Check if all dungeons are valid
        bool isRaid = false;
        if (joinData.result == LFG_JOIN_OK)
        {
            bool isDungeon = false;
            for (LfgDungeonSet::const_iterator it = dungeons.begin(); it != dungeons.end() && joinData.result == LFG_JOIN_OK; ++it)
            {
                LfgType type = GetDungeonType(*it);
                switch (type)
                {
                    case LFG_TYPE_RANDOM:
                        if (dungeons.size() > 1)               // Only allow 1 random dungeon
                            joinData.result = LFG_JOIN_DUNGEON_INVALID;
                        else
                            rDungeonId = (*dungeons.begin());
                        // No break on purpose (Random can only be dungeon or heroic dungeon)
                    case LFG_TYPE_DUNGEON:
                        if (isRaid)
                            joinData.result = LFG_JOIN_MIXED_RAID_DUNGEON;
                        isDungeon = true;
                        break;
                    case LFG_TYPE_RAID:
                        if (isDungeon)
                            joinData.result = LFG_JOIN_MIXED_RAID_DUNGEON;
                        isRaid = true;
                        break;
                    default:
                        joinData.result = LFG_JOIN_DUNGEON_INVALID;
                        break;
                }
            }

            // it could be changed
            if (joinData.result == LFG_JOIN_OK)
            {
                // Expand random dungeons and check restrictions
                if (rDungeonId)
                    dungeons = GetDungeonsByRandom(rDungeonId);

                // if we have lockmap then there are no compatible dungeons
                GetCompatibleDungeons(dungeons, players, joinData.lockmap, isContinue);
                if (dungeons.empty())
                    joinData.result = grp ? LFG_JOIN_INTERNAL_ERROR : LFG_JOIN_NOT_MEET_REQS;
            }
        }

        // Can't join. Send result
        if (joinData.result != LFG_JOIN_OK)
        {
            SF_LOG_DEBUG("lfg.join", "%u joining with %u members. Result: %u, Dungeons: %s",
                GUID_LOPART(guid), grp ? grp->GetMembersCount() : 1, joinData.result, ConcatenateDungeons(dungeons).c_str());
            if (!dungeons.empty())                             // Only should show lockmap when have no dungeons available
                joinData.lockmap.clear();
            player->GetSession()->SendLfgJoinResult(joinData);
            return;
        }

        SetComment(guid, comment);

        if (isRaid)
        {
            SF_LOG_DEBUG("lfg.join", "%u trying to join raid browser and it's disabled.", GUID_LOPART(guid));
            return;
        }

        std::string debugNames = "";
        if (grp)                                               // Begin rolecheck
        {
            // Create new rolecheck
            LfgRoleCheck& roleCheck = RoleChecksStore[gguid];
            roleCheck.cancelTime = time_t(time(NULL)) + LFG_TIME_ROLECHECK;
            roleCheck.state = grp->RoleCheckAllResponded() ? LFG_ROLECHECK_FINISHED : LFG_ROLECHECK_INITIALITING;
            roleCheck.leader = guid;
            roleCheck.dungeons = dungeons;
            roleCheck.rDungeonId = rDungeonId;

            if (rDungeonId)
            {
                dungeons.clear();
                dungeons.insert(rDungeonId);
            }

            SetState(gguid, LFG_STATE_ROLECHECK);
            // Send update to player
            LfgUpdateData updateData = LfgUpdateData(LFG_UPDATETYPE_JOIN_QUEUE, dungeons, comment);
            for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                if (Player* plrg = itr->GetSource())
                {
                    uint64 pguid = plrg->GetGUID();
                    plrg->GetSession()->SendLfgUpdateStatus(updateData, false);
                    SetState(pguid, LFG_STATE_ROLECHECK);
                    if (!isContinue)
                        SetSelectedDungeons(pguid, dungeons);
                    roleCheck.roles[pguid] = grp->GetMemberRole(pguid);
                    if (!debugNames.empty())
                        debugNames.append(", ");
                    debugNames.append(plrg->GetName());
                }
            }
            // Update leader role
            UpdateRoleCheck(gguid, guid, roles);
        }
        else                                                   // Add player to queue
        {
            LfgRolesMap rolesMap;
            rolesMap[guid] = roles;
            LFGQueue& queue = GetQueue(guid);
            queue.AddQueueData(guid, time(NULL), dungeons, rolesMap);

            if (!isContinue)
            {
                if (rDungeonId)
                {
                    dungeons.clear();
                    dungeons.insert(rDungeonId);
                }
                SetSelectedDungeons(guid, dungeons);
            }
            // Send update to player
            player->GetSession()->SendLfgJoinResult(joinData);
            player->GetSession()->SendLfgUpdateStatus(LfgUpdateData(LFG_UPDATETYPE_JOIN_QUEUE, dungeons, comment), false);
            SetState(gguid, LFG_STATE_QUEUED);
            SetRoles(guid, roles);
            debugNames.append(player->GetName());
        }

        SF_LOG_DEBUG("lfg.join", "%u joined (%s), Members: %s. Dungeons (%u): %s", GUID_LOPART(guid),
            grp ? "group" : "player", debugNames.c_str(), uint32(dungeons.size()), ConcatenateDungeons(dungeons).c_str());
    }

    /**
        Leaves Dungeon System. Player/Group is removed from queue, rolechecks, proposals
        or votekicks. Player or group needs to be not NULL and using Dungeon System

       @param[in]     guid Player or group guid
    */
    void LFGMgr::LeaveLfg(uint64 guid, bool disconnected)
    {
        uint64 gguid = IS_GROUP_GUID(guid) ? guid : GetGroup(guid);

        SF_LOG_DEBUG("lfg.leave", "%u left (%s)", GUID_LOPART(guid), guid == gguid ? "group" : "player");

        LfgState state = GetState(guid);
        switch (state)
        {
            case LFG_STATE_QUEUED:
                if (gguid)
                {
                    LFGQueue& queue = GetQueue(gguid);
                    queue.RemoveFromQueue(gguid);
                    SetState(gguid, LFG_STATE_NONE);
                    const LfgGuidSet& players = GetPlayers(gguid);
                    for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
                    {
                        SetState(*it, LFG_STATE_NONE);
                        SendLfgUpdateStatus(*it, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE), true);
                    }
                }
                else
                {
                    LFGQueue& queue = GetQueue(guid);
                    queue.RemoveFromQueue(guid);
                    SendLfgUpdateStatus(guid, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE), false);
                    SetState(guid, LFG_STATE_NONE);
                }
                break;
            case LFG_STATE_ROLECHECK:
                if (gguid)
                    UpdateRoleCheck(gguid);                    // No player to update role = LFG_ROLECHECK_ABORTED
                break;
            case LFG_STATE_PROPOSAL:
            {
                // Remove from Proposals
                LfgProposalContainer::iterator it = ProposalsStore.begin();
                uint64 pguid = gguid == guid ? GetLeader(gguid) : guid;
                while (it != ProposalsStore.end())
                {
                    LfgProposalPlayerContainer::iterator itPlayer = it->second.players.find(pguid);
                    if (itPlayer != it->second.players.end())
                    {
                        // Mark the player/leader of group who left as didn't accept the proposal
                        itPlayer->second.accept = LFG_ANSWER_DENY;
                        break;
                    }
                    ++it;
                }

                // Remove from queue - if proposal is found, RemoveProposal will call RemoveFromQueue
                if (it != ProposalsStore.end())
                    RemoveProposal(it, LFG_UPDATETYPE_PROPOSAL_DECLINED);
                break;
            }
            case LFG_STATE_NONE:
            case LFG_STATE_RAIDBROWSER:
                break;
            case LFG_STATE_DUNGEON:
            case LFG_STATE_FINISHED_DUNGEON:
                //case LFG_STATE_BOOT:
                if (guid != gguid && !disconnected) // Player
                    SetState(guid, LFG_STATE_NONE);
                break;
        }
    }

    void LFGMgr::LeaveSoloLfg(uint64 guid, uint32 queueId, bool disconnected)
    {
        SF_LOG_DEBUG("lfg.leave", "Player: %u left queue.", GUID_LOPART(guid));

        LfgState state = GetState(guid);
        switch (state)
        {
            case LFG_STATE_QUEUED:
            {
                LFGQueue& queue = GetQueue(queueId);
                queue.RemoveFromQueue(guid);
                SendLfgUpdateStatus(guid, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE), false);
                SetState(guid, LFG_STATE_NONE);
                break;
            }
            case LFG_STATE_PROPOSAL:
            {
                // Remove from Proposals
                LfgProposalContainer::iterator it = ProposalsStore.begin();
                while (it != ProposalsStore.end())
                {
                    LfgProposalPlayerContainer::iterator itPlayer = it->second.players.find(guid);
                    if (itPlayer != it->second.players.end())
                    {
                        // Mark the player/leader of group who left as didn't accept the proposal
                        itPlayer->second.accept = LFG_ANSWER_DENY;
                        break;
                    }
                    ++it;
                }

                // Remove from queue - if proposal is found, RemoveProposal will call RemoveFromQueue
                if (it != ProposalsStore.end())
                    RemoveProposal(it, LFG_UPDATETYPE_PROPOSAL_DECLINED);
                break;
            }
            case LFG_STATE_NONE:
            case LFG_STATE_RAIDBROWSER:
                break;
            case LFG_STATE_DUNGEON:
            case LFG_STATE_FINISHED_DUNGEON:
            case LFG_STATE_BOOT:
            {
                SetState(guid, LFG_STATE_NONE);
                break;
            }
        }
    }

    /**
       Update the Role check info with the player selected role.

       @param[in]     grp Group guid to update rolecheck
       @param[in]     guid Player guid (0 = rolecheck failed)
       @param[in]     roles Player selected roles
    */
    void LFGMgr::UpdateRoleCheck(uint64 gguid, uint64 guid /* = 0 */, uint8 roles /* = PLAYER_ROLE_NONE */)
    {
        if (!gguid)
            return;

        LfgRolesMap check_roles;
        LfgRoleCheckContainer::iterator itRoleCheck = RoleChecksStore.find(gguid);
        if (itRoleCheck == RoleChecksStore.end())
            return;

        LfgRoleCheck& roleCheck = itRoleCheck->second;
        bool sendRoleChosen = roleCheck.state != LFG_ROLECHECK_DEFAULT && guid;

        if (!guid)
            roleCheck.state = LFG_ROLECHECK_ABORTED;
        else if (roles < PLAYER_ROLE_TANK)                            // Player selected no role.
            roleCheck.state = LFG_ROLECHECK_NO_ROLE;
        else
        {
            roleCheck.roles[guid] = roles;

            // Check if all players have selected a role
            LfgRolesMap::const_iterator itRoles = roleCheck.roles.begin();
            while (itRoles != roleCheck.roles.end() && itRoles->second != PLAYER_ROLE_NONE)
                ++itRoles;

            if (itRoles == roleCheck.roles.end())
            {
                // use temporal var to check roles, CheckGroupRoles modifies the roles
                check_roles = roleCheck.roles;
                roleCheck.state = CheckGroupRoles(check_roles) ? LFG_ROLECHECK_FINISHED : LFG_ROLECHECK_WRONG_ROLES;
            }
        }

        LfgDungeonSet dungeons;
        if (roleCheck.rDungeonId)
            dungeons.insert(roleCheck.rDungeonId);
        else
            dungeons = roleCheck.dungeons;

        LfgJoinResult joinResult = LFG_JOIN_FAILED;
        if (roleCheck.state == LFG_ROLECHECK_MISSING_ROLE || roleCheck.state == LFG_ROLECHECK_WRONG_ROLES)
            joinResult = LFG_JOIN_ROLE_CHECK_FAILED;

        LfgJoinResultData joinData = LfgJoinResultData(joinResult, roleCheck.state);
        for (LfgRolesMap::const_iterator it = roleCheck.roles.begin(); it != roleCheck.roles.end(); ++it)
        {
            uint64 pguid = it->first;

            if (sendRoleChosen)
                SendLfgRoleChosen(pguid, guid, roles);

            SendLfgRoleCheckUpdate(pguid, roleCheck);
            switch (roleCheck.state)
            {
                case LFG_ROLECHECK_INITIALITING:
                    continue;
                case LFG_ROLECHECK_FINISHED:
                    SetState(pguid, LFG_STATE_QUEUED);
                    SetRoles(pguid, it->second);
                    SendLfgUpdateStatus(pguid, LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, dungeons, GetComment(pguid)), true);
                    break;
                default:
                    if (roleCheck.leader == pguid)
                        SendLfgJoinResult(pguid, joinData);
                    SendLfgUpdateStatus(pguid, LfgUpdateData(LFG_UPDATETYPE_ROLECHECK_FAILED), true);
                    RestoreState(pguid, "Rolecheck Failed");
                    break;
            }
        }

        if (roleCheck.state == LFG_ROLECHECK_FINISHED)
        {
            SetState(gguid, LFG_STATE_QUEUED);
            LFGQueue& queue = GetQueue(gguid);
            queue.AddQueueData(gguid, time_t(time(NULL)), roleCheck.dungeons, roleCheck.roles);
            RoleChecksStore.erase(itRoleCheck);
        }
        else if (roleCheck.state != LFG_ROLECHECK_INITIALITING)
        {
            RestoreState(gguid, "Rolecheck Failed");
            RoleChecksStore.erase(itRoleCheck);
        }
    }

    /**
       Given a list of dungeons remove the dungeons players have restrictions.

       @param[in, out] dungeons Dungeons to check restrictions
       @param[in]     players Set of players to check their dungeon restrictions
       @param[out]    lockMap Map of players Lock status info of given dungeons (Empty if dungeons is not empty)
    */
    void LFGMgr::GetCompatibleDungeons(LfgDungeonSet& dungeons, LfgGuidSet const& players, LfgLockPartyMap& lockMap, bool isContinue)
    {
        lockMap.clear();
        std::map<uint32, uint32> lockedDungeons;
        for (LfgGuidSet::const_iterator it = players.begin(); it != players.end() && !dungeons.empty(); ++it)
        {
            uint64 guid = (*it);
            LfgLockMap const& cachedLockMap = GetLockedDungeons(guid);
            Player* player = ObjectAccessor::FindPlayer(guid);
            for (LfgLockMap::const_iterator it2 = cachedLockMap.begin(); it2 != cachedLockMap.end() && !dungeons.empty(); ++it2)
            {
                uint32 dungeonId = (it2->first & 0x00FFFFFF); // Compare dungeon ids
                LfgDungeonSet::iterator itDungeon = dungeons.find(dungeonId);
                if (itDungeon != dungeons.end())
                {
                    bool eraseDungeon = true;
                    // Don't remove the dungeon if team members are trying to continue a locked instance
                    if (it2->second == LFG_LOCKSTATUS_RAID_LOCKED && isContinue)
                    {
                        LFGDungeonData const* dungeon = GetLFGDungeon(dungeonId);
                        ASSERT(dungeon);
                        ASSERT(player);
                        if (InstancePlayerBind* playerBind = player->GetBoundInstance(dungeon->map, DifficultyID(dungeon->difficulty)))
                        {
                            if (InstanceSave* playerSave = playerBind->save)
                            {
                                uint32 dungeonInstanceId = playerSave->GetInstanceId();
                                auto itLockedDungeon = lockedDungeons.find(dungeonId);
                                if (itLockedDungeon == lockedDungeons.end() || itLockedDungeon->second == dungeonInstanceId)
                                    eraseDungeon = false;
                                lockedDungeons[dungeonId] = dungeonInstanceId;
                            }
                        }
                    }

                    if (eraseDungeon)
                        dungeons.erase(itDungeon);

                    lockMap[guid][dungeonId] = it2->second;
                }
            }
        }
        if (!dungeons.empty())
            lockMap.clear();
    }

    /**
       Check if a group can be formed with the given group roles

       @param[in]     groles Map of roles to check
       @return True if roles are compatible
    */
    bool LFGMgr::CheckGroupRoles(LfgRolesMap& groles)
    {
        if (groles.empty())
            return false;

        uint8 damage = 0;
        uint8 tank = 0;
        uint8 healer = 0;

        for (LfgRolesMap::iterator it = groles.begin(); it != groles.end(); ++it)
        {
            uint8 role = it->second & ~PLAYER_ROLE_LEADER;
            if (role == PLAYER_ROLE_NONE)
                return false;

            if (role & PLAYER_ROLE_DAMAGE)
            {
                if (role != PLAYER_ROLE_DAMAGE)
                {
                    it->second -= PLAYER_ROLE_DAMAGE;
                    if (CheckGroupRoles(groles))
                        return true;
                    it->second += PLAYER_ROLE_DAMAGE;
                }
                else if (damage == LFG_DPS_NEEDED)
                    return false;
                else
                    damage++;
            }

            if (role & PLAYER_ROLE_HEALER)
            {
                if (role != PLAYER_ROLE_HEALER)
                {
                    it->second -= PLAYER_ROLE_HEALER;
                    if (CheckGroupRoles(groles))
                        return true;
                    it->second += PLAYER_ROLE_HEALER;
                }
                else if (healer == LFG_HEALERS_NEEDED)
                    return false;
                else
                    healer++;
            }

            if (role & PLAYER_ROLE_TANK)
            {
                if (role != PLAYER_ROLE_TANK)
                {
                    it->second -= PLAYER_ROLE_TANK;
                    if (CheckGroupRoles(groles))
                        return true;
                    it->second += PLAYER_ROLE_TANK;
                }
                else if (tank == LFG_TANKS_NEEDED)
                    return false;
                else
                    tank++;
            }
        }
        return (tank + healer + damage) == uint8(groles.size());
    }

    /**
       Makes a new group given a proposal
       @param[in]     proposal Proposal to get info from
    */
    void LFGMgr::MakeNewGroup(LfgProposal const& proposal)
    {
        LfgGuidList players;
        LfgGuidList playersToTeleport;

        for (LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
        {
            uint64 guid = it->first;
            if (guid == proposal.leader)
                players.push_front(guid);
            else
                players.push_back(guid);

            if (proposal.isNew || GetGroup(guid) != proposal.group)
                playersToTeleport.push_back(guid);
        }

        // Set the dungeon difficulty
        LFGDungeonData const* dungeon = GetLFGDungeon(proposal.dungeonId);
        ASSERT(dungeon);

        Group* grp = proposal.group ? sGroupMgr->GetGroupByGUID(GUID_LOPART(proposal.group)) : NULL;
        for (LfgGuidList::const_iterator it = players.begin(); it != players.end(); ++it)
        {
            uint64 pguid = (*it);
            Player* player = ObjectAccessor::FindPlayer(pguid);
            if (!player)
                continue;

            Group* group = player->GetGroup();
            if (group && group != grp)
                group->RemoveMember(player->GetGUID());

            if (!grp)
            {
                grp = new Group();
                grp->ConvertToLFG();
                grp->Create(player);
                uint64 gguid = grp->GetGUID();
                SetState(gguid, LFG_STATE_PROPOSAL);
                sGroupMgr->AddGroup(grp);
            }
            else if (group != grp)
                grp->AddMember(player);

            grp->SetLfgRoles(pguid, proposal.players.find(pguid)->second.role);

            // Add the cooldown spell if queued for a random dungeon
            if (dungeon->type == LFG_TYPE_RANDOM)
                player->CastSpell(player, LFG_SPELL_DUNGEON_COOLDOWN, false);
        }

        ASSERT(grp);
        grp->SetDungeonDifficulty(DifficultyID(dungeon->difficulty));
        uint64 gguid = grp->GetGUID();
        SetDungeon(gguid, dungeon->Entry());
        SetState(gguid, LFG_STATE_DUNGEON);

        _SaveToDB(gguid, grp->GetDbStoreId());

        // Teleport Player
        for (LfgGuidList::const_iterator it = playersToTeleport.begin(); it != playersToTeleport.end(); ++it)
            if (Player* player = ObjectAccessor::FindPlayer(*it))
                TeleportPlayer(player, false);

        // Update group info
        grp->SendUpdate();
    }

    uint32 LFGMgr::AddProposal(LfgProposal& proposal)
    {
        proposal.id = ++m_lfgProposalId;
        ProposalsStore[m_lfgProposalId] = proposal;
        return m_lfgProposalId;
    }

    /**
       Update Proposal info with player answer

       @param[in]     proposalId Proposal id to be updated
       @param[in]     guid Player guid to update answer
       @param[in]     accept Player answer
    */
    void LFGMgr::UpdateProposal(uint32 proposalId, uint64 guid, bool accept)
    {
        // Check if the proposal exists
        LfgProposalContainer::iterator itProposal = ProposalsStore.find(proposalId);
        if (itProposal == ProposalsStore.end())
            return;

        LfgProposal& proposal = itProposal->second;

        // Check if proposal have the current player
        LfgProposalPlayerContainer::iterator itProposalPlayer = proposal.players.find(guid);
        if (itProposalPlayer == proposal.players.end())
            return;

        LfgProposalPlayer& player = itProposalPlayer->second;
        player.accept = LfgAnswer(accept);

        SF_LOG_DEBUG("lfg.proposal.update", "Player %u, Proposal %u, Selection: %u", GUID_LOPART(guid), proposalId, accept);
        if (!accept)
        {
            RemoveProposal(itProposal, LFG_UPDATETYPE_PROPOSAL_DECLINED);
            return;
        }

        // check if all have answered and reorder players (leader first)
        bool allAnswered = true;
        for (LfgProposalPlayerContainer::const_iterator itPlayers = proposal.players.begin(); itPlayers != proposal.players.end(); ++itPlayers)
            if (itPlayers->second.accept != LFG_ANSWER_AGREE)   // No answer (-1) or not accepted (0)
                allAnswered = false;

        if (!allAnswered)
        {
            for (LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
                SendLfgUpdateProposal(it->first, proposal);

            return;
        }

        bool sendUpdate = proposal.state != LFG_PROPOSAL_SUCCESS;
        proposal.state = LFG_PROPOSAL_SUCCESS;
        time_t joinTime = time(NULL);

        LFGQueue& queue = GetQueue(guid);
        LfgUpdateData updateData = LfgUpdateData(LFG_UPDATETYPE_GROUP_FOUND);
        for (LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
        {
            uint64 pguid = it->first;
            uint64 gguid = it->second.group;
            uint32 dungeonId = (*GetSelectedDungeons(pguid).begin());
            int32 waitTime = -1;
            if (sendUpdate)
                SendLfgUpdateProposal(pguid, proposal);

            if (gguid)
            {
                waitTime = int32((joinTime - queue.GetJoinTime(gguid)) / IN_MILLISECONDS);
                SendLfgUpdateStatus(pguid, updateData, false);
            }
            else
            {
                waitTime = int32((joinTime - queue.GetJoinTime(pguid)) / IN_MILLISECONDS);
                SendLfgUpdateStatus(pguid, updateData, false);
            }
            updateData.updateType = LFG_UPDATETYPE_REMOVED_FROM_QUEUE;
            SendLfgUpdateStatus(pguid, updateData, true);
            SendLfgUpdateStatus(pguid, updateData, false);

            // Update timers
            uint8 role = GetRoles(pguid);
            role &= ~PLAYER_ROLE_LEADER;
            switch (role)
            {
                case PLAYER_ROLE_DAMAGE:
                    queue.UpdateWaitTimeDps(waitTime, dungeonId);
                    break;
                case PLAYER_ROLE_HEALER:
                    queue.UpdateWaitTimeHealer(waitTime, dungeonId);
                    break;
                case PLAYER_ROLE_TANK:
                    queue.UpdateWaitTimeTank(waitTime, dungeonId);
                    break;
                default:
                    queue.UpdateWaitTimeAvg(waitTime, dungeonId);
                    break;
            }

            SetState(pguid, LFG_STATE_DUNGEON);
        }

        // Remove players/groups from Queue
        for (LfgGuidList::const_iterator it = proposal.queues.begin(); it != proposal.queues.end(); ++it)
            queue.RemoveFromQueue(*it);

        MakeNewGroup(proposal);
        ProposalsStore.erase(itProposal);
    }

    /**
       Remove a proposal from the pool, remove the group that didn't accept (if needed) and readd the other members to the queue

       @param[in]     itProposal Iterator to the proposal to remove
       @param[in]     type Type of removal (LFG_UPDATETYPE_PROPOSAL_FAILED, LFG_UPDATETYPE_PROPOSAL_DECLINED)
    */
    void LFGMgr::RemoveProposal(LfgProposalContainer::iterator itProposal, LfgUpdateType type)
    {
        LfgProposal& proposal = itProposal->second;
        proposal.state = LFG_PROPOSAL_FAILED;

        SF_LOG_DEBUG("lfg.proposal.remove", "Proposal %u, state FAILED, UpdateType %u", itProposal->first, type);
        // Mark all people that didn't answered as no accept
        if (type == LFG_UPDATETYPE_PROPOSAL_FAILED)
            for (LfgProposalPlayerContainer::iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
                if (it->second.accept == LFG_ANSWER_PENDING)
                    it->second.accept = LFG_ANSWER_DENY;

        // Mark players/groups to be removed
        LfgGuidSet toRemove;
        for (LfgProposalPlayerContainer::iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
        {
            if (it->second.accept == LFG_ANSWER_AGREE)
                continue;

            uint64 guid = it->second.group ? it->second.group : it->first;
            // Player didn't accept or still pending when no secs left
            if (it->second.accept == LFG_ANSWER_DENY || type == LFG_UPDATETYPE_PROPOSAL_FAILED)
            {
                it->second.accept = LFG_ANSWER_DENY;
                toRemove.insert(guid);
            }
        }

        // Notify players
        for (LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
        {
            uint64 guid = it->first;
            uint64 gguid = it->second.group ? it->second.group : guid;

            SendLfgUpdateProposal(guid, proposal);

            if (toRemove.find(gguid) != toRemove.end())         // Didn't accept or in same group that someone that didn't accept
            {
                LfgUpdateData updateData;
                if (it->second.accept == LFG_ANSWER_DENY)
                {
                    updateData.updateType = type;
                    SF_LOG_DEBUG("lfg.proposal.remove", "%u didn't accept. Removing from queue and compatible cache", GUID_LOPART(guid));
                }
                else
                {
                    updateData.updateType = LFG_UPDATETYPE_REMOVED_FROM_QUEUE;
                    SF_LOG_DEBUG("lfg.proposal.remove", "%u in same group that someone that didn't accept. Removing from queue and compatible cache", GUID_LOPART(guid));
                }

                RestoreState(guid, "Proposal Fail (didn't accepted or in group with someone that didn't accept");
                if (gguid != guid)
                {
                    RestoreState(it->second.group, "Proposal Fail (someone in group didn't accepted)");
                    SendLfgUpdateStatus(guid, updateData, true);
                }
                else
                    SendLfgUpdateStatus(guid, updateData, false);
            }
            else
            {
                SF_LOG_DEBUG("lfg.proposal.remove", "Readding %u to queue.", GUID_LOPART(guid));
                SetState(guid, LFG_STATE_QUEUED);
                if (gguid != guid)
                {
                    SetState(gguid, LFG_STATE_QUEUED);
                    SendLfgUpdateStatus(guid, LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, GetSelectedDungeons(guid), GetComment(guid)), true);
                }
                else
                    SendLfgUpdateStatus(guid, LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, GetSelectedDungeons(guid), GetComment(guid)), false);
            }
        }

        LFGQueue& queue = GetQueue(proposal.players.begin()->first);
        // Remove players/groups from queue
        for (LfgGuidSet::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it)
        {
            uint64 guid = *it;
            queue.RemoveFromQueue(guid);
            proposal.queues.remove(guid);
        }

        // Readd to queue
        for (LfgGuidList::const_iterator it = proposal.queues.begin(); it != proposal.queues.end(); ++it)
        {
            uint64 guid = *it;
            queue.AddToQueue(guid, true);
        }

        ProposalsStore.erase(itProposal);
    }

    /**
       Initialize a boot kick vote

       @param[in]     gguid Group the vote kicks belongs to
       @param[in]     kicker Kicker guid
       @param[in]     victim Victim guid
       @param[in]     reason Kick reason
    */
    void LFGMgr::InitBoot(uint64 gguid, uint64 kicker, uint64 victim, std::string const& reason)
    {
        SetVoteKick(gguid, true);

        LfgPlayerBoot& boot = BootsStore[gguid];
        boot.inProgress = true;
        boot.cancelTime = time_t(time(NULL)) + LFG_TIME_BOOT;
        boot.reason = reason;
        boot.victim = victim;

        LfgGuidSet const& players = GetPlayers(gguid);

        // Set votes
        for (LfgGuidSet::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            uint64 guid = (*itr);
            boot.votes[guid] = LFG_ANSWER_PENDING;
        }

        boot.votes[victim] = LFG_ANSWER_DENY;                  // Victim auto vote NO
        boot.votes[kicker] = LFG_ANSWER_AGREE;                 // Kicker auto vote YES

        // Notify players
        for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
            SendLfgBootProposalUpdate(*it, boot);
    }

    /**
       Update Boot info with player answer

       @param[in]     guid Player who has answered
       @param[in]     player answer
    */
    void LFGMgr::UpdateBoot(uint64 guid, bool accept)
    {
        uint64 gguid = GetGroup(guid);
        if (!gguid)
            return;

        LfgPlayerBootContainer::iterator itBoot = BootsStore.find(gguid);
        if (itBoot == BootsStore.end())
            return;

        LfgPlayerBoot& boot = itBoot->second;

        if (boot.votes[guid] != LFG_ANSWER_PENDING)    // Cheat check: Player can't vote twice
            return;

        boot.votes[guid] = LfgAnswer(accept);

        uint8 votesNum = 0;
        uint8 agreeNum = 0;
        for (LfgAnswerContainer::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
        {
            if (itVotes->second != LFG_ANSWER_PENDING)
            {
                ++votesNum;
                if (itVotes->second == LFG_ANSWER_AGREE)
                    ++agreeNum;
            }
        }

        // if we don't have enough votes (agree or deny) do nothing
        if (agreeNum < LFG_GROUP_KICK_VOTES_NEEDED && (votesNum - agreeNum) < LFG_GROUP_KICK_VOTES_NEEDED)
            return;

        // Send update info to all players
        boot.inProgress = false;
        for (LfgAnswerContainer::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
        {
            uint64 pguid = itVotes->first;
            if (pguid != boot.victim)
                SendLfgBootProposalUpdate(pguid, boot);
        }

        SetVoteKick(gguid, false);
        if (agreeNum == LFG_GROUP_KICK_VOTES_NEEDED)           // Vote passed - Kick player
        {
            if (Group* group = sGroupMgr->GetGroupByGUID(GUID_LOPART(gguid)))
                Player::RemoveFromGroup(group, boot.victim, GROUP_REMOVEMETHOD_KICK_LFG);
            DecreaseKicksLeft(gguid);
        }
        BootsStore.erase(itBoot);
    }

    /**
       Teleports the player in or out the dungeon

       @param[in]     player Player to teleport
       @param[in]     out Teleport out (true) or in (false)
       @param[in]     fromOpcode Function called from opcode handlers? (Default false)
    */
    void LFGMgr::TeleportPlayer(Player* player, bool out, bool fromOpcode /*= false*/)
    {
        LFGDungeonData const* dungeon = NULL;
        Group* group = player->GetGroup();

        if (group && group->isLFGGroup())
            dungeon = GetLFGDungeon(GetDungeon(group->GetGUID()));

        if (!dungeon)
        {
            SF_LOG_DEBUG("lfg.teleport", "Player %s not in group/lfggroup or dungeon not found!",
                player->GetName().c_str());
            player->GetSession()->SendLfgTeleportError(uint8(LFG_TELEPORTERROR_INVALID_LOCATION));
            return;
        }

        if (out)
        {
            SF_LOG_DEBUG("lfg.teleport", "Player %s is being teleported out. Current Map %u - Expected Map %u",
                player->GetName().c_str(), player->GetMapId(), uint32(dungeon->map));
            if (player->GetMapId() == uint32(dungeon->map))
                player->TeleportToBGEntryPoint();

            return;
        }

        LfgTeleportError error = LFG_TELEPORTERROR_OK;

        if (!player->IsAlive())
            error = LFG_TELEPORTERROR_PLAYER_DEAD;
        else if (player->IsFalling() || player->HasUnitState(UNIT_STATE_JUMPING))
            error = LFG_TELEPORTERROR_FALLING;
        else if (player->IsMirrorTimerActive(FATIGUE_TIMER))
            error = LFG_TELEPORTERROR_FATIGUE;
        else if (player->GetVehicle())
            error = LFG_TELEPORTERROR_IN_VEHICLE;
        else if (player->GetCharmGUID())
            error = LFG_TELEPORTERROR_CHARMING;
        else if (player->GetMapId() != uint32(dungeon->map))  // Do not teleport players in dungeon to the entrance
        {
            uint32 mapid = dungeon->map;
            float x = dungeon->x;
            float y = dungeon->y;
            float z = dungeon->z;
            float orientation = dungeon->o;

            if (!fromOpcode)
            {
                // Select a player inside to be teleported to
                for (GroupReference* itr = group->GetFirstMember(); itr != NULL && !mapid; itr = itr->next())
                {
                    Player* plrg = itr->GetSource();
                    if (plrg && plrg != player && plrg->GetMapId() == uint32(dungeon->map))
                    {
                        mapid = plrg->GetMapId();
                        x = plrg->GetPositionX();
                        y = plrg->GetPositionY();
                        z = plrg->GetPositionZ();
                        orientation = plrg->GetOrientation();
                        break;
                    }
                }
            }

            if (!player->GetMap()->IsInstance())
                player->SetBattlegroundEntryPoint();

            if (player->IsInFlight())
            {
                player->GetMotionMaster()->MovementExpired();
                player->CleanupAfterTaxiFlight();
            }

            if (!player->TeleportTo(mapid, x, y, z, orientation))
                error = LFG_TELEPORTERROR_INVALID_LOCATION;
        }
        else
            error = LFG_TELEPORTERROR_INVALID_LOCATION;

        if (error != LFG_TELEPORTERROR_OK)
            player->GetSession()->SendLfgTeleportError(uint8(error));

        SF_LOG_DEBUG("lfg.teleport", "Player %s is being teleported in to map %u "
            "(x: %f, y: %f, z: %f) Result: %u", player->GetName().c_str(), dungeon->map,
            dungeon->x, dungeon->y, dungeon->z, error);
    }

    /**
       Finish a dungeon and give reward, if any.

       @param[in]     guid Group guid
       @param[in]     dungeonId Dungeonid
    */
    void LFGMgr::FinishDungeon(uint64 gguid, const uint32 dungeonId)
    {
        uint32 gDungeonId = GetDungeon(gguid);
        if (gDungeonId != dungeonId)
        {
            SF_LOG_DEBUG("lfg.dungeon.finish", "Group %u finished dungeon %u but queued for %u", GUID_LOPART(gguid), dungeonId, gDungeonId);
            return;
        }

        if (GetState(gguid) == LFG_STATE_FINISHED_DUNGEON) // Shouldn't happen. Do not reward multiple times
        {
            SF_LOG_DEBUG("lfg.dungeon.finish", "Group: %u already rewarded", GUID_LOPART(gguid));
            return;
        }

        SetState(gguid, LFG_STATE_FINISHED_DUNGEON);

        const LfgGuidSet& players = GetPlayers(gguid);
        for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
        {
            uint64 guid = (*it);
            if (GetState(guid) == LFG_STATE_FINISHED_DUNGEON)
            {
                SF_LOG_DEBUG("lfg.dungeon.finish", "Group: %u, Player: %u already rewarded", GUID_LOPART(gguid), GUID_LOPART(guid));
                continue;
            }

            uint32 rDungeonId = 0;
            const LfgDungeonSet& dungeons = GetSelectedDungeons(guid);
            if (!dungeons.empty())
                rDungeonId = (*dungeons.begin());

            SetState(guid, LFG_STATE_FINISHED_DUNGEON);

            // Give rewards only if its a random dungeon
            LFGDungeonData const* dungeon = GetLFGDungeon(rDungeonId);

            if (!dungeon || (dungeon->type != LFG_TYPE_RANDOM && !dungeon->seasonal))
            {
                SF_LOG_DEBUG("lfg.dungeon.finish", "Group: %u, Player: %u dungeon %u is not random or seasonal", GUID_LOPART(gguid), GUID_LOPART(guid), rDungeonId);
                continue;
            }

            Player* player = ObjectAccessor::FindPlayer(guid);
            if (!player || !player->IsInWorld())
            {
                SF_LOG_DEBUG("lfg.dungeon.finish", "Group: %u, Player: %u not found in world", GUID_LOPART(gguid), GUID_LOPART(guid));
                continue;
            }

            LFGDungeonData const* dungeonDone = GetLFGDungeon(dungeonId);
            uint32 mapId = dungeonDone ? uint32(dungeonDone->map) : 0;

            if (player->GetMapId() != mapId)
            {
                SF_LOG_DEBUG("lfg.dungeon.finish", "Group: %u, Player: %u is in map %u and should be in %u to get reward", GUID_LOPART(gguid), GUID_LOPART(guid), player->GetMapId(), mapId);
                continue;
            }

            // Update achievements
            if (dungeon->difficulty == DIFFICULTY_HEROIC)
                player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS, 1);

            LfgReward const* reward = GetRandomDungeonReward(rDungeonId, player->getLevel());
            if (!reward)
                continue;

            bool done = false;
            Quest const* quest = sObjectMgr->GetQuestTemplate(reward->firstQuest);
            if (!quest)
                continue;

            // if we can take the quest, means that we haven't done this kind of "run", IE: First Heroic Random of Day.
            if (player->CanRewardQuest(quest, false))
                player->RewardQuest(quest, 0, NULL, false);
            else
            {
                done = true;
                quest = sObjectMgr->GetQuestTemplate(reward->otherQuest);
                if (!quest)
                    continue;
                // we give reward without informing client (retail does this)
                player->RewardQuest(quest, 0, NULL, false);
            }

            // Give rewards
            SF_LOG_DEBUG("lfg.dungeon.finish", "Group: %u, Player: %u done dungeon %u, %s previously done.", GUID_LOPART(gguid), GUID_LOPART(guid), GetDungeon(gguid), done ? " " : " not");
            LfgPlayerRewardData data = LfgPlayerRewardData(dungeon->Entry(), GetDungeon(gguid, false), done, quest);
            player->GetSession()->SendLfgPlayerReward(data);
        }
    }

    // --------------------------------------------------------------------------//
    // Auxiliar Functions
    // --------------------------------------------------------------------------//

    /**
       Get the dungeon list that can be done given a random dungeon entry.

       @param[in]     randomdungeon Random dungeon id (if value = 0 will return all dungeons)
       @returns Set of dungeons that can be done.
    */
    LfgDungeonSet const& LFGMgr::GetDungeonsByRandom(uint32 randomdungeon)
    {
        LFGDungeonData const* dungeon = GetLFGDungeon(randomdungeon);
        uint32 group = dungeon ? dungeon->group : 0;
        return CachedDungeonMapStore[group];
    }

    /**
       Get the reward of a given random dungeon at a certain level

       @param[in]     dungeon dungeon id
       @param[in]     level Player level
       @returns Reward
    */
    LfgReward const* LFGMgr::GetRandomDungeonReward(uint32 dungeon, uint8 level)
    {
        LfgReward const* rew = NULL;
        LfgRewardContainerBounds bounds = RewardMapStore.equal_range(dungeon & 0x00FFFFFF);
        for (LfgRewardContainer::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
        {
            rew = itr->second;
            // ordered properly at loading
            if (itr->second->maxLevel >= level)
                break;
        }

        return rew;
    }

    /**
       Given a Dungeon id returns the dungeon Type

       @param[in]     dungeon dungeon id
       @returns Dungeon type
    */
    LfgType LFGMgr::GetDungeonType(uint32 dungeonId)
    {
        LFGDungeonData const* dungeon = GetLFGDungeon(dungeonId);
        if (!dungeon)
            return LFG_TYPE_NONE;

        return LfgType(dungeon->type);
    }

    LfgState LFGMgr::GetState(uint64 guid)
    {
        LfgState state;
        if (IS_GROUP_GUID(guid))
        {
            state = GroupsStore[guid].GetState();
            SF_LOG_TRACE("lfg.data.group.state.get", "Group: %u, State: %u", GUID_LOPART(guid), state);
        }
        else
        {
            state = PlayersStore[guid].GetState();
            SF_LOG_TRACE("lfg.data.player.state.get", "Player: %u, State: %u", GUID_LOPART(guid), state);
        }
        return state;
    }

    LfgState LFGMgr::GetOldState(uint64 guid)
    {
        LfgState state;
        if (IS_GROUP_GUID(guid))
        {
            state = GroupsStore[guid].GetOldState();
            SF_LOG_TRACE("lfg.data.group.oldstate.get", "Group: %u, Old state: %u", GUID_LOPART(guid), state);
        }
        else
        {
            state = PlayersStore[guid].GetOldState();
            SF_LOG_TRACE("lfg.data.player.oldstate.get", "Player: %u, Old state: %u", GUID_LOPART(guid), state);
        }
        return state;
    }

    uint32 LFGMgr::GetDungeon(uint64 guid, bool asId /*= true */)
    {
        uint32 dungeon = GroupsStore[guid].GetDungeon(asId);
        SF_LOG_TRACE("lfg.data.group.dungeon.get", "Group: %u, asId: %u, Dungeon: %u", GUID_LOPART(guid), asId, dungeon);
        return dungeon;
    }

    uint32 LFGMgr::GetDungeonMapId(uint64 guid)
    {
        uint32 dungeonId = GroupsStore[guid].GetDungeon(true);
        uint32 mapId = 0;
        if (dungeonId)
            if (LFGDungeonData const* dungeon = GetLFGDungeon(dungeonId))
                mapId = dungeon->map;

        SF_LOG_TRACE("lfg.data.group.dungeon.map", "Group: %u, MapId: %u (DungeonId: %u)", GUID_LOPART(guid), mapId, dungeonId);
        return mapId;
    }

    uint8 LFGMgr::GetRoles(uint64 guid)
    {
        uint8 roles = PlayersStore[guid].GetRoles();
        SF_LOG_TRACE("lfg.data.player.role.get", "Player: %u, Role: %u", GUID_LOPART(guid), roles);
        return roles;
    }

    bool LFGMgr::IsVoteKickActive(uint64 guid)
    {
        bool active = GroupsStore[guid].IsVoteKickActive();
        SF_LOG_TRACE("lfg.data.group.votekick.get", "Group: %u, Active: %d", GUID_LOPART(guid), active);
        return active;
    }

    void LFGMgr::SetVoteKick(uint64 guid, bool active)
    {
        LfgGroupData& data = GroupsStore[guid];
        SF_LOG_TRACE("lfg.data.group.votekick.set", "Group: %u, New state: %d, Previous: %d", GUID_LOPART(guid), active, data.IsVoteKickActive());
        data.SetVoteKick(active);
    }

    const std::string& LFGMgr::GetComment(uint64 guid)
    {
        SF_LOG_TRACE("lfg.data.player.comment.get", "Player: %u, Comment: %s", GUID_LOPART(guid), PlayersStore[guid].GetComment().c_str());
        return PlayersStore[guid].GetComment();
    }

    LfgDungeonSet const& LFGMgr::GetSelectedDungeons(uint64 guid)
    {
        SF_LOG_TRACE("lfg.data.player.dungeons.selected.get", "Player: %u, Selected Dungeons: %s", GUID_LOPART(guid), ConcatenateDungeons(PlayersStore[guid].GetSelectedDungeons()).c_str());
        return PlayersStore[guid].GetSelectedDungeons();
    }

    LfgLockMap const LFGMgr::GetLockedDungeons(uint64 guid)
    {
        SF_LOG_TRACE("lfg.data.player.dungeons.locked.get", "Player: %u, LockedDungeons.", GUID_LOPART(guid));
        LfgLockMap lock;
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player)
        {
            SF_LOG_WARN("lfg.data.player.dungeons.locked.get", "Player: %u not ingame while retrieving his LockedDungeons.", GUID_LOPART(guid));
            return lock;
        }
        uint8 level = player->getLevel();
        uint8 expansion = player->GetSession()->Expansion();
        LfgDungeonSet const& dungeons = GetDungeonsByRandom(0);
        bool denyJoin = !player->GetSession()->HasPermission(rbac::RBAC_PERM_JOIN_DUNGEON_FINDER);
        for (LfgDungeonSet::const_iterator it = dungeons.begin(); it != dungeons.end(); ++it)
        {
            LFGDungeonData const* dungeon = GetLFGDungeon(*it);
            if (!dungeon) // should never happen - We provide a list from sLFGDungeonStore
                continue;
            uint32 lockData = 0;
            if (denyJoin)
                lockData = LFG_LOCKSTATUS_RAID_LOCKED;
            else if (dungeon->expansion > expansion)
                lockData = LFG_LOCKSTATUS_INSUFFICIENT_EXPANSION;
            else if (DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP, dungeon->map, player))
                lockData = LFG_LOCKSTATUS_RAID_LOCKED;
            else if (dungeon->difficulty > DIFFICULTY_NORMAL && player->GetBoundInstance(dungeon->map, DifficultyID(dungeon->difficulty)))
                lockData = LFG_LOCKSTATUS_RAID_LOCKED;
            else if (dungeon->minlevel > level)
                lockData = LFG_LOCKSTATUS_TOO_LOW_LEVEL;
            else if (dungeon->maxlevel < level)
                lockData = LFG_LOCKSTATUS_TOO_HIGH_LEVEL;
            else if (dungeon->seasonal && !IsSeasonActive(dungeon->id))
                lockData = LFG_LOCKSTATUS_NOT_IN_SEASON;
            else if (AccessRequirement const* ar = sObjectMgr->GetAccessRequirement(dungeon->map, DifficultyID(dungeon->difficulty)))
            {
                if (ar->iLvl && player->GetAverageItemLevel() < ar->iLvl)
                    lockData = LFG_LOCKSTATUS_TOO_LOW_GEAR_SCORE;
                else if (ar->achievement && !player->HasAchieved(ar->achievement))
                    lockData = LFG_LOCKSTATUS_MISSING_ACHIEVEMENT;
                else if (player->GetTeam() == ALLIANCE && ar->quest_A && !player->GetQuestRewardStatus(ar->quest_A))
                    lockData = LFG_LOCKSTATUS_QUEST_NOT_COMPLETED;
                else if (player->GetTeam() == HORDE && ar->quest_H && !player->GetQuestRewardStatus(ar->quest_H))
                    lockData = LFG_LOCKSTATUS_QUEST_NOT_COMPLETED;
                else
                    if (ar->item)
                    {
                        if (!player->HasItemCount(ar->item) && (!ar->item2 || !player->HasItemCount(ar->item2)))
                            lockData = LFG_LOCKSTATUS_MISSING_ITEM;
                    }
                    else if (ar->item2 && !player->HasItemCount(ar->item2))
                        lockData = LFG_LOCKSTATUS_MISSING_ITEM;
            }
            /* @todo VoA closed if WG is not under team control (LFG_LOCKSTATUS_RAID_LOCKED)
            lockData = LFG_LOCKSTATUS_TOO_HIGH_GEAR_SCORE;
            lockData = LFG_LOCKSTATUS_ATTUNEMENT_TOO_LOW_LEVEL;
            lockData = LFG_LOCKSTATUS_ATTUNEMENT_TOO_HIGH_LEVEL;
            */
            if (lockData)
                lock[dungeon->Entry()] = lockData;
        }
        return lock;
    }

    uint8 LFGMgr::GetKicksLeft(uint64 guid)
    {
        uint8 kicks = GroupsStore[guid].GetKicksLeft();
        SF_LOG_TRACE("lfg.data.group.kickleft.get", "Group: %u, Kicks left: %u", GUID_LOPART(guid), kicks);
        return kicks;
    }

    void LFGMgr::RestoreState(uint64 guid, char const* debugMsg)
    {
        if (IS_GROUP_GUID(guid))
        {
            LfgGroupData& data = GroupsStore[guid];
            SF_LOG_TRACE("lfg.data.group.state.restore", "Group: %u (%s), State: %s, Old state: %s",
                GUID_LOPART(guid), debugMsg, GetStateString(data.GetState()).c_str(),
                GetStateString(data.GetOldState()).c_str());

            data.RestoreState();
        }
        else
        {
            LfgPlayerData& data = PlayersStore[guid];
            SF_LOG_TRACE("lfg.data.player.state.restore", "Player: %u (%s), State: %s, Old state: %s",
                GUID_LOPART(guid), debugMsg, GetStateString(data.GetState()).c_str(),
                GetStateString(data.GetOldState()).c_str());
            data.RestoreState();
        }
    }

    void LFGMgr::SetState(uint64 guid, LfgState state)
    {
        if (IS_GROUP_GUID(guid))
        {
            LfgGroupData& data = GroupsStore[guid];
            SF_LOG_TRACE("lfg.data.group.state.set", "Group: %u, New state: %s, Previous: %s, Old state: %s",
                GUID_LOPART(guid), GetStateString(state).c_str(), GetStateString(data.GetState()).c_str(),
                GetStateString(data.GetOldState()).c_str());
            data.SetState(state);
        }
        else
        {
            LfgPlayerData& data = PlayersStore[guid];
            SF_LOG_TRACE("lfg.data.player.state.set", "Player: %u, New state: %s, Previous: %s, OldState: %s",
                GUID_LOPART(guid), GetStateString(state).c_str(), GetStateString(data.GetState()).c_str(),
                GetStateString(data.GetOldState()).c_str());
            data.SetState(state);
        }
    }

    void LFGMgr::SetDungeon(uint64 guid, uint32 dungeon)
    {
        SF_LOG_TRACE("lfg.data.group.dungeon.set", "Group: %u, Dungeon: %u", GUID_LOPART(guid), dungeon);
        GroupsStore[guid].SetDungeon(dungeon);
    }

    void LFGMgr::SetRoles(uint64 guid, uint8 roles)
    {
        SF_LOG_TRACE("lfg.data.player.role.set", "Player: %u, Roles: %u", GUID_LOPART(guid), roles);
        PlayersStore[guid].SetRoles(roles);
    }

    void LFGMgr::SetComment(uint64 guid, std::string const& comment)
    {
        SF_LOG_TRACE("lfg.data.player.comment.set", "Player: %u, Comment: %s", GUID_LOPART(guid), comment.c_str());
        PlayersStore[guid].SetComment(comment);
    }

    void LFGMgr::SetSelectedDungeons(uint64 guid, LfgDungeonSet const& dungeons)
    {
        SF_LOG_TRACE("lfg.data.player.dungeon.selected.set", "Player: %u, Dungeons: %s", GUID_LOPART(guid), ConcatenateDungeons(dungeons).c_str());
        PlayersStore[guid].SetSelectedDungeons(dungeons);
    }

    void LFGMgr::DecreaseKicksLeft(uint64 guid)
    {
        GroupsStore[guid].DecreaseKicksLeft();
        SF_LOG_TRACE("lfg.data.group.kicksleft.decrease", "Group: %u, Kicks: %u", GUID_LOPART(guid), GroupsStore[guid].GetKicksLeft());
    }

    void LFGMgr::RemovePlayerData(uint64 guid)
    {
        SF_LOG_TRACE("lfg.data.player.remove", "Player: %u", GUID_LOPART(guid));
        LfgPlayerDataContainer::iterator it = PlayersStore.find(guid);
        if (it != PlayersStore.end())
            PlayersStore.erase(it);
    }

    void LFGMgr::RemoveGroupData(uint64 guid)
    {
        SF_LOG_TRACE("lfg.data.group.remove", "Group: %u", GUID_LOPART(guid));
        LfgGroupDataContainer::iterator it = GroupsStore.find(guid);
        if (it == GroupsStore.end())
            return;

        LfgState state = GetState(guid);
        // If group is being formed after proposal success do nothing more
        LfgGuidSet const& players = it->second.GetPlayers();
        for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
        {
            uint64 guid = (*it);
            SetGroup(*it, 0);
            if (state != LFG_STATE_PROPOSAL)
            {
                SetState(*it, LFG_STATE_NONE);
                SendLfgUpdateStatus(guid, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE), true);
            }
        }
        GroupsStore.erase(it);
    }

    uint8 LFGMgr::GetTeam(uint64 guid)
    {
        uint8 team = PlayersStore[guid].GetTeam();
        SF_LOG_TRACE("lfg.data.player.team.get", "Player: %u, Team: %u", GUID_LOPART(guid), team);
        return team;
    }

    uint8 LFGMgr::RemovePlayerFromGroup(uint64 gguid, uint64 guid)
    {
        return GroupsStore[gguid].RemovePlayer(guid);
    }

    void LFGMgr::AddPlayerToGroup(uint64 gguid, uint64 guid)
    {
        GroupsStore[gguid].AddPlayer(guid);
    }

    void LFGMgr::SetLeader(uint64 gguid, uint64 leader)
    {
        GroupsStore[gguid].SetLeader(leader);
    }

    void LFGMgr::SetTeam(uint64 guid, uint8 team)
    {
        if (sWorld->GetBoolConfig(WorldBoolConfigs::CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP))
            team = 0;

        PlayersStore[guid].SetTeam(team);
    }

    uint64 LFGMgr::GetGroup(uint64 guid)
    {
        return PlayersStore[guid].GetGroup();
    }

    void LFGMgr::SetGroup(uint64 guid, uint64 group)
    {
        PlayersStore[guid].SetGroup(group);
    }

    LfgGuidSet const& LFGMgr::GetPlayers(uint64 guid)
    {
        return GroupsStore[guid].GetPlayers();
    }

    uint8 LFGMgr::GetPlayerCount(uint64 guid)
    {
        return GroupsStore[guid].GetPlayerCount();
    }

    uint64 LFGMgr::GetLeader(uint64 guid)
    {
        return GroupsStore[guid].GetLeader();
    }

    bool LFGMgr::HasIgnore(uint64 guid1, uint64 guid2)
    {
        Player* plr1 = ObjectAccessor::FindPlayer(guid1);
        Player* plr2 = ObjectAccessor::FindPlayer(guid2);
        uint32 low1 = GUID_LOPART(guid1);
        uint32 low2 = GUID_LOPART(guid2);
        return plr1 && plr2 && (plr1->GetSocial()->HasIgnore(low2) || plr2->GetSocial()->HasIgnore(low1));
    }

    void LFGMgr::SendLfgRoleChosen(uint64 guid, uint64 pguid, uint8 roles)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            player->GetSession()->SendLfgRoleChosen(pguid, roles);
    }

    void LFGMgr::SendLfgRoleCheckUpdate(uint64 guid, LfgRoleCheck const& roleCheck)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            player->GetSession()->SendLfgRoleCheckUpdate(roleCheck);
    }

    void LFGMgr::SendLfgUpdateStatus(uint64 guid, LfgUpdateData const& data, bool party)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            player->GetSession()->SendLfgUpdateStatus(data, party);
    }

    void LFGMgr::SendLfgJoinResult(uint64 guid, LfgJoinResultData const& data)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            player->GetSession()->SendLfgJoinResult(data);
    }

    void LFGMgr::SendLfgBootProposalUpdate(uint64 guid, LfgPlayerBoot const& boot)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            player->GetSession()->SendLfgBootProposalUpdate(boot);
    }

    void LFGMgr::SendLfgUpdateProposal(uint64 guid, LfgProposal const& proposal)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            player->GetSession()->SendLfgUpdateProposal(proposal);
    }

    void LFGMgr::SendLfgQueueStatus(uint64 guid, LfgQueueStatusData const& data)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            player->GetSession()->SendLfgQueueStatus(data);
    }

    bool LFGMgr::IsLfgGroup(uint64 guid)
    {
        return guid && IS_GROUP_GUID(guid) && GroupsStore[guid].IsLfgGroup();
    }

    uint8 LFGMgr::GetQueueId(uint64 guid)
    {
        if (IS_GROUP_GUID(guid))
        {
            LfgGuidSet const& players = GetPlayers(guid);
            uint64 pguid = players.empty() ? 0 : (*players.begin());
            if (pguid)
                return GetTeam(pguid);
        }

        return GetTeam(guid);
    }

    LFGQueue& LFGMgr::GetQueue(uint64 guid)
    {
        uint8 queueId = GetQueueId(guid);
        return QueuesStore[queueId];
    }

    bool LFGMgr::AllQueued(LfgGuidList const& check)
    {
        if (check.empty())
            return false;

        for (LfgGuidList::const_iterator it = check.begin(); it != check.end(); ++it)
        {
            LfgState state = GetState(*it);
            if (state != LFG_STATE_QUEUED)
            {
                if (state != LFG_STATE_PROPOSAL)
                    SF_LOG_DEBUG("lfg.allqueued", "Unexpected state found while trying to form new group. Guid: %u, State: %s", GUID_LOPART((*it)), GetStateString(state).c_str());
                return false;
            }
        }
        return true;
    }

    time_t LFGMgr::GetQueueJoinTime(uint64 guid)
    {
        uint8 queueId = GetQueueId(guid);
        LfgQueueContainer::const_iterator itr = QueuesStore.find(queueId);
        if (itr != QueuesStore.end())
            return itr->second.GetJoinTime(guid);

        return 0;
    }

    // Only for debugging purposes
    void LFGMgr::Clean()
    {
        QueuesStore.clear();
    }

    bool LFGMgr::isOptionEnabled(uint32 option)
    {
        return m_options & option;
    }

    uint32 LFGMgr::GetOptions()
    {
        return m_options;
    }

    void LFGMgr::SetOptions(uint32 options)
    {
        m_options = options;
    }

    LfgUpdateData LFGMgr::GetLfgStatus(uint64 guid)
    {
        LfgPlayerData& playerData = PlayersStore[guid];
        return LfgUpdateData(LFG_UPDATETYPE_UPDATE_STATUS, playerData.GetState(), playerData.GetSelectedDungeons());
    }

    bool LFGMgr::IsSeasonActive(uint32 dungeonId)
    {
        switch (dungeonId)
        {
            case 285: // The Headless Horseman
                return IsHolidayActive(HolidayIds::HOLIDAY_HALLOWS_END);
            case 286: // The Frost Lord Ahune
                return IsHolidayActive(HolidayIds::HOLIDAY_FIRE_FESTIVAL);
            case 287: // Coren Direbrew
                return IsHolidayActive(HolidayIds::HOLIDAY_BREWFEST);
            case 288: // The Crown Chemical Co.
                return IsHolidayActive(HolidayIds::HOLIDAY_LOVE_IS_IN_THE_AIR);
        }
        return false;
    }

    std::string LFGMgr::DumpQueueInfo(bool full)
    {
        uint32 size = uint32(QueuesStore.size());
        std::ostringstream o;

        o << "Number of Queues: " << size << "\n";
        for (LfgQueueContainer::const_iterator itr = QueuesStore.begin(); itr != QueuesStore.end(); ++itr)
        {
            std::string const& queued = itr->second.DumpQueueInfo();
            std::string const& compatibles = itr->second.DumpCompatibleInfo(full);
            o << queued << compatibles;
        }

        return o.str();
    }

    void LFGMgr::SetupGroupMember(uint64 guid, uint64 gguid)
    {
        LfgDungeonSet dungeons;
        dungeons.insert(GetDungeon(gguid));
        SetSelectedDungeons(guid, dungeons);
        SetState(guid, GetState(gguid));
        SetGroup(guid, gguid);
        AddPlayerToGroup(gguid, guid);
    }

    bool LFGMgr::selectedRandomLfgDungeon(uint64 guid)
    {
        if (GetState(guid) != LFG_STATE_NONE)
        {
            LfgDungeonSet const& dungeons = GetSelectedDungeons(guid);
            if (!dungeons.empty())
            {
                LFGDungeonData const* dungeon = GetLFGDungeon(*dungeons.begin());
                if (dungeon && (dungeon->type == LFG_TYPE_RANDOM || dungeon->seasonal))
                    return true;
            }
        }

        return false;
    }

    bool LFGMgr::inLfgDungeonMap(uint64 guid, uint32 map, DifficultyID difficulty)
    {
        if (!IS_GROUP_GUID(guid))
            guid = GetGroup(guid);

        if (uint32 dungeonId = GetDungeon(guid, true))
            if (LFGDungeonData const* dungeon = GetLFGDungeon(dungeonId))
                if (uint32(dungeon->map) == map && dungeon->difficulty == difficulty)
                    return true;

        return false;
    }

    uint32 LFGMgr::GetLFGDungeonEntry(uint32 id)
    {
        if (id)
            if (LFGDungeonData const* dungeon = GetLFGDungeon(id))
                return dungeon->Entry();

        return 0;
    }

    LfgDungeonSet LFGMgr::GetRandomAndSeasonalDungeons(uint8 level, uint8 expansion)
    {
        LfgDungeonSet randomDungeons;
        for (lfg::LFGDungeonContainer::const_iterator itr = LfgDungeonStore.begin(); itr != LfgDungeonStore.end(); ++itr)
        {
            lfg::LFGDungeonData const& dungeon = itr->second;
            if ((dungeon.type == lfg::LFG_TYPE_RANDOM || (dungeon.seasonal && sLFGMgr->IsSeasonActive(dungeon.id)))
                && dungeon.expansion <= expansion && dungeon.minlevel <= level && level <= dungeon.maxlevel)
                randomDungeons.insert(dungeon.Entry());
        }
        return randomDungeons;
    }

} // namespace lfg
