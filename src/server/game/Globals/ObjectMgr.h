/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SF_OBJECTMGR_H
#define SF_OBJECTMGR_H

#include "Bag.h"
#include "ConditionMgr.h"
#include "Containers.h"
#include "Corpse.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "DynamicObject.h"
#include "GameObject.h"
#include "ItemPrototype.h"
#include "Log.h"
#include "Mail.h"
#include "Map.h"
#include "NPCHandler.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include "ObjectDefines.h"
#include "QuestDef.h"
#include "TemporarySummon.h"
#include "VehicleDefines.h"
#include <ace/Singleton.h>
#include <functional>
#include <limits>
#include <map>
#include <string>

class Item;
class PhaseMgr;
struct AccessRequirement;
struct PlayerInfo;
struct PlayerLevelInfo;

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push, N), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

struct PageText
{
    PageText() : NextPage(0) { }
    std::string Text;
    uint16 NextPage;
};

/// Key for storing temp summon data in TempSummonDataContainer
struct TempSummonGroupKey
{
    TempSummonGroupKey(uint32 summonerEntry, SummonerType summonerType, uint8 group)
        : _summonerEntry(summonerEntry), _summonerType(summonerType), _summonGroup(group)
    {
    }

    bool operator<(TempSummonGroupKey const& rhs) const
    {
        // memcmp is only reliable if struct doesn't have any padding (packed)
        return memcmp(this, &rhs, sizeof(TempSummonGroupKey)) < 0;
    }

private:
    uint32 _summonerEntry;      ///< Summoner's entry
    SummonerType _summonerType; ///< Summoner's type, see SummonerType for available types
    uint8 _summonGroup;         ///< Summon's group id
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

// DB scripting commands
enum class ScriptCommands
{
    SCRIPT_COMMAND_TALK = 0,                // source/target = Creature, target = any, datalong = talk type (0=say, 1=whisper, 2=yell, 3=emote text, 4=boss emote text), datalong2 & 1 = player talk (instead of creature), dataint = string_id
    SCRIPT_COMMAND_EMOTE = 1,                // source/target = Creature, datalong = emote id, datalong2 = 0: set emote state; > 0: play emote state
    SCRIPT_COMMAND_FIELD_SET = 2,                // source/target = Creature, datalong = field id, datalog2 = value
    SCRIPT_COMMAND_MOVE_TO = 3,                // source/target = Creature, datalong2 = time to reach, x/y/z = destination
    SCRIPT_COMMAND_FLAG_SET = 4,                // source/target = Creature, datalong = field id, datalog2 = bitmask
    SCRIPT_COMMAND_FLAG_REMOVE = 5,                // source/target = Creature, datalong = field id, datalog2 = bitmask
    SCRIPT_COMMAND_TELEPORT_TO = 6,                // source/target = Creature/Player (see datalong2), datalong = map_id, datalong2 = 0: Player; 1: Creature, x/y/z = destination, o = orientation
    SCRIPT_COMMAND_QUEST_EXPLORED = 7,                // target/source = Player, target/source = GO/Creature, datalong = quest id, datalong2 = distance or 0
    SCRIPT_COMMAND_KILL_CREDIT = 8,                // target/source = Player, datalong = creature entry, datalong2 = 0: personal credit, 1: group credit
    SCRIPT_COMMAND_RESPAWN_GAMEOBJECT = 9,                // source = WorldObject (summoner), datalong = GO guid, datalong2 = despawn delay
    SCRIPT_COMMAND_TEMP_SUMMON_CREATURE = 10,               // source = WorldObject (summoner), datalong = creature entry, datalong2 = despawn delay, x/y/z = summon position, o = orientation
    SCRIPT_COMMAND_OPEN_DOOR = 11,               // source = Unit, datalong = GO guid, datalong2 = reset delay (min 15)
    SCRIPT_COMMAND_CLOSE_DOOR = 12,               // source = Unit, datalong = GO guid, datalong2 = reset delay (min 15)
    SCRIPT_COMMAND_ACTIVATE_OBJECT = 13,               // source = Unit, target = GO
    SCRIPT_COMMAND_REMOVE_AURA = 14,               // source (datalong2 != 0) or target (datalong2 == 0) = Unit, datalong = spell id
    SCRIPT_COMMAND_CAST_SPELL = 15,               // source and/or target = Unit, datalong2 = cast direction (0: s->t 1: s->s 2: t->t 3: t->s 4: s->creature with dataint entry), dataint & 1 = triggered flag
    SCRIPT_COMMAND_PLAY_SOUND = 16,               // source = WorldObject, target = none/Player, datalong = sound id, datalong2 (bitmask: 0/1=anyone/player, 0/2=without/with distance dependency, so 1|2 = 3 is target with distance dependency)
    SCRIPT_COMMAND_CREATE_ITEM = 17,               // target/source = Player, datalong = item entry, datalong2 = amount
    SCRIPT_COMMAND_DESPAWN_SELF = 18,               // target/source = Creature, datalong = despawn delay

    SCRIPT_COMMAND_LOAD_PATH = 20,               // source = Unit, datalong = path id, datalong2 = is repeatable
    SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT = 21,               // source = WorldObject (if present used as a search center), datalong = script id, datalong2 = unit lowguid, dataint = script table to use (see ScriptsType)
    SCRIPT_COMMAND_KILL = 22,               // source/target = Creature, dataint = remove corpse attribute

    // Skyfire only
    SCRIPT_COMMAND_ORIENTATION = 30,               // source = Unit, target (datalong > 0) = Unit, datalong = > 0 turn source to face target, o = orientation
    SCRIPT_COMMAND_EQUIP = 31,               // soucre = Creature, datalong = equipment id
    SCRIPT_COMMAND_MODEL = 32,               // source = Creature, datalong = model id
    SCRIPT_COMMAND_CLOSE_GOSSIP = 33,               // source = Player
    SCRIPT_COMMAND_PLAYMOVIE = 34                // source = Player, datalong = movie id
};

// Benchmarked: Faster than UNORDERED_MAP (insert/find)
typedef std::map<uint32, PageText> PageTextContainer;

// Benchmarked: Faster than std::map (insert/find)
typedef UNORDERED_MAP<uint16, InstanceTemplate> InstanceTemplateContainer;

struct GameTele
{
    GameTele() : position_x(0.0f), position_y(0.0f), position_z(0.0f), orientation(0.0f), mapId(0) { }
    float  position_x;
    float  position_y;
    float  position_z;
    float  orientation;
    uint32 mapId;
    std::string name;
    std::wstring wnameLow;
};

typedef UNORDERED_MAP<uint32, GameTele > GameTeleContainer;

enum class ScriptsType
{
    SCRIPTS_SPELL = 1,
    SCRIPTS_EVENT = 2,
    SCRIPTS_WAYPOINT = 3
};

enum eScriptFlags
{
    // Talk Flags
    SF_TALK_USE_PLAYER = 0x1,

    // Emote flags
    SF_EMOTE_USE_STATE = 0x1,

    // TeleportTo flags
    SF_TELEPORT_USE_CREATURE = 0x1,

    // KillCredit flags
    SF_KILLCREDIT_REWARD_GROUP = 0x1,

    // RemoveAura flags
    SF_REMOVEAURA_REVERSE = 0x1,

    // CastSpell flags
    SF_CASTSPELL_SOURCE_TO_TARGET = 0,
    SF_CASTSPELL_SOURCE_TO_SOURCE = 1,
    SF_CASTSPELL_TARGET_TO_TARGET = 2,
    SF_CASTSPELL_TARGET_TO_SOURCE = 3,
    SF_CASTSPELL_SEARCH_CREATURE = 4,
    SF_CASTSPELL_TRIGGERED = 0x1,

    // PlaySound flags
    SF_PLAYSOUND_TARGET_PLAYER = 0x1,
    SF_PLAYSOUND_DISTANCE_SOUND = 0x2,

    // Orientation flags
    SF_ORIENTATION_FACE_TARGET = 0x1
};

struct ScriptInfo
{
    ScriptsType type;
    uint32 id;
    uint32 delay;
    ScriptCommands command;

    union
    {
        struct
        {
            uint32 nData[3];
            float  fData[4];
        } Raw;

        struct                      // SCRIPT_COMMAND_TALK (0)
        {
            uint32 ChatType;        // datalong
            uint32 Flags;           // datalong2
            int32  TextID;          // dataint
        } Talk;

        struct                      // SCRIPT_COMMAND_EMOTE (1)
        {
            uint32 EmoteID;         // datalong
            uint32 Flags;           // datalong2
        } Emote;

        struct                      // SCRIPT_COMMAND_FIELD_SET (2)
        {
            uint32 FieldID;         // datalong
            uint32 FieldValue;      // datalong2
        } FieldSet;

        struct                      // SCRIPT_COMMAND_MOVE_TO (3)
        {
            uint32 Unused1;         // datalong
            uint32 TravelTime;      // datalong2
            int32  Unused2;         // dataint

            float DestX;
            float DestY;
            float DestZ;
        } MoveTo;

        struct                      // SCRIPT_COMMAND_FLAG_SET (4)
                                    // SCRIPT_COMMAND_FLAG_REMOVE (5)
        {
            uint32 FieldID;         // datalong
            uint32 FieldValue;      // datalong2
        } FlagToggle;

        struct                      // SCRIPT_COMMAND_TELEPORT_TO (6)
        {
            uint32 MapID;           // datalong
            uint32 Flags;           // datalong2
            int32  Unused1;         // dataint

            float DestX;
            float DestY;
            float DestZ;
            float Orientation;
        } TeleportTo;

        struct                      // SCRIPT_COMMAND_QUEST_EXPLORED (7)
        {
            uint32 QuestID;         // datalong
            uint32 Distance;        // datalong2
        } QuestExplored;

        struct                      // SCRIPT_COMMAND_KILL_CREDIT (8)
        {
            uint32 CreatureEntry;   // datalong
            uint32 Flags;           // datalong2
        } KillCredit;

        struct                      // SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (9)
        {
            uint32 GOGuid;          // datalong
            uint32 DespawnDelay;    // datalong2
        } RespawnGameobject;

        struct                      // SCRIPT_COMMAND_TEMP_SUMMON_CREATURE (10)
        {
            uint32 CreatureEntry;   // datalong
            uint32 DespawnDelay;    // datalong2
            int32  Unused1;         // dataint

            float PosX;
            float PosY;
            float PosZ;
            float Orientation;
        } TempSummonCreature;

        struct                      // SCRIPT_COMMAND_CLOSE_DOOR (12)
                                    // SCRIPT_COMMAND_OPEN_DOOR (11)
        {
            uint32 GOGuid;          // datalong
            uint32 ResetDelay;      // datalong2
        } ToggleDoor;

        // SCRIPT_COMMAND_ACTIVATE_OBJECT (13)

        struct                      // SCRIPT_COMMAND_REMOVE_AURA (14)
        {
            uint32 SpellID;         // datalong
            uint32 Flags;           // datalong2
        } RemoveAura;

        struct                      // SCRIPT_COMMAND_CAST_SPELL (15)
        {
            uint32 SpellID;         // datalong
            uint32 Flags;           // datalong2
            int32  CreatureEntry;   // dataint

            float SearchRadius;
        } CastSpell;

        struct                      // SCRIPT_COMMAND_PLAY_SOUND (16)
        {
            uint32 SoundID;         // datalong
            uint32 Flags;           // datalong2
        } PlaySound;

        struct                      // SCRIPT_COMMAND_CREATE_ITEM (17)
        {
            uint32 ItemEntry;       // datalong
            uint32 Amount;          // datalong2
        } CreateItem;

        struct                      // SCRIPT_COMMAND_DESPAWN_SELF (18)
        {
            uint32 DespawnDelay;    // datalong
        } DespawnSelf;

        struct                      // SCRIPT_COMMAND_LOAD_PATH (20)
        {
            uint32 PathID;          // datalong
            uint32 IsRepeatable;    // datalong2
        } LoadPath;

        struct                      // SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT (21)
        {
            uint32 CreatureEntry;   // datalong
            uint32 ScriptID;        // datalong2
            uint32 ScriptType;      // dataint
        } CallScript;

        struct                      // SCRIPT_COMMAND_KILL (22)
        {
            uint32 Unused1;         // datalong
            uint32 Unused2;         // datalong2
            int32  RemoveCorpse;    // dataint
        } Kill;

        struct                      // SCRIPT_COMMAND_ORIENTATION (30)
        {
            uint32 Flags;           // datalong
            uint32 Unused1;         // datalong2
            int32  Unused2;         // dataint

            float Unused3;
            float Unused4;
            float Unused5;
            float Orientation;
        } Orientation;

        struct                      // SCRIPT_COMMAND_EQUIP (31)
        {
            uint32 EquipmentID;     // datalong
        } Equip;

        struct                      // SCRIPT_COMMAND_MODEL (32)
        {
            uint32 ModelID;         // datalong
        } Model;

        // SCRIPT_COMMAND_CLOSE_GOSSIP (33)

        struct                      // SCRIPT_COMMAND_PLAYMOVIE (34)
        {
            uint32 MovieID;         // datalong
        } PlayMovie;
    };

    std::string GetDebugInfo() const;
};

typedef std::multimap<uint32, ScriptInfo> ScriptMap;
typedef std::map<uint32, ScriptMap > ScriptMapMap;
typedef std::multimap<uint32, uint32> SpellScriptsContainer;
typedef std::pair<SpellScriptsContainer::iterator, SpellScriptsContainer::iterator> SpellScriptsBounds;
extern ScriptMapMap sSpellScripts;
extern ScriptMapMap sEventScripts;
extern ScriptMapMap sWaypointScripts;

std::string GetScriptsTableNameByType(ScriptsType type);
ScriptMapMap* GetScriptsMapByType(ScriptsType type);
std::string GetScriptCommandName(ScriptCommands command);

struct SpellClickInfo
{
    uint32 spellId;
    uint8 castFlags;
    SpellClickUserTypes userType;

    // helpers
    bool IsFitToRequirements(Unit const* clicker, Unit const* clickee) const;
};

typedef std::multimap<uint32, SpellClickInfo> SpellClickInfoContainer;
typedef std::pair<SpellClickInfoContainer::const_iterator, SpellClickInfoContainer::const_iterator> SpellClickInfoMapBounds;

struct AreaTriggerStruct
{
    uint32 target_mapId;
    float  target_X;
    float  target_Y;
    float  target_Z;
    float  target_Orientation;
};

typedef std::set<uint32> CellGuidSet;
typedef std::map<uint32/*player guid*/, uint32/*instance*/> CellCorpseSet;
struct CellObjectGuids
{
    CellGuidSet creatures;
    CellGuidSet gameobjects;
    CellCorpseSet corpses;
};
typedef UNORDERED_MAP<uint32/*cell_id*/, CellObjectGuids> CellObjectGuidsMap;
typedef UNORDERED_MAP<uint32/*(mapid, spawnMode) pair*/, CellObjectGuidsMap> MapObjectGuids;

// Skyfire string ranges
#define MIN_SKYFIRE_STRING_ID           1                    // 'skyfire_string'
#define MAX_SKYFIRE_STRING_ID           2000000000
#define MIN_DB_SCRIPT_STRING_ID        MAX_SKYFIRE_STRING_ID // 'db_script_string'
#define MAX_DB_SCRIPT_STRING_ID        2000010000
#define MIN_CREATURE_AI_TEXT_STRING_ID (-1)                 // 'creature_ai_texts'
#define MAX_CREATURE_AI_TEXT_STRING_ID (-1000000)

// Skyfire Trainer Reference start range
#define SKYFIRE_TRAINER_START_REF      200000

struct SkyFireStringLocale
{
    StringVector Content;
};

typedef std::map<uint64, uint64> LinkedRespawnContainer;
typedef UNORDERED_MAP<uint32, CreatureData> CreatureDataContainer;
typedef UNORDERED_MAP<uint32, GameObjectData> GameObjectDataContainer;
typedef std::map<TempSummonGroupKey, std::vector<TempSummonData> > TempSummonDataContainer;
typedef UNORDERED_MAP<uint32, CreatureLocale> CreatureLocaleContainer;
typedef UNORDERED_MAP<uint32, GameObjectLocale> GameObjectLocaleContainer;
typedef UNORDERED_MAP<uint32, ItemLocale> ItemLocaleContainer;
typedef UNORDERED_MAP<uint32, QuestLocale> QuestLocaleContainer;
typedef UNORDERED_MAP<uint32, NpcTextLocale> NpcTextLocaleContainer;
typedef UNORDERED_MAP<uint32, PageTextLocale> PageTextLocaleContainer;
typedef UNORDERED_MAP<int32, SkyFireStringLocale> SkyFireStringLocaleContainer;
typedef UNORDERED_MAP<uint32, GossipMenuItemsLocale> GossipMenuItemsLocaleContainer;
typedef UNORDERED_MAP<uint32, PointOfInterestLocale> PointOfInterestLocaleContainer;
typedef UNORDERED_MAP<uint32, QuestObjectiveLocale> QuestObjectiveLocaleContainer;

typedef std::multimap<uint32, uint32> QuestRelations; // unit/go -> quest
typedef std::multimap<uint32, uint32> QuestRelationsReverse; // quest -> unit/go
typedef std::pair<QuestRelations::const_iterator, QuestRelations::const_iterator> QuestRelationBounds;
typedef std::pair<QuestRelationsReverse::const_iterator, QuestRelationsReverse::const_iterator> QuestRelationReverseBounds;

struct PetLevelInfo
{
    PetLevelInfo() : health(0), mana(0), armor(0) { for (uint8 i = 0; i < MAX_STATS; ++i) stats[i] = 0; }

    uint16 stats[MAX_STATS] = { };
    uint16 health;
    uint16 mana;
    uint16 armor;
};

struct MailLevelReward
{
    MailLevelReward() : raceMask(0), mailTemplateId(0), senderEntry(0) { }
    MailLevelReward(uint32 _raceMask, uint32 _mailTemplateId, uint32 _senderEntry) : raceMask(_raceMask), mailTemplateId(_mailTemplateId), senderEntry(_senderEntry) { }

    uint32 raceMask;
    uint32 mailTemplateId;
    uint32 senderEntry;
};

typedef std::list<MailLevelReward> MailLevelRewardList;
typedef UNORDERED_MAP<uint8, MailLevelRewardList> MailLevelRewardContainer;

// We assume the rate is in general the same for all three types below, but chose to keep three for scalability and customization
struct RepRewardRate
{
    float questRate;            // We allow rate = 0.0 in database. For this case, it means that
    float questDailyRate;
    float questWeeklyRate;
    float questMonthlyRate;
    float questRepeatableRate;
    float creatureRate;         // no reputation are given at all for this faction/rate type.
    float spellRate;
};

struct ReputationOnKillEntry
{
    uint32 RepFaction1;
    uint32 RepFaction2;
    uint32 ReputationMaxCap1;
    int32 RepValue1;
    uint32 ReputationMaxCap2;
    int32 RepValue2;
    bool IsTeamAward1;
    bool IsTeamAward2;
    bool TeamDependent;
};

struct RepSpilloverTemplate
{
    uint32 faction[MAX_SPILLOVER_FACTIONS];
    float faction_rate[MAX_SPILLOVER_FACTIONS];
    uint32 faction_rank[MAX_SPILLOVER_FACTIONS];
};

struct PointOfInterest
{
    uint32 entry;
    float x;
    float y;
    uint32 icon;
    uint32 flags;
    uint32 data;
    std::string icon_name;
};

struct GossipMenuItems
{
    uint32          MenuId;
    uint32          OptionIndex;
    uint8           OptionIcon;
    std::string     OptionText;
    uint32          OptionType;
    uint32          OptionNpcflag;
    uint32          ActionMenuId;
    uint32          ActionPoiId;
    bool            BoxCoded;
    uint32          BoxMoney;
    std::string     BoxText;
    ConditionList   Conditions;
};

struct GossipMenus
{
    uint32          entry;
    uint32          text_id;
    ConditionList   conditions;
};

typedef std::multimap<uint32, GossipMenus> GossipMenusContainer;
typedef std::pair<GossipMenusContainer::const_iterator, GossipMenusContainer::const_iterator> GossipMenusMapBounds;
typedef std::pair<GossipMenusContainer::iterator, GossipMenusContainer::iterator> GossipMenusMapBoundsNonConst;
typedef std::multimap<uint32, GossipMenuItems> GossipMenuItemsContainer;
typedef std::pair<GossipMenuItemsContainer::const_iterator, GossipMenuItemsContainer::const_iterator> GossipMenuItemsMapBounds;
typedef std::pair<GossipMenuItemsContainer::iterator, GossipMenuItemsContainer::iterator> GossipMenuItemsMapBoundsNonConst;

struct QuestPOIPoint
{
    int32 x;
    int32 y;

    QuestPOIPoint() : x(0), y(0) { }
    QuestPOIPoint(int32 _x, int32 _y) : x(_x), y(_y) { }
};

struct QuestPOI
{
    uint32 Id;
    int32 ObjectiveIndex;
    uint32 MapId;
    uint32 AreaId;
    uint32 FloorId;
    uint32 Unk3;
    uint32 Unk4;
    std::vector<QuestPOIPoint> points;

    QuestPOI() : Id(0), ObjectiveIndex(0), MapId(0), AreaId(0), FloorId(0), Unk3(0), Unk4(0) { }
    QuestPOI(uint32 id, int32 objIndex, uint32 mapId, uint32 areaId, uint32 floorId, uint32 unk3, uint32 unk4) : Id(id), ObjectiveIndex(objIndex), MapId(mapId), AreaId(areaId), FloorId(floorId), Unk3(unk3), Unk4(unk4) { }
};

typedef std::vector<QuestPOI> QuestPOIVector;
typedef UNORDERED_MAP<uint32, QuestPOIVector> QuestPOIContainer;

struct SceneTemplate
{
    uint32 SceneId;
    uint32 PlaybackFlags;
    uint32 ScenePackageId;
    uint32 ScriptId;
};

typedef UNORDERED_MAP<uint32, SceneTemplate> SceneTemplateContainer;

struct GraveYardData
{
    uint32 safeLocId;
    uint32 team;
};

typedef std::multimap<uint32, GraveYardData> GraveYardContainer;
typedef UNORDERED_MAP<uint32 /* graveyard Id */, float /* orientation */> GraveyardOrientationContainer;
typedef std::pair<GraveYardContainer::const_iterator, GraveYardContainer::const_iterator> GraveYardMapBounds;
typedef std::pair<GraveYardContainer::iterator, GraveYardContainer::iterator> GraveYardMapBoundsNonConst;

typedef UNORDERED_MAP<uint32, VendorItemData> CacheVendorItemContainer;
typedef UNORDERED_MAP<uint32, TrainerSpellData> CacheTrainerSpellContainer;

enum SkillRangeType
{
    SKILL_RANGE_LANGUAGE,                                   // 300..300
    SKILL_RANGE_LEVEL,                                      // 1..max skill for level
    SKILL_RANGE_MONO,                                       // 1..1, grey monolite bar
    SKILL_RANGE_RANK,                                       // 1..skill for known rank
    SKILL_RANGE_NONE                                        // 0..0 always
};

SkillRangeType GetSkillRangeType(SkillLineEntry const* pSkill, bool racial);

#define MAX_PLAYER_NAME          12                         // max allowed by client name length
#define MAX_INTERNAL_PLAYER_NAME 15                         // max server internal player name length (> MAX_PLAYER_NAME for support declined names)
#define MAX_PET_NAME             12                         // max allowed by client name length
#define MAX_CHARTER_NAME         24                         // max allowed by client name length

bool normalizePlayerName(std::string& name);

struct LanguageDesc
{
    Language lang_id;
    uint32   spell_id;
    uint32   skill_id;
};

extern LanguageDesc lang_description[LANGUAGES_COUNT];
LanguageDesc const* GetLanguageDescByID(Language lang);

enum class EncounterCreditType
{
    ENCOUNTER_CREDIT_KILL_CREATURE = 0,
    ENCOUNTER_CREDIT_CAST_SPELL = 1
};

struct DungeonEncounter
{
    DungeonEncounter(DungeonEncounterEntry const* _dbcEntry, EncounterCreditType _creditType, uint32 _creditEntry, uint32 _lastEncounterDungeon)
        : dbcEntry(_dbcEntry), creditType(_creditType), creditEntry(_creditEntry), lastEncounterDungeon(_lastEncounterDungeon) { }

    DungeonEncounterEntry const* dbcEntry;
    EncounterCreditType creditType;
    uint32 creditEntry;
    uint32 lastEncounterDungeon;
};

typedef std::list<DungeonEncounter const*> DungeonEncounterList;
typedef UNORDERED_MAP<uint32, DungeonEncounterList> DungeonEncounterContainer;

struct HotfixInfo
{
    HotfixInfo() : Type(0), Timestamp(0), Entry(0) {}
    uint32 Type;
    uint32 Timestamp;
    uint32 Entry;
};

typedef std::vector<HotfixInfo> HotfixData;
typedef std::map<uint32, uint32> QuestObjectiveLookupMap;

struct ResearchDigsiteInfo
{
    uint32 digsiteId;
    uint32 branchId;
    uint32 requiredSkillValue;
    uint32 requiredLevel;
};

typedef std::list<ResearchDigsiteInfo> ResearchDigsiteList;
typedef UNORDERED_MAP<uint32 /*mapId*/, ResearchDigsiteList> ResearchDigsiteContainer;

struct ArchaeologyFindInfo
{
    uint32 guid;
    uint32 goEntry;
    float x;
    float y;
    float z;
};

typedef std::list<ArchaeologyFindInfo> ArchaeologyFindList;
typedef UNORDERED_MAP<uint32 /*digsiteId*/, ArchaeologyFindList> ArchaeologyFindContainer;

struct ResearchProjectRequirements
{
    uint32 requiredSkillValue;
    float chance;
};

typedef UNORDERED_MAP<uint32, ResearchProjectRequirements> ResearchProjectRequirementContainer;

struct PhaseInfoStruct
{
    uint32 id;
    ConditionList Conditions;
};

typedef UNORDERED_MAP<uint32, std::vector<PhaseInfoStruct>> TerrainPhaseInfo;
typedef UNORDERED_MAP<uint32, std::vector<uint32>> TerrainUIPhaseInfo;
typedef UNORDERED_MAP<uint32, std::vector<PhaseInfoStruct>> PhaseInfo;
class PlayerDumpReader;

class ObjectMgr
{
    friend class PlayerDumpReader;
    friend class ACE_Singleton<ObjectMgr, ACE_Null_Mutex>;

private:
    ObjectMgr();
    ~ObjectMgr();

public:
    typedef UNORDERED_MAP<uint32, Item*> ItemMap;

    typedef UNORDERED_MAP<uint32, Quest*> QuestMap;

    typedef UNORDERED_MAP<uint32, AreaTriggerStruct> AreaTriggerContainer;

    typedef UNORDERED_MAP<uint32, uint32> AreaTriggerScriptContainer;

    typedef UNORDERED_MAP<uint32, AccessRequirement*> AccessRequirementContainer;

    typedef UNORDERED_MAP<uint32, RepRewardRate > RepRewardRateContainer;
    typedef UNORDERED_MAP<uint32, ReputationOnKillEntry> RepOnKillContainer;
    typedef UNORDERED_MAP<uint32, RepSpilloverTemplate> RepSpilloverTemplateContainer;

    typedef UNORDERED_MAP<uint32, PointOfInterest> PointOfInterestContainer;

    typedef std::vector<std::string> ScriptNameContainer;

    typedef std::map<uint32, uint32> CharacterConversionMap;

    Player* GetPlayerByLowGUID(uint32 lowguid) const;

    GameObjectTemplate const* GetGameObjectTemplate(uint32 entry);
    GameObjectTemplateContainer const* GetGameObjectTemplates() const { return &_gameObjectTemplateStore; }
    int LoadReferenceVendor(int32 vendor, int32 item, uint8 type, std::set<uint32>* skip_vendors);

    void LoadGameObjectTemplate();

    CreatureTemplate const* GetCreatureTemplate(uint32 entry);
    CreatureTemplateContainer const* GetCreatureTemplates() const { return &_creatureTemplateStore; }
    CreatureModelInfo const* GetCreatureModelInfo(uint32 modelId);
    CreatureModelInfo const* GetCreatureModelRandomGender(uint32* displayID);
    static uint32 ChooseDisplayId(CreatureTemplate const* cinfo, CreatureData const* data = NULL);
    static void ChooseCreatureFlags(CreatureTemplate const* cinfo, uint32& npcflag, uint32& unit_flags, uint32& dynamicflags, CreatureData const* data = NULL);
    EquipmentInfo const* GetEquipmentInfo(uint32 entry, int8& id);
    CreatureAddon const* GetCreatureAddon(uint32 lowguid);
    CreatureAddon const* GetCreatureTemplateAddon(uint32 entry);
    ItemTemplate const* GetItemTemplate(uint32 entry);
    ItemTemplateContainer const* GetItemTemplateStore() const { return &_itemTemplateStore; }

    InstanceTemplate const* GetInstanceTemplate(uint32 mapId);

    PetLevelInfo const* GetPetLevelInfo(uint32 creature_id, uint8 level) const;

    void GetPlayerClassLevelInfo(uint32 class_, uint8 level, uint32& baseHP, uint32& baseMana) const;

    PlayerInfo const* GetPlayerInfo(uint32 race, uint32 class_) const;

    void GetPlayerLevelInfo(uint32 race, uint32 class_, uint8 level, PlayerLevelInfo* info) const;

    uint64 GetPlayerGUIDByName(std::string const& name) const;

    /**
    * Retrieves the player name by guid.
    *
    * If the player is online, the name is retrieved immediately otherwise
    * a database query is done.
    *
    * @remark Use sWorld->GetCharacterNameData because it doesn't require a database query when player is offline
    *
    * @param guid player full guid
    * @param name returned name
    *
    * @return true if player was found, false otherwise
    */
    bool GetPlayerNameByGUID(uint64 guid, std::string& name) const;
    uint32 GetPlayerTeamByGUID(uint64 guid) const;
    uint32 GetPlayerAccountIdByGUID(uint64 guid) const;
    uint32 GetPlayerAccountIdByPlayerName(std::string const& name) const;

    uint32 GetNearestTaxiNode(float x, float y, float z, uint32 mapid, uint32 team);
    void GetTaxiPath(uint32 source, uint32 destination, uint32& path, uint32& cost);
    uint32 GetTaxiMountDisplayId(uint32 id, uint32 team, bool allowed_alt_team = false);

    Quest const* GetQuestTemplate(uint32 quest_id) const
    {
        QuestMap::const_iterator itr = _questTemplates.find(quest_id);
        return itr != _questTemplates.end() ? itr->second : NULL;
    }

    QuestMap const& GetQuestTemplates() const { return _questTemplates; }

    uint32 GetQuestForAreaTrigger(uint32 Trigger_ID) const
    {
        QuestAreaTriggerContainer::const_iterator itr = _questAreaTriggerStore.find(Trigger_ID);
        if (itr != _questAreaTriggerStore.end())
            return itr->second;
        return 0;
    }

    bool IsTavernAreaTrigger(uint32 Trigger_ID) const
    {
        return _tavernAreaTriggerStore.find(Trigger_ID) != _tavernAreaTriggerStore.end();
    }

    bool IsGameObjectForQuests(uint32 entry) const
    {
        return _gameObjectForQuestStore.find(entry) != _gameObjectForQuestStore.end();
    }

    GossipText const* GetGossipText(uint32 Text_ID) const;

    WorldSafeLocsEntry const* GetDefaultGraveYard(uint32 team);
    WorldSafeLocsEntry const* GetClosestGraveYard(float x, float y, float z, uint32 MapId, uint32 team);
    bool AddGraveYardLink(uint32 id, uint32 zoneId, uint32 team, bool persist = true);
    void RemoveGraveYardLink(uint32 id, uint32 zoneId, uint32 team, bool persist = false);
    void LoadGraveyardZones();
    GraveYardData const* FindGraveYardData(uint32 id, uint32 zone);

    AreaTriggerStruct const* GetAreaTrigger(uint32 trigger) const
    {
        AreaTriggerContainer::const_iterator itr = _areaTriggerStore.find(trigger);
        if (itr != _areaTriggerStore.end())
            return &itr->second;
        return NULL;
    }

    AccessRequirement const* GetAccessRequirement(uint32 mapid, DifficultyID difficulty) const
    {
        AccessRequirementContainer::const_iterator itr = _accessRequirementStore.find(MAKE_PAIR32(mapid, difficulty));
        if (itr != _accessRequirementStore.end())
            return itr->second;
        return NULL;
    }

    AreaTriggerStruct const* GetGoBackTrigger(uint32 Map) const;
    AreaTriggerStruct const* GetMapEntranceTrigger(uint32 Map) const;

    uint32 GetAreaTriggerScriptId(uint32 trigger_id);
    SpellScriptsBounds GetSpellScriptsBounds(uint32 spellId);

    RepRewardRate const* GetRepRewardRate(uint32 factionId) const
    {
        RepRewardRateContainer::const_iterator itr = _repRewardRateStore.find(factionId);
        if (itr != _repRewardRateStore.end())
            return &itr->second;

        return NULL;
    }

    ReputationOnKillEntry const* GetReputationOnKilEntry(uint32 id) const
    {
        RepOnKillContainer::const_iterator itr = _repOnKillStore.find(id);
        if (itr != _repOnKillStore.end())
            return &itr->second;
        return NULL;
    }

    int32 GetBaseReputationOf(FactionEntry const* factionEntry, uint8 race, uint8 playerClass);

    RepSpilloverTemplate const* GetRepSpilloverTemplate(uint32 factionId) const
    {
        RepSpilloverTemplateContainer::const_iterator itr = _repSpilloverTemplateStore.find(factionId);
        if (itr != _repSpilloverTemplateStore.end())
            return &itr->second;

        return NULL;
    }

    PointOfInterest const* GetPointOfInterest(uint32 id) const
    {
        PointOfInterestContainer::const_iterator itr = _pointsOfInterestStore.find(id);
        if (itr != _pointsOfInterestStore.end())
            return &itr->second;
        return NULL;
    }

    QuestPOIVector const* GetQuestPOIVector(uint32 questId)
    {
        QuestPOIContainer::const_iterator itr = _questPOIStore.find(questId);
        if (itr != _questPOIStore.end())
            return &itr->second;
        return NULL;
    }

    VehicleAccessoryList const* GetVehicleAccessoryList(Vehicle* veh) const;

    DungeonEncounterList const* GetDungeonEncounterList(uint32 mapId, DifficultyID difficulty)
    {
        UNORDERED_MAP<uint32, DungeonEncounterList>::const_iterator itr = _dungeonEncounterStore.find(MAKE_PAIR32(mapId, difficulty));
        if (itr != _dungeonEncounterStore.end())
            return &itr->second;
        return NULL;
    }

    void LoadQuests();
    void LoadQuestObjectives();
    void LoadQuestObjectiveVisualEffects();
    void LoadQuestObjectiveLocales();
    void LoadQuestStartersAndEnders()
    {
        SF_LOG_INFO("server.loading", "Loading GO Start Quest Data...");
        LoadGameobjectQuestStarters();
        SF_LOG_INFO("server.loading", "Loading GO End Quest Data...");
        LoadGameobjectQuestEnders();
        SF_LOG_INFO("server.loading", "Loading Creature Start Quest Data...");
        LoadCreatureQuestStarters();
        SF_LOG_INFO("server.loading", "Loading Creature End Quest Data...");
        LoadCreatureQuestEnders();
    }
    void LoadGameobjectQuestStarters();
    void LoadGameobjectQuestEnders();
    void LoadCreatureQuestStarters();
    void LoadCreatureQuestEnders();

    QuestRelations* GetGOQuestRelationMap()
    {
        return &_goQuestRelations;
    }

    QuestRelationBounds GetGOQuestRelationBounds(uint32 go_entry)
    {
        return _goQuestRelations.equal_range(go_entry);
    }

    QuestRelationBounds GetGOQuestInvolvedRelationBounds(uint32 go_entry)
    {
        return _goQuestInvolvedRelations.equal_range(go_entry);
    }

    QuestRelationReverseBounds GetGOQuestInvolvedRelationReverseBounds(uint32 questId)
    {
        return _goQuestInvolvedRelationsReverse.equal_range(questId);
    }

    QuestRelations* GetCreatureQuestRelationMap()
    {
        return &_creatureQuestRelations;
    }

    QuestRelationBounds GetCreatureQuestRelationBounds(uint32 creature_entry)
    {
        return _creatureQuestRelations.equal_range(creature_entry);
    }

    QuestRelationBounds GetCreatureQuestInvolvedRelationBounds(uint32 creature_entry)
    {
        return _creatureQuestInvolvedRelations.equal_range(creature_entry);
    }

    QuestRelationReverseBounds GetCreatureQuestInvolvedRelationReverseBounds(uint32 questId)
    {
        return _creatureQuestInvolvedRelationsReverse.equal_range(questId);
    }

    void LoadEventScripts();
    void LoadSpellScripts();
    void LoadWaypointScripts();

    void LoadSpellScriptNames();
    void ValidateSpellScripts();

    bool LoadSkyFireStrings(char const* table, int32 min_value, int32 max_value);
    bool LoadSkyFireStrings() { return LoadSkyFireStrings("skyfire_string", MIN_SKYFIRE_STRING_ID, MAX_SKYFIRE_STRING_ID); }
    void LoadDbScriptStrings();
    void LoadCreatureClassLevelStats();
    void LoadCreatureLocales();
    void LoadGraveyardOrientations();
    void LoadCreatureTemplates();
    void LoadCreatureTemplateAddons();
    void CheckCreatureTemplate(CreatureTemplate const* cInfo);
    void LoadTempSummons();
    void LoadCreatures();
    void LoadLinkedRespawn();
    bool SetCreatureLinkedRespawn(uint32 guid, uint32 linkedGuid);
    void LoadCreatureAddons();
    void LoadCreatureModelInfo();
    void LoadEquipmentTemplates();
    void LoadGameObjectLocales();
    void LoadGameobjects();
    void LoadItemTemplates();
    void LoadItemTemplateAddon();
    void LoadItemScriptNames();
    void LoadItemLocales();
    void LoadQuestLocales();
    void LoadNpcTextLocales();
    void LoadPageTextLocales();
    void LoadGossipMenuItemsLocales();
    void LoadPointOfInterestLocales();
    void LoadInstanceTemplate();
    void LoadInstanceEncounters();
    void LoadMailLevelRewards();
    void LoadVehicleTemplateAccessories();
    void LoadVehicleAccessories();

    void LoadGossipText();

    void LoadAreaTriggerTeleports();
    void LoadAccessRequirements();
    void LoadQuestAreaTriggers();
    void LoadAreaTriggerScripts();
    void LoadTavernAreaTriggers();
    void LoadGameObjectForQuests();

    void LoadPageTexts();
    PageText const* GetPageText(uint32 pageEntry);

    void LoadPlayerInfo();
    bool IsValidPlayerCreateRace(uint32 currentRace);
    bool IsValidPlayerCreateClass(uint32 currentClass);
    void LoadPlayerCreateItemsData();
    void LoadPlayerCreateSpellsData();
    void LoadPlayerCreateActionData();
    void LoadPlayerCreateLevelStatsData();
    void LoadPlayerCreateXpData();
    void LoadPlayerCreateCastSpellsData();

    void LoadPetLevelInfo();
    void LoadExplorationBaseXP();
    void LoadPetNames();
    void LoadPetNumber();
    void LoadCorpses();
    void LoadFishingBaseSkillLevel();

    void LoadReputationRewardRate();
    void LoadReputationOnKill();
    void LoadReputationSpilloverTemplate();

    void LoadPointsOfInterest();
    void LoadQuestPOI();

    void LoadNPCSpellClickSpells();

    void LoadGameTele();

    void LoadGossipMenu();
    void LoadGossipMenuItems();

    void LoadVendors();
    void LoadTrainerSpell();
    void AddSpellToTrainer(uint32 entry, uint32 spell, uint32 spellCost, uint32 reqSkill, uint32 reqSkillValue, uint32 reqLevel);

    void LoadTerrainPhaseInfo();
    void LoadTerrainSwapDefaults();
    void LoadTerrainWorldMaps();
    void LoadAreaPhases();

    void LoadSceneTemplates();

    std::vector<PhaseInfoStruct> const* GetPhaseTerrainSwaps(uint32 phaseid) const
    {
        auto itr = _terrainPhaseInfoStore.find(phaseid);
        return itr != _terrainPhaseInfoStore.end() ? &itr->second : nullptr;
    }
    std::vector<PhaseInfoStruct> const* GetDefaultTerrainSwaps(uint32 mapid) const
    {
        auto itr = _terrainMapDefaultStore.find(mapid);
        return itr != _terrainMapDefaultStore.end() ? &itr->second : nullptr;
    }
    std::vector<uint32> const* GetTerrainWorldMaps(uint32 terrainId) const
    {
        auto itr = _terrainWorldMapStore.find(terrainId);
        return itr != _terrainWorldMapStore.end() ? &itr->second : nullptr;
    }
    std::vector<PhaseInfoStruct> const* GetPhasesForArea(uint32 area) const
    {
        auto itr = _phases.find(area);
        return itr != _phases.end() ? &itr->second : nullptr;
    }
    TerrainPhaseInfo const& GetDefaultTerrainSwapStore() const { return _terrainMapDefaultStore; }
    PhaseInfo const& GetAreaPhases() const { return _phases; }

    std::vector<PhaseInfoStruct>* GetPhasesForAreaForLoading(uint32 area)
    {
        auto itr = _phases.find(area);
        return itr != _phases.end() ? &itr->second : nullptr;
    }
    TerrainPhaseInfo& GetPhaseTerrainSwapStoreForLoading() { return _terrainPhaseInfoStore; }
    TerrainPhaseInfo& GetDefaultTerrainSwapStoreForLoading() { return _terrainMapDefaultStore; }
    PhaseInfo& GetAreaPhasesForLoading() { return _phases; }

    void LoadBattlePetBreedData();
    void LoadBattlePetQualityData();

    uint64 BattlePetGetNewId();
    uint8 BattlePetGetRandomBreed(uint32 speciesId) const;
    uint8 BattlePetGetRandomQuality(uint32 speciesId) const;

    std::string GeneratePetName(uint32 entry);
    uint32 GetBaseXP(uint8 level);
    uint32 GetXPForLevel(uint8 level) const;

    int32 GetFishingBaseSkillLevel(uint32 entry) const
    {
        FishingBaseSkillContainer::const_iterator itr = _fishingBaseForAreaStore.find(entry);
        return itr != _fishingBaseForAreaStore.end() ? itr->second : 0;
    }

    void ReturnOrDeleteOldMails(bool serverUp);

    CreatureBaseStats const* GetCreatureBaseStats(uint8 level, uint8 unitClass);

    void SetHighestGuids();
    uint32 GenerateLowGuid(HighGuid guidhigh);
    uint32 GenerateAuctionID();
    uint64 GenerateEquipmentSetGuid();
    uint32 GenerateMailID();
    uint32 GeneratePetNumber();
    uint64 GenerateVoidStorageItemId();

    typedef std::multimap<int32, uint32> ExclusiveQuestGroups;
    typedef std::pair<ExclusiveQuestGroups::const_iterator, ExclusiveQuestGroups::const_iterator> ExclusiveQuestGroupsBounds;

    ExclusiveQuestGroups mExclusiveQuestGroups;

    MailLevelReward const* GetMailLevelReward(uint32 level, uint32 raceMask)
    {
        MailLevelRewardContainer::const_iterator map_itr = _mailLevelRewardStore.find(level);
        if (map_itr == _mailLevelRewardStore.end())
            return NULL;

        for (MailLevelRewardList::const_iterator set_itr = map_itr->second.begin(); set_itr != map_itr->second.end(); ++set_itr)
            if (set_itr->raceMask & raceMask)
                return &*set_itr;

        return NULL;
    }

    CellObjectGuids const& GetCellObjectGuids(uint16 mapid, uint8 spawnMode, uint32 cell_id)
    {
        return _mapObjectGuidsStore[MAKE_PAIR32(mapid, spawnMode)][cell_id];
    }

    CellObjectGuidsMap const& GetMapObjectGuids(uint16 mapid, uint8 spawnMode)
    {
        return _mapObjectGuidsStore[MAKE_PAIR32(mapid, spawnMode)];
    }

    /**
     * Gets temp summon data for all creatures of specified group.
     *
     * @param summonerId   Summoner's entry.
     * @param summonerType Summoner's type, see SummonerType for available types.
     * @param group        Id of required group.
     *
     * @return null if group was not found, otherwise reference to the creature group data
     */
    std::vector<TempSummonData> const* GetSummonGroup(uint32 summonerId, SummonerType summonerType, uint8 group) const
    {
        TempSummonDataContainer::const_iterator itr = _tempSummonDataStore.find(TempSummonGroupKey(summonerId, summonerType, group));
        if (itr != _tempSummonDataStore.end())
            return &itr->second;

        return NULL;
    }

    CreatureData const* GetCreatureData(uint32 guid) const
    {
        CreatureDataContainer::const_iterator itr = _creatureDataStore.find(guid);
        if (itr == _creatureDataStore.end()) return NULL;
        return &itr->second;
    }
    CreatureData& NewOrExistCreatureData(uint32 guid) { return _creatureDataStore[guid]; }
    void DeleteCreatureData(uint32 guid);
    uint64 GetLinkedRespawnGuid(uint64 guid) const
    {
        LinkedRespawnContainer::const_iterator itr = _linkedRespawnStore.find(guid);
        if (itr == _linkedRespawnStore.end()) return 0;
        return itr->second;
    }
    CreatureLocale const* GetCreatureLocale(uint32 entry) const
    {
        CreatureLocaleContainer::const_iterator itr = _creatureLocaleStore.find(entry);
        if (itr == _creatureLocaleStore.end()) return NULL;
        return &itr->second;
    }
    GameObjectLocale const* GetGameObjectLocale(uint32 entry) const
    {
        GameObjectLocaleContainer::const_iterator itr = _gameObjectLocaleStore.find(entry);
        if (itr == _gameObjectLocaleStore.end()) return NULL;
        return &itr->second;
    }
    ItemLocale const* GetItemLocale(uint32 entry) const
    {
        ItemLocaleContainer::const_iterator itr = _itemLocaleStore.find(entry);
        if (itr == _itemLocaleStore.end()) return NULL;
        return &itr->second;
    }
    QuestLocale const* GetQuestLocale(uint32 entry) const
    {
        QuestLocaleContainer::const_iterator itr = _questLocaleStore.find(entry);
        if (itr == _questLocaleStore.end()) return NULL;
        return &itr->second;
    }
    NpcTextLocale const* GetNpcTextLocale(uint32 entry) const
    {
        NpcTextLocaleContainer::const_iterator itr = _npcTextLocaleStore.find(entry);
        if (itr == _npcTextLocaleStore.end()) return NULL;
        return &itr->second;
    }
    PageTextLocale const* GetPageTextLocale(uint32 entry) const
    {
        PageTextLocaleContainer::const_iterator itr = _pageTextLocaleStore.find(entry);
        if (itr == _pageTextLocaleStore.end()) return NULL;
        return &itr->second;
    }
    GossipMenuItemsLocale const* GetGossipMenuItemsLocale(uint32 entry) const
    {
        GossipMenuItemsLocaleContainer::const_iterator itr = _gossipMenuItemsLocaleStore.find(entry);
        if (itr == _gossipMenuItemsLocaleStore.end()) return NULL;
        return &itr->second;
    }
    PointOfInterestLocale const* GetPointOfInterestLocale(uint32 poi_id) const
    {
        PointOfInterestLocaleContainer::const_iterator itr = _pointOfInterestLocaleStore.find(poi_id);
        if (itr == _pointOfInterestLocaleStore.end()) return NULL;
        return &itr->second;
    }
    QuestObjectiveLocale const* GetQuestObjectiveLocale(uint32 objectiveId) const;

    GameObjectData const* GetGOData(uint32 guid) const
    {
        GameObjectDataContainer::const_iterator itr = _gameObjectDataStore.find(guid);
        if (itr == _gameObjectDataStore.end()) return NULL;
        return &itr->second;
    }
    GameObjectData& NewGOData(uint32 guid) { return _gameObjectDataStore[guid]; }
    void DeleteGOData(uint32 guid);

    SkyFireStringLocale const* GetSkyFireStringLocale(int32 entry) const
    {
        SkyFireStringLocaleContainer::const_iterator itr = _SkyfireStringLocaleStore.find(entry);
        if (itr == _SkyfireStringLocaleStore.end()) return NULL;
        return &itr->second;
    }
    const char* GetSkyFireString(int32 entry, LocaleConstant locale_idx) const;
    const char* GetSkyFireStringForDBCLocale(int32 entry) const { return GetSkyFireString(entry, DBCLocaleIndex); }
    LocaleConstant GetDBCLocaleIndex() const { return DBCLocaleIndex; }
    void SetDBCLocaleIndex(LocaleConstant locale) { DBCLocaleIndex = locale; }

    void AddCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid, uint32 instance);
    void DeleteCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid);

    // grid objects
    void AddCreatureToGrid(uint32 guid, CreatureData const* data);
    void RemoveCreatureFromGrid(uint32 guid, CreatureData const* data);
    void AddGameobjectToGrid(uint32 guid, GameObjectData const* data);
    void RemoveGameobjectFromGrid(uint32 guid, GameObjectData const* data);
    uint32 AddGOData(uint32 entry, uint32 map, float x, float y, float z, float o, uint32 spawntimedelay = 0, float rotation0 = 0, float rotation1 = 0, float rotation2 = 0, float rotation3 = 0);
    uint32 AddCreData(uint32 entry, uint32 team, uint32 map, float x, float y, float z, float o, uint32 spawntimedelay = 0);
    bool MoveCreData(uint32 guid, uint32 map, const Position& pos);

    // reserved names
    void LoadReservedPlayersNames();
    bool IsReservedName(std::string const& name) const;

    // name with valid structure and symbols
    static ResponseCodes CheckPlayerName(std::string const& name, bool create = false);
    static PetNameInvalidReason CheckPetName(std::string const& name);
    static bool IsValidCharterName(std::string const& name);

    static bool CheckDeclinedNames(const std::wstring& w_ownname, DeclinedName const& names);

    GameTele const* GetGameTele(uint32 id) const
    {
        GameTeleContainer::const_iterator itr = _gameTeleStore.find(id);
        if (itr == _gameTeleStore.end()) return NULL;
        return &itr->second;
    }
    GameTele const* GetGameTele(std::string const& name) const;
    GameTele const* GetGameTeleExactName(std::string const& name) const;
    GameTeleContainer const& GetGameTeleMap() const { return _gameTeleStore; }
    bool AddGameTele(GameTele& data);
    bool DeleteGameTele(std::string const& name);

    TrainerSpellData const* GetNpcTrainerSpells(uint32 entry) const
    {
        CacheTrainerSpellContainer::const_iterator  iter = _cacheTrainerSpellStore.find(entry);
        if (iter == _cacheTrainerSpellStore.end())
            return NULL;

        return &iter->second;
    }

    VendorItemData const* GetNpcVendorItemList(uint32 entry) const
    {
        CacheVendorItemContainer::const_iterator iter = _cacheVendorItemStore.find(entry);
        if (iter == _cacheVendorItemStore.end())
            return NULL;

        return &iter->second;
    }

    float const* GetGraveyardOrientation(uint32 id) const
    {
        GraveyardOrientationContainer::const_iterator iter = _graveyardOrientations.find(id);
        if (iter != _graveyardOrientations.end())
            return &iter->second;

        return NULL;
    }

    void AddVendorItem(uint32 entry, uint32 item, int32 maxcount, uint32 incrtime, uint32 extendedCost, uint8 type, bool persist = true); // for event
    bool RemoveVendorItem(uint32 entry, uint32 item, uint8 type, bool persist = true); // for event
    bool IsVendorItemValid(uint32 vendor_entry, uint32 id, int32 maxcount, uint32 ptime, uint32 ExtendedCost, uint8 type, Player* player = NULL, std::set<uint32>* skip_vendors = NULL, uint32 ORnpcflag = 0) const;

    void LoadScriptNames();
    ScriptNameContainer& GetScriptNames() { return _scriptNamesStore; }
    const char* GetScriptName(uint32 id) const { return id < _scriptNamesStore.size() ? _scriptNamesStore[id].c_str() : ""; }
    uint32 GetScriptId(const char* name);

    SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 creature_id) const
    {
        return _spellClickInfoStore.equal_range(creature_id);
    }

    GossipMenusMapBounds GetGossipMenusMapBounds(uint32 uiMenuId) const
    {
        return _gossipMenusStore.equal_range(uiMenuId);
    }

    GossipMenusMapBoundsNonConst GetGossipMenusMapBoundsNonConst(uint32 uiMenuId)
    {
        return _gossipMenusStore.equal_range(uiMenuId);
    }

    GossipMenuItemsMapBounds GetGossipMenuItemsMapBounds(uint32 uiMenuId) const
    {
        return _gossipMenuItemsStore.equal_range(uiMenuId);
    }
    GossipMenuItemsMapBoundsNonConst GetGossipMenuItemsMapBoundsNonConst(uint32 uiMenuId)
    {
        return _gossipMenuItemsStore.equal_range(uiMenuId);
    }

    // for wintergrasp only
    GraveYardContainer GraveYardStore;

    static void AddLocaleString(std::string const& s, LocaleConstant locale, StringVector& data);
    static inline void GetLocaleString(const StringVector& data, int loc_idx, std::string& value)
    {
        if (data.size() > size_t(loc_idx) && !data[loc_idx].empty())
            value = data[loc_idx];
    }

    CharacterConversionMap FactionChangeAchievements;
    CharacterConversionMap FactionChangeItems;
    CharacterConversionMap FactionChangeQuests;
    CharacterConversionMap FactionChangeReputation;
    CharacterConversionMap FactionChangeSpells;
    CharacterConversionMap FactionChangeTitles;

    void LoadFactionChangeAchievements();
    void LoadFactionChangeItems();
    void LoadFactionChangeQuests();
    void LoadFactionChangeReputations();
    void LoadFactionChangeSpells();
    void LoadFactionChangeTitles();

    void LoadHotfixData();
    HotfixData const& GetHotfixData() const { return _hotfixData; }
    time_t GetHotfixDate(uint32 entry, uint32 type) const
    {
        time_t ret = 0;
        for (HotfixData::const_iterator itr = _hotfixData.begin(); itr != _hotfixData.end(); ++itr)
            if (itr->Entry == entry && itr->Type == type)
                if (itr->Timestamp > ret)
                    ret = itr->Timestamp;

        return ret ? ret : time(NULL);
    }

    void LoadMissingKeyChains();

    bool QuestObjectiveExists(uint32 objectiveId) const;
    uint32 GetQuestObjectiveQuestId(uint32 objectiveId) const;
    SceneTemplate const* GetSceneTemplate(uint32 sceneId) const;

    void LoadResearchDigsiteInfo();
    void LoadArchaeologyFindInfo();
    void LoadResearchProjectRequirements();

    ResearchDigsiteInfo const* GetResearchDigsiteInfo(uint32 digsiteId) const
    {
        for (ResearchDigsiteContainer::const_iterator itr = _researchDigsiteStore.begin(); itr != _researchDigsiteStore.end(); ++itr)
            for (ResearchDigsiteList::const_iterator digsite = itr->second.begin(); digsite != itr->second.end(); ++digsite)
                if (digsite->digsiteId == digsiteId)
                    return &(*digsite);

        return NULL;
    }

    ResearchDigsiteList const* GetResearchDigsitesForContinent(uint32 mapId) const
    {
        ResearchDigsiteContainer::const_iterator iter = _researchDigsiteStore.find(mapId);
        if (iter != _researchDigsiteStore.end())
            return &iter->second;

        return NULL;
    }

    ArchaeologyFindInfo const* GetArchaeologyFindInfo(uint32 findGUID, uint32 digsiteId)
    {
        ArchaeologyFindContainer::const_iterator itr = _archaeologyFindStore.find(digsiteId);
        if (itr == _archaeologyFindStore.end())
            return NULL;

        for (ArchaeologyFindList::const_iterator find = itr->second.begin(); find != itr->second.end(); ++find)
            if (find->guid == findGUID)
                return &(*find);

        return NULL;
    }

    ArchaeologyFindInfo const* GetRandomArchaeologyFindForDigsite(uint32 digsiteId)
    {
        ArchaeologyFindContainer::const_iterator itr = _archaeologyFindStore.find(digsiteId);
        if (itr == _archaeologyFindStore.end())
            return NULL;

        if (itr->second.empty())
            return NULL;

        return &Skyfire::Containers::SelectRandomContainerElement(itr->second);
    }

    ResearchProjectRequirements const* GetResearchProjectRequirements(uint32 projectId) const
    {
        ResearchProjectRequirementContainer::const_iterator iter = _researchProjectRequirementStore.find(projectId);
        if (iter != _researchProjectRequirementStore.end())
            return &iter->second;

        return NULL;
    }

private:
    // first free id for selected id type
    uint32 _auctionId;
    uint64 _equipmentSetGuid;
    uint32 _itemTextId;
    uint32 _mailId;
    uint32 _hiPetNumber;
    uint64 _voidItemId;

    // first free low guid for selected guid type
    uint32 _hiCharGuid;
    uint32 _hiCreatureGuid;
    uint32 _hiPetGuid;
    uint32 _hiVehicleGuid;
    uint32 _hiItemGuid;
    uint32 _hiGoGuid;
    uint32 _hiDoGuid;
    uint32 _hiCorpseGuid;
    uint32 _hiAreaTriggerGuid;
    uint32 _hiMoTransGuid;
    uint32 m_battlePetId;

    QuestMap _questTemplates;
    QuestObjectiveLookupMap m_questObjectiveLookup;

    typedef UNORDERED_MAP<uint32, GossipText> GossipTextContainer;
    typedef UNORDERED_MAP<uint32, uint32> QuestAreaTriggerContainer;
    typedef std::set<uint32> TavernAreaTriggerContainer;
    typedef std::set<uint32> GameObjectForQuestContainer;

    QuestAreaTriggerContainer _questAreaTriggerStore;
    TavernAreaTriggerContainer _tavernAreaTriggerStore;
    GameObjectForQuestContainer _gameObjectForQuestStore;
    GossipTextContainer _gossipTextStore;
    AreaTriggerContainer _areaTriggerStore;
    AreaTriggerScriptContainer _areaTriggerScriptStore;
    AccessRequirementContainer _accessRequirementStore;
    DungeonEncounterContainer _dungeonEncounterStore;

    RepRewardRateContainer _repRewardRateStore;
    RepOnKillContainer _repOnKillStore;
    RepSpilloverTemplateContainer _repSpilloverTemplateStore;

    GossipMenusContainer _gossipMenusStore;
    GossipMenuItemsContainer _gossipMenuItemsStore;
    PointOfInterestContainer _pointsOfInterestStore;

    QuestPOIContainer _questPOIStore;

    QuestRelations _goQuestRelations;
    QuestRelations _goQuestInvolvedRelations;
    QuestRelationsReverse _goQuestInvolvedRelationsReverse;
    QuestRelations _creatureQuestRelations;
    QuestRelations _creatureQuestInvolvedRelations;
    QuestRelationsReverse _creatureQuestInvolvedRelationsReverse;

    //character reserved names
    typedef std::set<std::wstring> ReservedNamesContainer;
    ReservedNamesContainer _reservedNamesStore;

    GameTeleContainer _gameTeleStore;

    ScriptNameContainer _scriptNamesStore;

    SpellClickInfoContainer _spellClickInfoStore;

    SpellScriptsContainer _spellScriptsStore;

    VehicleAccessoryContainer _vehicleTemplateAccessoryStore;
    VehicleAccessoryContainer _vehicleAccessoryStore;

    LocaleConstant DBCLocaleIndex;

    PageTextContainer _pageTextStore;
    InstanceTemplateContainer _instanceTemplateStore;

    TerrainPhaseInfo _terrainPhaseInfoStore;
    TerrainPhaseInfo _terrainMapDefaultStore;
    TerrainUIPhaseInfo _terrainWorldMapStore;
    PhaseInfo _phases;

    typedef std::set<uint8> BattleBetBreedSet;
    typedef UNORDERED_MAP<uint16, BattleBetBreedSet> BattlePetBreedXSpeciesMap;
    typedef std::set<uint8> BattlePetQualitySet;
    typedef UNORDERED_MAP<uint16, BattlePetQualitySet> BattlePetQualityXSpeciesMap;

    BattlePetBreedXSpeciesMap sBattlePetBreedXSpeciesStore;
    BattlePetQualityXSpeciesMap sBattlePetQualityXSpeciesStore;

private:
    void LoadScripts(ScriptsType type);
    void CheckScripts(ScriptsType type, std::set<int32>& ids);
    void LoadQuestRelationsHelper(QuestRelations& map, std::string const& table, bool starter, bool go);
    void PlayerCreateInfoAddItemHelper(uint32 race_, uint32 class_, uint32 itemId, int32 count);

    MailLevelRewardContainer _mailLevelRewardStore;

    CreatureBaseStatsContainer _creatureBaseStatsStore;

    typedef std::map<uint32, PetLevelInfo*> PetLevelInfoContainer;
    // PetLevelInfoContainer[creature_id][level]
    PetLevelInfoContainer _petInfoStore;                            // [creature_id][level]

    void BuildPlayerLevelInfo(uint8 race, uint8 class_, uint8 level, PlayerLevelInfo* plinfo) const;

    PlayerInfo* _playerInfo[MAX_RACES][MAX_CLASSES];

    typedef std::vector<uint32> PlayerXPperLevel;       // [level]
    PlayerXPperLevel _playerXPperLevel;

    typedef std::map<uint32, uint32> BaseXPContainer;          // [area level][base xp]
    BaseXPContainer _baseXPTable;

    typedef std::map<uint32, int32> FishingBaseSkillContainer; // [areaId][base skill level]
    FishingBaseSkillContainer _fishingBaseForAreaStore;

    typedef std::map<uint32, StringVector> HalfNameContainer;
    HalfNameContainer _petHalfName0;
    HalfNameContainer _petHalfName1;

    MapObjectGuids _mapObjectGuidsStore;
    CreatureDataContainer _creatureDataStore;
    CreatureTemplateContainer _creatureTemplateStore;
    CreatureModelContainer _creatureModelStore;
    CreatureAddonContainer _creatureAddonStore;
    CreatureAddonContainer _creatureTemplateAddonStore;
    EquipmentInfoContainer _equipmentInfoStore;
    LinkedRespawnContainer _linkedRespawnStore;
    CreatureLocaleContainer _creatureLocaleStore;
    GameObjectDataContainer _gameObjectDataStore;
    GameObjectLocaleContainer _gameObjectLocaleStore;
    GameObjectTemplateContainer _gameObjectTemplateStore;
    /// Stores temp summon data grouped by summoner's entry, summoner's type and group id
    TempSummonDataContainer _tempSummonDataStore;

    ItemTemplateContainer _itemTemplateStore;
    ItemLocaleContainer _itemLocaleStore;
    QuestLocaleContainer _questLocaleStore;
    NpcTextLocaleContainer _npcTextLocaleStore;
    PageTextLocaleContainer _pageTextLocaleStore;
    SkyFireStringLocaleContainer _SkyfireStringLocaleStore;
    GossipMenuItemsLocaleContainer _gossipMenuItemsLocaleStore;
    PointOfInterestLocaleContainer _pointOfInterestLocaleStore;
    QuestObjectiveLocaleContainer m_questObjectiveLocaleStore;

    CacheVendorItemContainer _cacheVendorItemStore;
    CacheTrainerSpellContainer _cacheTrainerSpellStore;

    SceneTemplateContainer _sceneTemplateStore;
    GraveyardOrientationContainer _graveyardOrientations;

    std::set<uint32> _difficultyEntries[4 - 1]; // already loaded difficulty 1 value in creatures, used in CheckCreatureTemplate
    std::set<uint32> _hasDifficultyEntries[4 - 1]; // already loaded creatures with difficulty 1 values, used in CheckCreatureTemplate

    enum CreatureLinkedRespawnType
    {
        CREATURE_TO_CREATURE,
        CREATURE_TO_GO,         // Creature is dependant on GO
        GO_TO_GO,
        GO_TO_CREATURE          // GO is dependant on creature
    };
    HotfixData _hotfixData;

    ResearchDigsiteContainer _researchDigsiteStore;
    ArchaeologyFindContainer _archaeologyFindStore;
    ResearchProjectRequirementContainer _researchProjectRequirementStore;
};

#define sObjectMgr ACE_Singleton<ObjectMgr, ACE_Null_Mutex>::instance()

// scripting access functions
bool LoadSkyFireStrings(char const* table, int32 start_value = MAX_CREATURE_AI_TEXT_STRING_ID, int32 end_value = std::numeric_limits<int32>::min());

#endif
