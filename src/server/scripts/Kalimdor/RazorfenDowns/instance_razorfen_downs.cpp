/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "razorfen_downs.h"
#include "Player.h"
#include "TemporarySummon.h"

#define    MAX_ENCOUNTER  1

class instance_razorfen_downs : public InstanceMapScript
{
public:
    instance_razorfen_downs() : InstanceMapScript("instance_razorfen_downs", 129) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const OVERRIDE
    {
        return new instance_razorfen_downs_InstanceMapScript(map);
    }

    struct instance_razorfen_downs_InstanceMapScript : public InstanceScript
    {
        instance_razorfen_downs_InstanceMapScript(Map* map) : InstanceScript(map)
        {
        }

        uint64 uiGongGUID;

        uint32 m_auiEncounter[MAX_ENCOUNTER];

        uint16 uiGongWaves;

        std::string str_data;

        void Initialize() OVERRIDE
        {
            uiGongGUID = 0;

            uiGongWaves = 0;

            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        std::string GetSaveData() OVERRIDE
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;

            saveStream << "T C " << m_auiEncounter[0]
                << ' ' << uiGongWaves;

            str_data = saveStream.str();

            OUT_SAVE_INST_DATA_COMPLETE;
            return str_data;
        }

        void Load(const char* in) OVERRIDE
        {
            if (!in)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(in);

            char dataHead1, dataHead2;
            uint16 data0, data1;

            std::istringstream loadStream(in);
            loadStream >> dataHead1 >> dataHead2 >> data0 >> data1;

            if (dataHead1 == 'T' && dataHead2 == 'C')
            {
                m_auiEncounter[0] = data0;

                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                    if (m_auiEncounter[i] == IN_PROGRESS)
                        m_auiEncounter[i] = NOT_STARTED;

                uiGongWaves = data1;
            } else OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void OnGameObjectCreate(GameObject* go) OVERRIDE
        {
            switch (go->GetEntry())
            {
                case GO_GONG:
                    uiGongGUID = go->GetGUID();
                    if (m_auiEncounter[0] == DONE)
                        go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                default:
                    break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) OVERRIDE
        {
            if (uiType == DATA_GONG_WAVES)
            {
                uiGongWaves = uiData;

                switch (uiGongWaves)
                {
                    case 9:
                    case 14:
                        if (GameObject* go = instance->GetGameObject(uiGongGUID))
                            go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    case 1:
                    case 10:
                    case 16:
                    {
                        GameObject* go = instance->GetGameObject(uiGongGUID);

                        if (!go)
                            return;

                        go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);

                        uint32 uiCreature = 0;
                        uint8 uiSummonTimes = 0;

                        switch (uiGongWaves)
                        {
                            case 1:
                                uiCreature = NPC_TOMB_FIEND;
                                uiSummonTimes = 7;
                                break;
                            case 10:
                                uiCreature = NPC_TOMB_REAVER;
                                uiSummonTimes = 3;
                                break;
                            case 16:
                                uiCreature = NPC_TUTEN_KASH;
                                break;
                            default:
                                break;
                        }

                        if (Creature* creature = go->SummonCreature(uiCreature, 2502.635f, 844.140f, 46.896f, 0.633f))
                        {
                            if (uiGongWaves == 10 || uiGongWaves == 1)
                            {
                                for (uint8 i = 0; i < uiSummonTimes; ++i)
                                {
                                    if (Creature* summon = go->SummonCreature(uiCreature, 2502.635f + float(std::rand() % 5 + -5), 844.140f + float(std::rand() % 5 + -5), 46.896f, 0.633f))
                                        summon->GetMotionMaster()->MovePoint(0, 2533.479f + float(std::rand() % 5 + -5), 870.020f + float(std::rand() % 5 + -5), 47.678f);
                                }
                            }
                            creature->GetMotionMaster()->MovePoint(0, 2533.479f + float(std::rand() % 5 + -5), 870.020f + float(std::rand() % 5 + -5), 47.678f);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            if (uiType == BOSS_TUTEN_KASH)
            {
                m_auiEncounter[0] = uiData;

                if (uiData == DONE)
                    SaveToDB();
            }
        }

        uint32 GetData(uint32 uiType) const OVERRIDE
        {
            switch (uiType)
            {
                case DATA_GONG_WAVES:
                    return uiGongWaves;
            }

            return 0;
        }

        uint64 GetData64(uint32 uiType) const OVERRIDE
        {
            switch (uiType)
            {
                case DATA_GONG: return uiGongGUID;
            }

            return 0;
        }
    };
};

void AddSC_instance_razorfen_downs()
{
    new instance_razorfen_downs();
}
