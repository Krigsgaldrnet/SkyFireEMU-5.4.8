/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SKYFIRE_CREATURE_TEXT_MGR_H
#define SKYFIRE_CREATURE_TEXT_MGR_H

#include "Creature.h"
#include "GridNotifiers.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "SharedDefines.h"

struct CreatureTextEntry
{
    uint32 entry;
    uint8 group;
    uint8 id;
    std::string text;
    ChatMsg type;
    Language lang;
    float probability;
    Emote emote;
    uint32 duration;
    uint32 sound;
};

enum CreatureTextRange
{
    TEXT_RANGE_NORMAL = 0,
    TEXT_RANGE_AREA = 1,
    TEXT_RANGE_ZONE = 2,
    TEXT_RANGE_MAP = 3,
    TEXT_RANGE_WORLD = 4
};

struct CreatureTextLocale
{
    StringVector Text;
};

struct CreatureTextId
{
    CreatureTextId(uint32 e, uint32 g, uint32 i) : entry(e), textGroup(g), textId(i)
    {
    }

    bool operator<(CreatureTextId const& right) const
    {
        return memcmp(this, &right, sizeof(CreatureTextId)) < 0;
    }

    uint32 entry;
    uint32 textGroup;
    uint32 textId;
};

typedef std::vector<CreatureTextEntry> CreatureTextGroup;              //texts in a group
typedef UNORDERED_MAP<uint8, CreatureTextGroup> CreatureTextHolder;    //groups for a creature by groupid
typedef UNORDERED_MAP<uint32, CreatureTextHolder> CreatureTextMap;     //all creatures by entry

typedef std::map<CreatureTextId, CreatureTextLocale> LocaleCreatureTextMap;

//used for handling non-repeatable random texts
typedef std::vector<uint8> CreatureTextRepeatIds;
typedef UNORDERED_MAP<uint8, CreatureTextRepeatIds> CreatureTextRepeatGroup;
typedef UNORDERED_MAP<uint64, CreatureTextRepeatGroup> CreatureTextRepeatMap;//guid based

class CreatureTextMgr
{
    friend class ACE_Singleton<CreatureTextMgr, ACE_Null_Mutex>;
    CreatureTextMgr() { };

public:
    ~CreatureTextMgr() { };
    void LoadCreatureTexts();
    void LoadCreatureTextLocales();
    CreatureTextMap  const& GetTextMap() const { return mTextMap; }

    void SendSound(Creature* source, uint32 sound, ChatMsg msgType, WorldObject const* whisperTarget, CreatureTextRange range, Team team, bool gmOnly);
    void SendEmote(Unit* source, uint32 emote);

    //if sent, returns the 'duration' of the text else 0 if error
    uint32 SendChat(Creature* source, uint8 textGroup, WorldObject const* whisperTarget = NULL, ChatMsg msgType = ChatMsg::CHAT_MSG_ADDON, Language language = Language::LANG_ADDON, CreatureTextRange range = TEXT_RANGE_NORMAL, uint32 sound = 0, Team team = TEAM_OTHER, bool gmOnly = false, Player* srcPlr = NULL);
    bool TextExist(uint32 sourceEntry, uint8 textGroup);
    std::string GetLocalizedChatString(uint32 entry, uint8 textGroup, uint32 id, LocaleConstant locale) const;

    template<class Builder>
    void SendChatPacket(WorldObject* source, Builder const& builder, ChatMsg msgType, WorldObject const* whisperTarget = NULL, CreatureTextRange range = TEXT_RANGE_NORMAL, Team team = TEAM_OTHER, bool gmOnly = false) const;
private:
    CreatureTextRepeatIds GetRepeatGroup(Creature* source, uint8 textGroup);
    void SetRepeatId(Creature* source, uint8 textGroup, uint8 id);

    void SendNonChatPacket(WorldObject* source, WorldPacket* data, ChatMsg msgType, WorldObject const* whisperTarget, CreatureTextRange range, Team team, bool gmOnly) const;
    float GetRangeForChatType(ChatMsg msgType) const;

    CreatureTextMap mTextMap;
    CreatureTextRepeatMap mTextRepeatMap;
    LocaleCreatureTextMap mLocaleTextMap;
};

#define sCreatureTextMgr ACE_Singleton<CreatureTextMgr, ACE_Null_Mutex>::instance()

template<class Builder>
class CreatureTextLocalizer
{
public:
    CreatureTextLocalizer(Builder const& builder, ChatMsg msgType) : _builder(builder), _msgType(msgType)
    {
        _packetCache.resize(TOTAL_LOCALES, NULL);
    }

    ~CreatureTextLocalizer()
    {
        for (size_t i = 0; i < _packetCache.size(); ++i)
        {
            if (_packetCache[i])
                delete _packetCache[i]->first;
            delete _packetCache[i];
        }
    }

    void operator()(Player* player)
    {
        LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
        WorldPacket* messageTemplate;
        size_t whisperGUIDpos;

        // create if not cached yet
        if (!_packetCache[loc_idx])
        {
            messageTemplate = new WorldPacket();
            whisperGUIDpos = _builder(messageTemplate, loc_idx);
            //ASSERT(messageTemplate->GetOpcode() != MSG_NULL_ACTION); // ???
            _packetCache[loc_idx] = new std::pair<WorldPacket*, size_t>(messageTemplate, whisperGUIDpos);
        }
        else
        {
            messageTemplate = _packetCache[loc_idx]->first;
            whisperGUIDpos = _packetCache[loc_idx]->second;
        }

        WorldPacket data(*messageTemplate);
        switch (_msgType)
        {
            case ChatMsg::CHAT_MSG_MONSTER_WHISPER:
            case ChatMsg::CHAT_MSG_RAID_BOSS_WHISPER:
                data.put<uint64>(whisperGUIDpos, player->GetGUID());
                break;
            default:
                break;
        }

        player->SendDirectMessage(&data);
    }

private:
    std::vector<std::pair<WorldPacket*, size_t>* > _packetCache;
    Builder const& _builder;
    ChatMsg _msgType;
};

template<class Builder>
void CreatureTextMgr::SendChatPacket(WorldObject* source, Builder const& builder, ChatMsg msgType, WorldObject const* whisperTarget /*= NULL*/, CreatureTextRange range, Team team, bool gmOnly) const
{
    if (!source)
        return;

    CreatureTextLocalizer<Builder> localizer(builder, msgType);

    switch (msgType)
    {
        case ChatMsg::CHAT_MSG_MONSTER_WHISPER:
        case ChatMsg::CHAT_MSG_RAID_BOSS_WHISPER:
        {
            if (range == TEXT_RANGE_NORMAL) //ignores team and gmOnly
            {
                if (!whisperTarget || whisperTarget->GetTypeId() != TypeID::TYPEID_PLAYER)
                    return;

                localizer(const_cast<Player*>(whisperTarget->ToPlayer()));
                return;
            }
            break;
        }
        default:
            break;
    }

    switch (range)
    {
        case TEXT_RANGE_AREA:
        {
            uint32 areaId = source->GetAreaId();
            Map::PlayerList const& players = source->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                if (itr->GetSource()->GetAreaId() == areaId && (!team || Team(itr->GetSource()->GetTeam()) == team) && (!gmOnly || itr->GetSource()->IsGameMaster()))
                    localizer(itr->GetSource());
            return;
        }
        case TEXT_RANGE_ZONE:
        {
            uint32 zoneId = source->GetZoneId();
            Map::PlayerList const& players = source->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                if (itr->GetSource()->GetZoneId() == zoneId && (!team || Team(itr->GetSource()->GetTeam()) == team) && (!gmOnly || itr->GetSource()->IsGameMaster()))
                    localizer(itr->GetSource());
            return;
        }
        case TEXT_RANGE_MAP:
        {
            Map::PlayerList const& players = source->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                if ((!team || Team(itr->GetSource()->GetTeam()) == team) && (!gmOnly || itr->GetSource()->IsGameMaster()))
                    localizer(itr->GetSource());
            return;
        }
        case TEXT_RANGE_WORLD:
        {
            SessionMap const& smap = sWorld->GetAllSessions();
            for (SessionMap::const_iterator iter = smap.begin(); iter != smap.end(); ++iter)
                if (Player* player = iter->second->GetPlayer())
                    if (player->GetSession() && (!team || Team(player->GetTeam()) == team) && (!gmOnly || player->IsGameMaster()))
                        localizer(player);
            return;
        }
        case TEXT_RANGE_NORMAL:
        default:
            break;
    }

    float dist = GetRangeForChatType(msgType);
    Skyfire::PlayerDistWorker<CreatureTextLocalizer<Builder> > worker(source, dist, localizer);
    source->VisitNearbyWorldObject(dist, worker);
}

#endif
