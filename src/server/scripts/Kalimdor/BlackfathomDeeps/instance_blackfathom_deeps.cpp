/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Instance_Blackfathom_Deeps
SD%Complete: 50
SDComment:
SDCategory: Blackfathom Deeps
EndScriptData */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "blackfathom_deeps.h"

#define MAX_ENCOUNTER 4

/* Encounter 0 = Gelihast
   Encounter 1 = Twilight Lord Kelris
   Encounter 2 = Shrine event
   Encounter 3 = Aku'Mai
 */

const Position LorgusPosition[4] =
{
    { -458.500610f, -38.343079f, -33.474445f, 0.0f },
    { -469.423615f, -88.400513f, -39.265102f, 0.0f },
    { -622.354980f, -10.350100f, -22.777000f, 0.0f },
    { -759.640564f,  16.658913f, -29.159529f, 0.0f }
};

const Position SpawnsLocation[] =
{
    {-775.431f, -153.853f, -25.871f, 3.207f},
    {-775.404f, -174.132f, -25.871f, 3.185f},
    {-862.430f, -154.937f, -25.871f, 0.060f},
    {-862.193f, -174.251f, -25.871f, 6.182f},
    {-863.895f, -458.899f, -33.891f, 5.637f}
};

class instance_blackfathom_deeps : public InstanceMapScript
{
public:
    instance_blackfathom_deeps() : InstanceMapScript("instance_blackfathom_deeps", 48) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const OVERRIDE
    {
        return new instance_blackfathom_deeps_InstanceMapScript(map);
    }

    struct instance_blackfathom_deeps_InstanceMapScript : public InstanceScript
    {
        instance_blackfathom_deeps_InstanceMapScript(Map* map) : InstanceScript(map) { }

        uint64 twilightLordKelrisGUID;
        uint64 shrine1GUID;
        uint64 shrine2GUID;
        uint64 shrine3GUID;
        uint64 shrine4GUID;
        uint64 shrineOfGelihastGUID;
        uint64 altarOfTheDeepsGUID;
        uint64 mainDoorGUID;

        uint8 encounter[MAX_ENCOUNTER];
        uint8 countFires;
        uint8 deathTimes;

        void Initialize() OVERRIDE
        {
            memset(&encounter, 0, sizeof(encounter));

            twilightLordKelrisGUID = 0;
            shrine1GUID = 0;
            shrine2GUID = 0;
            shrine3GUID = 0;
            shrine4GUID = 0;
            shrineOfGelihastGUID = 0;
            altarOfTheDeepsGUID = 0;
            mainDoorGUID = 0;
            countFires = 0;
            deathTimes = 0;
        }

        void OnCreatureCreate(Creature* creature) OVERRIDE
        {
            switch (creature->GetEntry())
            {
                case NPC_TWILIGHT_LORD_KELRIS:
                    twilightLordKelrisGUID = creature->GetGUID();
                    break;
                case NPC_LORGUS_JETT:
                    creature->SetHomePosition(LorgusPosition[std::rand() % 3]);
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go) OVERRIDE
        {
            switch (go->GetEntry())
            {
                case GO_FIRE_OF_AKU_MAI_1:
                    shrine1GUID = go->GetGUID();
                    break;
                case GO_FIRE_OF_AKU_MAI_2:
                    shrine2GUID = go->GetGUID();
                    break;
                case GO_FIRE_OF_AKU_MAI_3:
                    shrine3GUID = go->GetGUID();
                    break;
                case GO_FIRE_OF_AKU_MAI_4:
                    shrine4GUID = go->GetGUID();
                    break;
                case GO_SHRINE_OF_GELIHAST:
                    shrineOfGelihastGUID = go->GetGUID();
                    if (encounter[0] != DONE)
                        go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_ALTAR_OF_THE_DEEPS:
                    altarOfTheDeepsGUID = go->GetGUID();
                    if (encounter[3] != DONE)
                        go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_AKU_MAI_DOOR:
                    if (encounter[2] == DONE)
                        HandleGameObject(0, true, go);
                    mainDoorGUID = go->GetGUID();
                    break;
            }
        }

        void SetData(uint32 type, uint32 data) OVERRIDE
        {
            switch (type)
            {
                case TYPE_GELIHAST:
                    encounter[0] = data;
                    if (data == DONE)
                        if (GameObject* go = instance->GetGameObject(shrineOfGelihastGUID))
                            go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case TYPE_AKU_MAI:
                    encounter[3] = data;
                    if (data == DONE)
                        if (GameObject* go = instance->GetGameObject(altarOfTheDeepsGUID))
                        {
                            go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                            go->SummonCreature(NPC_MORRIDUNE, SpawnsLocation[4], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                        }
                    break;
                case DATA_FIRE:
                    countFires = data;
                    switch (countFires)
                    {
                        case 1:
                            if (GameObject* go = instance->GetGameObject(shrine1GUID))
                            {
                                go->SummonCreature(NPC_AKU_MAI_SNAPJAW, SpawnsLocation[0], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                go->SummonCreature(NPC_AKU_MAI_SNAPJAW, SpawnsLocation[1], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                go->SummonCreature(NPC_AKU_MAI_SNAPJAW, SpawnsLocation[2], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                go->SummonCreature(NPC_AKU_MAI_SNAPJAW, SpawnsLocation[3], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                            }
                            break;
                        case 2:
                            if (GameObject* go = instance->GetGameObject(shrine1GUID))
                            {
                                for (uint8 i = 0; i < 2; ++i)
                                {
                                    go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[0], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                    go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[1], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                    go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[2], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                    go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[3], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                }
                            }
                            break;
                        case 3:
                            if (GameObject* go = instance->GetGameObject(shrine1GUID))
                            {
                                go->SummonCreature(NPC_AKU_MAI_SERVANT, SpawnsLocation[1], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                go->SummonCreature(NPC_AKU_MAI_SERVANT, SpawnsLocation[2], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                            }
                            break;
                        case 4:
                            if (GameObject* go = instance->GetGameObject(shrine1GUID))
                            {
                                go->SummonCreature(NPC_BARBED_CRUSTACEAN, SpawnsLocation[0], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                go->SummonCreature(NPC_BARBED_CRUSTACEAN, SpawnsLocation[1], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                go->SummonCreature(NPC_BARBED_CRUSTACEAN, SpawnsLocation[2], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                                go->SummonCreature(NPC_BARBED_CRUSTACEAN, SpawnsLocation[3], TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);
                            }
                            break;
                    }
                    break;
                case DATA_EVENT:
                    deathTimes = data;
                    if (deathTimes == 18)
                        HandleGameObject(mainDoorGUID, true);
                    break;
            }
        }

        uint32 GetData(uint32 type) const OVERRIDE
        {
            switch (type)
            {
                case TYPE_GELIHAST:
                    return encounter[0];
                case TYPE_KELRIS:
                    return encounter[1];
                case TYPE_SHRINE:
                    return encounter[2];
                case TYPE_AKU_MAI:
                    return encounter[3];
                case DATA_FIRE:
                    return countFires;
                case DATA_EVENT:
                    return deathTimes;
            }

            return 0;
        }

        uint64 GetData64(uint32 data) const OVERRIDE
        {
            switch (data)
            {
                case DATA_TWILIGHT_LORD_KELRIS:
                    return twilightLordKelrisGUID;
                case DATA_SHRINE1:
                    return shrine1GUID;
                case DATA_SHRINE2:
                    return shrine2GUID;
                case DATA_SHRINE3:
                    return shrine3GUID;
                case DATA_SHRINE4:
                    return shrine4GUID;
                case DATA_SHRINE_OF_GELIHAST:
                    return shrineOfGelihastGUID;
                case DATA_MAINDOOR:
                    return mainDoorGUID;
            }

            return 0;
        }
    };
};

void AddSC_instance_blackfathom_deeps()
{
    new instance_blackfathom_deeps();
}
