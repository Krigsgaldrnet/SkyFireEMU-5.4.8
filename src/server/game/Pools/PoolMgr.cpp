/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Containers.h"
#include "Log.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "PoolMgr.h"

////////////////////////////////////////////////////////////
// template class ActivePoolData

// Method that tell amount spawned objects/subpools
uint32 ActivePoolData::GetActiveObjectCount(uint32 pool_id) const
{
    ActivePoolPools::const_iterator itr = mSpawnedPools.find(pool_id);
    return itr != mSpawnedPools.end() ? itr->second : 0;
}

// Method that tell if a creature is spawned currently
template<>
bool ActivePoolData::IsActiveObject<Creature>(uint32 db_guid) const
{
    return mSpawnedCreatures.find(db_guid) != mSpawnedCreatures.end();
}

// Method that tell if a gameobject is spawned currently
template<>
bool ActivePoolData::IsActiveObject<GameObject>(uint32 db_guid) const
{
    return mSpawnedGameobjects.find(db_guid) != mSpawnedGameobjects.end();
}

// Method that tell if a pool is spawned currently
template<>
bool ActivePoolData::IsActiveObject<Pool>(uint32 sub_pool_id) const
{
    return mSpawnedPools.find(sub_pool_id) != mSpawnedPools.end();
}

// Method that tell if a quest can be started
template<>
bool ActivePoolData::IsActiveObject<Quest>(uint32 quest_id) const
{
    return mActiveQuests.find(quest_id) != mActiveQuests.end();
}

template<>
void ActivePoolData::ActivateObject<Creature>(uint32 db_guid, uint32 pool_id)
{
    mSpawnedCreatures.insert(db_guid);
    ++mSpawnedPools[pool_id];
}

template<>
void ActivePoolData::ActivateObject<GameObject>(uint32 db_guid, uint32 pool_id)
{
    mSpawnedGameobjects.insert(db_guid);
    ++mSpawnedPools[pool_id];
}

template<>
void ActivePoolData::ActivateObject<Pool>(uint32 sub_pool_id, uint32 pool_id)
{
    mSpawnedPools[sub_pool_id] = 0;
    ++mSpawnedPools[pool_id];
}

template<>
void ActivePoolData::ActivateObject<Quest>(uint32 quest_id, uint32 pool_id)
{
    mActiveQuests.insert(quest_id);
    ++mSpawnedPools[pool_id];
}

template<>
void ActivePoolData::RemoveObject<Creature>(uint32 db_guid, uint32 pool_id)
{
    mSpawnedCreatures.erase(db_guid);
    uint32& val = mSpawnedPools[pool_id];
    if (val > 0)
        --val;
}

template<>
void ActivePoolData::RemoveObject<GameObject>(uint32 db_guid, uint32 pool_id)
{
    mSpawnedGameobjects.erase(db_guid);
    uint32& val = mSpawnedPools[pool_id];
    if (val > 0)
        --val;
}

template<>
void ActivePoolData::RemoveObject<Pool>(uint32 sub_pool_id, uint32 pool_id)
{
    mSpawnedPools.erase(sub_pool_id);
    uint32& val = mSpawnedPools[pool_id];
    if (val > 0)
        --val;
}

template<>
void ActivePoolData::RemoveObject<Quest>(uint32 quest_id, uint32 pool_id)
{
    mActiveQuests.erase(quest_id);
    uint32& val = mSpawnedPools[pool_id];
    if (val > 0)
        --val;
}

////////////////////////////////////////////////////////////
// Methods of template class PoolGroup

// Method to add a gameobject/creature guid to the proper list depending on pool type and chance value
template <class T>
void PoolGroup<T>::AddEntry(PoolObject& poolitem, uint32 maxentries)
{
    if (poolitem.chance != 0 && maxentries == 1)
        ExplicitlyChanced.push_back(poolitem);
    else
        EqualChanced.push_back(poolitem);
}

// Method to check the chances are proper in this object pool
template <class T>
bool PoolGroup<T>::CheckPool() const
{
    if (EqualChanced.empty())
    {
        float chance = 0;
        for (uint32 i = 0; i < ExplicitlyChanced.size(); ++i)
            chance += ExplicitlyChanced[i].chance;
        if (chance != 100 && chance != 0)
            return false;
    }
    return true;
}

template <class T>
PoolObject* PoolGroup<T>::RollOne(ActivePoolData& spawns, uint32 triggerFrom)
{
    if (!ExplicitlyChanced.empty())
    {
        float roll = (float)rand_chance();

        for (uint32 i = 0; i < ExplicitlyChanced.size(); ++i)
        {
            roll -= ExplicitlyChanced[i].chance;
            // Triggering object is marked as spawned at this time and can be also rolled (respawn case)
            // so this need explicit check for this case
            if (roll < 0 && (ExplicitlyChanced[i].guid == triggerFrom || !spawns.IsActiveObject<T>(ExplicitlyChanced[i].guid)))
                return &ExplicitlyChanced[i];
        }
    }
    if (!EqualChanced.empty())
    {
        int32 index = std::rand() % EqualChanced.size();
        // Triggering object is marked as spawned at this time and can be also rolled (respawn case)
        // so this need explicit check for this case
        if (EqualChanced[index].guid == triggerFrom || !spawns.IsActiveObject<T>(EqualChanced[index].guid))
            return &EqualChanced[index];
    }

    return NULL;
}

// Main method to despawn a creature or gameobject in a pool
// If no guid is passed, the pool is just removed (event end case)
// If guid is filled, cache will be used and no removal will occur, it just fill the cache
template<class T>
void PoolGroup<T>::DespawnObject(ActivePoolData& spawns, uint32 guid)
{
    for (size_t i = 0; i < EqualChanced.size(); ++i)
    {
        // if spawned
        if (spawns.IsActiveObject<T>(EqualChanced[i].guid))
        {
            if (!guid || EqualChanced[i].guid == guid)
            {
                Despawn1Object(EqualChanced[i].guid);
                spawns.RemoveObject<T>(EqualChanced[i].guid, poolId);
            }
        }
    }

    for (size_t i = 0; i < ExplicitlyChanced.size(); ++i)
    {
        // spawned
        if (spawns.IsActiveObject<T>(ExplicitlyChanced[i].guid))
        {
            if (!guid || ExplicitlyChanced[i].guid == guid)
            {
                Despawn1Object(ExplicitlyChanced[i].guid);
                spawns.RemoveObject<T>(ExplicitlyChanced[i].guid, poolId);
            }
        }
    }
}

// Method that is actualy doing the removal job on one creature
template<>
void PoolGroup<Creature>::Despawn1Object(uint32 guid)
{
    if (CreatureData const* data = sObjectMgr->GetCreatureData(guid))
    {
        sObjectMgr->RemoveCreatureFromGrid(guid, data);

        if (Creature* creature = ObjectAccessor::GetObjectInWorld(MAKE_NEW_GUID(guid, data->id, HIGHGUID_UNIT), (Creature*)NULL))
            creature->AddObjectToRemoveList();
    }
}

// Same on one gameobject
template<>
void PoolGroup<GameObject>::Despawn1Object(uint32 guid)
{
    if (GameObjectData const* data = sObjectMgr->GetGOData(guid))
    {
        sObjectMgr->RemoveGameobjectFromGrid(guid, data);

        if (GameObject* pGameobject = ObjectAccessor::GetObjectInWorld(MAKE_NEW_GUID(guid, data->id, HIGHGUID_GAMEOBJECT), (GameObject*)NULL))
            pGameobject->AddObjectToRemoveList();
    }
}

// Same on one pool
template<>
void PoolGroup<Pool>::Despawn1Object(uint32 child_pool_id)
{
    sPoolMgr->DespawnPool(child_pool_id);
}

// Same on one quest
template<>
void PoolGroup<Quest>::Despawn1Object(uint32 quest_id)
{
    // Creatures
    QuestRelations* questMap = sObjectMgr->GetCreatureQuestRelationMap();
    PooledQuestRelationBoundsNC qr = sPoolMgr->mQuestCreatureRelation.equal_range(quest_id);
    for (PooledQuestRelation::iterator itr = qr.first; itr != qr.second; ++itr)
    {
        QuestRelations::iterator qitr = questMap->find(itr->second);
        if (qitr == questMap->end())
            continue;
        QuestRelations::iterator lastElement = questMap->upper_bound(itr->second);
        for (; qitr != lastElement; ++qitr)
        {
            if (qitr->first == itr->second && qitr->second == itr->first)
            {
                questMap->erase(qitr);                  // iterator is now no more valid
                break;                                  // but we can exit loop since the element is found
            }
        }
    }

    // Gameobjects
    questMap = sObjectMgr->GetGOQuestRelationMap();
    qr = sPoolMgr->mQuestGORelation.equal_range(quest_id);
    for (PooledQuestRelation::iterator itr = qr.first; itr != qr.second; ++itr)
    {
        QuestRelations::iterator qitr = questMap->find(itr->second);
        if (qitr == questMap->end())
            continue;
        QuestRelations::iterator lastElement = questMap->upper_bound(itr->second);
        for (; qitr != lastElement; ++qitr)
        {
            if (qitr->first == itr->second && qitr->second == itr->first)
            {
                questMap->erase(qitr);                  // iterator is now no more valid
                break;                                  // but we can exit loop since the element is found
            }
        }
    }
}

// Method for a pool only to remove any found record causing a circular dependency loop
template<>
void PoolGroup<Pool>::RemoveOneRelation(uint32 child_pool_id)
{
    for (PoolObjectList::iterator itr = ExplicitlyChanced.begin(); itr != ExplicitlyChanced.end(); ++itr)
    {
        if (itr->guid == child_pool_id)
        {
            ExplicitlyChanced.erase(itr);
            break;
        }
    }
    for (PoolObjectList::iterator itr = EqualChanced.begin(); itr != EqualChanced.end(); ++itr)
    {
        if (itr->guid == child_pool_id)
        {
            EqualChanced.erase(itr);
            break;
        }
    }
}

template <class T>
void PoolGroup<T>::SpawnObject(ActivePoolData& spawns, uint32 limit, uint32 triggerFrom)
{
    uint32 lastDespawned = 0;
    int count = limit - spawns.GetActiveObjectCount(poolId);

    // If triggered from some object respawn this object is still marked as spawned
    // and also counted into m_SpawnedPoolAmount so we need increase count to be
    // spawned by 1
    if (triggerFrom)
        ++count;

    // This will try to spawn the rest of pool, not guaranteed
    for (int i = 0; i < count; ++i)
    {
        PoolObject* obj = RollOne(spawns, triggerFrom);
        if (!obj)
            continue;
        if (obj->guid == lastDespawned)
            continue;

        if (obj->guid == triggerFrom)
        {
            ReSpawn1Object(obj);
            triggerFrom = 0;
            continue;
        }
        spawns.ActivateObject<T>(obj->guid, poolId);
        Spawn1Object(obj);

        if (triggerFrom)
        {
            // One spawn one despawn no count increase
            DespawnObject(spawns, triggerFrom);
            lastDespawned = triggerFrom;
            triggerFrom = 0;
        }
    }
}

// Method that is actualy doing the spawn job on 1 creature
template <>
void PoolGroup<Creature>::Spawn1Object(PoolObject* obj)
{
    if (CreatureData const* data = sObjectMgr->GetCreatureData(obj->guid))
    {
        sObjectMgr->AddCreatureToGrid(obj->guid, data);

        // Spawn if necessary (loaded grids only)
        Map* map = sMapMgr->CreateBaseMap(data->mapid);
        // We use spawn coords to spawn
        if (!map->Instanceable() && map->IsGridLoaded(data->posX, data->posY))
        {
            Creature* creature = new Creature;
            //SF_LOG_DEBUG("pool", "Spawning creature %u", guid);
            if (!creature->LoadCreatureFromDB(obj->guid, map))
            {
                delete creature;
                return;
            }
        }
    }
}

// Same for 1 gameobject
template <>
void PoolGroup<GameObject>::Spawn1Object(PoolObject* obj)
{
    if (GameObjectData const* data = sObjectMgr->GetGOData(obj->guid))
    {
        sObjectMgr->AddGameobjectToGrid(obj->guid, data);
        // Spawn if necessary (loaded grids only)
        // this base map checked as non-instanced and then only existed
        Map* map = sMapMgr->CreateBaseMap(data->mapid);
        // We use current coords to unspawn, not spawn coords since creature can have changed grid
        if (!map->Instanceable() && map->IsGridLoaded(data->posX, data->posY))
        {
            GameObject* pGameobject = new GameObject;
            //SF_LOG_DEBUG("pool", "Spawning gameobject %u", guid);
            if (!pGameobject->LoadGameObjectFromDB(obj->guid, map, false))
            {
                delete pGameobject;
                return;
            }
            else
            {
                if (pGameobject->isSpawnedByDefault())
                    map->AddToMap(pGameobject);
            }
        }
    }
}

// Same for 1 pool
template <>
void PoolGroup<Pool>::Spawn1Object(PoolObject* obj)
{
    sPoolMgr->SpawnPool(obj->guid);
}

// Same for 1 quest
template<>
void PoolGroup<Quest>::Spawn1Object(PoolObject* obj)
{
    // Creatures
    QuestRelations* questMap = sObjectMgr->GetCreatureQuestRelationMap();
    PooledQuestRelationBoundsNC qr = sPoolMgr->mQuestCreatureRelation.equal_range(obj->guid);
    for (PooledQuestRelation::iterator itr = qr.first; itr != qr.second; ++itr)
    {
        SF_LOG_DEBUG("pool", "PoolGroup<Quest>: Adding quest %u to creature %u", itr->first, itr->second);
        questMap->insert(QuestRelations::value_type(itr->second, itr->first));
    }

    // Gameobjects
    questMap = sObjectMgr->GetGOQuestRelationMap();
    qr = sPoolMgr->mQuestGORelation.equal_range(obj->guid);
    for (PooledQuestRelation::iterator itr = qr.first; itr != qr.second; ++itr)
    {
        SF_LOG_DEBUG("pool", "PoolGroup<Quest>: Adding quest %u to GO %u", itr->first, itr->second);
        questMap->insert(QuestRelations::value_type(itr->second, itr->first));
    }
}

template <>
void PoolGroup<Quest>::SpawnObject(ActivePoolData& spawns, uint32 limit, uint32 triggerFrom)
{
    SF_LOG_DEBUG("pool", "PoolGroup<Quest>: Spawning pool %u", poolId);
    // load state from db
    if (!triggerFrom)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_POOL_QUEST_SAVE);

        stmt->setUInt32(0, poolId);

        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        if (result)
        {
            do
            {
                uint32 questId = result->Fetch()[0].GetUInt32();
                spawns.ActivateObject<Quest>(questId, poolId);
                PoolObject tempObj(questId, 0.0f);
                Spawn1Object(&tempObj);
                --limit;
            } while (result->NextRow() && limit);
            return;
        }
    }

    ActivePoolObjects currentQuests = spawns.GetActiveQuests();
    ActivePoolObjects newQuests;

    // always try to select different quests
    for (PoolObjectList::iterator itr = EqualChanced.begin(); itr != EqualChanced.end(); ++itr)
    {
        if (spawns.IsActiveObject<Quest>(itr->guid))
            continue;
        newQuests.insert(itr->guid);
    }

    // clear the pool
    DespawnObject(spawns);

    // recycle minimal amount of quests if possible count is lower than limit
    if (limit > newQuests.size() && !currentQuests.empty())
    {
        do
        {
            uint32 questId = Skyfire::Containers::SelectRandomContainerElement(currentQuests);
            newQuests.insert(questId);
            currentQuests.erase(questId);
        } while (newQuests.size() < limit && !currentQuests.empty()); // failsafe
    }

    if (newQuests.empty())
        return;

    // activate <limit> random quests
    do
    {
        uint32 questId = Skyfire::Containers::SelectRandomContainerElement(newQuests);
        spawns.ActivateObject<Quest>(questId, poolId);
        PoolObject tempObj(questId, 0.0f);
        Spawn1Object(&tempObj);
        newQuests.erase(questId);
        --limit;
    } while (limit && !newQuests.empty());

    // if we are here it means the pool is initialized at startup and did not have previous saved state
    if (!triggerFrom)
        sPoolMgr->SaveQuestsToDB();
}

// Method that does the respawn job on the specified creature
template <>
void PoolGroup<Creature>::ReSpawn1Object(PoolObject* obj)
{
    if (CreatureData const* data = sObjectMgr->GetCreatureData(obj->guid))
        if (Creature* creature = ObjectAccessor::GetObjectInWorld(MAKE_NEW_GUID(obj->guid, data->id, HIGHGUID_UNIT), (Creature*)NULL))
            creature->GetMap()->AddToMap(creature);
}

// Method that does the respawn job on the specified gameobject
template <>
void PoolGroup<GameObject>::ReSpawn1Object(PoolObject* obj)
{
    if (GameObjectData const* data = sObjectMgr->GetGOData(obj->guid))
        if (GameObject* pGameobject = ObjectAccessor::GetObjectInWorld(MAKE_NEW_GUID(obj->guid, data->id, HIGHGUID_GAMEOBJECT), (GameObject*)NULL))
            pGameobject->GetMap()->AddToMap(pGameobject);
}

// Nothing to do for a child Pool
template <>
void PoolGroup<Pool>::ReSpawn1Object(PoolObject* /*obj*/) { }

// Nothing to do for a quest
template <>
void PoolGroup<Quest>::ReSpawn1Object(PoolObject* /*obj*/) { }

////////////////////////////////////////////////////////////
// Methods of class PoolMgr

PoolMgr::PoolMgr() : max_pool_id(0) { }

void PoolMgr::Initialize()
{
    QueryResult result = WorldDatabase.Query("SELECT MAX(entry) FROM pool_template");
    if (result)
    {
        Field* fields = result->Fetch();
        max_pool_id = fields[0].GetUInt32();
    }

    mPoolTemplate.resize(max_pool_id + 1);
    mPoolCreatureGroups.resize(max_pool_id + 1);
    mPoolGameobjectGroups.resize(max_pool_id + 1);
    mPoolPoolGroups.resize(max_pool_id + 1);
    mPoolQuestGroups.resize(max_pool_id + 1);

    mQuestSearchMap.clear();
    mGameobjectSearchMap.clear();
    mCreatureSearchMap.clear();
}

void PoolMgr::LoadFromDB()
{
    // Pool templates
    {
        uint32 oldMSTime = getMSTime();

        QueryResult result = WorldDatabase.Query("SELECT entry, max_limit FROM pool_template");
        if (!result)
        {
            mPoolTemplate.clear();
            SF_LOG_INFO("server.loading", ">> Loaded 0 object pools. DB table `pool_template` is empty.");
            return;
        }

        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();

            uint32 pool_id = fields[0].GetUInt32();

            PoolTemplateData& pPoolTemplate = mPoolTemplate[pool_id];
            pPoolTemplate.MaxLimit = fields[1].GetUInt32();

            ++count;
        } while (result->NextRow());

        SF_LOG_INFO("server.loading", ">> Loaded %u objects pools in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    // Creatures

    SF_LOG_INFO("server.loading", "Loading Creatures Pooling Data...");
    {
        uint32 oldMSTime = getMSTime();

        //                                                 1       2         3
        QueryResult result = WorldDatabase.Query("SELECT guid, pool_entry, chance FROM pool_creature");

        if (!result)
        {
            SF_LOG_INFO("server.loading", ">> Loaded 0 creatures in  pools. DB table `pool_creature` is empty.");
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();

                uint32 guid = fields[0].GetUInt32();
                uint32 pool_id = fields[1].GetUInt32();
                float chance = fields[2].GetFloat();

                CreatureData const* data = sObjectMgr->GetCreatureData(guid);
                if (!data)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_creature` has a non existing creature spawn (GUID: %u) defined for pool id (%u), skipped.", guid, pool_id);
                    continue;
                }
                if (pool_id > max_pool_id)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_creature` pool id (%u) is out of range compared to max pool id in `pool_template`, skipped.", pool_id);
                    continue;
                }
                if (chance < 0 || chance > 100)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_creature` has an invalid chance (%f) for creature guid (%u) in pool id (%u), skipped.", chance, guid, pool_id);
                    continue;
                }
                PoolTemplateData* pPoolTemplate = &mPoolTemplate[pool_id];
                PoolObject plObject = PoolObject(guid, chance);
                PoolGroup<Creature>& cregroup = mPoolCreatureGroups[pool_id];
                cregroup.SetPoolId(pool_id);
                cregroup.AddEntry(plObject, pPoolTemplate->MaxLimit);
                SearchPair p(guid, pool_id);
                mCreatureSearchMap.insert(p);

                ++count;
            } while (result->NextRow());

            SF_LOG_INFO("server.loading", ">> Loaded %u creatures in pools in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // Gameobjects

    SF_LOG_INFO("server.loading", "Loading Gameobject Pooling Data...");
    {
        uint32 oldMSTime = getMSTime();

        //                                                 1        2         3
        QueryResult result = WorldDatabase.Query("SELECT guid, pool_entry, chance FROM pool_gameobject");

        if (!result)
        {
            SF_LOG_INFO("server.loading", ">> Loaded 0 gameobjects in  pools. DB table `pool_gameobject` is empty.");
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();

                uint32 guid = fields[0].GetUInt32();
                uint32 pool_id = fields[1].GetUInt32();
                float chance = fields[2].GetFloat();

                GameObjectData const* data = sObjectMgr->GetGOData(guid);
                if (!data)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_gameobject` has a non existing gameobject spawn (GUID: %u) defined for pool id (%u), skipped.", guid, pool_id);
                    continue;
                }

                GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(data->id);
                if (goinfo->type != GAMEOBJECT_TYPE_CHEST &&
                    goinfo->type != GAMEOBJECT_TYPE_GOOBER &&
                    goinfo->type != GAMEOBJECT_TYPE_FISHINGHOLE)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_gameobject` has a not lootable gameobject spawn (GUID: %u, type: %u) defined for pool id (%u), skipped.", guid, goinfo->type, pool_id);
                    continue;
                }

                if (pool_id > max_pool_id)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_gameobject` pool id (%u) is out of range compared to max pool id in `pool_template`, skipped.", pool_id);
                    continue;
                }

                if (chance < 0 || chance > 100)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_gameobject` has an invalid chance (%f) for gameobject guid (%u) in pool id (%u), skipped.", chance, guid, pool_id);
                    continue;
                }

                PoolTemplateData* pPoolTemplate = &mPoolTemplate[pool_id];
                PoolObject plObject = PoolObject(guid, chance);
                PoolGroup<GameObject>& gogroup = mPoolGameobjectGroups[pool_id];
                gogroup.SetPoolId(pool_id);
                gogroup.AddEntry(plObject, pPoolTemplate->MaxLimit);
                SearchPair p(guid, pool_id);
                mGameobjectSearchMap.insert(p);

                ++count;
            } while (result->NextRow());

            SF_LOG_INFO("server.loading", ">> Loaded %u gameobject in pools in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // Pool of pools

    SF_LOG_INFO("server.loading", "Loading Mother Pooling Data...");
    {
        uint32 oldMSTime = getMSTime();

        //                                                  1        2            3
        QueryResult result = WorldDatabase.Query("SELECT pool_id, mother_pool, chance FROM pool_pool");

        if (!result)
        {
            SF_LOG_INFO("server.loading", ">> Loaded 0 pools in pools");
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();

                uint32 child_pool_id = fields[0].GetUInt32();
                uint32 mother_pool_id = fields[1].GetUInt32();
                float chance = fields[2].GetFloat();

                if (mother_pool_id > max_pool_id)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_pool` mother_pool id (%u) is out of range compared to max pool id in `pool_template`, skipped.", mother_pool_id);
                    continue;
                }
                if (child_pool_id > max_pool_id)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_pool` included pool_id (%u) is out of range compared to max pool id in `pool_template`, skipped.", child_pool_id);
                    continue;
                }
                if (mother_pool_id == child_pool_id)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_pool` pool_id (%u) includes itself, dead-lock detected, skipped.", child_pool_id);
                    continue;
                }
                if (chance < 0 || chance > 100)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_pool` has an invalid chance (%f) for pool id (%u) in mother pool id (%u), skipped.", chance, child_pool_id, mother_pool_id);
                    continue;
                }
                PoolTemplateData* pPoolTemplateMother = &mPoolTemplate[mother_pool_id];
                PoolObject plObject = PoolObject(child_pool_id, chance);
                PoolGroup<Pool>& plgroup = mPoolPoolGroups[mother_pool_id];
                plgroup.SetPoolId(mother_pool_id);
                plgroup.AddEntry(plObject, pPoolTemplateMother->MaxLimit);
                SearchPair p(child_pool_id, mother_pool_id);
                mPoolSearchMap.insert(p);

                ++count;
            } while (result->NextRow());

            // Now check for circular reference
            for (uint32 i = 0; i < max_pool_id; ++i)
            {
                std::set<uint32> checkedPools;
                for (SearchMap::iterator poolItr = mPoolSearchMap.find(i); poolItr != mPoolSearchMap.end(); poolItr = mPoolSearchMap.find(poolItr->second))
                {
                    checkedPools.insert(poolItr->first);
                    if (checkedPools.find(poolItr->second) != checkedPools.end())
                    {
                        std::ostringstream ss;
                        ss << "The pool(s) ";
                        for (std::set<uint32>::const_iterator itr = checkedPools.begin(); itr != checkedPools.end(); ++itr)
                            ss << *itr << ' ';
                        ss << "create(s) a circular reference, which can cause the server to freeze.\nRemoving the last link between mother pool "
                            << poolItr->first << " and child pool " << poolItr->second;
                        SF_LOG_ERROR("sql.sql", "%s", ss.str().c_str());
                        mPoolPoolGroups[poolItr->second].RemoveOneRelation(poolItr->first);
                        mPoolSearchMap.erase(poolItr);
                        --count;
                        break;
                    }
                }
            }

            SF_LOG_INFO("server.loading", ">> Loaded %u pools in mother pools in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    SF_LOG_INFO("server.loading", "Loading Quest Pooling Data...");
    {
        uint32 oldMSTime = getMSTime();

        PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_QUEST_POOLS);
        PreparedQueryResult result = WorldDatabase.Query(stmt);

        if (!result)
        {
            SF_LOG_INFO("server.loading", ">> Loaded 0 quests in pools");
        }
        else
        {
            PooledQuestRelationBounds creBounds;
            PooledQuestRelationBounds goBounds;

            enum eQuestTypes
            {
                QUEST_NONE = 0,
                QUEST_DAILY = 1,
                QUEST_WEEKLY = 2
            };

            std::map<uint32, int32> poolTypeMap;
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();

                uint32 entry = fields[0].GetUInt32();
                uint32 pool_id = fields[1].GetUInt32();

                Quest const* quest = sObjectMgr->GetQuestTemplate(entry);
                if (!quest)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_quest` has a non existing quest template (Entry: %u) defined for pool id (%u), skipped.", entry, pool_id);
                    continue;
                }

                if (pool_id > max_pool_id)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_quest` pool id (%u) is out of range compared to max pool id in `pool_template`, skipped.", pool_id);
                    continue;
                }

                if (!quest->IsDailyOrWeekly())
                {
                    SF_LOG_ERROR("sql.sql", "`pool_quest` has an quest (%u) which is not daily or weekly in pool id (%u), use ExclusiveGroup instead, skipped.", entry, pool_id);
                    continue;
                }

                if (poolTypeMap[pool_id] == QUEST_NONE)
                    poolTypeMap[pool_id] = quest->IsDaily() ? QUEST_DAILY : QUEST_WEEKLY;

                int32 currType = quest->IsDaily() ? QUEST_DAILY : QUEST_WEEKLY;

                if (poolTypeMap[pool_id] != currType)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_quest` quest %u is %s but pool (%u) is specified for %s, mixing not allowed, skipped.",
                        entry, currType == QUEST_DAILY ? "QUEST_DAILY" : "QUEST_WEEKLY", pool_id, poolTypeMap[pool_id] == QUEST_DAILY ? "QUEST_DAILY" : "QUEST_WEEKLY");
                    continue;
                }

                creBounds = mQuestCreatureRelation.equal_range(entry);
                goBounds = mQuestGORelation.equal_range(entry);

                if (creBounds.first == creBounds.second && goBounds.first == goBounds.second)
                {
                    SF_LOG_ERROR("sql.sql", "`pool_quest` lists entry (%u) as member of pool (%u) but is not started anywhere, skipped.", entry, pool_id);
                    continue;
                }

                PoolTemplateData* pPoolTemplate = &mPoolTemplate[pool_id];
                PoolObject plObject = PoolObject(entry, 0.0f);
                PoolGroup<Quest>& questgroup = mPoolQuestGroups[pool_id];
                questgroup.SetPoolId(pool_id);
                questgroup.AddEntry(plObject, pPoolTemplate->MaxLimit);
                SearchPair p(entry, pool_id);
                mQuestSearchMap.insert(p);

                ++count;
            } while (result->NextRow());

            SF_LOG_INFO("server.loading", ">> Loaded %u quests in pools in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // The initialize method will spawn all pools not in an event and not in another pool, this is why there is 2 left joins with 2 null checks
    SF_LOG_INFO("server.loading", "Starting objects pooling system...");
    {
        uint32 oldMSTime = getMSTime();

        QueryResult result = WorldDatabase.Query("SELECT DISTINCT pool_template.entry, pool_pool.pool_id, pool_pool.mother_pool FROM pool_template"
            " LEFT JOIN game_event_pool ON pool_template.entry=game_event_pool.pool_entry"
            " LEFT JOIN pool_pool ON pool_template.entry=pool_pool.pool_id WHERE game_event_pool.pool_entry IS NULL");

        if (!result)
        {
            SF_LOG_INFO("server.loading", ">> Pool handling system initialized, 0 pools spawned.");
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                uint32 pool_entry = fields[0].GetUInt32();
                uint32 pool_pool_id = fields[1].GetUInt32();

                if (!CheckPool(pool_entry))
                {
                    if (pool_pool_id)
                        // The pool is a child pool in pool_pool table. Ideally we should remove it from the pool handler to ensure it never gets spawned,
                        // however that could recursively invalidate entire chain of mother pools. It can be done in the future but for now we'll do nothing.
                        SF_LOG_ERROR("sql.sql", "Pool Id %u has no equal chance pooled entites defined and explicit chance sum is not 100. This broken pool is a child pool of Id %u and cannot be safely removed.", pool_entry, fields[2].GetUInt32());
                    else
                        SF_LOG_ERROR("sql.sql", "Pool Id %u has no equal chance pooled entites defined and explicit chance sum is not 100. The pool will not be spawned.", pool_entry);
                    continue;
                }

                // Don't spawn child pools, they are spawned recursively by their parent pools
                if (!pool_pool_id)
                {
                    SpawnPool(pool_entry);
                    count++;
                }
            } while (result->NextRow());

            SF_LOG_DEBUG("pool", "Pool handling system initialized, %u pools spawned in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }
}

void PoolMgr::LoadQuestPools() { }

void PoolMgr::SaveQuestsToDB()
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    for (PoolGroupQuestMap::iterator itr = mPoolQuestGroups.begin(); itr != mPoolQuestGroups.end(); ++itr)
    {
        if (itr->isEmpty())
            continue;
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_POOL_SAVE);
        stmt->setUInt32(0, itr->GetPoolId());
        trans->Append(stmt);
    }

    for (SearchMap::iterator itr = mQuestSearchMap.begin(); itr != mQuestSearchMap.end(); ++itr)
    {
        if (IsSpawnedObject<Quest>(itr->first))
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_QUEST_POOL_SAVE);
            stmt->setUInt32(0, itr->second);
            stmt->setUInt32(1, itr->first);
            trans->Append(stmt);
        }
    }

    CharacterDatabase.CommitTransaction(trans);
}

void PoolMgr::ChangeDailyQuests()
{
    for (PoolGroupQuestMap::iterator itr = mPoolQuestGroups.begin(); itr != mPoolQuestGroups.end(); ++itr)
    {
        if (Quest const* quest = sObjectMgr->GetQuestTemplate(itr->GetFirstEqualChancedObjectId()))
        {
            if (quest->IsWeekly())
                continue;

            UpdatePool<Quest>(itr->GetPoolId(), 1);    // anything non-zero means don't load from db
        }
    }

    SaveQuestsToDB();
}

void PoolMgr::ChangeWeeklyQuests()
{
    for (PoolGroupQuestMap::iterator itr = mPoolQuestGroups.begin(); itr != mPoolQuestGroups.end(); ++itr)
    {
        if (Quest const* quest = sObjectMgr->GetQuestTemplate(itr->GetFirstEqualChancedObjectId()))
        {
            if (quest->IsDaily())
                continue;

            UpdatePool<Quest>(itr->GetPoolId(), 1);
        }
    }

    SaveQuestsToDB();
}

// Call to spawn a pool, if cache if true the method will spawn only if cached entry is different
// If it's same, the creature is respawned only (added back to map)
template<>
void PoolMgr::SpawnPool<Creature>(uint32 pool_id, uint32 db_guid)
{
    if (!mPoolCreatureGroups[pool_id].isEmpty())
        mPoolCreatureGroups[pool_id].SpawnObject(mSpawnedData, mPoolTemplate[pool_id].MaxLimit, db_guid);
}

// Call to spawn a pool, if cache if true the method will spawn only if cached entry is different
// If it's same, the gameobject is respawned only (added back to map)
template<>
void PoolMgr::SpawnPool<GameObject>(uint32 pool_id, uint32 db_guid)
{
    if (!mPoolGameobjectGroups[pool_id].isEmpty())
        mPoolGameobjectGroups[pool_id].SpawnObject(mSpawnedData, mPoolTemplate[pool_id].MaxLimit, db_guid);
}

// Call to spawn a pool, if cache if true the method will spawn only if cached entry is different
// If it's same, the pool is respawned only
template<>
void PoolMgr::SpawnPool<Pool>(uint32 pool_id, uint32 sub_pool_id)
{
    if (!mPoolPoolGroups[pool_id].isEmpty())
        mPoolPoolGroups[pool_id].SpawnObject(mSpawnedData, mPoolTemplate[pool_id].MaxLimit, sub_pool_id);
}

// Call to spawn a pool
template<>
void PoolMgr::SpawnPool<Quest>(uint32 pool_id, uint32 quest_id)
{
    if (!mPoolQuestGroups[pool_id].isEmpty())
        mPoolQuestGroups[pool_id].SpawnObject(mSpawnedData, mPoolTemplate[pool_id].MaxLimit, quest_id);
}

void PoolMgr::SpawnPool(uint32 pool_id)
{
    SpawnPool<Pool>(pool_id, 0);
    SpawnPool<GameObject>(pool_id, 0);
    SpawnPool<Creature>(pool_id, 0);
    SpawnPool<Quest>(pool_id, 0);
}

// Call to despawn a pool, all gameobjects/creatures in this pool are removed
void PoolMgr::DespawnPool(uint32 pool_id)
{
    if (!mPoolCreatureGroups[pool_id].isEmpty())
        mPoolCreatureGroups[pool_id].DespawnObject(mSpawnedData);

    if (!mPoolGameobjectGroups[pool_id].isEmpty())
        mPoolGameobjectGroups[pool_id].DespawnObject(mSpawnedData);

    if (!mPoolPoolGroups[pool_id].isEmpty())
        mPoolPoolGroups[pool_id].DespawnObject(mSpawnedData);

    if (!mPoolQuestGroups[pool_id].isEmpty())
        mPoolQuestGroups[pool_id].DespawnObject(mSpawnedData);
}

// Method that check chance integrity of the creatures and gameobjects in this pool
bool PoolMgr::CheckPool(uint32 pool_id) const
{
    return pool_id <= max_pool_id &&
        mPoolGameobjectGroups[pool_id].CheckPool() &&
        mPoolCreatureGroups[pool_id].CheckPool() &&
        mPoolPoolGroups[pool_id].CheckPool() &&
        mPoolQuestGroups[pool_id].CheckPool();
}

// Call to update the pool when a gameobject/creature part of pool [pool_id] is ready to respawn
// Here we cache only the creature/gameobject whose guid is passed as parameter
// Then the spawn pool call will use this cache to decide
template<typename T>
void PoolMgr::UpdatePool(uint32 pool_id, uint32 db_guid_or_pool_id)
{
    if (uint32 motherpoolid = IsPartOfAPool<Pool>(pool_id))
        SpawnPool<Pool>(motherpoolid, pool_id);
    else
        SpawnPool<T>(pool_id, db_guid_or_pool_id);
}

template void PoolMgr::UpdatePool<Pool>(uint32 pool_id, uint32 db_guid_or_pool_id);
template void PoolMgr::UpdatePool<GameObject>(uint32 pool_id, uint32 db_guid_or_pool_id);
template void PoolMgr::UpdatePool<Creature>(uint32 pool_id, uint32 db_guid_or_pool_id);
template void PoolMgr::UpdatePool<Quest>(uint32 pool_id, uint32 db_guid_or_pool_id);
