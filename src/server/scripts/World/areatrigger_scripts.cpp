/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Areatrigger_Scripts
SD%Complete: 100
SDComment: Scripts for areatriggers
SDCategory: Areatrigger
EndScriptData */

/* ContentData
at_dawning_valley
at_wu_song_village
at_fus_pond
at_dawning_valley2
at_pool_of_reflection
at_the_dawning_valley           q29409
at_coilfang_waterfall           4591
at_legion_teleporter            4560 Teleporter TO Invasion Point: Cataclysm
at_stormwright_shelf            q12741
at_last_rites                   q12019
at_sholazar_waygate             q12548
at_nats_landing                 q11209
at_bring_your_orphan_to         q910 q910 q1800 q1479 q1687 q1558 q10951 q10952
at_brewfest
at_area_52_entrance
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"

//5777
class AreaTrigger_at_darkspear_isle : public AreaTriggerScript
{
public:
    AreaTrigger_at_darkspear_isle() : AreaTriggerScript("at_darkspear_isle") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(24622) == QUEST_STATUS_COMPLETE)
        {
            if (player->FindNearestCreature(38930, 15.0f, true))
            {
                return false;
            }
            else
            {
                float x, y, z;
                player->GetClosePoint(x, y, z, player->GetObjectSize() / 3, -10.0f);
                if (Creature* zuni = player->SummonCreature(38930, x, y, z, 0.0f, TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 120000))
                {
                    zuni->GetMotionMaster()->MoveFollow(player, 2.5f, M_PI);
                    zuni->MonsterSay("Wait up, mon!", Language::LANG_UNIVERSAL, zuni);
                    zuni->SendPlaySound(21368, false);
                    return true;
                }
            }
        }
        return false;
    }
};

class AreaTrigger_at_dawning_span : public AreaTriggerScript
{
public:
    AreaTrigger_at_dawning_span() : AreaTriggerScript("at_dawning_span") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(29776) == QUEST_STATUS_COMPLETE)
        {
            if (Creature* lorewalkerZan = player->FindNearestCreature(64885, 25.0f, true))
            {
                if (!player->HasAura(116219))
                {
                    player->CastSpell(player, 116219);
                    lorewalkerZan->AI()->Talk(0, player);
                    return true;
                }
            }
        }
        return false;
    }
};

class AreaTrigger_at_chamber_of_whispers_entrance : public AreaTriggerScript
{
public:
    AreaTrigger_at_chamber_of_whispers_entrance() : AreaTriggerScript("at_chamber_of_whispers_entrance") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(29785) == QUEST_STATUS_INCOMPLETE)
        {
            if (!player->HasAura(104571))
            {
                player->CastSpell(player, 104593);
                return true;
            }
        }
        return false;
    }
};

const Position WugouPosMandori = { 927.5729f, 3610.2399f, 196.4969f };
//7858
class AreaTrigger_at_mandori_village_wugou : AreaTriggerScript
{
public:
    AreaTrigger_at_mandori_village_wugou() : AreaTriggerScript("at_mandori_village_wugou") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(29775) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(29775) == QUEST_STATUS_COMPLETE)
        {
            if (Creature* wugou = player->FindNearestCreature(55539, 25.0f, true))
            {
                wugou->SetOwnerGUID(0);
                wugou->GetMotionMaster()->MovePoint(0, WugouPosMandori);
                wugou->DespawnOrUnsummon(4000);
                return true;
            }
        }
        return false;
    }
};

const Position ShuPosMandori = { 880.8524f, 3606.0269f, 192.22139f };
//7116
class AreaTrigger_at_mandori_village_shu : AreaTriggerScript
{
public:
    AreaTrigger_at_mandori_village_shu() : AreaTriggerScript("at_mandori_village_shu") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(29775) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(29775) == QUEST_STATUS_COMPLETE)
        {
            if (Creature* shu = player->FindNearestCreature(55558, 25.0f, true))
            {
                shu->SetOwnerGUID(0);
                shu->GetMotionMaster()->MovePoint(0, ShuPosMandori);
                shu->DespawnOrUnsummon(7000);
                return true;
            }
        }
        return false;
    }
};

enum shrineOfInnerLight
{
    QUEST_THE_SPIRITS_GUARDIAN = 29420,
    NPC_HUOJIN_MONK = 60176,
};
//7736
class AreaTrigger_at_shrine_of_inner_light : AreaTriggerScript
{
public:
    AreaTrigger_at_shrine_of_inner_light() : AreaTriggerScript("at_shrine_of_inner_light") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_THE_SPIRITS_GUARDIAN) == QUEST_STATUS_COMPLETE)
        {
            if (!player->GetAura(92571))
            {
                if (Creature* huojinMonk = player->FindNearestCreature(NPC_HUOJIN_MONK, 15.0f, true))
                {
                    huojinMonk->CastSpell(player, 92571);
                    huojinMonk->AI()->Talk(0);
                    return true;
                }
            }
        }
        return false;
    }
};
enum dawningValley
{
    NPC_CHIA_HUI = 60248,
    NPC_BREWER_LIN = 60253,
    QUEST_PASSION_OF_SHEN_ZIN_SU = 29423,
};
//7750
class AreaTrigger_at_dawning_valley : AreaTriggerScript
{
public:
    AreaTrigger_at_dawning_valley() : AreaTriggerScript("at_dawning_valley") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_PASSION_OF_SHEN_ZIN_SU) == QUEST_STATUS_INCOMPLETE)
        {
            if (!player->GetAura(116220))
            {
                if (Creature* chiahui = player->FindNearestCreature(NPC_CHIA_HUI, 45.0f, true))
                {
                    if (chiahui->FindNearestCreature(54958, 45.0f, true))
                        chiahui->AI()->Talk(0);

                    if (Creature* lin = player->FindNearestCreature(NPC_BREWER_LIN, 45.0f, true))
                    {
                        if (lin->FindNearestCreature(54958, 45.0f, true))
                            lin->AI()->Talk(0);
                    }
                    chiahui->CastSpell(player, 116220);

                    return true;
                }
            }
        }
        return false;
    }
};

enum WuSongVillage
{
    QUEST_JI_OF_THE_HUOJIN = 29522,
    NPC_JI_FIREPAW = 54568
};
//7749
class AreaTrigger_at_wu_song_village : AreaTriggerScript
{
public:
    AreaTrigger_at_wu_song_village() : AreaTriggerScript("at_wu_song_village") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_JI_OF_THE_HUOJIN) == QUEST_STATUS_COMPLETE)
        {
            if (!player->GetAura(116219))
            {
                if (Creature* ji = player->FindNearestCreature(NPC_JI_FIREPAW, 15.0f, true))
                {
                    ji->CastSpell(player, 116219);
                    ji->AI()->Talk(0);
                    return true;
                }
            }
        }
        return false;
    }
};

enum fusPond
{
    QUEST_AYSA_OF_THE_TUSHUI = 29410,
    QUEST_MISSING_DRIVER = 29419,

    NPC_AYSA = 54567,
    NPC_LORVO = 54943,
};
//7748
class AreaTrigger_at_fus_pond : AreaTriggerScript
{
public:
    AreaTrigger_at_fus_pond() : AreaTriggerScript("at_fus_pond") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* trigger) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_AYSA_OF_THE_TUSHUI) == QUEST_STATUS_COMPLETE)
        {
            std::list<Creature*> lorvos;
            player->GetCreatureListWithEntryInGrid(lorvos, NPC_LORVO, 15.0f);
            if (!lorvos.empty())
            {
                for (std::list<Creature*>::iterator itr = lorvos.begin(); itr != lorvos.end(); ++itr)
                {
                    if ((*itr)->IsAlive() && (*itr)->GetGUIDLow() == 224519)
                    {
                        (*itr)->AI()->Talk(0);
                        return true;
                    }
                }
            }
        }

        if (player->GetQuestStatus(QUEST_MISSING_DRIVER) == QUEST_STATUS_COMPLETE)
        {
            if (Creature* aysa = player->FindNearestCreature(NPC_AYSA, 15.0f, true))
            {
                aysa->AI()->Talk(0, player);
                return true;
            }
        }
        return false;
    }
};

enum dawningValley2
{
    NPC_TRAINEE_GUANG = 60244,
};
//7747
class AreaTrigger_at_dawning_valley2 : AreaTriggerScript
{
public:
    AreaTrigger_at_dawning_valley2() : AreaTriggerScript("at_dawning_valley2") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_AYSA_OF_THE_TUSHUI) == QUEST_STATUS_COMPLETE)
        {
            if (!player->GetAura(116220))
            {
                if (Creature* guang = player->FindNearestCreature(NPC_TRAINEE_GUANG, 45.0f, true))
                {
                    guang->AI()->Talk(0, player);
                    guang->CastSpell(player, 116220);
                    return true;
                }
            }
        }
        return false;
    }
};

class AreaTrigger_at_pool_of_reflection : public AreaTriggerScript
{
public:
    AreaTrigger_at_pool_of_reflection() : AreaTriggerScript("at_pool_of_reflection") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        player->CastSpell(player, 108590);
        return true;
    }
};

enum the_dawning_valley
{
    NPC_TRAINEE_NIM = 60183,
    QUEST_DISCIPLE_CHALLENGE = 29409,
    AT_SPELL_FORCE_REACTION = 102429,
};

class AreaTrigger_at_the_dawning_valley : AreaTriggerScript
{
public:
    AreaTrigger_at_the_dawning_valley() : AreaTriggerScript("at_the_dawning_valley") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_DISCIPLE_CHALLENGE) == QUEST_STATUS_INCOMPLETE)
        {
            if (!player->HasAura(AT_SPELL_FORCE_REACTION))
            {
                if (Creature* creature = player->FindNearestCreature(NPC_TRAINEE_NIM, 25.0f, true))
                {
                    player->CastSpell(player, AT_SPELL_FORCE_REACTION, false);
                    creature->AI()->Talk(0, player);
                    return true;
                }
            }
        }
        return false;
    }
};


/*######
## at_coilfang_waterfall
######*/

enum CoilfangGOs
{
    GO_COILFANG_WATERFALL   = 184212
};

class AreaTrigger_at_coilfang_waterfall : public AreaTriggerScript
{
    public:
        AreaTrigger_at_coilfang_waterfall() : AreaTriggerScript("at_coilfang_waterfall") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
        {
            if (GameObject* go = GetClosestGameObjectWithEntry(player, GO_COILFANG_WATERFALL, 35.0f))
                if (go->getLootState() == LootState::GO_READY)
                    go->UseDoorOrButton();

            return false;
        }
};

/*#####
## at_legion_teleporter
#####*/

enum LegionTeleporter
{
    SPELL_TELE_A_TO         = 37387,
    QUEST_GAINING_ACCESS_A  = 10589,

    SPELL_TELE_H_TO         = 37389,
    QUEST_GAINING_ACCESS_H  = 10604
};

class AreaTrigger_at_legion_teleporter : public AreaTriggerScript
{
    public:
        AreaTrigger_at_legion_teleporter() : AreaTriggerScript("at_legion_teleporter") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
        {
            if (player->IsAlive() && !player->IsInCombat())
            {
                if (player->GetTeam() == ALLIANCE && player->GetQuestRewardStatus(QUEST_GAINING_ACCESS_A))
                {
                    player->CastSpell(player, SPELL_TELE_A_TO, false);
                    return true;
                }

                if (player->GetTeam() == HORDE && player->GetQuestRewardStatus(QUEST_GAINING_ACCESS_H))
                {
                    player->CastSpell(player, SPELL_TELE_H_TO, false);
                    return true;
                }

                return false;
            }
            return false;
        }
};

/*######
## at_stormwright_shelf
######*/

enum StormwrightShelf
{
    QUEST_STRENGTH_OF_THE_TEMPEST               = 12741,

    SPELL_CREATE_TRUE_POWER_OF_THE_TEMPEST      = 53067
};

class AreaTrigger_at_stormwright_shelf : public AreaTriggerScript
{
    public:
        AreaTrigger_at_stormwright_shelf() : AreaTriggerScript("at_stormwright_shelf") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
        {
            if (!player->isDead() && player->GetQuestStatus(QUEST_STRENGTH_OF_THE_TEMPEST) == QUEST_STATUS_INCOMPLETE)
                player->CastSpell(player, SPELL_CREATE_TRUE_POWER_OF_THE_TEMPEST, false);

            return true;
        }
};

/*######
## at_scent_larkorwi
######*/

enum ScentLarkorwi
{
    QUEST_SCENT_OF_LARKORWI                     = 4291,
    NPC_LARKORWI_MATE                           = 9683
};

class AreaTrigger_at_scent_larkorwi : public AreaTriggerScript
{
    public:
        AreaTrigger_at_scent_larkorwi() : AreaTriggerScript("at_scent_larkorwi") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
        {
            if (!player->isDead() && player->GetQuestStatus(QUEST_SCENT_OF_LARKORWI) == QUEST_STATUS_INCOMPLETE)
            {
                if (!player->FindNearestCreature(NPC_LARKORWI_MATE, 15))
                    player->SummonCreature(NPC_LARKORWI_MATE, player->GetPositionX()+5, player->GetPositionY(), player->GetPositionZ(), 3.3f, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100000);
            }

            return false;
        }
};

/*#####
## at_last_rites
#####*/

enum AtLastRites
{
    QUEST_LAST_RITES                          = 12019,
    QUEST_BREAKING_THROUGH                    = 11898,
};

class AreaTrigger_at_last_rites : public AreaTriggerScript
{
    public:
        AreaTrigger_at_last_rites() : AreaTriggerScript("at_last_rites") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger) OVERRIDE
        {
            if (!(player->GetQuestStatus(QUEST_LAST_RITES) == QUEST_STATUS_INCOMPLETE ||
                player->GetQuestStatus(QUEST_LAST_RITES) == QUEST_STATUS_COMPLETE ||
                player->GetQuestStatus(QUEST_BREAKING_THROUGH) == QUEST_STATUS_INCOMPLETE ||
                player->GetQuestStatus(QUEST_BREAKING_THROUGH) == QUEST_STATUS_COMPLETE))
                return false;

            WorldLocation pPosition;

            switch (trigger->id)
            {
                case 5332:
                case 5338:
                    pPosition = WorldLocation(571, 3733.68f, 3563.25f, 290.812f, 3.665192f);
                    break;
                case 5334:
                    pPosition = WorldLocation(571, 3802.38f, 3585.95f, 49.5765f, 0.0f);
                    break;
                case 5340:
                    if (player->GetQuestStatus(QUEST_LAST_RITES) == QUEST_STATUS_INCOMPLETE ||
                        player->GetQuestStatus(QUEST_LAST_RITES) == QUEST_STATUS_COMPLETE)
                        pPosition = WorldLocation(571, 3687.91f, 3577.28f, 473.342f);
                    else
                        pPosition = WorldLocation(571, 3739.38f, 3567.09f, 341.58f);
                    break;
                default:
                    return false;
            }

            player->TeleportTo(pPosition);

            return false;
        }
};

/*######
## at_sholazar_waygate
######*/

enum Waygate
{
    SPELL_SHOLAZAR_TO_UNGORO_TELEPORT           = 52056,
    SPELL_UNGORO_TO_SHOLAZAR_TELEPORT           = 52057,

    AT_SHOLAZAR                                 = 5046,
    AT_UNGORO                                   = 5047,

    QUEST_THE_MAKERS_OVERLOOK                   = 12613,
    QUEST_THE_MAKERS_PERCH                      = 12559,
    QUEST_MEETING_A_GREAT_ONE                   = 13956,
};

class AreaTrigger_at_sholazar_waygate : public AreaTriggerScript
{
    public:
        AreaTrigger_at_sholazar_waygate() : AreaTriggerScript("at_sholazar_waygate") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger) OVERRIDE
        {
            if (!player->isDead() && (player->GetQuestStatus(QUEST_MEETING_A_GREAT_ONE) != QUEST_STATUS_NONE ||
                (player->GetQuestStatus(QUEST_THE_MAKERS_OVERLOOK) == QUEST_STATUS_REWARDED && player->GetQuestStatus(QUEST_THE_MAKERS_PERCH) == QUEST_STATUS_REWARDED)))
            {
                switch (trigger->id)
                {
                    case AT_SHOLAZAR:
                        player->CastSpell(player, SPELL_SHOLAZAR_TO_UNGORO_TELEPORT, false);
                        break;

                    case AT_UNGORO:
                        player->CastSpell(player, SPELL_UNGORO_TO_SHOLAZAR_TELEPORT, false);
                        break;
                }
            }

            return false;
        }
};

/*######
## at_nats_landing
######*/

enum NatsLanding
{
    QUEST_NATS_BARGAIN = 11209,
    SPELL_FISH_PASTE   = 42644,
    NPC_LURKING_SHARK  = 23928
};

class AreaTrigger_at_nats_landing : public AreaTriggerScript
{
    public:
        AreaTrigger_at_nats_landing() : AreaTriggerScript("at_nats_landing") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) OVERRIDE
        {
            if (!player->IsAlive() || !player->HasAura(SPELL_FISH_PASTE))
                return false;

            if (player->GetQuestStatus(QUEST_NATS_BARGAIN) == QUEST_STATUS_INCOMPLETE)
            {
                if (!player->FindNearestCreature(NPC_LURKING_SHARK, 20.0f))
                {
                    if (Creature* shark = player->SummonCreature(NPC_LURKING_SHARK, -4246.243f, -3922.356f, -7.488f, 5.0f, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100000))
                        shark->AI()->AttackStart(player);

                    return false;
                }
            }
            return true;
        }
};

/*######
## at_brewfest
######*/

enum Brewfest
{
    NPC_TAPPER_SWINDLEKEG       = 24711,
    NPC_IPFELKOFER_IRONKEG      = 24710,

    AT_BREWFEST_DUROTAR         = 4829,
    AT_BREWFEST_DUN_MOROGH      = 4820,

    SAY_WELCOME                 = 4,

    AREATRIGGER_TALK_COOLDOWN   = 5, // in seconds
};

class AreaTrigger_at_brewfest : public AreaTriggerScript
{
    public:
        AreaTrigger_at_brewfest() : AreaTriggerScript("at_brewfest")
        {
            // Initialize for cooldown
            _triggerTimes[AT_BREWFEST_DUROTAR] = _triggerTimes[AT_BREWFEST_DUN_MOROGH] = 0;
        }

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger) OVERRIDE
        {
            uint32 triggerId = trigger->id;
            // Second trigger happened too early after first, skip for now
            if (sWorld->GetGameTime() - _triggerTimes[triggerId] < AREATRIGGER_TALK_COOLDOWN)
                return false;

            switch (triggerId)
            {
                case AT_BREWFEST_DUROTAR:
                    if (Creature* tapper = player->FindNearestCreature(NPC_TAPPER_SWINDLEKEG, 20.0f))
                        tapper->AI()->Talk(SAY_WELCOME, player);
                    break;
                case AT_BREWFEST_DUN_MOROGH:
                    if (Creature* ipfelkofer = player->FindNearestCreature(NPC_IPFELKOFER_IRONKEG, 20.0f))
                        ipfelkofer->AI()->Talk(SAY_WELCOME, player);
                    break;
                default:
                    break;
            }

            _triggerTimes[triggerId] = sWorld->GetGameTime();
            return false;
        }

    private:
        std::map<uint32, time_t> _triggerTimes;
};

/*######
## at_area_52_entrance
######*/

enum Area52Entrance
{
    SPELL_A52_NEURALYZER  = 34400,
    NPC_SPOTLIGHT         = 19913,
    SUMMON_COOLDOWN       = 5,

    AT_AREA_52_SOUTH      = 4472,
    AT_AREA_52_NORTH      = 4466,
    AT_AREA_52_WEST       = 4471,
    AT_AREA_52_EAST       = 4422,
};

class AreaTrigger_at_area_52_entrance : public AreaTriggerScript
{
    public:
        AreaTrigger_at_area_52_entrance() : AreaTriggerScript("at_area_52_entrance")
        {
            _triggerTimes[AT_AREA_52_SOUTH] = _triggerTimes[AT_AREA_52_NORTH] = _triggerTimes[AT_AREA_52_WEST] = _triggerTimes[AT_AREA_52_EAST] = 0;
        }

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger) OVERRIDE
        {
            float x = 0.0f, y = 0.0f, z = 0.0f;

            if (!player->IsAlive())
                return false;

            uint32 triggerId = trigger->id;
            if (sWorld->GetGameTime() - _triggerTimes[trigger->id] < SUMMON_COOLDOWN)
                return false;

            switch (triggerId)
            {
                case AT_AREA_52_EAST:
                    x = 3044.176f;
                    y = 3610.692f;
                    z = 143.61f;
                    break;
                case AT_AREA_52_NORTH:
                    x = 3114.87f;
                    y = 3687.619f;
                    z = 143.62f;
                    break;
                case AT_AREA_52_WEST:
                    x = 3017.79f;
                    y = 3746.806f;
                    z = 144.27f;
                    break;
                case AT_AREA_52_SOUTH:
                    x = 2950.63f;
                    y = 3719.905f;
                    z = 143.33f;
                    break;
            }

            player->SummonCreature(NPC_SPOTLIGHT, x, y, z, 0.0f, TempSummonType::TEMPSUMMON_TIMED_DESPAWN, 5000);
            player->AddAura(SPELL_A52_NEURALYZER, player);
            _triggerTimes[trigger->id] = sWorld->GetGameTime();
            return false;
        }

    private:
        std::map<uint32, time_t> _triggerTimes;
};

/*######
 ## at_frostgrips_hollow
 ######*/

enum FrostgripsHollow
{
    QUEST_THE_LONESOME_WATCHER      = 12877,

    NPC_STORMFORGED_MONITOR         = 29862,
    NPC_STORMFORGED_ERADICTOR       = 29861,

    TYPE_WAYPOINT                   = 0,
    DATA_START                      = 0
};

Position const stormforgedMonitorPosition = {6963.95f, 45.65f, 818.71f, 4.948f};
Position const stormforgedEradictorPosition = {6983.18f, 7.15f, 806.33f, 2.228f};

class AreaTrigger_at_frostgrips_hollow : public AreaTriggerScript
{
public:
    AreaTrigger_at_frostgrips_hollow() : AreaTriggerScript("at_frostgrips_hollow")
    {
        stormforgedMonitorGUID = 0;
        stormforgedEradictorGUID = 0;
    }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /* trigger */) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_THE_LONESOME_WATCHER) != QUEST_STATUS_INCOMPLETE)
            return false;

        Creature* stormforgedMonitor = Creature::GetCreature(*player, stormforgedMonitorGUID);
        if (stormforgedMonitor)
            return false;

        Creature* stormforgedEradictor = Creature::GetCreature(*player, stormforgedEradictorGUID);
        if (stormforgedEradictor)
            return false;

        stormforgedMonitor = player->SummonCreature(NPC_STORMFORGED_MONITOR, stormforgedMonitorPosition, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
        if (stormforgedMonitor)
        {
            stormforgedMonitorGUID = stormforgedMonitor->GetGUID();
            stormforgedMonitor->SetWalk(false);
            /// The npc would search an alternative way to get to the last waypoint without this unit state.
            stormforgedMonitor->AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);
            stormforgedMonitor->GetMotionMaster()->MovePath(NPC_STORMFORGED_MONITOR * 100, false);
        }

        stormforgedEradictor = player->SummonCreature(NPC_STORMFORGED_ERADICTOR, stormforgedEradictorPosition, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
        if (stormforgedEradictor)
        {
            stormforgedEradictorGUID = stormforgedEradictor->GetGUID();
            stormforgedEradictor->GetMotionMaster()->MovePath(NPC_STORMFORGED_ERADICTOR * 100, false);
        }

        return true;
    }

private:
    uint64 stormforgedMonitorGUID;
    uint64 stormforgedEradictorGUID;
};

void AddSC_areatrigger_scripts()
{
    new AreaTrigger_at_darkspear_isle();
    new AreaTrigger_at_dawning_span();
    new AreaTrigger_at_chamber_of_whispers_entrance();
    new AreaTrigger_at_mandori_village_wugou();
    new AreaTrigger_at_mandori_village_shu();
    new AreaTrigger_at_shrine_of_inner_light();
    new AreaTrigger_at_dawning_valley2();
    new AreaTrigger_at_dawning_valley();
    new AreaTrigger_at_wu_song_village();
    new AreaTrigger_at_fus_pond();
    new AreaTrigger_at_pool_of_reflection();
    new AreaTrigger_at_the_dawning_valley();
    new AreaTrigger_at_coilfang_waterfall();
    new AreaTrigger_at_legion_teleporter();
    new AreaTrigger_at_stormwright_shelf();
    new AreaTrigger_at_scent_larkorwi();
    new AreaTrigger_at_last_rites();
    new AreaTrigger_at_sholazar_waygate();
    new AreaTrigger_at_nats_landing();
    new AreaTrigger_at_brewfest();
    new AreaTrigger_at_area_52_entrance();
    new AreaTrigger_at_frostgrips_hollow();
}
