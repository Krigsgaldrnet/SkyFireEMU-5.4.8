/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "BattlegroundTP.h"
#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

enum BG_TP_Rewards
{
    BG_TP_WIN = 0,
    BG_TP_FLAG_CAP,
    BG_TP_MAP_COMPLETE,
    BG_TP_REWARD_NUM
};

uint32 BG_TP_Honor[BG_HONOR_MODE_NUM][BG_TP_REWARD_NUM] =
{
    {20, 40, 40}, // normal honor
    {60, 40, 80}  // holiday
};

uint32 BG_TP_Reputation[BG_HONOR_MODE_NUM][BG_TP_REWARD_NUM] =
{
    {0, 35, 0}, // normal honor
    {0, 45, 0}  // holiday
};

BattlegroundTP::BattlegroundTP()
{
    BgObjects.resize(BG_TP_OBJECT_MAX);
    BgCreatures.resize(BG_CREATURES_MAX_TP);


    StartMessageIds[BG_STARTING_EVENT_FIRST] = LANG_BG_TP_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_TP_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD] = LANG_BG_TP_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_TP_HAS_BEGUN;
    _flagSpellForceTimer = 0;
    _bothFlagsKept = false;
    _flagDebuffState = 0;
}


BattlegroundTP::~BattlegroundTP()
{
}


void BattlegroundTP::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        if (GetElapsedTime() >= 30 * MINUTE * IN_MILLISECONDS)
        {
            if (GetTeamScore(TEAM_ALLIANCE) == 0)
            {
                if (GetTeamScore(TEAM_HORDE) == 0)        // No one scored - result is tie
                    EndBattleground(WINNER_NONE);
                else                                 // Horde has more points and thus wins
                    EndBattleground(HORDE);
            }
            else if (GetTeamScore(TEAM_HORDE) == 0)
                EndBattleground(ALLIANCE);           // Alliance has > 0, Horde has 0, alliance wins
            else if (GetTeamScore(TEAM_HORDE) == GetTeamScore(TEAM_ALLIANCE)) // Team score equal, winner is team that scored the last flag
                EndBattleground(_lastFlagCaptureTeam);
            else if (GetTeamScore(TEAM_HORDE) > GetTeamScore(TEAM_ALLIANCE))  // Last but not least, check who has the higher score
                EndBattleground(HORDE);
            else
                EndBattleground(ALLIANCE);
        }
        // first update needed after 1 minute of game already in progress
        else if (GetElapsedTime() > uint32(_minutesElapsed * MINUTE * IN_MILLISECONDS) + 3 * MINUTE * IN_MILLISECONDS)
        {
            ++_minutesElapsed;
            UpdateWorldState(BG_TP_STATE_TIMER, 28 - _minutesElapsed);
        }


        if (_flagState[TEAM_ALLIANCE] == BG_TP_FLAG_STATE_WAIT_RESPAWN)
        {
            _flagsTimer[TEAM_ALLIANCE] -= diff;


            if (_flagsTimer[TEAM_ALLIANCE] < 0)
            {
                _flagsTimer[TEAM_ALLIANCE] = 0;
                RespawnFlag(ALLIANCE, true);
            }
        }


        if (_flagState[TEAM_ALLIANCE] == BG_TP_FLAG_STATE_ON_GROUND)
        {
            _flagsDropTimer[TEAM_ALLIANCE] -= diff;


            if (_flagsDropTimer[TEAM_ALLIANCE] < 0)
            {
                _flagsDropTimer[TEAM_ALLIANCE] = 0;
                RespawnFlagAfterDrop(ALLIANCE);
                _bothFlagsKept = false;
            }
        }


        if (_flagState[TEAM_HORDE] == BG_TP_FLAG_STATE_WAIT_RESPAWN)
        {
            _flagsTimer[TEAM_HORDE] -= diff;


            if (_flagsTimer[TEAM_HORDE] < 0)
            {
                _flagsTimer[TEAM_HORDE] = 0;
                RespawnFlag(HORDE, true);
            }
        }


        if (_flagState[TEAM_HORDE] == BG_TP_FLAG_STATE_ON_GROUND)
        {
            _flagsDropTimer[TEAM_HORDE] -= diff;


            if (_flagsDropTimer[TEAM_HORDE] < 0)
            {
                _flagsDropTimer[TEAM_HORDE] = 0;
                RespawnFlagAfterDrop(HORDE);
                _bothFlagsKept = false;
            }
        }


        if (_bothFlagsKept)
        {
            _flagSpellForceTimer += diff;
            if (_flagDebuffState == 0 && _flagSpellForceTimer >= 10 * MINUTE * IN_MILLISECONDS)  //10 minutes
            {
                if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[0]))
                    player->CastSpell(player, TP_SPELL_FOCUSED_ASSAULT, true);
                if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[1]))
                    player->CastSpell(player, TP_SPELL_FOCUSED_ASSAULT, true);
                _flagDebuffState = 1;
            }
            else if (_flagDebuffState == 1 && _flagSpellForceTimer >= 900000) // 15 minutes
            {
                if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[0]))
                {
                    player->RemoveAurasDueToSpell(TP_SPELL_FOCUSED_ASSAULT);
                    player->CastSpell(player, TP_SPELL_BRUTAL_ASSAULT, true);
                }
                if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[1]))
                {
                    player->RemoveAurasDueToSpell(TP_SPELL_FOCUSED_ASSAULT);
                    player->CastSpell(player, TP_SPELL_BRUTAL_ASSAULT, true);
                }
                _flagDebuffState = 2;
            }
        }
        else
        {
            if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[0]))
            {
                player->RemoveAurasDueToSpell(TP_SPELL_FOCUSED_ASSAULT);
                player->RemoveAurasDueToSpell(TP_SPELL_BRUTAL_ASSAULT);
            }
            if (Player* player = ObjectAccessor::FindPlayer(m_FlagKeepers[1]))
            {
                player->RemoveAurasDueToSpell(TP_SPELL_FOCUSED_ASSAULT);
                player->RemoveAurasDueToSpell(TP_SPELL_BRUTAL_ASSAULT);
            }


            _flagSpellForceTimer = 0; //reset timer.
            _flagDebuffState = 0;
        }
    }
}


void BattlegroundTP::StartingEventCloseDoors()
{
    for (uint32 i = BG_TP_OBJECT_DOOR_A_1; i <= BG_TP_OBJECT_DOOR_H_4; ++i)
    {
        DoorClose(i);
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
    }
    for (uint32 i = BG_TP_OBJECT_A_FLAG; i <= BG_TP_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_ONE_DAY);


    UpdateWorldState(BG_TP_STATE_TIMER_ACTIVE, 1);
    UpdateWorldState(BG_TP_STATE_TIMER, 25);
}


void BattlegroundTP::StartingEventOpenDoors()
{
    DoorOpen(BG_TP_OBJECT_DOOR_A_1);
    DoorOpen(BG_TP_OBJECT_DOOR_A_2);
    DoorOpen(BG_TP_OBJECT_DOOR_A_3);
    DoorOpen(BG_TP_OBJECT_DOOR_A_4);
    DoorOpen(BG_TP_OBJECT_DOOR_H_1);
    DoorOpen(BG_TP_OBJECT_DOOR_H_2);
    DoorOpen(BG_TP_OBJECT_DOOR_H_3);
    DoorOpen(BG_TP_OBJECT_DOOR_H_4);


    for (uint32 i = BG_TP_OBJECT_A_FLAG; i <= BG_TP_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);


    // players joining later are not eligible
    //StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, TP_EVENT_START_BATTLE);
}


void BattlegroundTP::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    //create score and add it to map, default values are set in constructor
    BattlegroundTPScore* sc = new BattlegroundTPScore;


    PlayerScores[player->GetGUID()] = sc;
}


void BattlegroundTP::RespawnFlag(uint32 Team, bool captured)
{
    if (Team == ALLIANCE)
    {
        SF_LOG_DEBUG("bg.battleground", "Respawn Alliance flag");
        _flagState[TEAM_ALLIANCE] = BG_TP_FLAG_STATE_ON_BASE;
    }
    else
    {
        SF_LOG_DEBUG("bg.battleground", "Respawn Horde flag");
        _flagState[TEAM_HORDE] = BG_TP_FLAG_STATE_ON_BASE;
    }


    if (captured)
    {
        //when map_update will be allowed for battlegrounds this code will be useless
        SpawnBGObject(BG_TP_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SpawnBGObject(BG_TP_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_TP_F_PLACED, ChatMsg::CHAT_MSG_BG_SYSTEM_NEUTRAL);
        PlaySoundToAll(BG_TP_SOUND_FLAGS_RESPAWNED);        // flag respawned sound...
    }
    _bothFlagsKept = false;
}


void BattlegroundTP::RespawnFlagAfterDrop(uint32 team)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;


    RespawnFlag(team, false);
    if (team == ALLIANCE)
    {
        SpawnBGObject(BG_TP_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_TP_ALLIANCE_FLAG_RESPAWNED, ChatMsg::CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
    else
    {
        SpawnBGObject(BG_TP_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_TP_HORDE_FLAG_RESPAWNED, ChatMsg::CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }


    PlaySoundToAll(BG_TP_SOUND_FLAGS_RESPAWNED);


    if (GameObject* obj = GetBgMap()->GetGameObject(GetDroppedFlagGUID(team)))
        obj->Delete();
    else
        SF_LOG_ERROR("bg.battleground", "unknown droped flag bg, guid: %u", GUID_LOPART(GetDroppedFlagGUID(team)));


    SetDroppedFlagGUID(0, GetTeamIndexByTeamId(team));
    _bothFlagsKept = false;
}


void BattlegroundTP::EventPlayerCapturedFlag(Player* Source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;


    uint32 winner = 0;


    Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    if (Source->GetTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;
        SetHordeFlagPicker(0);                              // must be before aura remove to prevent 2 events (drop+capture) at the same time
        // horde flag in base (but not respawned yet)
        _flagState[TEAM_HORDE] = BG_TP_FLAG_STATE_WAIT_RESPAWN;
        // Drop Horde Flag from Player
        Source->RemoveAurasDueToSpell(BG_TP_SPELL_HORDE_FLAG);
        if (_flagDebuffState == 1)
            Source->RemoveAurasDueToSpell(TP_SPELL_FOCUSED_ASSAULT);
        if (_flagDebuffState == 2)
            Source->RemoveAurasDueToSpell(TP_SPELL_BRUTAL_ASSAULT);
        if (GetTeamScore(TEAM_ALLIANCE) < BG_TP_MAX_TEAM_SCORE)
            AddPoint(ALLIANCE, 1);
        PlaySoundToAll(BG_TP_SOUND_FLAG_CAPTURED_ALLIANCE);
        RewardReputationToTeam(1174, m_ReputationCapture, ALLIANCE);
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;
        SetAllianceFlagPicker(0);                           // must be before aura remove to prevent 2 events (drop+capture) at the same time
        // alliance flag in base (but not respawned yet)
        _flagState[TEAM_ALLIANCE] = BG_TP_FLAG_STATE_WAIT_RESPAWN;
        // Drop Alliance Flag from Player
        Source->RemoveAurasDueToSpell(BG_TP_SPELL_ALLIANCE_FLAG);
        if (_flagDebuffState == 1)
            Source->RemoveAurasDueToSpell(TP_SPELL_FOCUSED_ASSAULT);
        if (_flagDebuffState == 2)
            Source->RemoveAurasDueToSpell(TP_SPELL_BRUTAL_ASSAULT);
        if (GetTeamScore(TEAM_HORDE) < BG_TP_MAX_TEAM_SCORE)
            AddPoint(HORDE, 1);
        PlaySoundToAll(BG_TP_SOUND_FLAG_CAPTURED_HORDE);
        RewardReputationToTeam(1172, m_ReputationCapture, HORDE);
    }
    //for flag capture is reward 2 honorable kills
    RewardHonorToTeam(GetBonusHonorFromKill(2), Source->GetTeam());


    SpawnBGObject(BG_TP_OBJECT_H_FLAG, BG_TP_FLAG_RESPAWN_TIME);
    SpawnBGObject(BG_TP_OBJECT_A_FLAG, BG_TP_FLAG_RESPAWN_TIME);


    if (Source->GetTeam() == ALLIANCE)
        SendMessageToAll(LANG_BG_TP_CAPTURED_HF, ChatMsg::CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
    else
        SendMessageToAll(LANG_BG_TP_CAPTURED_AF, ChatMsg::CHAT_MSG_BG_SYSTEM_HORDE, Source);


    UpdateFlagState(Source->GetTeam(), 1);                  // flag state none
    UpdateTeamScore(Source->GetTeamId());
    // only flag capture should be updated
    UpdatePlayerScore(Source, SCORE_FLAG_CAPTURES, 1);      // +1 flag captures


    // update last flag capture to be used if teamscore is equal
    SetLastFlagCapture(Source->GetTeam());


    if (GetTeamScore(TEAM_ALLIANCE) == BG_TP_MAX_TEAM_SCORE)
        winner = ALLIANCE;


    if (GetTeamScore(TEAM_HORDE) == BG_TP_MAX_TEAM_SCORE)
        winner = HORDE;


    if (winner)
    {
        UpdateWorldState(BG_TP_FLAG_UNK_ALLIANCE, 0);
        UpdateWorldState(BG_TP_FLAG_UNK_HORDE, 0);
        UpdateWorldState(BG_TP_FLAG_STATE_ALLIANCE, 1);
        UpdateWorldState(BG_TP_FLAG_STATE_HORDE, 1);
        UpdateWorldState(BG_TP_STATE_TIMER_ACTIVE, 0);


        RewardHonorToTeam(BG_TP_Honor[m_HonorMode][BG_TP_WIN], winner);
        EndBattleground(winner);
    }
    else
    {
        _flagsTimer[GetTeamIndexByTeamId(Source->GetTeam()) ? 0 : 1] = BG_TP_FLAG_RESPAWN_TIME;
    }
}


void BattlegroundTP::EventPlayerDroppedFlag(Player* Source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        // if not running, do not cast things at the dropper player (prevent spawning the "dropped" flag), neither send unnecessary messages
        // just take off the aura
        if (Source->GetTeam() == ALLIANCE)
        {
            if (!IsHordeFlagPickedup())
                return;
            if (GetFlagPickerGUID(TEAM_HORDE) == Source->GetGUID())
            {
                SetHordeFlagPicker(0);
                Source->RemoveAurasDueToSpell(BG_TP_SPELL_HORDE_FLAG);
            }
        }
        else
        {
            if (!IsAllianceFlagPickedup())
                return;
            if (GetFlagPickerGUID(TEAM_ALLIANCE) == Source->GetGUID())
            {
                SetAllianceFlagPicker(0);
                Source->RemoveAurasDueToSpell(BG_TP_SPELL_ALLIANCE_FLAG);
            }
        }
        return;
    }


    bool set = false;


    if (Source->GetTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;
        if (GetFlagPickerGUID(TEAM_HORDE) == Source->GetGUID())
        {
            SetHordeFlagPicker(0);
            Source->RemoveAurasDueToSpell(BG_TP_SPELL_HORDE_FLAG);
            if (_flagDebuffState == 1)
                Source->RemoveAurasDueToSpell(TP_SPELL_FOCUSED_ASSAULT);
            if (_flagDebuffState == 2)
                Source->RemoveAurasDueToSpell(TP_SPELL_BRUTAL_ASSAULT);
            _flagState[TEAM_HORDE] = BG_TP_FLAG_STATE_ON_GROUND;
            Source->CastSpell(Source, BG_TP_SPELL_HORDE_FLAG_DROPPED, true);
            set = true;
        }
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;
        if (GetFlagPickerGUID(TEAM_ALLIANCE) == Source->GetGUID())
        {
            SetAllianceFlagPicker(0);
            Source->RemoveAurasDueToSpell(BG_TP_SPELL_ALLIANCE_FLAG);
            if (_flagDebuffState == 1)
                Source->RemoveAurasDueToSpell(TP_SPELL_FOCUSED_ASSAULT);
            if (_flagDebuffState == 2)
                Source->RemoveAurasDueToSpell(TP_SPELL_BRUTAL_ASSAULT);
            _flagState[TEAM_ALLIANCE] = BG_TP_FLAG_STATE_ON_GROUND;
            Source->CastSpell(Source, BG_TP_SPELL_ALLIANCE_FLAG_DROPPED, true);
            set = true;
        }
    }


    if (set)
    {
        Source->CastSpell(Source, SPELL_RECENTLY_DROPPED_FLAG, true);
        UpdateFlagState(Source->GetTeam(), 1);


        if (Source->GetTeam() == ALLIANCE)
        {
            SendMessageToAll(LANG_BG_TP_DROPPED_HF, ChatMsg::CHAT_MSG_BG_SYSTEM_HORDE, Source);
            UpdateWorldState(BG_TP_FLAG_UNK_HORDE, uint32(-1));
        }
        else
        {
            SendMessageToAll(LANG_BG_TP_DROPPED_AF, ChatMsg::CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
            UpdateWorldState(BG_TP_FLAG_UNK_ALLIANCE, uint32(-1));
        }


        _flagsDropTimer[GetTeamIndexByTeamId(Source->GetTeam()) ? 0 : 1] = BG_TP_FLAG_DROP_TIME;
    }
}


void BattlegroundTP::EventPlayerClickedOnFlag(Player* Source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;


    int32 message_id = 0;
    ChatMsg type = ChatMsg::CHAT_MSG_BG_SYSTEM_NEUTRAL;


    //alliance flag picked up from base
    if (Source->GetTeam() == HORDE && GetFlagState(ALLIANCE) == BG_TP_FLAG_STATE_ON_BASE
        && BgObjects[BG_TP_OBJECT_A_FLAG] == target_obj->GetGUID())
    {
        message_id = LANG_BG_TP_PICKEDUP_AF;
        type = ChatMsg::CHAT_MSG_BG_SYSTEM_HORDE;
        PlaySoundToAll(BG_TP_SOUND_ALLIANCE_FLAG_PICKED_UP);
        SpawnBGObject(BG_TP_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
        SetAllianceFlagPicker(Source->GetGUID());
        _flagState[TEAM_ALLIANCE] = BG_TP_FLAG_STATE_ON_PLAYER;
        //update world state to show correct flag carrier
        UpdateFlagState(HORDE, BG_TP_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_TP_FLAG_UNK_ALLIANCE, 1);
        Source->CastSpell(Source, BG_TP_SPELL_ALLIANCE_FLAG, true);
        Source->StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_TP_SPELL_ALLIANCE_FLAG_PICKED);
        if (_flagState[1] == BG_TP_FLAG_STATE_ON_PLAYER)
            _bothFlagsKept = true;
    }


    //horde flag picked up from base
    if (Source->GetTeam() == ALLIANCE && GetFlagState(HORDE) == BG_TP_FLAG_STATE_ON_BASE
        && BgObjects[BG_TP_OBJECT_H_FLAG] == target_obj->GetGUID())
    {
        message_id = LANG_BG_TP_PICKEDUP_HF;
        type = ChatMsg::CHAT_MSG_BG_SYSTEM_ALLIANCE;
        PlaySoundToAll(BG_TP_SOUND_HORDE_FLAG_PICKED_UP);
        SpawnBGObject(BG_TP_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
        SetHordeFlagPicker(Source->GetGUID());
        _flagState[TEAM_HORDE] = BG_TP_FLAG_STATE_ON_PLAYER;
        //update world state to show correct flag carrier
        UpdateFlagState(ALLIANCE, BG_TP_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_TP_FLAG_UNK_HORDE, 1);
        Source->CastSpell(Source, BG_TP_SPELL_HORDE_FLAG, true);
        Source->StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_TP_SPELL_HORDE_FLAG_PICKED);
        if (_flagState[0] == BG_TP_FLAG_STATE_ON_PLAYER)
            _bothFlagsKept = true;
    }


    //Alliance flag on ground(not in base) (returned or picked up again from ground!)
    if (GetFlagState(ALLIANCE) == BG_TP_FLAG_STATE_ON_GROUND && Source->IsWithinDistInMap(target_obj, 10)
        && target_obj->GetGOInfo()->entry == BG_OBJECT_A_FLAG_GROUND_TP_ENTRY)
    {
        if (Source->GetTeam() == ALLIANCE)
        {
            message_id = LANG_BG_TP_RETURNED_AF;
            type = ChatMsg::CHAT_MSG_BG_SYSTEM_ALLIANCE;
            UpdateFlagState(HORDE, BG_TP_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(ALLIANCE, false);
            SpawnBGObject(BG_TP_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
            PlaySoundToAll(BG_TP_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(Source, SCORE_FLAG_RETURNS, 1);
            _bothFlagsKept = false;
        }
        else
        {
            message_id = LANG_BG_TP_PICKEDUP_AF;
            type = ChatMsg::CHAT_MSG_BG_SYSTEM_HORDE;
            PlaySoundToAll(BG_TP_SOUND_ALLIANCE_FLAG_PICKED_UP);
            SpawnBGObject(BG_TP_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
            SetAllianceFlagPicker(Source->GetGUID());
            Source->CastSpell(Source, BG_TP_SPELL_ALLIANCE_FLAG, true);
            _flagState[TEAM_ALLIANCE] = BG_TP_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(HORDE, BG_TP_FLAG_STATE_ON_PLAYER);
            if (_flagDebuffState == 1)
                Source->CastSpell(Source, TP_SPELL_FOCUSED_ASSAULT, true);
            if (_flagDebuffState == 2)
                Source->CastSpell(Source, TP_SPELL_BRUTAL_ASSAULT, true);
            UpdateWorldState(BG_TP_FLAG_UNK_ALLIANCE, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }


    //Horde flag on ground(not in base) (returned or picked up again)
    if (GetFlagState(HORDE) == BG_TP_FLAG_STATE_ON_GROUND && Source->IsWithinDistInMap(target_obj, 10)
        && target_obj->GetGOInfo()->entry == BG_OBJECT_H_FLAG_GROUND_TP_ENTRY)
    {
        if (Source->GetTeam() == HORDE)
        {
            message_id = LANG_BG_TP_RETURNED_HF;
            type = ChatMsg::CHAT_MSG_BG_SYSTEM_HORDE;
            UpdateFlagState(ALLIANCE, BG_TP_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(HORDE, false);
            SpawnBGObject(BG_TP_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
            PlaySoundToAll(BG_TP_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(Source, SCORE_FLAG_RETURNS, 1);
            _bothFlagsKept = false;
        }
        else
        {
            message_id = LANG_BG_TP_PICKEDUP_HF;
            type = ChatMsg::CHAT_MSG_BG_SYSTEM_ALLIANCE;
            PlaySoundToAll(BG_TP_SOUND_HORDE_FLAG_PICKED_UP);
            SpawnBGObject(BG_TP_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
            SetHordeFlagPicker(Source->GetGUID());
            Source->CastSpell(Source, BG_TP_SPELL_HORDE_FLAG, true);
            _flagState[TEAM_HORDE] = BG_TP_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(ALLIANCE, BG_TP_FLAG_STATE_ON_PLAYER);
            if (_flagDebuffState == 1)
                Source->CastSpell(Source, TP_SPELL_FOCUSED_ASSAULT, true);
            if (_flagDebuffState == 2)
                Source->CastSpell(Source, TP_SPELL_BRUTAL_ASSAULT, true);
            UpdateWorldState(BG_TP_FLAG_UNK_HORDE, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }


    if (!message_id)
        return;


    SendMessageToAll(message_id, type, Source);
    Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}


void BattlegroundTP::RemovePlayer(Player* player, uint64 guid, uint32 /*team*/)
{
    // sometimes flag aura not removed :(
    if (IsAllianceFlagPickedup() && m_FlagKeepers[TEAM_ALLIANCE] == guid)
    {
        if (!player)
        {
            SF_LOG_ERROR("bg.battleground", "BattlegroundTP: Removing offline player who has the FLAG!!");
            SetAllianceFlagPicker(0);
            RespawnFlag(ALLIANCE, false);
        }
        else
            EventPlayerDroppedFlag(player);
    }
    if (IsHordeFlagPickedup() && m_FlagKeepers[TEAM_HORDE] == guid)
    {
        if (!player)
        {
            SF_LOG_ERROR("bg.battleground", "BattlegroundTP: Removing offline player who has the FLAG!!");
            SetHordeFlagPicker(0);
            RespawnFlag(HORDE, false);
        }
        else
            EventPlayerDroppedFlag(player);
    }
}


void BattlegroundTP::UpdateFlagState(uint32 team, uint32 value)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_TP_FLAG_STATE_ALLIANCE, value);
    else
        UpdateWorldState(BG_TP_FLAG_STATE_HORDE, value);
}


void BattlegroundTP::UpdateTeamScore(uint32 team)
{
    if (team == TEAM_ALLIANCE)
        UpdateWorldState(BG_TP_FLAG_CAPTURES_ALLIANCE, GetTeamScore(team));
    else
        UpdateWorldState(BG_TP_FLAG_CAPTURES_HORDE, GetTeamScore(team));
}


void BattlegroundTP::HandleAreaTrigger(Player* player, uint32 trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;


    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch (trigger)
    {
        case 5904:                                          // Alliance Flag spawn
            if (_flagState[TEAM_HORDE] && !_flagState[TEAM_ALLIANCE])
                if (GetFlagPickerGUID(TEAM_HORDE) == player->GetGUID())
                    EventPlayerCapturedFlag(player);
            break;
        case 5905:                                          // Horde Flag spawn
            if (_flagState[TEAM_ALLIANCE] && !_flagState[TEAM_HORDE])
                if (GetFlagPickerGUID(TEAM_ALLIANCE) == player->GetGUID())
                    EventPlayerCapturedFlag(player);
            break;
        case 5908:                                          // Horde Tower
        case 5909:                                          // Twin Peak House big
        case 5910:                                          // Horde House
        case 5911:                                          // Twin Peak House small
        case 5914:                                          // Alliance Start right
        case 5916:                                          // Alliance Start
        case 5917:                                          // Alliance Start left
        case 5918:                                          // Horde Start
        case 5920:                                          // Horde Start Front entrance
        case 5921:                                          // Horde Start left Water channel
            break;
        default:
            Battleground::HandleAreaTrigger(player, trigger);
            break;
    }


    //if (buff_guid)
    //    HandleTriggerBuff(buff_guid, Source);
}


bool BattlegroundTP::SetupBattleground()
{
    // flags X Y Z Orientation Rotation2 Rotation3
    if (!AddObject(BG_TP_OBJECT_A_FLAG, BG_OBJECT_A_FLAG_TP_ENTRY, 2118.210f, 191.621f, 44.052f, 5.741259f, 0, 0, 0.9996573f, 0.02617699f, BG_TP_FLAG_RESPAWN_TIME / 1000)
        || !AddObject(BG_TP_OBJECT_H_FLAG, BG_OBJECT_H_FLAG_TP_ENTRY, 1578.380f, 344.037f, 2.419f, 3.055978f, 0, 0, 0.008726535f, 0.9999619f, BG_TP_FLAG_RESPAWN_TIME / 1000)
        // buffs
        || !AddObject(BG_TP_OBJECT_SPEEDBUFF_1, BG_OBJECTID_SPEEDBUFF_ENTRY, 1545.402f, 304.028f, 0.5923f, -1.64061f, 0, 0, 0.7313537f, -0.6819983f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_TP_OBJECT_SPEEDBUFF_2, BG_OBJECTID_SPEEDBUFF_ENTRY, 2171.279f, 222.334f, 43.8001f, 2.663309f, 0, 0, 0.7313537f, 0.6819984f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_TP_OBJECT_REGENBUFF_1, BG_OBJECTID_REGENBUFF_ENTRY, 1753.957f, 242.092f, -14.1170f, 1.105848f, 0, 0, 0.1305263f, -0.9914448f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_TP_OBJECT_REGENBUFF_2, BG_OBJECTID_REGENBUFF_ENTRY, 1952.121f, 383.857f, -10.2870f, 4.192612f, 0, 0, 0.333807f, -0.9426414f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_TP_OBJECT_BERSERKBUFF_1, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1934.369f, 226.064f, -17.0441f, 2.499154f, 0, 0, 0.5591929f, 0.8290376f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_TP_OBJECT_BERSERKBUFF_2, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1725.240f, 446.431f, -7.8327f, 5.709677f, 0, 0, 0.9396926f, -0.3420201f, BUFF_RESPAWN_TIME)
        // alliance gates
        || !AddObject(BG_TP_OBJECT_DOOR_A_1, BG_OBJECT_DOOR_A_1_TP_ENTRY, 2115.399f, 150.175f, 43.526f, 2.544690f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_TP_OBJECT_DOOR_A_2, BG_OBJECT_DOOR_A_2_TP_ENTRY, 2156.803f, 220.331f, 43.482f, 2.544690f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_TP_OBJECT_DOOR_A_3, BG_OBJECT_DOOR_A_3_TP_ENTRY, 2127.512f, 223.711f, 43.640f, 2.544690f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_TP_OBJECT_DOOR_A_4, BG_OBJECT_DOOR_A_4_TP_ENTRY, 2096.102f, 166.920f, 54.230f, 2.544690f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        // horde gates
        || !AddObject(BG_TP_OBJECT_DOOR_H_1, BG_OBJECT_DOOR_H_1_TP_ENTRY, 1556.595f, 314.502f, 1.2230f, 6.179126f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_TP_OBJECT_DOOR_H_2, BG_OBJECT_DOOR_H_2_TP_ENTRY, 1587.093f, 319.853f, 1.5233f, 6.179126f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_TP_OBJECT_DOOR_H_3, BG_OBJECT_DOOR_H_3_TP_ENTRY, 1591.463f, 365.732f, 13.494f, 6.179126f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_TP_OBJECT_DOOR_H_4, BG_OBJECT_DOOR_H_4_TP_ENTRY, 1558.315f, 372.709f, 1.4840f, 6.179126f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        )
    {
        SF_LOG_ERROR("sql.sql", "BatteGroundTP: Failed to spawn some objects Battleground not created!");
        return false;
    }


    WorldSafeLocsEntry const* sg = sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_MIDDLE_ALLIANCE);
    if (!sg || !AddSpiritGuide(TP_SPIRIT_ALLIANCE, sg->x, sg->y, sg->z, 3.641396f, ALLIANCE))
    {
        SF_LOG_ERROR("sql.sql", "BatteGroundTP: Failed to spawn Alliance spirit guides! Battleground not created!");
        return false;
    }


    sg = sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_START_ALLIANCE);
    if (!sg || !AddSpiritGuide(TP_SPIRIT_ALLIANCE, sg->x, sg->y, sg->z, 3.641396f, ALLIANCE))
    {
        SF_LOG_ERROR("sql.sql", "BatteGroundTP: Failed to spawn Alliance start spirit guides! Battleground not created!");
        return false;
    }


    sg = sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_MIDDLE_HORDE);
    if (!sg || !AddSpiritGuide(TP_SPIRIT_HORDE, sg->x, sg->y, sg->z, 3.641396f, HORDE))
    {
        SF_LOG_ERROR("sql.sql", "BatteGroundTP: Failed to spawn Horde spirit guides! Battleground not created!");
        return false;
    }
    sg = sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_START_HORDE);
    if (!sg || !AddSpiritGuide(TP_SPIRIT_ALLIANCE, sg->x, sg->y, sg->z, 3.641396f, HORDE))
    {
        SF_LOG_ERROR("sql.sql", "BatteGroundTP: Failed to spawn Horde start spirit guide! Battleground not created!");
        return false;
    }


    SF_LOG_DEBUG("bg.battleground", "BatteGroundTP: BG objects and spirit guides spawned");


    return true;
}


void BattlegroundTP::Reset()
{
    //call parent's class reset
    Battleground::Reset();


    m_FlagKeepers[TEAM_ALLIANCE] = 0;
    m_FlagKeepers[TEAM_HORDE] = 0;
    m_DroppedFlagGUID[TEAM_ALLIANCE] = 0;
    m_DroppedFlagGUID[TEAM_HORDE] = 0;
    _flagState[TEAM_ALLIANCE] = BG_TP_FLAG_STATE_ON_BASE;
    _flagState[TEAM_HORDE] = BG_TP_FLAG_STATE_ON_BASE;
    m_TeamScores[TEAM_ALLIANCE] = 0;
    m_TeamScores[TEAM_HORDE] = 0;


    if (sBattlegroundMgr->IsBGWeekend(GetTypeID()))
    {
        m_ReputationCapture = 45;
        m_HonorWinKills = 3;
        m_HonorEndKills = 4;
    }
    else
    {
        m_ReputationCapture = 35;
        m_HonorWinKills = 1;
        m_HonorEndKills = 2;
    }
    _minutesElapsed = 0;
    _lastFlagCaptureTeam = 0;
    _bothFlagsKept = false;
    _flagDebuffState = 0;
    _flagSpellForceTimer = 0;
    _flagsDropTimer[TEAM_ALLIANCE] = 0;
    _flagsDropTimer[TEAM_HORDE] = 0;
    _flagsTimer[TEAM_ALLIANCE] = 0;
    _flagsTimer[TEAM_HORDE] = 0;
}


void BattlegroundTP::EndBattleground(uint32 winner)
{
    // win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), HORDE);
    // complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), HORDE);


    Battleground::EndBattleground(winner);
}


void BattlegroundTP::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;


    EventPlayerDroppedFlag(player);


    Battleground::HandleKillPlayer(player, killer);
}


void BattlegroundTP::UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(Source->GetGUID());
    if (itr == PlayerScores.end())                         // player not found
        return;


    switch (type)
    {
        case SCORE_FLAG_CAPTURES:                           // flags captured
            ((BattlegroundTPScore*)itr->second)->FlagCaptures += value;
            Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, TP_OBJECTIVE_CAPTURE_FLAG);
            break;
        case SCORE_FLAG_RETURNS:                            // flags returned
            ((BattlegroundTPScore*)itr->second)->FlagReturns += value;
            Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, TP_OBJECTIVE_RETURN_FLAG);
            break;
        default:
            Battleground::UpdatePlayerScore(Source, type, value, doAddHonor);
            break;
    }
}


WorldSafeLocsEntry const* BattlegroundTP::GetClosestGraveYard(Player* player)
{
    // if status in progress, it returns main or start graveyards with spiritguides
    // else it will return the graveyard in the flagroom - this is especially good
    // if a player dies in preparation phase - then the player can't cheat
    // and teleport to the graveyard outside the flagroom
    // and start running around, while the doors are still closed
    if (player->GetTeam() == ALLIANCE)
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_MIDDLE_ALLIANCE);
        else
            return sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_FLAGROOM_ALLIANCE);
    }
    else
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_MIDDLE_HORDE);
        else
            return sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_FLAGROOM_HORDE);
    }
}


void BattlegroundTP::FillInitialWorldStates(WorldStateBuilder& builder)
{
    builder.AppendState(BG_TP_FLAG_CAPTURES_ALLIANCE, GetTeamScore(TEAM_ALLIANCE));
    builder.AppendState(BG_TP_FLAG_CAPTURES_HORDE, GetTeamScore(TEAM_HORDE));


    if (_flagState[TEAM_ALLIANCE] == BG_TP_FLAG_STATE_ON_GROUND)
        builder.AppendState(BG_TP_FLAG_UNK_ALLIANCE, -1);
    else if (_flagState[TEAM_ALLIANCE] == BG_TP_FLAG_STATE_ON_PLAYER)
        builder.AppendState(BG_TP_FLAG_UNK_ALLIANCE, 1);
    else
        builder.AppendState(BG_TP_FLAG_UNK_ALLIANCE, 0);


    if (_flagState[TEAM_HORDE] == BG_TP_FLAG_STATE_ON_GROUND)
        builder.AppendState(BG_TP_FLAG_UNK_HORDE, -1);
    else if (_flagState[TEAM_HORDE] == BG_TP_FLAG_STATE_ON_PLAYER)
        builder.AppendState(BG_TP_FLAG_UNK_HORDE, 1);
    else
        builder.AppendState(BG_TP_FLAG_UNK_HORDE, 0);


    builder.AppendState(BG_TP_FLAG_CAPTURES_MAX, BG_TP_MAX_TEAM_SCORE);


    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        builder.AppendState(BG_TP_STATE_TIMER_ACTIVE, 1);
        builder.AppendState(BG_TP_STATE_TIMER, 25 - _minutesElapsed);
    }
    else
        builder.AppendState(BG_TP_STATE_TIMER_ACTIVE, 0);


    if (_flagState[TEAM_HORDE] == BG_TP_FLAG_STATE_ON_PLAYER)
        builder.AppendState(BG_TP_FLAG_STATE_HORDE, 2);
    else
        builder.AppendState(BG_TP_FLAG_STATE_HORDE, 1);


    if (_flagState[TEAM_ALLIANCE] == BG_TP_FLAG_STATE_ON_PLAYER)
        builder.AppendState(BG_TP_FLAG_STATE_ALLIANCE, 2);
    else
        builder.AppendState(BG_TP_FLAG_STATE_ALLIANCE, 1);
}


uint32 BattlegroundTP::GetPrematureWinner()
{
    if (GetTeamScore(TEAM_ALLIANCE) > GetTeamScore(TEAM_HORDE))
        return ALLIANCE;
    else if (GetTeamScore(TEAM_HORDE) > GetTeamScore(TEAM_ALLIANCE))
        return HORDE;


    return Battleground::GetPrematureWinner();
}
