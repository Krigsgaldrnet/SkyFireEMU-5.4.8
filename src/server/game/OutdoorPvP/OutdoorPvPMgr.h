/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef OUTDOOR_PVP_MGR_H_
#define OUTDOOR_PVP_MGR_H_

#define OUTDOORPVP_OBJECTIVE_UPDATE_INTERVAL 1000

#include "OutdoorPvP.h"
#include <ace/Singleton.h>

class Player;
class GameObject;
class Creature;
class ZoneScript;
struct GossipMenuItems;

struct OutdoorPvPData
{
    OutdoorPvPTypes TypeId;
    uint32 ScriptId;
};

// class to handle player enter / leave / areatrigger / GO use events
class OutdoorPvPMgr
{
    friend class ACE_Singleton<OutdoorPvPMgr, ACE_Null_Mutex>;

private:
    OutdoorPvPMgr() : m_UpdateTimer(0) { };
    ~OutdoorPvPMgr() { };

public:
    // create outdoor pvp events
    void InitOutdoorPvP();

    // cleanup
    void Die();

    // called when a player enters an outdoor pvp area
    void HandlePlayerEnterZone(Player* player, uint32 areaflag);

    // called when player leaves an outdoor pvp area
    void HandlePlayerLeaveZone(Player* player, uint32 areaflag);

    // called when player resurrects
    void HandlePlayerResurrects(Player* player, uint32 areaflag);

    // return assigned outdoor pvp
    OutdoorPvP* GetOutdoorPvPToZoneId(uint32 zoneid);

    // handle custom (non-exist in dbc) spell if registered
    bool HandleCustomSpell(Player* player, uint32 spellId, GameObject* go);

    // handle custom go if registered
    bool HandleOpenGo(Player* player, uint64 guid);

    ZoneScript* GetZoneScript(uint32 zoneId);

    void AddZone(uint32 zoneid, OutdoorPvP* handle);

    void Update(uint32 diff);

    void HandleGossipOption(Player* player, uint64 guid, uint32 gossipid);

    bool CanTalkTo(Player* player, Creature* creature, GossipMenuItems const& gso);

    void HandleDropFlag(Player* player, uint32 spellId);

private:
    typedef std::vector<OutdoorPvP*> OutdoorPvPSet;
    typedef std::map<uint32 /* zoneid */, OutdoorPvP*> OutdoorPvPMap;
    typedef std::map<OutdoorPvPTypes, OutdoorPvPData*> OutdoorPvPDataMap;

    // contains all initiated outdoor pvp events
    // used when initing / cleaning up
    OutdoorPvPSet  m_OutdoorPvPSet;

    // maps the zone ids to an outdoor pvp event
    // used in player event handling
    OutdoorPvPMap   m_OutdoorPvPMap;

    // Holds the outdoor PvP templates
    OutdoorPvPDataMap m_OutdoorPvPDatas;

    // update interval
    uint32 m_UpdateTimer;
};

#define sOutdoorPvPMgr ACE_Singleton<OutdoorPvPMgr, ACE_Null_Mutex>::instance()

#endif /*OUTDOOR_PVP_MGR_H_*/
