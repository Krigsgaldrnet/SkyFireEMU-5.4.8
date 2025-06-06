/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SKYFIRE_MAPMANAGER_H
#define SKYFIRE_MAPMANAGER_H

#include "GridStates.h"
#include "Map.h"
#include "MapUpdater.h"
#include "Object.h"

#include <ace/Singleton.h>
#include <mutex>


class Transport;
struct TransportCreatureProto;

class MapManager
{
    friend class ACE_Singleton<MapManager, ACE_Thread_Mutex>;

public:
    Map* CreateBaseMap(uint32 mapId);
    Map* FindBaseNonInstanceMap(uint32 mapId) const;
    Map* CreateMap(uint32 mapId, Player* player);
    Map* FindMap(uint32 mapId, uint32 instanceId) const;

    uint16 GetAreaFlag(uint32 mapid, float x, float y, float z) const
    {
        Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
        return m->GetAreaFlag(x, y, z);
    }
    uint32 GetAreaId(uint32 mapid, float x, float y, float z) const
    {
        return Map::GetAreaIdByAreaFlag(GetAreaFlag(mapid, x, y, z), mapid);
    }
    uint32 GetZoneId(uint32 mapid, float x, float y, float z) const
    {
        return Map::GetZoneIdByAreaFlag(GetAreaFlag(mapid, x, y, z), mapid);
    }
    void GetZoneAndAreaId(uint32& zoneid, uint32& areaid, uint32 mapid, float x, float y, float z)
    {
        Map::GetZoneAndAreaIdByAreaFlag(zoneid, areaid, GetAreaFlag(mapid, x, y, z), mapid);
    }

    void Initialize(void);
    void Update(uint32);

    void SetGridCleanUpDelay(uint32 t)
    {
        if (t < MIN_GRID_DELAY)
            i_gridCleanUpDelay = MIN_GRID_DELAY;
        else
            i_gridCleanUpDelay = t;
    }

    void SetMapUpdateInterval(uint32 t)
    {
        if (t < MIN_MAP_UPDATE_DELAY)
            t = MIN_MAP_UPDATE_DELAY;

        i_timer.SetInterval(t);
        i_timer.Reset();
    }

    //void LoadGrid(int mapid, int instId, float x, float y, const WorldObject* obj, bool no_unload = false);
    void UnloadAll();

    static bool ExistMapAndVMap(uint32 mapid, float x, float y);
    static bool IsValidMAP(uint32 mapid, bool startUp);

    static bool IsValidMapCoord(uint32 mapid, float x, float y)
    {
        return IsValidMAP(mapid, false) && Skyfire::IsValidMapCoord(x, y);
    }

    static bool IsValidMapCoord(uint32 mapid, float x, float y, float z)
    {
        return IsValidMAP(mapid, false) && Skyfire::IsValidMapCoord(x, y, z);
    }

    static bool IsValidMapCoord(uint32 mapid, float x, float y, float z, float o)
    {
        return IsValidMAP(mapid, false) && Skyfire::IsValidMapCoord(x, y, z, o);
    }

    static bool IsValidMapCoord(WorldLocation const& loc)
    {
        return IsValidMapCoord(loc.GetMapId(), loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetOrientation());
    }

    void DoDelayedMovesAndRemoves();

    bool CanPlayerEnter(uint32 mapid, Player* player, bool loginCheck = false);
    void InitializeVisibilityDistanceInfo();

    /* statistics */
    uint32 GetNumInstances();
    uint32 GetNumPlayersInInstances();

    // Instance ID management
    void InitInstanceIds();
    uint32 GenerateInstanceId();
    void RegisterInstanceId(uint32 instanceId);
    void FreeInstanceId(uint32 instanceId);

    uint32 GetNextInstanceId() const { return _nextInstanceId; };
    void SetNextInstanceId(uint32 nextInstanceId) { _nextInstanceId = nextInstanceId; };

    MapUpdater* GetMapUpdater() { return &m_updater; }

private:
    typedef UNORDERED_MAP<uint32, Map*> MapMapType;
    typedef std::vector<bool> InstanceIds;

    // debugging code, should be deleted some day
    void checkAndCorrectGridStatesArray();              // just for debugging to find some memory overwrites
    GridState* i_GridStates[MAX_GRID_STATE] = { };            // shadow entries to the global array in Map.cpp
    int i_GridStateErrorCount;

    MapManager();
    ~MapManager();

    Map* FindBaseMap(uint32 mapId) const
    {
        MapMapType::const_iterator iter = i_maps.find(mapId);
        return (iter == i_maps.end() ? NULL : iter->second);
    }

    MapManager(const MapManager&);
    MapManager& operator=(const MapManager&);

    std::mutex Lock;
    uint32 i_gridCleanUpDelay;
    MapMapType i_maps;
    IntervalTimer i_timer;

    InstanceIds _instanceIds;
    uint32 _nextInstanceId;
    MapUpdater m_updater;
};
#define sMapMgr ACE_Singleton<MapManager, ACE_Thread_Mutex>::instance()
#endif
