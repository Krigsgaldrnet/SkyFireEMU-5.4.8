/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SKYFIRE_GAMEEVENT_MGR_H
#define SKYFIRE_GAMEEVENT_MGR_H

#include "Common.h"
#include "Define.h"
#include "SharedDefines.h"
#include <ace/Singleton.h>

#define max_ge_check_delay DAY  // 1 day in seconds

enum GameEventState
{
    GAMEEVENT_NORMAL = 0, // standard game events
    GAMEEVENT_WORLD_INACTIVE = 1, // not yet started
    GAMEEVENT_WORLD_CONDITIONS = 2, // condition matching phase
    GAMEEVENT_WORLD_NEXTPHASE = 3, // conditions are met, now 'length' timer to start next event
    GAMEEVENT_WORLD_FINISHED = 4, // next events are started, unapply this one
    GAMEEVENT_INTERNAL = 5  // never handled in update
};

struct GameEventFinishCondition
{
    float reqNum;  // required number // use float, since some events use percent
    float done;    // done number
    uint32 max_world_state;  // max resource count world state update id
    uint32 done_world_state; // done resource count world state update id
};

struct GameEventQuestToEventConditionNum
{
    uint16 event_id;
    uint32 condition;
    float num;
};

typedef std::map<uint32 /*condition id*/, GameEventFinishCondition> GameEventConditionMap;

struct GameEventData
{
    GameEventData() : start(1), end(0), nextstart(0), occurence(0), length(0), holiday_id(HolidayIds::HOLIDAY_NONE), state(GAMEEVENT_NORMAL), announce(0) { }
    time_t start;           // occurs after this time
    time_t end;             // occurs before this time
    time_t nextstart;       // after this time the follow-up events count this phase completed
    uint32 occurence;       // time between end and start
    uint32 length;          // length of the event (minutes) after finishing all conditions
    HolidayIds holiday_id;
    GameEventState state;   // state of the game event, these are saved into the game_event table on change!
    GameEventConditionMap conditions;  // conditions to finish
    std::set<uint16 /*gameevent id*/> prerequisite_events;  // events that must be completed before starting this event
    std::string description;
    uint8 announce;         // if 0 dont announce, if 1 announce, if 2 take config value

    bool isValid() const { return length > 0 || state > GAMEEVENT_NORMAL; }
};

struct ModelEquip
{
    uint32 modelid;
    uint32 modelid_prev;
    uint8 equipment_id;
    uint8 equipement_id_prev;
};

struct NPCVendorEntry
{
    uint32 entry;                                           // creature entry
    uint32 item;                                            // item id
    int32  maxcount;                                        // 0 for infinite
    uint32 incrtime;                                        // time for restore items amount if maxcount != 0
    uint32 ExtendedCost;
    uint8 Type;                                             // 1 item, 2 currency
};

class Player;
class Creature;
class Quest;

class GameEventMgr
{
    friend class ACE_Singleton<GameEventMgr, ACE_Null_Mutex>;

private:
    GameEventMgr();
    ~GameEventMgr() { };

public:
    typedef std::set<uint16> ActiveEvents;
    typedef std::vector<GameEventData> GameEventDataMap;
    ActiveEvents const& GetActiveEventList() const { return m_ActiveEvents; }
    GameEventDataMap const& GetEventMap() const { return mGameEvent; }
    bool CheckOneGameEvent(uint16 entry) const;
    uint32 NextCheck(uint16 entry) const;
    void LoadFromDB();
    uint32 Update();
    bool IsActiveEvent(uint16 event_id) { return (m_ActiveEvents.find(event_id) != m_ActiveEvents.end()); }
    uint32 StartSystem();
    void Initialize();
    void StartArenaSeason();
    void StartInternalEvent(uint16 event_id);
    bool StartEvent(uint16 event_id, bool overwrite = false);
    void StopEvent(uint16 event_id, bool overwrite = false);
    void HandleQuestComplete(uint32 quest_id);  // called on world event type quest completions
    void HandleWorldEventGossip(Player* player, Creature* c);
    uint32 GetNPCFlag(Creature* cr);
    uint32 GetNpcTextId(uint32 guid);
    uint16 GetEventIdForQuest(Quest const* quest) const;
private:
    void SendWorldStateUpdate(Player* player, uint16 event_id);
    void AddActiveEvent(uint16 event_id) { m_ActiveEvents.insert(event_id); }
    void RemoveActiveEvent(uint16 event_id) { m_ActiveEvents.erase(event_id); }
    void ApplyNewEvent(uint16 event_id);
    void UnApplyEvent(uint16 event_id);
    void GameEventSpawn(int16 event_id);
    void GameEventUnspawn(int16 event_id);
    void ChangeEquipOrModel(int16 event_id, bool activate);
    void UpdateEventQuests(uint16 event_id, bool activate);
    void UpdateWorldStates(uint16 event_id, bool Activate);
    void UpdateEventNPCFlags(uint16 event_id);
    void UpdateEventNPCVendor(uint16 event_id, bool activate);
    void UpdateBattlegroundSettings();
    void RunSmartAIScripts(uint16 event_id, bool activate);    //! Runs SMART_EVENT_GAME_EVENT_START/_END SAI
    bool CheckOneGameEventConditions(uint16 event_id);
    void SaveWorldEventStateToDB(uint16 event_id);
    bool hasCreatureQuestActiveEventExcept(uint32 quest_id, uint16 event_id);
    bool hasGameObjectQuestActiveEventExcept(uint32 quest_id, uint16 event_id);
    bool hasCreatureActiveEventExcept(uint32 creature_guid, uint16 event_id);
    bool hasGameObjectActiveEventExcept(uint32 go_guid, uint16 event_id);

    typedef std::list<uint32> GuidList;
    typedef std::list<uint32> IdList;
    typedef std::vector<GuidList> GameEventGuidMap;
    typedef std::vector<IdList> GameEventIdMap;
    typedef std::pair<uint32, ModelEquip> ModelEquipPair;
    typedef std::list<ModelEquipPair> ModelEquipList;
    typedef std::vector<ModelEquipList> GameEventModelEquipMap;
    typedef std::pair<uint32, uint32> QuestRelation;
    typedef std::list<QuestRelation> QuestRelList;
    typedef std::vector<QuestRelList> GameEventQuestMap;
    typedef std::list<NPCVendorEntry> NPCVendorList;
    typedef std::vector<NPCVendorList> GameEventNPCVendorMap;
    typedef std::map<uint32 /*quest id*/, GameEventQuestToEventConditionNum> QuestIdToEventConditionMap;
    typedef std::pair<uint32 /*guid*/, uint32 /*npcflag*/> GuidNPCFlagPair;
    typedef std::list<GuidNPCFlagPair> NPCFlagList;
    typedef std::vector<NPCFlagList> GameEventNPCFlagMap;
    typedef std::vector<uint32> GameEventBitmask;
    GameEventQuestMap mGameEventCreatureQuests;
    GameEventQuestMap mGameEventGameObjectQuests;
    GameEventNPCVendorMap mGameEventVendors;
    GameEventModelEquipMap mGameEventModelEquip;
    //GameEventGuidMap  mGameEventCreatureGuids;
    //GameEventGuidMap  mGameEventGameobjectGuids;
    GameEventIdMap    mGameEventPoolIds;
    GameEventDataMap  mGameEvent;
    GameEventBitmask  mGameEventBattlegroundHolidays;
    QuestIdToEventConditionMap mQuestToEventConditions;
    GameEventNPCFlagMap mGameEventNPCFlags;
    ActiveEvents m_ActiveEvents;
    UNORDERED_MAP<uint32, uint16> _questToEventLinks;
    bool isSystemInit;
public:
    GameEventGuidMap  mGameEventCreatureGuids;
    GameEventGuidMap  mGameEventGameobjectGuids;
};

#define sGameEventMgr ACE_Singleton<GameEventMgr, ACE_Null_Mutex>::instance()

bool IsHolidayActive(HolidayIds id);
bool IsEventActive(uint16 event_id);

#endif
