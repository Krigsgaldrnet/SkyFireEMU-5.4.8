/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "AchievementMgr.h"
#include "DisableMgr.h"
#include "ObjectMgr.h"
#include "OutdoorPvP.h"
#include "Player.h"
#include "SpellMgr.h"
#include "VMapManager2.h"

namespace DisableMgr
{

    namespace
    {
        struct DisableData
        {
            uint8 flags;
            std::set<uint32> params[2];                             // params0, params1
        };

        // single disables here with optional data
        typedef std::map<uint32, DisableData> DisableTypeMap;
        // global disable map by source
        typedef std::map<DisableType, DisableTypeMap> DisableMap;

        DisableMap m_DisableMap;

        uint8 MAX_DISABLE_TYPES = 8;
    }

    void LoadDisables()
    {
        uint32 oldMSTime = getMSTime();

        // reload case
        for (DisableMap::iterator itr = m_DisableMap.begin(); itr != m_DisableMap.end(); ++itr)
            itr->second.clear();

        m_DisableMap.clear();

        QueryResult result = WorldDatabase.Query("SELECT sourceType, entry, flags, params_0, params_1 FROM disables");

        uint32 total_count = 0;

        if (!result)
        {
            SF_LOG_INFO("server.loading", ">> Loaded 0 disables. DB table `disables` is empty!");
            return;
        }

        Field* fields;
        do
        {
            fields = result->Fetch();
            DisableType type = DisableType(fields[0].GetUInt32());
            if (type >= MAX_DISABLE_TYPES)
            {
                SF_LOG_ERROR("sql.sql", "Invalid type %u specified in `disables` table, skipped.", type);
                continue;
            }

            uint32 entry = fields[1].GetUInt32();
            uint8 flags = fields[2].GetUInt8();
            std::string params_0 = fields[3].GetString();
            std::string params_1 = fields[4].GetString();

            DisableData data;
            data.flags = flags;

            switch (type)
            {
                case DISABLE_TYPE_SPELL:
                    if (!(sSpellMgr->GetSpellInfo(entry) || flags & SPELL_DISABLE_DEPRECATED_SPELL))
                    {
                        SF_LOG_ERROR("sql.sql", "Spell entry %u from `disables` doesn't exist in dbc, skipped.", entry);
                        continue;
                    }

                    if (!flags || flags > MAX_SPELL_DISABLE_TYPE)
                    {
                        SF_LOG_ERROR("sql.sql", "Disable flags for spell %u are invalid, skipped.", entry);
                        continue;
                    }

                    if (flags & SPELL_DISABLE_MAP)
                    {
                        Tokenizer tokens(params_0, ',');
                        for (uint8 i = 0; i < tokens.size(); )
                            data.params[0].insert(atoi(tokens[i++]));
                    }

                    if (flags & SPELL_DISABLE_AREA)
                    {
                        Tokenizer tokens(params_1, ',');
                        for (uint8 i = 0; i < tokens.size(); )
                            data.params[1].insert(atoi(tokens[i++]));
                    }

                    break;
                    // checked later
                case DISABLE_TYPE_QUEST:
                    break;
                case DISABLE_TYPE_MAP:
                {
                    MapEntry const* mapEntry = sMapStore.LookupEntry(entry);
                    if (!mapEntry)
                    {
                        SF_LOG_ERROR("sql.sql", "Map entry %u from `disables` doesn't exist in dbc, skipped.", entry);
                        continue;
                    }
                    bool isFlagInvalid = false;
                    switch (mapEntry->map_type)
                    {
                        case MAP_COMMON:
                            if (flags)
                                isFlagInvalid = true;
                            break;
                        case MAP_DUNGEON:
                            if (flags & DUNGEON_STATUSFLAG_HEROIC && !GetMapDifficultyData(entry, DIFFICULTY_HEROIC))
                                flags -= DUNGEON_STATUSFLAG_HEROIC;
                            if (!flags)
                                isFlagInvalid = true;
                            break;
                        case MAP_RAID:
                            if (flags & RAID_STATUSFLAG_10MAN_HEROIC && !GetMapDifficultyData(entry, DIFFICULTY_10MAN_HEROIC))
                                flags -= RAID_STATUSFLAG_10MAN_HEROIC;
                            if (flags & RAID_STATUSFLAG_25MAN_HEROIC && !GetMapDifficultyData(entry, DIFFICULTY_25MAN_HEROIC))
                                flags -= RAID_STATUSFLAG_25MAN_HEROIC;
                            if (flags & RAID_STATUSFLAG_10MAN_FLEX && !GetMapDifficultyData(entry, DIFFICULTY_FLEX))
                                flags -= RAID_STATUSFLAG_10MAN_FLEX;
                            if (flags & RAID_STATUSFLAG_25MAN_LFR && !GetMapDifficultyData(entry, DIFFICULTY_25MAN_LFR))
                                flags -= RAID_STATUSFLAG_25MAN_LFR;
                            if (!flags)
                                isFlagInvalid = true;
                            break;
                        case MAP_BATTLEGROUND:
                        case MAP_ARENA:
                            SF_LOG_ERROR("sql.sql", "Battleground map %u specified to be disabled in map case, skipped.", entry);
                            continue;
                    }
                    if (isFlagInvalid)
                    {
                        SF_LOG_ERROR("sql.sql", "Disable flags for map %u are invalid, skipped.", entry);
                        continue;
                    }
                    break;
                }
                case DISABLE_TYPE_BATTLEGROUND:
                    if (!sBattlemasterListStore.LookupEntry(entry))
                    {
                        SF_LOG_ERROR("sql.sql", "Battleground entry %u from `disables` doesn't exist in dbc, skipped.", entry);
                        continue;
                    }
                    if (flags)
                        SF_LOG_ERROR("sql.sql", "Disable flags specified for battleground %u, useless data.", entry);
                    break;
                case DISABLE_TYPE_OUTDOORPVP:
                    if (entry > MAX_OUTDOORPVP_TYPES)
                    {
                        SF_LOG_ERROR("sql.sql", "OutdoorPvPTypes value %u from `disables` is invalid, skipped.", entry);
                        continue;
                    }
                    if (flags)
                        SF_LOG_ERROR("sql.sql", "Disable flags specified for outdoor PvP %u, useless data.", entry);
                    break;
                case DISABLE_TYPE_ACHIEVEMENT_CRITERIA:
                    if (!sAchievementMgr->GetAchievementCriteria(entry))
                    {
                        SF_LOG_ERROR("sql.sql", "Achievement Criteria entry %u from `disables` doesn't exist in dbc, skipped.", entry);
                        continue;
                    }
                    if (flags)
                        SF_LOG_ERROR("sql.sql", "Disable flags specified for Achievement Criteria %u, useless data.", entry);
                    break;
                case DISABLE_TYPE_VMAP:
                {
                    MapEntry const* mapEntry = sMapStore.LookupEntry(entry);
                    if (!mapEntry)
                    {
                        SF_LOG_ERROR("sql.sql", "Map entry %u from `disables` doesn't exist in dbc, skipped.", entry);
                        continue;
                    }
                    switch (mapEntry->map_type)
                    {
                        case MAP_COMMON:
                            if (flags & VMAP_DISABLE_AREAFLAG)
                                SF_LOG_INFO("misc", "Areaflag disabled for world map %u.", entry);
                            if (flags & VMAP_DISABLE_LIQUIDSTATUS)
                                SF_LOG_INFO("misc", "Liquid status disabled for world map %u.", entry);
                            break;
                        case MAP_DUNGEON:
                            if (flags & VMAP_DISABLE_HEIGHT)
                                SF_LOG_INFO("misc", "Height disabled for instance map %u.", entry);
                            if (flags & VMAP_DISABLE_LOS)
                                SF_LOG_INFO("misc", "LoS disabled for instance map %u.", entry);
                            break;
                        case MAP_RAID:
                            if (flags & VMAP_DISABLE_HEIGHT)
                                SF_LOG_INFO("misc", "Height disabled for raid map %u.", entry);
                            if (flags & VMAP_DISABLE_LOS)
                                SF_LOG_INFO("misc", "LoS disabled for raid map %u.", entry);
                            break;
                        case MAP_BATTLEGROUND:
                            if (flags & VMAP_DISABLE_HEIGHT)
                                SF_LOG_INFO("misc", "Height disabled for battleground map %u.", entry);
                            if (flags & VMAP_DISABLE_LOS)
                                SF_LOG_INFO("misc", "LoS disabled for battleground map %u.", entry);
                            break;
                        case MAP_ARENA:
                            if (flags & VMAP_DISABLE_HEIGHT)
                                SF_LOG_INFO("misc", "Height disabled for arena map %u.", entry);
                            if (flags & VMAP_DISABLE_LOS)
                                SF_LOG_INFO("misc", "LoS disabled for arena map %u.", entry);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case DISABLE_TYPE_MMAP:
                {
                    MapEntry const* mapEntry = sMapStore.LookupEntry(entry);
                    if (!mapEntry)
                    {
                        SF_LOG_ERROR("sql.sql", "Map entry %u from `disables` doesn't exist in dbc, skipped.", entry);
                        continue;
                    }
                    switch (mapEntry->map_type)
                    {
                        case MAP_COMMON:
                            SF_LOG_INFO("misc", "Pathfinding disabled for world map %u.", entry);
                            break;
                        case MAP_DUNGEON:
                        case MAP_RAID:
                            SF_LOG_INFO("misc", "Pathfinding disabled for instance map %u.", entry);
                            break;
                        case MAP_BATTLEGROUND:
                            SF_LOG_INFO("misc", "Pathfinding disabled for battleground map %u.", entry);
                            break;
                        case MAP_ARENA:
                            SF_LOG_INFO("misc", "Pathfinding disabled for arena map %u.", entry);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }

            m_DisableMap[type].insert(DisableTypeMap::value_type(entry, data));
            ++total_count;
        } while (result->NextRow());

        SF_LOG_INFO("server.loading", ">> Loaded %u disables in %u ms", total_count, GetMSTimeDiffToNow(oldMSTime));
    }

    void CheckQuestDisables()
    {
        uint32 oldMSTime = getMSTime();

        uint32 count = m_DisableMap[DISABLE_TYPE_QUEST].size();
        if (!count)
        {
            SF_LOG_INFO("server.loading", ">> Checked 0 quest disables.");
            return;
        }

        // check only quests, rest already done at startup
        for (DisableTypeMap::iterator itr = m_DisableMap[DISABLE_TYPE_QUEST].begin(); itr != m_DisableMap[DISABLE_TYPE_QUEST].end();)
        {
            const uint32 entry = itr->first;
            if (!sObjectMgr->GetQuestTemplate(entry))
            {
                SF_LOG_ERROR("sql.sql", "Quest entry %u from `disables` doesn't exist, skipped.", entry);
                m_DisableMap[DISABLE_TYPE_QUEST].erase(itr++);
                continue;
            }
            if (itr->second.flags)
                SF_LOG_ERROR("sql.sql", "Disable flags specified for quest %u, useless data.", entry);
            ++itr;
        }

        SF_LOG_INFO("server.loading", ">> Checked %u quest disables in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    bool IsDisabledFor(DisableType type, uint32 entry, Unit const* unit, uint8 flags)
    {
        ASSERT(type < MAX_DISABLE_TYPES);
        if (m_DisableMap[type].empty())
            return false;

        DisableTypeMap::iterator itr = m_DisableMap[type].find(entry);
        if (itr == m_DisableMap[type].end())    // not disabled
            return false;

        switch (type)
        {
            case DISABLE_TYPE_SPELL:
            {
                uint8 spellFlags = itr->second.flags;
                if (unit)
                {
                    if ((spellFlags & SPELL_DISABLE_PLAYER && unit->GetTypeId() == TypeID::TYPEID_PLAYER) ||
                        (unit->GetTypeId() == TypeID::TYPEID_UNIT && ((unit->ToCreature()->IsPet() && spellFlags & SPELL_DISABLE_PET) || spellFlags & SPELL_DISABLE_CREATURE)))
                    {
                        if (spellFlags & SPELL_DISABLE_MAP)
                        {
                            std::set<uint32> const& mapIds = itr->second.params[0];
                            if (mapIds.find(unit->GetMapId()) != mapIds.end())
                                return true;                                        // Spell is disabled on current map

                            if (!(spellFlags & SPELL_DISABLE_AREA))
                                return false;                                       // Spell is disabled on another map, but not this one, return false

                            // Spell is disabled in an area, but not explicitly our current mapId. Continue processing.
                        }

                        if (spellFlags & SPELL_DISABLE_AREA)
                        {
                            std::set<uint32> const& areaIds = itr->second.params[1];
                            if (areaIds.find(unit->GetAreaId()) != areaIds.end())
                                return true;                                        // Spell is disabled in this area
                            return false;                                           // Spell is disabled in another area, but not this one, return false
                        }
                        else
                            return true;                                            // Spell disabled for all maps
                    }

                    return false;
                }
                else if (spellFlags & SPELL_DISABLE_DEPRECATED_SPELL)    // call not from spellcast
                    return true;
                else if (flags & SPELL_DISABLE_LOS)
                    return spellFlags & SPELL_DISABLE_LOS;

                break;
            }
            case DISABLE_TYPE_MAP:
                if (Player const* player = unit->ToPlayer())
                {
                    MapEntry const* mapEntry = sMapStore.LookupEntry(entry);
                    if (mapEntry->IsInstance())
                    {
                        uint8 disabledModes = itr->second.flags;
                        DifficultyID targetDifficulty = player->GetDifficulty(mapEntry);
                        GetDownscaledMapDifficultyData(entry, targetDifficulty);
                        switch (targetDifficulty)
                        {
                            case DIFFICULTY_NORMAL:
                                return disabledModes & DUNGEON_STATUSFLAG_NORMAL;
                            case DIFFICULTY_HEROIC:
                                return disabledModes & DUNGEON_STATUSFLAG_HEROIC;
                            case DIFFICULTY_10MAN_NORMAL:
                                return disabledModes & RAID_STATUSFLAG_10MAN_NORMAL;
                            case DIFFICULTY_10MAN_HEROIC:
                                return disabledModes & RAID_STATUSFLAG_10MAN_HEROIC;
                            case DIFFICULTY_25MAN_NORMAL:
                                return disabledModes & RAID_STATUSFLAG_25MAN_NORMAL;
                            case DIFFICULTY_25MAN_HEROIC:
                                return disabledModes & RAID_STATUSFLAG_25MAN_HEROIC;
                            case DIFFICULTY_FLEX:
                                return disabledModes & RAID_STATUSFLAG_10MAN_FLEX;
                            case DIFFICULTY_25MAN_LFR:
                                return disabledModes & RAID_STATUSFLAG_25MAN_LFR;
                        }
                    }
                    else if (mapEntry->map_type == MAP_COMMON)
                        return true;
                }
                return false;
            case DISABLE_TYPE_QUEST:
                if (!unit)
                    return true;
                if (Player const* player = unit->ToPlayer())
                    if (player->IsGameMaster())
                        return false;
                return true;
            case DISABLE_TYPE_BATTLEGROUND:
            case DISABLE_TYPE_OUTDOORPVP:
            case DISABLE_TYPE_ACHIEVEMENT_CRITERIA:
            case DISABLE_TYPE_MMAP:
                return true;
            case DISABLE_TYPE_VMAP:
                return flags & itr->second.flags;
        }

        return false;
    }

} // Namespace
