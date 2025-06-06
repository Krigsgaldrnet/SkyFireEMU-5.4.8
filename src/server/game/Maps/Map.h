/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SKYFIRE_MAP_H
#define SKYFIRE_MAP_H

#include "Define.h"

#include "Cell.h"
#include "DBCStructure.h"
#include "DynamicTree.h"
#include "GameObjectModel.h"
#include "GridDefines.h"
#include "GridRefManager.h"
#include "MapRefManager.h"
#include "SharedDefines.h"
#include "Timer.h"

#include <bitset>
#include <list>
#include <mutex>

class Unit;
class WorldPacket;
class InstanceScript;
class Group;
class InstanceSave;
class Object;
class WorldObject;
class TempSummon;
class Player;
class CreatureGroup;
struct ScriptInfo;
struct ScriptAction;
struct Position;
class Battleground;
class MapInstanced;
class InstanceMap;
class Transport;
namespace Skyfire { struct ObjectUpdater; }

struct ScriptAction
{
    uint64 sourceGUID;
    uint64 targetGUID;
    uint64 ownerGUID;                                       ///> owner of source if source is item
    ScriptInfo const* script;                               ///> pointer to static script data
};

/// Represents a map magic value of 4 bytes (used in versions)
union u_map_magic
{
    char asChar[4]; ///> Non-null terminated string
    uint32 asUInt;  ///> uint32 representation
};

// ******************************************
// Map file format defines
// ******************************************
struct map_fileheader
{
    u_map_magic mapMagic;
    u_map_magic versionMagic;
    u_map_magic buildMagic;
    uint32 areaMapOffset;
    uint32 areaMapSize;
    uint32 heightMapOffset;
    uint32 heightMapSize;
    uint32 liquidMapOffset;
    uint32 liquidMapSize;
    uint32 holesOffset;
    uint32 holesSize;
};

#define MAP_AREA_NO_AREA      0x0001

struct map_areaHeader
{
    uint32 fourcc;
    uint16 flags;
    uint16 gridArea;
};

#define MAP_HEIGHT_NO_HEIGHT  0x0001
#define MAP_HEIGHT_AS_INT16   0x0002
#define MAP_HEIGHT_AS_INT8    0x0004

struct map_heightHeader
{
    uint32 fourcc;
    uint32 flags;
    float  gridHeight;
    float  gridMaxHeight;
};

#define MAP_LIQUID_NO_TYPE    0x0001
#define MAP_LIQUID_NO_HEIGHT  0x0002

struct map_liquidHeader
{
    uint32 fourcc;
    uint16 flags;
    uint16 liquidType;
    uint8  offsetX;
    uint8  offsetY;
    uint8  width;
    uint8  height;
    float  liquidLevel;
};

enum ZLiquidStatus
{
    LIQUID_MAP_NO_WATER = 0x00000000,
    LIQUID_MAP_ABOVE_WATER = 0x00000001,
    LIQUID_MAP_WATER_WALK = 0x00000002,
    LIQUID_MAP_IN_WATER = 0x00000004,
    LIQUID_MAP_UNDER_WATER = 0x00000008
};

#define MAP_LIQUID_TYPE_NO_WATER    0x00
#define MAP_LIQUID_TYPE_WATER       0x01
#define MAP_LIQUID_TYPE_OCEAN       0x02
#define MAP_LIQUID_TYPE_MAGMA       0x04
#define MAP_LIQUID_TYPE_SLIME       0x08

#define MAP_ALL_LIQUIDS   (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_SLIME)

#define MAP_LIQUID_TYPE_DARK_WATER  0x10
#define MAP_LIQUID_TYPE_WMO_WATER   0x20

struct LiquidData
{
    uint32 type_flags;
    uint32 entry;
    float  level;
    float  depth_level;
};

#define MAX_HEIGHT            100000.0f                     // can be use for find ground height at surface
#define INVALID_HEIGHT       -100000.0f                     // for check, must be equal to VMAP_INVALID_HEIGHT, real value for unknown height is VMAP_INVALID_HEIGHT_VALUE
#define MAX_FALL_DISTANCE     250000.0f                     // "unlimited fall" to find VMap ground if it is available, just larger than MAX_HEIGHT - INVALID_HEIGHT
#define DEFAULT_HEIGHT_SEARCH     50.0f                     // default search distance to find height at nearby locations
#define MIN_UNLOAD_DELAY      1                             // immediate unload

class GridMap
{
    uint32  m_flags;
    union {
        float* m_V9;
        uint16* m_uint16_V9;
        uint8* m_uint8_V9;
    };
    union {
        float* m_V8;
        uint16* m_uint16_V8;
        uint8* m_uint8_V8;
    };
    // Height level data
    float m_gridHeight;
    float m_gridIntHeightMultiplier;

    // Area data
    uint16* m_areaMap;

    // Liquid data
    float m_liquidLevel;
    uint16* m_liquidEntry;
    uint8* m_liquidFlags;
    float* m_liquidMap;
    uint16 m_gridArea;
    uint16 m_liquidType;
    uint8 m_liquidOffX;
    uint8 m_liquidOffY;
    uint8 m_liquidWidth;
    uint8 m_liquidHeight;


    bool loadAreaData(FILE* in, uint32 offset, uint32 size);
    bool loadHeightData(FILE* in, uint32 offset, uint32 size);
    bool loadLiquidData(FILE* in, uint32 offset, uint32 size);

    // Get height functions and pointers
    typedef float (GridMap::* GetHeightPtr) (float x, float y) const;
    GetHeightPtr m_gridGetHeight;
    float getHeightFromFloat(float x, float y) const;
    float getHeightFromUint16(float x, float y) const;
    float getHeightFromUint8(float x, float y) const;
    float getHeightFromFlat(float x, float y) const;

public:
    GridMap() : m_flags(0), m_V9(NULL), m_V8(NULL), m_gridHeight(INVALID_HEIGHT), m_gridIntHeightMultiplier(0.0f), m_areaMap(NULL), m_liquidLevel(INVALID_HEIGHT),
        m_liquidEntry(NULL), m_liquidFlags(NULL), m_liquidMap(NULL), m_gridArea(0), m_liquidType(0), m_liquidOffX(0), m_liquidOffY(0), m_liquidWidth(0), m_liquidHeight(0)
    {
        m_gridGetHeight = &GridMap::getHeightFromFlat;
    }
    ~GridMap() { unloadData(); }

    bool loadData(char* filaname);
    void unloadData();

    uint16 getArea(float x, float y) const;
    inline float getHeight(float x, float y) const { return (this->*m_gridGetHeight)(x, y); }
    float getLiquidLevel(float x, float y) const;
    uint8 getTerrainType(float x, float y) const;
    ZLiquidStatus getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, LiquidData* data = 0);
};

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push, N), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

struct InstanceTemplate
{
    uint32 Parent;
    uint32 ScriptId;
    bool AllowMount;
};

enum LevelRequirementVsMode
{
    LEVELREQUIREMENT_HEROIC = 70
};

#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

typedef std::map<uint32/*leaderDBGUID*/, CreatureGroup*>        CreatureGroupHolderType;

class Map : public GridRefManager<NGridType>
{
    friend class MapReference;
public:
    Map(uint32 id, time_t, uint32 InstanceId, uint8 SpawnMode, Map* _parent = NULL);
    virtual ~Map();

    MapEntry const* GetEntry() const { return i_mapEntry; }

    // currently unused for normal maps
    bool CanUnload(uint32 diff)
    {
        if (!m_unloadTimer)
            return false;

        if (m_unloadTimer <= diff)
            return true;

        m_unloadTimer -= diff;
        return false;
    }

    virtual bool AddPlayerToMap(Player*);
    virtual void RemovePlayerFromMap(Player*, bool);
    template<class T> bool AddToMap(T*);
    template<class T> void RemoveFromMap(T*, bool);

    void VisitNearbyCellsOf(WorldObject* obj, TypeContainerVisitor<Skyfire::ObjectUpdater, GridTypeMapContainer>& gridVisitor, TypeContainerVisitor<Skyfire::ObjectUpdater, WorldTypeMapContainer>& worldVisitor);
    virtual void Update(const uint32);

    float GetVisibilityRange() const { return m_VisibleDistance; }
    //function for setting up visibility distance for maps on per-type/per-Id basis
    virtual void InitVisibilityDistance();

    void PlayerRelocation(Player*, float x, float y, float z, float orientation);
    void CreatureRelocation(Creature* creature, float x, float y, float z, float ang, bool respawnRelocationOnFail = true);
    void GameObjectRelocation(GameObject* go, float x, float y, float z, float orientation, bool respawnRelocationOnFail = true);

    template<class T, class CONTAINER> void Visit(const Cell& cell, TypeContainerVisitor<T, CONTAINER>& visitor);

    bool IsRemovalGrid(float x, float y) const
    {
        GridCoord p = Skyfire::ComputeGridCoord(x, y);
        return !getNGrid(p.x_coord, p.y_coord) || getNGrid(p.x_coord, p.y_coord)->GetGridState() == GRID_STATE_REMOVAL;
    }

    bool IsGridLoaded(float x, float y) const
    {
        return IsGridLoaded(Skyfire::ComputeGridCoord(x, y));
    }

    bool GetUnloadLock(const GridCoord& p) const { return getNGrid(p.x_coord, p.y_coord)->getUnloadLock(); }
    void SetUnloadLock(const GridCoord& p, bool on) { getNGrid(p.x_coord, p.y_coord)->setUnloadExplicitLock(on); }
    void LoadGrid(float x, float y);
    bool UnloadGrid(NGridType& ngrid, bool pForce);
    virtual void UnloadAll();

    void ResetGridExpiry(NGridType& grid, float factor = 1) const
    {
        grid.ResetTimeTracker(time_t(float(i_gridExpiry) * factor));
    }

    time_t GetGridExpiry(void) const { return i_gridExpiry; }
    uint32 GetId(void) const { return i_mapEntry->MapID; }

    static bool ExistMap(uint32 mapid, int gx, int gy);
    static bool ExistVMap(uint32 mapid, int gx, int gy);

    static void InitStateMachine();
    static void DeleteStateMachine();

    Map const* GetParent() const { return m_parentMap; }

    // some calls like isInWater should not use vmaps due to processor power
    // can return INVALID_HEIGHT if under z+2 z coord not found height
    float GetHeight(float x, float y, float z, bool checkVMap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const;

    ZLiquidStatus getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, LiquidData* data = 0) const;

    uint16 GetAreaFlag(float x, float y, float z, bool* isOutdoors = 0) const;
    bool GetAreaInfo(float x, float y, float z, uint32& mogpflags, int32& adtId, int32& rootId, int32& groupId) const;

    bool IsOutdoors(float x, float y, float z) const;

    uint8 GetTerrainType(float x, float y) const;
    float GetWaterLevel(float x, float y) const;
    bool IsInWater(float x, float y, float z, LiquidData* data = 0) const;
    bool IsUnderWater(float x, float y, float z) const;

    static uint32 GetAreaIdByAreaFlag(uint16 areaflag, uint32 map_id);
    static uint32 GetZoneIdByAreaFlag(uint16 areaflag, uint32 map_id);
    static void GetZoneAndAreaIdByAreaFlag(uint32& zoneid, uint32& areaid, uint16 areaflag, uint32 map_id);

    uint32 GetAreaId(float x, float y, float z) const
    {
        return GetAreaIdByAreaFlag(GetAreaFlag(x, y, z), GetId());
    }

    uint32 GetZoneId(float x, float y, float z) const
    {
        return GetZoneIdByAreaFlag(GetAreaFlag(x, y, z), GetId());
    }

    void GetZoneAndAreaId(uint32& zoneid, uint32& areaid, float x, float y, float z) const
    {
        GetZoneAndAreaIdByAreaFlag(zoneid, areaid, GetAreaFlag(x, y, z), GetId());
    }

    void MoveAllCreaturesInMoveList();
    void MoveAllGameObjectsInMoveList();
    void RemoveAllObjectsInRemoveList();
    virtual void RemoveAllPlayers();

    // used only in MoveAllCreaturesInMoveList and ObjectGridUnloader
    bool CreatureRespawnRelocation(Creature* c, bool diffGridOnly);
    bool GameObjectRespawnRelocation(GameObject* go, bool diffGridOnly);

    // assert print helper
    bool CheckGridIntegrity(Creature* c, bool moved) const;

    uint32 GetInstanceId() const { return i_InstanceId; }
    uint8 GetSpawnMode() const { return (i_spawnMode); }
    virtual bool CanEnter(Player* /*player*/) { return true; }
    const char* GetMapName() const;

    // have meaning only for instanced map (that have set real difficulty)
    DifficultyID GetDifficulty() const { return DifficultyID(GetSpawnMode()); }
    bool IsRegularDifficulty() const { return GetDifficulty() == 0; }
    MapDifficulty const* GetMapDifficulty() const;

    bool Instanceable() const { return i_mapEntry && i_mapEntry->Instanceable(); }
    bool IsInstance() const { return IsRaid() || IsDungeon() || IsScenario(); }
    bool IsDungeon() const { return i_mapEntry && i_mapEntry->IsDungeon(); }
    bool IsScenario() const { return i_mapEntry && i_mapEntry->IsScenario(); }
    bool IsNonRaidInstance() const { return i_mapEntry && i_mapEntry->IsNonRaidInstance(); }
    bool IsRaid() const { return i_mapEntry && i_mapEntry->IsRaid(); }
    bool IsRaidOrHeroicDungeon() const { return IsRaid() || i_spawnMode == DIFFICULTY_HEROIC; }
    bool IsHeroic() const;
    bool Is25ManRaid() const;
    bool IsBattleground() const { return i_mapEntry && i_mapEntry->IsBattleground(); }
    bool IsBattleArena() const { return i_mapEntry && i_mapEntry->IsBattleArena(); }
    bool IsBattlegroundOrArena() const { return i_mapEntry && i_mapEntry->IsBattlegroundOrArena(); }
    bool GetEntrancePos(int32& mapid, float& x, float& y)
    {
        if (!i_mapEntry)
            return false;
        return i_mapEntry->GetEntrancePos(mapid, x, y);
    }

    void AddObjectToRemoveList(WorldObject* obj);
    void AddObjectToSwitchList(WorldObject* obj, bool on);
    virtual void DelayedUpdate(const uint32 diff);

    void UpdateObjectVisibility(WorldObject* obj, Cell cell, CellCoord cellpair);
    void UpdateObjectsVisibilityFor(Player* player, Cell cell, CellCoord cellpair);

    void resetMarkedCells() { marked_cells.reset(); }
    bool isCellMarked(uint32 pCellId) { return marked_cells.test(pCellId); }
    void markCell(uint32 pCellId) { marked_cells.set(pCellId); }

    bool HavePlayers() const { return !m_mapRefManager.isEmpty(); }
    uint32 GetPlayersCountExceptGMs() const;
    bool ActiveObjectsNearGrid(NGridType const& ngrid) const;

    void AddWorldObject(WorldObject* obj) { i_worldObjects.insert(obj); }
    void RemoveWorldObject(WorldObject* obj) { i_worldObjects.erase(obj); }

    void SendToPlayers(WorldPacket const* data) const;

    typedef MapRefManager PlayerList;
    PlayerList const& GetPlayers() const { return m_mapRefManager; }

    //per-map script storage
    void ScriptsStart(std::map<uint32, std::multimap<uint32, ScriptInfo> > const& scripts, uint32 id, Object* source, Object* target);
    void ScriptCommandStart(ScriptInfo const& script, uint32 delay, Object* source, Object* target);

    // must called with AddToWorld
    template<class T>
    void AddToActive(T* obj);

    // must called with RemoveFromWorld
    template<class T>
    void RemoveFromActive(T* obj);

    template<class T> void SwitchGridContainers(T* obj, bool on);
    template<class NOTIFIER> void VisitAll(const float& x, const float& y, float radius, NOTIFIER& notifier);
    template<class NOTIFIER> void VisitFirstFound(const float& x, const float& y, float radius, NOTIFIER& notifier);
    template<class NOTIFIER> void VisitWorld(const float& x, const float& y, float radius, NOTIFIER& notifier);
    template<class NOTIFIER> void VisitGrid(const float& x, const float& y, float radius, NOTIFIER& notifier);
    CreatureGroupHolderType CreatureGroupHolder;

    void UpdateIteratorBack(Player* player);

    TempSummon* SummonCreature(uint32 entry, Position const& pos, SummonPropertiesEntry const* properties = NULL, uint32 duration = 0, Unit* summoner = NULL, uint32 spellId = 0, uint32 vehId = 0);
    void SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list = NULL);
    Creature* GetCreature(uint64 guid);
    GameObject* GetGameObject(uint64 guid);
    Transport* GetTransport(uint64 guid);
    DynamicObject* GetDynamicObject(uint64 guid);

    MapInstanced* ToMapInstanced() { if (Instanceable())  return reinterpret_cast<MapInstanced*>(this); else return NULL; }
    const MapInstanced* ToMapInstanced() const { if (Instanceable())  return (const MapInstanced*)((MapInstanced*)this); else return NULL; }

    InstanceMap* ToInstanceMap() { if (IsInstance())  return reinterpret_cast<InstanceMap*>(this); else return NULL; }
    const InstanceMap* ToInstanceMap() const { if (IsInstance())  return (const InstanceMap*)((InstanceMap*)this); else return NULL; }
    float GetWaterOrGroundLevel(float x, float y, float z, float* ground = NULL, bool swim = false) const;
    float GetHeight(uint32 phasemask, float x, float y, float z, bool vmap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const;
    bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask) const;
    void Balance() { _dynamicTree.balance(); }
    void RemoveGameObjectModel(const GameObjectModel& model) { _dynamicTree.remove(model); }
    void InsertGameObjectModel(const GameObjectModel& model) { _dynamicTree.insert(model); }
    bool ContainsGameObjectModel(const GameObjectModel& model) const { return _dynamicTree.contains(model); }
    bool getObjectHitPos(uint32 phasemask, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float& ry, float& rz, float modifyDist);

    virtual uint32 GetOwnerGuildId(uint32 /*team*/ = TEAM_OTHER) const { return 0; }
    /*
        RESPAWN TIMES
    */
    time_t GetLinkedRespawnTime(uint64 guid) const;
    time_t GetCreatureRespawnTime(uint32 dbGuid) const
    {
        UNORDERED_MAP<uint32 /*dbGUID*/, time_t>::const_iterator itr = _creatureRespawnTimes.find(dbGuid);
        if (itr != _creatureRespawnTimes.end())
            return itr->second;

        return time_t(0);
    }

    time_t GetGORespawnTime(uint32 dbGuid) const
    {
        UNORDERED_MAP<uint32 /*dbGUID*/, time_t>::const_iterator itr = _goRespawnTimes.find(dbGuid);
        if (itr != _goRespawnTimes.end())
            return itr->second;

        return time_t(0);
    }

    void SaveCreatureRespawnTime(uint32 dbGuid, time_t respawnTime);
    void RemoveCreatureRespawnTime(uint32 dbGuid);
    void SaveGORespawnTime(uint32 dbGuid, time_t respawnTime);
    void RemoveGORespawnTime(uint32 dbGuid);
    void LoadRespawnTimes();
    void DeleteRespawnTimes();

    static void DeleteRespawnTimesInDB(uint16 mapId, uint32 instanceId);

    void SendInitTransports(Player* player);
    void SendRemoveTransports(Player* player);

private:
    void LoadMapAndVMap(int gx, int gy);
    void LoadVMap(int gx, int gy);
    void LoadMap(int gx, int gy, bool reload = false);
    void LoadMMap(int gx, int gy);
    GridMap* GetGrid(float x, float y);

    void SetTimer(uint32 t) { i_gridExpiry = t < MIN_GRID_DELAY ? MIN_GRID_DELAY : t; }

    void SendInitSelf(Player* player);

    bool CreatureCellRelocation(Creature* creature, Cell new_cell);
    bool GameObjectCellRelocation(GameObject* go, Cell new_cell);

    template<class T> void InitializeObject(T* obj);
    void AddCreatureToMoveList(Creature* c, float x, float y, float z, float ang);
    void RemoveCreatureFromMoveList(Creature* c);
    void AddGameObjectToMoveList(GameObject* go, float x, float y, float z, float ang);
    void RemoveGameObjectFromMoveList(GameObject* go);

    bool _creatureToMoveLock;
    std::vector<Creature*> _creaturesToMove;

    bool _gameObjectsToMoveLock;
    std::vector<GameObject*> _gameObjectsToMove;

    bool IsGridLoaded(const GridCoord&) const;
    void EnsureGridCreated(const GridCoord&);
    void EnsureGridCreated_i(const GridCoord&);
    bool EnsureGridLoaded(Cell const&);
    void EnsureGridLoadedForActiveObject(Cell const&, WorldObject* object);

    void buildNGridLinkage(NGridType* pNGridType) { pNGridType->link(this); }

    NGridType* getNGrid(uint32 x, uint32 y) const
    {
        ASSERT(x < MAX_NUMBER_OF_GRIDS && y < MAX_NUMBER_OF_GRIDS);
        return i_grids[x][y];
    }

    bool isGridObjectDataLoaded(uint32 x, uint32 y) const { return getNGrid(x, y)->isGridObjectDataLoaded(); }
    void setGridObjectDataLoaded(bool pLoaded, uint32 x, uint32 y) { getNGrid(x, y)->setGridObjectDataLoaded(pLoaded); }

    void setNGrid(NGridType* grid, uint32 x, uint32 y);
    void ScriptsProcess();

    void UpdateActiveCells(const float& x, const float& y, const uint32 t_diff);

protected:
    void SetUnloadReferenceLock(const GridCoord& p, bool on) { getNGrid(p.x_coord, p.y_coord)->setUnloadReferenceLock(on); }

    std::mutex Lock;
    std::mutex GridLock;

    MapEntry const* i_mapEntry;
    uint8 i_spawnMode;
    uint32 i_InstanceId;
    uint32 m_unloadTimer;
    float m_VisibleDistance;
    DynamicMapTree _dynamicTree;

    MapRefManager m_mapRefManager;
    MapRefManager::iterator m_mapRefIter;

    int32 m_VisibilityNotifyPeriod;

    typedef std::set<WorldObject*> ActiveNonPlayers;
    ActiveNonPlayers m_activeNonPlayers;
    ActiveNonPlayers::iterator m_activeNonPlayersIter;

    // Objects that must update even in inactive grids without activating them
    typedef std::set<Transport*> TransportsContainer;
    TransportsContainer _transports;
    TransportsContainer::iterator _transportsUpdateIter;

private:
    Player* _GetScriptPlayerSourceOrTarget(Object* source, Object* target, const ScriptInfo* scriptInfo) const;
    Creature* _GetScriptCreatureSourceOrTarget(Object* source, Object* target, const ScriptInfo* scriptInfo, bool bReverse = false) const;
    Unit* _GetScriptUnit(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const;
    Player* _GetScriptPlayer(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const;
    Creature* _GetScriptCreature(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const;
    WorldObject* _GetScriptWorldObject(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const;
    void _ScriptProcessDoor(Object* source, Object* target, const ScriptInfo* scriptInfo) const;
    GameObject* _FindGameObject(WorldObject* pWorldObject, uint32 guid) const;

    time_t i_gridExpiry;

    //used for fast base_map (e.g. MapInstanced class object) search for
    //InstanceMaps and BattlegroundMaps...
    Map* m_parentMap;

    NGridType* i_grids[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];
    GridMap* GridMaps[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];
    std::bitset<TOTAL_NUMBER_OF_CELLS_PER_MAP* TOTAL_NUMBER_OF_CELLS_PER_MAP> marked_cells;

    //these functions used to process player/mob aggro reactions and
    //visibility calculations. Highly optimized for massive calculations
    void ProcessRelocationNotifies(const uint32 diff);

    bool i_scriptLock;
    std::set<WorldObject*> i_objectsToRemove;
    std::map<WorldObject*, bool> i_objectsToSwitch;
    std::set<WorldObject*> i_worldObjects;

    typedef std::multimap<time_t, ScriptAction> ScriptScheduleMap;
    ScriptScheduleMap m_scriptSchedule;

    // Type specific code for add/remove to/from grid
    template<class T>
    void AddToGrid(T* object, Cell const& cell);

    template<class T>
    void DeleteFromWorld(T*);

    void AddToActiveHelper(WorldObject* obj)
    {
        m_activeNonPlayers.insert(obj);
    }

    void RemoveFromActiveHelper(WorldObject* obj)
    {
        // Map::Update for active object in proccess
        if (m_activeNonPlayersIter != m_activeNonPlayers.end())
        {
            ActiveNonPlayers::iterator itr = m_activeNonPlayers.find(obj);
            if (itr == m_activeNonPlayers.end())
                return;
            if (itr == m_activeNonPlayersIter)
                ++m_activeNonPlayersIter;
            m_activeNonPlayers.erase(itr);
        }
        else
            m_activeNonPlayers.erase(obj);
    }

    UNORDERED_MAP<uint32 /*dbGUID*/, time_t> _creatureRespawnTimes;
    UNORDERED_MAP<uint32 /*dbGUID*/, time_t> _goRespawnTimes;
};

enum class InstanceResetMethod
{
    INSTANCE_RESET_ALL,
    INSTANCE_RESET_CHANGE_DIFFICULTY,
    INSTANCE_RESET_GLOBAL,
    INSTANCE_RESET_GROUP_DISBAND,
    INSTANCE_RESET_GROUP_JOIN,
    INSTANCE_RESET_RESPAWN_DELAY
};

class InstanceMap : public Map
{
public:
    InstanceMap(uint32 id, time_t, uint32 InstanceId, uint8 SpawnMode, Map* _parent);
    ~InstanceMap();
    bool AddPlayerToMap(Player*) OVERRIDE;
    void RemovePlayerFromMap(Player*, bool) OVERRIDE;
    void Update(const uint32) OVERRIDE;
    void CreateInstanceData(bool load);
    bool Reset(InstanceResetMethod method);
    uint32 GetScriptId() const { return i_script_id; }
    InstanceScript* GetInstanceScript() { return i_data; }
    void PermBindAllPlayers(Player* source);
    void UnloadAll() OVERRIDE;
    bool CanEnter(Player* player) OVERRIDE;
    void SendResetWarnings(uint32 timeLeft) const;
    void SetResetSchedule(bool on);

    uint32 GetMaxPlayers() const;
    uint32 GetMaxResetDelay() const;

    virtual void InitVisibilityDistance() OVERRIDE;
private:
    bool m_resetAfterUnload;
    bool m_unloadWhenEmpty;
    InstanceScript* i_data;
    uint32 i_script_id;
};

class BattlegroundMap : public Map
{
public:
    BattlegroundMap(uint32 id, time_t, uint32 InstanceId, Map* _parent, uint8 spawnMode);
    ~BattlegroundMap();

    bool AddPlayerToMap(Player*) OVERRIDE;
    void RemovePlayerFromMap(Player*, bool) OVERRIDE;
    bool CanEnter(Player* player) OVERRIDE;
    void SetUnload();
    //void UnloadAll(bool pForce);
    void RemoveAllPlayers() OVERRIDE;

    virtual void InitVisibilityDistance() OVERRIDE;
    Battleground* GetBG() { return m_bg; }
    void SetBG(Battleground* bg) { m_bg = bg; }
private:
    Battleground* m_bg;
};

template<class T, class CONTAINER>
inline void Map::Visit(Cell const& cell, TypeContainerVisitor<T, CONTAINER>& visitor)
{
    const uint32 x = cell.GridX();
    const uint32 y = cell.GridY();
    const uint32 cell_x = cell.CellX();
    const uint32 cell_y = cell.CellY();

    if (!cell.NoCreate() || IsGridLoaded(GridCoord(x, y)))
    {
        EnsureGridLoaded(cell);
        getNGrid(x, y)->VisitGrid(cell_x, cell_y, visitor);
    }
}

template<class NOTIFIER>
inline void Map::VisitAll(float const& x, float const& y, float radius, NOTIFIER& notifier)
{
    CellCoord p(Skyfire::ComputeCellCoord(x, y));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<NOTIFIER, WorldTypeMapContainer> world_object_notifier(notifier);
    cell.Visit(p, world_object_notifier, *this, radius, x, y);
    TypeContainerVisitor<NOTIFIER, GridTypeMapContainer >  grid_object_notifier(notifier);
    cell.Visit(p, grid_object_notifier, *this, radius, x, y);
}

// should be used with Searcher notifiers, tries to search world if nothing found in grid
template<class NOTIFIER>
inline void Map::VisitFirstFound(const float& x, const float& y, float radius, NOTIFIER& notifier)
{
    CellCoord p(Skyfire::ComputeCellCoord(x, y));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<NOTIFIER, WorldTypeMapContainer> world_object_notifier(notifier);
    cell.Visit(p, world_object_notifier, *this, radius, x, y);
    if (!notifier.i_object)
    {
        TypeContainerVisitor<NOTIFIER, GridTypeMapContainer >  grid_object_notifier(notifier);
        cell.Visit(p, grid_object_notifier, *this, radius, x, y);
    }
}

template<class NOTIFIER>
inline void Map::VisitWorld(const float& x, const float& y, float radius, NOTIFIER& notifier)
{
    CellCoord p(Skyfire::ComputeCellCoord(x, y));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<NOTIFIER, WorldTypeMapContainer> world_object_notifier(notifier);
    cell.Visit(p, world_object_notifier, *this, radius, x, y);
}

template<class NOTIFIER>
inline void Map::VisitGrid(const float& x, const float& y, float radius, NOTIFIER& notifier)
{
    CellCoord p(Skyfire::ComputeCellCoord(x, y));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<NOTIFIER, GridTypeMapContainer >  grid_object_notifier(notifier);
    cell.Visit(p, grid_object_notifier, *this, radius, x, y);
}
#endif
