/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/*
Name: Instance_The_Black_Morass
%Complete: 50
Comment: Quest support: 9836, 10297. Currently in progress.
Category: Caverns of Time, The Black Morass
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "the_black_morass.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "SpellInfo.h"
#include "ScriptedCreature.h"

enum Misc
{
    SPELL_RIFT_CHANNEL                = 31387,
    RIFT_BOSS                         = 1
};

inline uint32 RandRiftBoss() { return ((rand()%2) ? NPC_RIFT_KEEPER : NPC_RIFT_LORD); }

float PortalLocation[4][4]=
{
    {-2041.06f, 7042.08f, 29.99f, 1.30f},
    {-1968.18f, 7042.11f, 21.93f, 2.12f},
    {-1885.82f, 7107.36f, 22.32f, 3.07f},
    {-1928.11f, 7175.95f, 22.11f, 3.44f}
};

struct Wave
{
    uint32 PortalBoss;                                      //protector of current portal
    uint32 NextPortalTime;                                  //time to next portal, or 0 if portal boss need to be killed
};

static Wave RiftWaves[]=
{
    { RIFT_BOSS,                0 },
    { NPC_CRONO_LORD_DEJA,      0 },
    { RIFT_BOSS,           120000 },
    { NPC_TEMPORUS,        140000 },
    { RIFT_BOSS,           120000 },
    { NPC_AEONUS,               0 }
};

enum EventIds
{
    EVENT_NEXT_PORTAL = 1
};

class instance_the_black_morass : public InstanceMapScript
{
public:
    instance_the_black_morass() : InstanceMapScript("instance_the_black_morass", 269) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const OVERRIDE
    {
        return new instance_the_black_morass_InstanceMapScript(map);
    }

    struct instance_the_black_morass_InstanceMapScript : public InstanceScript
    {
        instance_the_black_morass_InstanceMapScript(Map* map) : InstanceScript(map) { }

        uint32 m_auiEncounter[EncounterCount];

        uint32 mRiftPortalCount;
        uint32 mShieldPercent;
        uint8  mRiftWaveCount;
        uint8  mRiftWaveId;

        uint64 _medivhGUID;
        uint8  _currentRiftId;

        void Initialize() OVERRIDE
        {
            _medivhGUID         = 0;
            Clear();
        }

        void Clear()
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

            mRiftPortalCount    = 0;
            mShieldPercent      = 100;
            mRiftWaveCount      = 0;
            mRiftWaveId         = 0;

            _currentRiftId      = 0;
        }

        void InitWorldState(bool Enable = true)
        {
            DoUpdateWorldState(WORLD_STATE_BM, Enable ? 1 : 0);
            DoUpdateWorldState(WORLD_STATE_BM_SHIELD, 100);
            DoUpdateWorldState(WORLD_STATE_BM_RIFT, 0);
        }

        bool IsEncounterInProgress() const OVERRIDE
        {
            if (GetData(TYPE_MEDIVH) == IN_PROGRESS)
                return true;

            return false;
        }

        void OnPlayerEnter(Player* player) OVERRIDE
        {
            if (GetData(TYPE_MEDIVH) == IN_PROGRESS)
                return;

            player->SendUpdateWorldState(WORLD_STATE_BM, 0);
        }

        void OnCreatureCreate(Creature* creature) OVERRIDE
        {
            if (creature->GetEntry() == NPC_MEDIVH)
                _medivhGUID = creature->GetGUID();
        }

        //what other conditions to check?
        bool CanProgressEvent()
        {
            if (instance->GetPlayers().isEmpty())
                return false;

            return true;
        }

        uint8 GetRiftWaveId()
        {
            switch (mRiftPortalCount)
            {
            case 6:
                mRiftWaveId = 2;
                return 1;
            case 12:
                mRiftWaveId = 4;
                return 3;
            case 18:
                return 5;
            default:
                return mRiftWaveId;
            }
        }

        void SetData(uint32 type, uint32 data) OVERRIDE
        {
            switch (type)
            {
            case TYPE_MEDIVH:
                if (data == SPECIAL && m_auiEncounter[0] == IN_PROGRESS)
                {
                    --mShieldPercent;

                    DoUpdateWorldState(WORLD_STATE_BM_SHIELD, mShieldPercent);

                    if (!mShieldPercent)
                    {
                        if (Creature* medivh = instance->GetCreature(_medivhGUID))
                        {
                            if (medivh->IsAlive())
                            {
                                medivh->DealDamage(medivh, medivh->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                                m_auiEncounter[0] = FAIL;
                                m_auiEncounter[1] = NOT_STARTED;
                            }
                        }
                    }
                }
                else
                {
                    if (data == IN_PROGRESS)
                    {
                        SF_LOG_DEBUG("scripts", "Instance The Black Morass: Starting event.");
                        InitWorldState();
                        m_auiEncounter[1] = IN_PROGRESS;
                        ScheduleEventNextPortal(15000);
                    }

                    if (data == DONE)
                    {
                        //this may be completed further out in the post-event
                        SF_LOG_DEBUG("scripts", "Instance The Black Morass: Event completed.");
                        Map::PlayerList const& players = instance->GetPlayers();

                        if (!players.isEmpty())
                        {
                            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                            {
                                if (Player* player = itr->GetSource())
                                {
                                    if (player->GetQuestStatus(QUEST_OPENING_PORTAL) == QUEST_STATUS_INCOMPLETE)
                                        player->AreaExploredOrEventHappens(QUEST_OPENING_PORTAL);

                                    if (player->GetQuestStatus(QUEST_MASTER_TOUCH) == QUEST_STATUS_INCOMPLETE)
                                        player->AreaExploredOrEventHappens(QUEST_MASTER_TOUCH);
                                }
                            }
                        }
                    }

                    m_auiEncounter[0] = data;
                }
                break;
            case TYPE_RIFT:
                if (data == SPECIAL)
                {
                    if (mRiftPortalCount < 7)
                        ScheduleEventNextPortal(5000);
                }
                else
                    m_auiEncounter[1] = data;
                break;
            }
        }

        uint32 GetData(uint32 type) const OVERRIDE
        {
            switch (type)
            {
            case TYPE_MEDIVH:
                return m_auiEncounter[0];
            case TYPE_RIFT:
                return m_auiEncounter[1];
            case DATA_PORTAL_COUNT:
                return mRiftPortalCount;
            case DATA_SHIELD:
                return mShieldPercent;
            }
            return 0;
        }

        uint64 GetData64(uint32 data) const OVERRIDE
        {
            if (data == DATA_MEDIVH)
                return _medivhGUID;

            return 0;
        }

        Creature* SummonedPortalBoss(Creature* me)
        {
            uint32 entry = RiftWaves[GetRiftWaveId()].PortalBoss;

            if (entry == RIFT_BOSS)
                entry = RandRiftBoss();

            SF_LOG_DEBUG("scripts", "Instance The Black Morass: Summoning rift boss entry %u.", entry);

            Position pos;
            me->GetRandomNearPosition(pos, 10.0f);

            //normalize Z-level if we can, if rift is not at ground level.
            pos.m_positionZ = std::max(me->GetMap()->GetHeight(pos.m_positionX, pos.m_positionY, MAX_HEIGHT), me->GetMap()->GetWaterLevel(pos.m_positionX, pos.m_positionY));

            if (Creature* summon = me->SummonCreature(entry, pos, TempSummonType::TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000))
                return summon;

            SF_LOG_DEBUG("scripts", "Instance The Black Morass: What just happened there? No boss, no loot, no fun...");
            return NULL;
        }

        void DoSpawnPortal()
        {
            if (Creature* medivh = instance->GetCreature(_medivhGUID))
            {
                uint8 tmp = std::rand() % 2;

                if (tmp >= _currentRiftId)
                    ++tmp;

                SF_LOG_DEBUG("scripts", "Instance The Black Morass: Creating Time Rift at locationId %i (old locationId was %u).", tmp, _currentRiftId);

                _currentRiftId = tmp;

                Creature* temp = medivh->SummonCreature(NPC_TIME_RIFT,
                    PortalLocation[tmp][0], PortalLocation[tmp][1], PortalLocation[tmp][2], PortalLocation[tmp][3],
                    TempSummonType::TEMPSUMMON_CORPSE_DESPAWN, 0);
                if (temp)
                {
                    temp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    temp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                    if (Creature* boss = SummonedPortalBoss(temp))
                    {
                        if (boss->GetEntry() == NPC_AEONUS)
                            boss->AddThreat(medivh, 0.0f);
                        else
                        {
                            boss->AddThreat(temp, 0.0f);
                            temp->CastSpell(boss, SPELL_RIFT_CHANNEL, false);
                        }
                    }
                }
            }
        }

        void Update(uint32 diff) OVERRIDE
        {
            if (m_auiEncounter[1] != IN_PROGRESS)
                return;

            //add delay timer?
            if (!CanProgressEvent())
            {
                Clear();
                return;
            }

            Events.Update(diff);

            if (Events.ExecuteEvent() == EVENT_NEXT_PORTAL)
            {
                ++mRiftPortalCount;
                DoUpdateWorldState(WORLD_STATE_BM_RIFT, mRiftPortalCount);
                DoSpawnPortal();
                ScheduleEventNextPortal(RiftWaves[GetRiftWaveId()].NextPortalTime);
            }
        }

        void ScheduleEventNextPortal(uint32 nextPortalTime)
        {
            if (nextPortalTime > 0)
                Events.RescheduleEvent(EVENT_NEXT_PORTAL, nextPortalTime);
        }

        protected:
            EventMap Events;
    };
};

void AddSC_instance_the_black_morass()
{
    new instance_the_black_morass();
}
