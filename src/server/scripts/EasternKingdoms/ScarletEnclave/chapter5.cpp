/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "Player.h"

#define LESS_MOB // if you do not have a good server and do not want it to be laggy as hell
//Light of Dawn
enum mograine
{
#ifdef LESS_MOB
    ENCOUNTER_DK_NUMBER               = 5,  // how many player queue to start the quest, or -
    ENCOUNTER_DK_TIMER                = 10, // *every 5 minutes. These have to be done in instance data
    ENCOUNTER_DEFENDER_NUMBER         = 10, // how many of defender
    ENCOUNTER_EARTHSHATTER_NUMBER     = 1, // how many of earthshatter
    ENCOUNTER_ABOMINATION_NUMBER      = 2,  // how many of abomination
    ENCOUNTER_BEHEMOTH_NUMBER         = 1,  // how many of behemoth
    ENCOUNTER_GHOUL_NUMBER            = 5, // how many of ghoul
    ENCOUNTER_WARRIOR_NUMBER          = 1,  // how many of warrior
#else
    ENCOUNTER_DK_NUMBER               = 5,  // how many player queue to start the quest, or -
    ENCOUNTER_DK_TIMER                = 10, // *every 5 minutes. These have to be done in instance data
    ENCOUNTER_DEFENDER_NUMBER         = 20, // how many of defender
    ENCOUNTER_EARTHSHATTER_NUMBER     = 20, // how many of earthshatter
    ENCOUNTER_ABOMINATION_NUMBER      = 3,  // how many of abomination
    ENCOUNTER_BEHEMOTH_NUMBER         = 2,  // how many of behemoth
    ENCOUNTER_GHOUL_NUMBER            = 10, // how many of ghoul
    ENCOUNTER_WARRIOR_NUMBER          = 2,  // how many of warrior
#endif
    ENCOUNTER_TOTAL_DAWN              = 300,  // Total number
    ENCOUNTER_TOTAL_SCOURGE           = 10000,

    WORLD_STATE_REMAINS               = 3592,
    WORLD_STATE_COUNTDOWN             = 3603,
    WORLD_STATE_EVENT_BEGIN           = 3605,

    SAY_LIGHT_OF_DAWN01               = 0, // pre text
    SAY_LIGHT_OF_DAWN02               = 1,
    SAY_LIGHT_OF_DAWN03               = 2,
    SAY_LIGHT_OF_DAWN04               = 3, // intro
    SAY_LIGHT_OF_DAWN05               = 4,
    SAY_LIGHT_OF_DAWN06               = 5,
    SAY_LIGHT_OF_DAWN07               = 6, // During the fight - Korfax, Champion of the Light
    SAY_LIGHT_OF_DAWN08               = 7, // Lord Maxwell Tyrosus
    SAY_LIGHT_OF_DAWN09               = 8, // Highlord Darion Mograine
    SAY_LIGHT_OF_DAWN25               = 24, // After the fight
    SAY_LIGHT_OF_DAWN26               = 25, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN27               = 26, // Highlord Darion Mograine
    SAY_LIGHT_OF_DAWN28               = 27, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN29               = 28, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN30               = 29, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN31               = 30, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN32               = 31, // Highlord Alexandros Mograine
    SAY_LIGHT_OF_DAWN33               = 32, // Highlord Darion Mograine
    SAY_LIGHT_OF_DAWN34               = 33, // Highlord Darion Mograine
    SAY_LIGHT_OF_DAWN35               = 34, // Darion Mograine
    SAY_LIGHT_OF_DAWN36               = 35, // Darion Mograine
    SAY_LIGHT_OF_DAWN37               = 36, // Highlord Alexandros Mograine
    SAY_LIGHT_OF_DAWN38               = 37, // Darion Mograine
    SAY_LIGHT_OF_DAWN39               = 38, // Highlord Alexandros Mograine
    SAY_LIGHT_OF_DAWN40               = 39, // Darion Mograine
    SAY_LIGHT_OF_DAWN41               = 40, // Highlord Alexandros Mograine
    SAY_LIGHT_OF_DAWN42               = 41, // Highlord Alexandros Mograine
    SAY_LIGHT_OF_DAWN43               = 42, // The Lich King
    SAY_LIGHT_OF_DAWN44               = 43, // Highlord Darion Mograine
    SAY_LIGHT_OF_DAWN45               = 44, // The Lich King
    SAY_LIGHT_OF_DAWN46               = 45, // The Lich King
    SAY_LIGHT_OF_DAWN47               = 46, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN48               = 47, // The Lich King
    SAY_LIGHT_OF_DAWN49               = 48, // The Lich King
    SAY_LIGHT_OF_DAWN50               = 49, // Lord Maxwell Tyrosus
    SAY_LIGHT_OF_DAWN51               = 50, // The Lich King
    SAY_LIGHT_OF_DAWN52               = 51, // Highlord Darion Mograine
    SAY_LIGHT_OF_DAWN53               = 52, // Highlord Darion Mograine
    SAY_LIGHT_OF_DAWN54               = 53, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN55               = 54, // The Lich King
    SAY_LIGHT_OF_DAWN56               = 55, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN57               = 56, // The Lich King
    SAY_LIGHT_OF_DAWN58               = 57, // The Lich King
    SAY_LIGHT_OF_DAWN59               = 58, // The Lich King
    SAY_LIGHT_OF_DAWN60               = 59, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN61               = 60, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN62               = 61, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN63               = 62, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN64               = 63, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN65               = 64, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN66               = 65, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN67               = 66, // Highlord Tirion Fordring
    SAY_LIGHT_OF_DAWN68               = 67, // Highlord Darion Mograine

    EMOTE_LIGHT_OF_DAWN01             = 68,  // Emotes
    EMOTE_LIGHT_OF_DAWN02             = 69,
    EMOTE_LIGHT_OF_DAWN03             = 70,
    EMOTE_LIGHT_OF_DAWN04             = 71,
    EMOTE_LIGHT_OF_DAWN05             = 72,
    EMOTE_LIGHT_OF_DAWN06             = 73,
    EMOTE_LIGHT_OF_DAWN07             = 74,
    EMOTE_LIGHT_OF_DAWN08             = 75,
    EMOTE_LIGHT_OF_DAWN09             = 76,
    EMOTE_LIGHT_OF_DAWN10             = 77,
    EMOTE_LIGHT_OF_DAWN11             = 78,
    EMOTE_LIGHT_OF_DAWN12             = 79,
    EMOTE_LIGHT_OF_DAWN13             = 80,
    EMOTE_LIGHT_OF_DAWN14             = 81,
    EMOTE_LIGHT_OF_DAWN15             = 82,
    EMOTE_LIGHT_OF_DAWN16             = 83,
    EMOTE_LIGHT_OF_DAWN17             = 84,
    EMOTE_LIGHT_OF_DAWN18             = 85,

    GO_LIGHT_OF_DAWN                  = 191330,
    SPELL_THE_LIGHT_OF_DAWN_Q         = 53606, // quest credit

    // ---- Dark Knight npc --------------------
    // Highlord Darion Mograine
    NPC_HIGHLORD_DARION_MOGRAINE      = 29173,
    SPELL_ANTI_MAGIC_ZONE1            = 52893,
    SPELL_DEATH_STRIKE                = 53639,
    SPELL_DEATH_EMBRACE               = 53635,
    SPELL_ICY_TOUCH1                  = 49723,
    SPELL_THE_LIGHT_OF_DAWN           = 53658,
    SPELL_THE_MIGHT_OF_MOGRAINE       = 53642, // on players when begins
    SPELL_UNHOLY_BLIGHT               = 53640,
    SPELL_ALEXANDROS_MOGRAINE_SPAWN   = 53667,
    SPELL_MOGRAINE_CHARGE             = 53679,
    SPELL_ASHBRINGER                  = 53701,

    // Koltira Deathweaver & Orbaz Bloodbane are using the same abilities
    NPC_KOLTIRA_DEATHWEAVER           = 29199,
    NPC_ORBAZ_BLOODBANE               = 29204, // this guy fleed
    NPC_THASSARIAN                    = 29200, // he also does SPELL_THE_LIGHT_OF_DAWN 53658
    SPELL_BLOOD_STRIKE1               = 52374,
    SPELL_DEATH_GRIP                  = 49576,
    SPELL_ICY_TOUCH2                  = 52372,
    SPELL_PLAGUE_STRIKE1              = 50668,
    // all do SPELL_HERO_AGGRO_AURA    53627

    // Lich King
    NPC_THE_LICH_KING                 = 29183, // show up at end
    SPELL_APOCALYPSE                  = 53210,
    SPELL_TELEPORT_VISUAL             = 52233,
    SPELL_SOUL_FEAST_ALEX             = 53677, // on Alexandros
    SPELL_SOUL_FEAST_TIRION           = 53685, // on Tirion
    SPELL_ICEBOUND_VISAGE             = 53274, // not sure what is it for
    SPELL_REBUKE                      = 53680,

    // others
    NPC_RAMPAGING_ABOMINATION         = 29186,
    SPELL_CLEAVE1                     = 53633,
    SPELL_SCOURGE_HOOK                = 50335,
    SPELL_SCOURGE_AGGRO_AURA          = 53624,

    NPC_FLESH_BEHEMOTH                = 29190, // giant guy
    SPELL_STOMP                       = 53634,
    SPELL_THUNDERCLAP                 = 36706,
    SPELL_HERO_AGGRO_AURA             = 53627,

    NPC_ACHERUS_GHOUL                 = 29219, // just ghoul....
    SPELL_GHOULPLOSION                = 53632,

    NPC_WARRIOR_OF_THE_FROZEN_WASTES  = 29206, // use SPELL_CLEAVE 53631

    NPC_HIGHLORD_ALEXANDROS_MOGRAINE  = 29227, // ghost
    NPC_DARION_MOGRAINE               = 29228, // ghost

    // ---- Dawn npc --------------------
    // Highlord Tirion Fordring
    NPC_HIGHLORD_TIRION_FORDRING      = 29175,
    EQUIP_HIGHLORD_TIRION_FORDRING    = 13262,
    SPELL_LAY_ON_HANDS                = 53778,
    SPELL_REBIRTH_OF_THE_ASHBRINGER   = 53702,
    SPELL_TIRION_CHARGE               = 53705,
    SPELL_TIRION_CHARGE_VISUAL        = 53706,

    // others
    NPC_KORFAX_CHAMPION_OF_THE_LIGHT  = 29176,
    SPELL_CLEAVE                      = 53631,
    SPELL_HEROIC_LEAP                 = 53625,

    NPC_LORD_MAXWELL_TYROSUS          = 29178,
    NPC_LEONID_BARTHALOMEW_THE_REVERED = 29179,
    NPC_DUKE_NICHOLAS_ZVERENHOFF      = 29180,

    NPC_COMMANDER_ELIGOR_DAWNBRINGER  = 29177,
    SPELL_HOLY_LIGHT2                 = 37979,

    NPC_RAYNE                         = 29181,
    SPELL_REJUVENATION                = 20664,
    SPELL_STARFALL                    = 20678,
    SPELL_TRANQUILITY                 = 25817,
    SPELL_WRATH                       = 21807,

    NPC_DEFENDER_OF_THE_LIGHT         = 29174, // also does SPELL_HEROIC_LEAP 53625
    SPELL_HOLY_LIGHT1                 = 29427,
    SPELL_HOLY_STRIKE                 = 53643,
    SPELL_HOLY_WRATH                  = 53638,
    SPELL_UPPERCUT                    = 53629,

    NPC_RIMBLAT_EARTHSHATTER          = 29182,
    SPELL_CHAIN_HEAL                  = 33642,
    SPELL_THUNDER                     = 53630
};

struct Location
{
    float x, y, z, o;
};

void UpdateWorldState(Map* map, uint32 id, uint32 state)
{
    Map::PlayerList const& players = map->GetPlayers();

    if (!players.isEmpty())
    {
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            if (Player* player = itr->GetSource())
                player->SendUpdateWorldState(id, state);
        }
    }
}

static Location LightofDawnLoc[]=
{
    {2281.335f, -5300.409f, 85.170f, 0},     // 0 Tirion Fordring loc
    {2283.896f, -5287.914f, 83.066f, 1.55f},  // 1 Tirion Fordring loc2
    {2281.461f, -5263.014f, 81.164f, 0},     // 2 Tirion charges
    {2262.277f, -5293.477f, 82.167f, 0},     // 3 Tirion run
    {2270.286f, -5287.73f, 82.262f, 0},      // 4 Tirion relocate
    {2269.511f, -5288.289f, 82.225f, 0},     // 5 Tirion forward
    {2262.277f, -5293.477f, 82.167f, 0},     // 6 Tirion runs to Darion
    {2270.286f, -5287.73f, 82.262f, 0},
    {2269.511f, -5288.289f, 82.225f, 0},
    {2273.205f, -5288.848f, 82.617f, 0},     // 9 Korfax loc1
    {2274.739f, -5287.926f, 82.684f, 0},     // 10 Korfax loc2
    {2253.673f, -5318.004f, 81.724f, 0},     // 11 Korfax kicked
    {2287.028f, -5309.644f, 87.253f, 0},     // 12 Maxwell loc1
    {2286.978f, -5308.025f, 86.83f, 0},      // 13 Maxwell loc2
    {2248.877f, -5307.586f, 82.166f, 0},     // 14 maxwell kicked
    {2278.58f, -5316.933f, 88.319f, 0},      // 15 Eligor loc1
    {2278.535f, -5315.479f, 88.08f, 0},      // 16 Eligor loc2
    {2259.416f, -5304.505f, 82.149f, 0},     // 17 eligor kicked
    {2289.259f, -5280.355f, 82.112f, 0},     // 18 Koltira loc1
    {2289.02f, -5281.985f, 82.207f, 0},      // 19 Koltira loc2
    {2273.289f, -5273.675f, 81.701f, 0},     // 20 Thassarian loc1
    {2273.332f, -5275.544f, 81.849f, 0},     // 21 Thassarian loc2
    {2281.198f, -5257.397f, 80.224f, 4.66f}, // 22 Alexandros loc1
    {2281.156f, -5259.934f, 80.647f, 0},     // 23 Alexandros loc2
    {2281.294f, -5281.895f, 82.445f, 1.35f}, // 24 Darion loc1
    {2281.093f, -5263.013f, 81.125f, 0},     // 25 Darion loc1
    {2281.313f, -5250.282f, 79.322f, 4.69f}, // 26 Lich King spawns
    {2281.523f, -5261.058f, 80.877f, 0},     // 27 Lich king move forwards
    {2272.709f, -5255.552f, 78.226f, 0},     // 28 Lich king kicked
    {2273.972f, -5257.676f, 78.862f, 0},     // 29 Lich king moves forward
};

class npc_highlord_darion_mograine : public CreatureScript
{
public:
    npc_highlord_darion_mograine() : CreatureScript("npc_highlord_darion_mograine") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                player->CLOSE_GOSSIP_MENU();
                CAST_AI(npc_highlord_darion_mograine::npc_highlord_darion_mograineAI, creature->AI())->uiStep = 1;
                CAST_AI(npc_highlord_darion_mograine::npc_highlord_darion_mograineAI, creature->AI())->Start(true, false, player->GetGUID());
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(12801) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(0, "I am ready.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_highlord_darion_mograineAI(creature);
    }

    struct npc_highlord_darion_mograineAI : public npc_escortAI
    {
        npc_highlord_darion_mograineAI(Creature* creature) : npc_escortAI(creature)
        {
            uiTirionGUID = 0;
            uiKorfaxGUID = 0;
            uiMaxwellGUID = 0;
            uiEligorGUID = 0;
            uiRayneGUID = 0;
            uiKoltiraGUID = 0;
            uiOrbazGUID = 0;
            uiThassarianGUID = 0;
            uiLichKingGUID = 0;

            memset(uiDefenderGUID, 0, sizeof(uiDefenderGUID));
            memset(uiEarthshatterGUID, 0, sizeof(uiEarthshatterGUID));
            memset(uiAbominationGUID, 0, sizeof(uiAbominationGUID));
            memset(uiBehemothGUID, 0, sizeof(uiBehemothGUID));
            memset(uiGhoulGUID, 0, sizeof(uiGhoulGUID));
            memset(uiWarriorGUID, 0, sizeof(uiWarriorGUID));

            Reset();
        }

        bool bIsBattle;
        uint32 uiStep;
        uint32 uiPhase_timer;
        uint32 uiFight_duration;
        uint32 uiTotal_dawn;
        uint32 uiTotal_scourge;
        uint32 uiSummon_counter;

        // Darion Mograine
        uint32 uiAnti_magic_zone;
        uint32 uiDeath_strike;
        uint32 uiDeath_embrace;
        uint32 uiIcy_touch;
        uint32 uiUnholy_blight;
        uint32 uiFight_speech;
        uint32 uiSpawncheck;
        uint32 uiTargetcheck;

        // Dawn
        uint64 uiTirionGUID;
        uint64 uiAlexandrosGUID;
        uint64 uiDarionGUID;
        uint64 uiKorfaxGUID;
        uint64 uiMaxwellGUID;
        uint64 uiEligorGUID;
        uint64 uiRayneGUID;
        uint64 uiDefenderGUID[ENCOUNTER_DEFENDER_NUMBER];
        uint64 uiEarthshatterGUID[ENCOUNTER_EARTHSHATTER_NUMBER];

        // Death
        uint64 uiKoltiraGUID;
        uint64 uiOrbazGUID;
        uint64 uiThassarianGUID;
        uint64 uiLichKingGUID;
        uint64 uiAbominationGUID[ENCOUNTER_ABOMINATION_NUMBER];
        uint64 uiBehemothGUID[ENCOUNTER_BEHEMOTH_NUMBER];
        uint64 uiGhoulGUID[ENCOUNTER_GHOUL_NUMBER];
        uint64 uiWarriorGUID[ENCOUNTER_WARRIOR_NUMBER];

        void Reset() OVERRIDE
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                bIsBattle = false;
                uiStep = 0;
                uiPhase_timer = 3000;
                uiFight_duration = 300000; // 5 minutes
                uiTotal_dawn = ENCOUNTER_TOTAL_DAWN;
                uiTotal_scourge = ENCOUNTER_TOTAL_SCOURGE;
                uiSummon_counter = 0;

                uiAnti_magic_zone = std::rand() % 6000 + 1000;
                uiDeath_strike = std::rand() % 10000 + 5000;
                uiDeath_embrace = std::rand() % 10000 + 5000;
                uiIcy_touch = std::rand() % 10000 + 5000;
                uiUnholy_blight = std::rand() % 10000 + 5000;

                uiFight_speech = 15000;
                uiSpawncheck = 1000;
                uiTargetcheck = 10000;

                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->Mount(25279);
                me->SetVisible(true);

                UpdateWorldState(me->GetMap(), WORLD_STATE_REMAINS, 0);
                //UpdateWorldState(me->GetMap(), WORLD_STATE_COUNTDOWN, 0);
                UpdateWorldState(me->GetMap(), WORLD_STATE_EVENT_BEGIN, 0);

                if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                    temp->setDeathState(DeathState::JUST_DIED);
                if (Creature* temp = Unit::GetCreature(*me, uiKorfaxGUID))
                    temp->setDeathState(DeathState::JUST_DIED);
                if (Creature* temp = Unit::GetCreature(*me, uiMaxwellGUID))
                    temp->setDeathState(DeathState::JUST_DIED);
                if (Creature* temp = Unit::GetCreature(*me, uiEligorGUID))
                    temp->setDeathState(DeathState::JUST_DIED);
                if (Creature* temp = Unit::GetCreature(*me, uiRayneGUID))
                    temp->setDeathState(DeathState::JUST_DIED);

                uiTirionGUID = 0;
                uiKorfaxGUID = 0;
                uiMaxwellGUID = 0;
                uiEligorGUID = 0;
                uiRayneGUID = 0;

                for (uint8 i = 0; i < ENCOUNTER_DEFENDER_NUMBER; ++i)
                {
                    if (Creature* temp = Unit::GetCreature(*me, uiDefenderGUID[i]))
                        temp->setDeathState(DeathState::JUST_DIED);
                    uiDefenderGUID[i] = 0;
                }
                for (uint8 i = 0; i < ENCOUNTER_EARTHSHATTER_NUMBER; ++i)
                {
                    if (Creature* temp = Unit::GetCreature(*me, uiEarthshatterGUID[i]))
                        temp->setDeathState(DeathState::JUST_DIED);
                    uiEarthshatterGUID[i] = 0;
                }

                if (Creature* temp = Unit::GetCreature(*me, uiKoltiraGUID))
                    temp->Respawn();
                if (Creature* temp = Unit::GetCreature(*me, uiOrbazGUID))
                    temp->Respawn();
                if (Creature* temp = Unit::GetCreature(*me, uiThassarianGUID))
                    temp->Respawn();
                if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                    temp->Respawn();

                uiKoltiraGUID = 0;
                uiOrbazGUID = 0;
                uiThassarianGUID = 0;
                uiLichKingGUID = 0;
                for (uint8 i = 0; i < ENCOUNTER_ABOMINATION_NUMBER; ++i)
                {
                    if (Creature* temp = Unit::GetCreature(*me, uiAbominationGUID[i]))
                        temp->setDeathState(DeathState::JUST_DIED);
                    uiAbominationGUID[i] = 0;
                }
                for (uint8 i = 0; i < ENCOUNTER_BEHEMOTH_NUMBER; ++i)
                {
                    if (Creature* temp = Unit::GetCreature(*me, uiBehemothGUID[i]))
                        temp->setDeathState(DeathState::JUST_DIED);
                    uiBehemothGUID[i] = 0;
                }
                for (uint8 i = 0; i < ENCOUNTER_GHOUL_NUMBER; ++i)
                {
                    if (Creature* temp = Unit::GetCreature(*me, uiGhoulGUID[i]))
                        temp->setDeathState(DeathState::JUST_DIED);
                    uiGhoulGUID[i] = 0;
                }
                for (uint8 i = 0; i < ENCOUNTER_WARRIOR_NUMBER; ++i)
                {
                    if (Creature* temp = Unit::GetCreature(*me, uiWarriorGUID[i]))
                        temp->setDeathState(DeathState::JUST_DIED);
                    uiWarriorGUID[i] = 0;
                }
            }
        }

        void AttackStart(Unit* who) OVERRIDE
        {
            if (!who)
                return;

            if (who == me)
                return;

            if (me->Attack(who, true))
            {
                me->AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void MoveInLineOfSight(Unit* who) OVERRIDE

        {
            if (!who)
                return;

            if (me->IsValidAttackTarget(who))
                if (me->IsWithinDistInMap(who, 20) && me->IsWithinLOSInMap(who))
                    AttackStart(who);
        }

        void SetHoldState(bool bOnHold)
        {
            SetEscortPaused(bOnHold);
        }

        void WaypointReached(uint32 waypointId) OVERRIDE
        {
            switch (waypointId)
            {
                case 0:
                    me->SetWalk(false);
                    SetHoldState(true);
                    break;
                case 1:
                    SetHoldState(true);
                    SpawnNPC();
                    if (Creature* temp = Unit::GetCreature(*me, uiKorfaxGUID))
                        temp->AI()->Talk(SAY_LIGHT_OF_DAWN07);
                    if (Creature* temp = Unit::GetCreature(*me, uiMaxwellGUID))
                        temp->AI()->Talk(SAY_LIGHT_OF_DAWN08);

                    for (uint8 i = 0; i < ENCOUNTER_GHOUL_NUMBER; ++i)
                        NPCChangeTarget(uiGhoulGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_WARRIOR_NUMBER; ++i)
                        NPCChangeTarget(uiWarriorGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_ABOMINATION_NUMBER; ++i)
                        NPCChangeTarget(uiAbominationGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_BEHEMOTH_NUMBER; ++i)
                        NPCChangeTarget(uiBehemothGUID[i]);
                    NPCChangeTarget(uiKoltiraGUID);
                    NPCChangeTarget(uiOrbazGUID);
                    NPCChangeTarget(uiThassarianGUID);

                    me->Dismount();
                    me->CastSpell(me, SPELL_THE_MIGHT_OF_MOGRAINE, true); // need to fix, on player only

                    if (Creature* temp = Unit::GetCreature(*me, uiKoltiraGUID))
                        temp->Dismount();
                    if (Creature* temp = Unit::GetCreature(*me, uiThassarianGUID))
                        temp->Dismount();

                    bIsBattle = true;
                    break;
                case 2:
                    me->SetWalk(true);
                    DoCast(me, SPELL_THE_LIGHT_OF_DAWN);
                    break;
                case 3:
                {
                    //Unit* pTirion = Unit::GetCreature(*me, uiTirionGUID);

                    Talk(EMOTE_LIGHT_OF_DAWN05);
                    if (me->HasAura(SPELL_THE_LIGHT_OF_DAWN, 0))
                        me->RemoveAurasDueToSpell(SPELL_THE_LIGHT_OF_DAWN);
                    if (Creature* temp = Unit::GetCreature(*me, uiKoltiraGUID))
                    {
                        if (temp->HasAura(SPELL_THE_LIGHT_OF_DAWN, 0))
                            temp->RemoveAurasDueToSpell(SPELL_THE_LIGHT_OF_DAWN);
                        temp->SetWalk(true);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[19].x, LightofDawnLoc[19].y, LightofDawnLoc[19].z);
                    }
                    if (Creature* temp = Unit::GetCreature(*me, uiThassarianGUID))
                    {
                        if (temp->HasAura(SPELL_THE_LIGHT_OF_DAWN, 0))
                            temp->RemoveAurasDueToSpell(SPELL_THE_LIGHT_OF_DAWN);
                        temp->SetWalk(true);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[21].x, LightofDawnLoc[21].y, LightofDawnLoc[21].z);
                    }
                    if (Creature* temp = Unit::GetCreature(*me, uiKorfaxGUID))
                    {
                        temp->SetWalk(true);
                        temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_READY2H);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[10].x, LightofDawnLoc[10].y, LightofDawnLoc[10].z);
                    }
                    if (Creature* temp = Unit::GetCreature(*me, uiMaxwellGUID))
                    {
                        temp->SetWalk(true);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[13].x, LightofDawnLoc[13].y, LightofDawnLoc[13].z);
                    }
                    if (Creature* temp = Unit::GetCreature(*me, uiEligorGUID))
                    {
                        temp->SetWalk(true);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[16].x, LightofDawnLoc[16].y, LightofDawnLoc[16].z);
                    }
                    JumpToNextStep(10000);
                }
                break;
                case 4:
                    Talk(SAY_LIGHT_OF_DAWN27);
                    me->SetStandState(UNIT_STAND_STATE_KNEEL);

                    if (Creature* temp = Unit::GetCreature(*me, uiKoltiraGUID))
                        temp->SetStandState(UNIT_STAND_STATE_KNEEL);
                    if (Creature* temp = Unit::GetCreature(*me, uiThassarianGUID))
                        temp->SetStandState(UNIT_STAND_STATE_KNEEL);
                    SetHoldState(true);
                    break;
                case 5:
                    Talk(SAY_LIGHT_OF_DAWN33);
                    SetHoldState(true);
                    break;
                case 6:
                    SetHoldState(true);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_SPECIALATTACK1H);
                    JumpToNextStep(1000);
                    break;
                case 7:
                    SetHoldState(true);
                    JumpToNextStep(2000);
                    break;
                case 8:
                    me->SetUInt32Value(UNIT_FIELD_VIRTUAL_ITEM_ID + 0, uint32(EQUIP_UNEQUIP));
                    if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                        me->CastSpell(temp, SPELL_ASHBRINGER, true);
                    Talk(EMOTE_LIGHT_OF_DAWN14);
                    SetHoldState(true);
                    break;
            }
        }

        void EnterEvadeMode() OVERRIDE
        {
            if (!bIsBattle)//do not reset self if we are in battle
                npc_escortAI::EnterEvadeMode();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            npc_escortAI::UpdateAI(diff);

            if (!bIsBattle)
            {
                if (uiPhase_timer <= diff)
                {
                    // ******* Before battle *****************************************************************
                    switch (uiStep)
                    {
                        case 0:  // countdown
                            //UpdateWorldState(me->GetMap(), WORLD_STATE_COUNTDOWN, 1);
                            break;

                        case 1:  // just delay
                            //UpdateWorldState(me->GetMap(), WORLD_STATE_REMAINS, 1);
                            UpdateWorldState(me->GetMap(), WORLD_STATE_COUNTDOWN, 0);
                            UpdateWorldState(me->GetMap(), WORLD_STATE_EVENT_BEGIN, 1);
                            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            JumpToNextStep(3000);
                            break;

                        case 2:
                            Talk(SAY_LIGHT_OF_DAWN04);
                            if (Creature* pKoltira = GetClosestCreatureWithEntry(me, NPC_KOLTIRA_DEATHWEAVER, 50.0f))
                                uiKoltiraGUID = pKoltira->GetGUID();
                            if (Creature* pOrbaz = GetClosestCreatureWithEntry(me, NPC_ORBAZ_BLOODBANE, 50.0f))
                                uiOrbazGUID = pOrbaz->GetGUID();
                            if (Creature* pThassarian = GetClosestCreatureWithEntry(me, NPC_THASSARIAN, 50.0f))
                                uiThassarianGUID = pThassarian->GetGUID();
                            JumpToNextStep(10000);
                            break;

                        case 3: // rise
                            Talk(SAY_LIGHT_OF_DAWN05);
                            JumpToNextStep(3000);
                            break;

                        case 4: // summon ghoul
                            // Dunno whats the summon spell, so workaround
                            DoCast(me, 33271); // shack effect
                            uiPhase_timer = 500;
                            if (uiSummon_counter < ENCOUNTER_GHOUL_NUMBER)
                            {
                                Unit* temp = me->SummonCreature(NPC_ACHERUS_GHOUL, (me->GetPositionX()-20)+rand()%40, (me->GetPositionY()-20)+rand()%40, me->GetPositionZ(), 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                                temp->SetWalk(false);
                                temp->setFaction(2084);
                                uiGhoulGUID[uiSummon_counter] = temp->GetGUID();
                                ++uiSummon_counter;
                            }
                            else
                            {
                                uiSummon_counter = 0;
                                ++uiStep;
                            }
                            break;

                        case 5: // summon abomination
                            DoCast(me, 33271); // shack effect
                            uiPhase_timer = 500;
                            if (uiSummon_counter < ENCOUNTER_ABOMINATION_NUMBER)
                            {
                                Unit* temp = me->SummonCreature(NPC_RAMPAGING_ABOMINATION, (me->GetPositionX()-20)+rand()%40, (me->GetPositionY()-20)+rand()%40, me->GetPositionZ(), 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                                temp->SetWalk(false);
                                temp->setFaction(2084);
                                uiAbominationGUID[uiSummon_counter] = temp->GetGUID();
                                ++uiSummon_counter;
                            }
                            else
                            {
                                uiSummon_counter = 0;
                                ++uiStep;
                            }
                            break;

                        case 6: // summon warrior
                            DoCast(me, 33271); // shack effect
                            uiPhase_timer = 500;
                            if (uiSummon_counter < ENCOUNTER_WARRIOR_NUMBER)
                            {
                                Unit* temp = me->SummonCreature(NPC_WARRIOR_OF_THE_FROZEN_WASTES, (me->GetPositionX()-20)+rand()%40, (me->GetPositionY()-20)+rand()%40, me->GetPositionZ(), 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                                temp->SetWalk(false);
                                temp->setFaction(2084);
                                uiWarriorGUID[uiSummon_counter] = temp->GetGUID();
                                ++uiSummon_counter;
                            }
                            else
                            {
                                uiSummon_counter = 0;
                                ++uiStep;
                            }
                            break;

                        case 7: // summon warrior
                            DoCast(me, 33271); // shack effect
                            uiPhase_timer = 500;
                            if (uiSummon_counter < ENCOUNTER_BEHEMOTH_NUMBER)
                            {
                                Unit* temp = me->SummonCreature(NPC_FLESH_BEHEMOTH, (me->GetPositionX()-20)+rand()%40, (me->GetPositionY()-20)+rand()%40, me->GetPositionZ(), 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                                temp->SetWalk(false);
                                temp->setFaction(2084);
                                uiBehemothGUID[uiSummon_counter] = temp->GetGUID();
                                ++uiSummon_counter;
                            }
                            else
                            {
                                uiSummon_counter = 0;
                                ++uiStep;
                            }
                            break;

                        case 8: // summon announce
                            Talk(SAY_LIGHT_OF_DAWN06);
                            JumpToNextStep(5000);
                            break;

                        case 9: // charge begins
                            SetHoldState(false);
                            if (Creature* temp = Unit::GetCreature(*me, uiKoltiraGUID))
                            {
                                temp->SetWalk(false);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z);
                            }
                            if (Creature* temp = Unit::GetCreature(*me, uiOrbazGUID))
                            {
                                temp->SetWalk(false);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z);
                            }
                            if (Creature* temp = Unit::GetCreature(*me, uiThassarianGUID))
                            {
                                temp->SetWalk(false);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z);
                            }
                            for (uint8 i = 0; i < ENCOUNTER_ABOMINATION_NUMBER; ++i)
                                if (Creature* temp = Unit::GetCreature(*me, uiAbominationGUID[i]))
                                    temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z);
                            for (uint8 i = 0; i < ENCOUNTER_BEHEMOTH_NUMBER; ++i)
                                if (Creature* temp = Unit::GetCreature(*me, uiBehemothGUID[i]))
                                    temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z);
                            for (uint8 i = 0; i < ENCOUNTER_GHOUL_NUMBER; ++i)
                                if (Creature* temp = Unit::GetCreature(*me, uiGhoulGUID[i]))
                                    temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z);
                            for (uint8 i = 0; i < ENCOUNTER_WARRIOR_NUMBER; ++i)
                                if (Creature* temp = Unit::GetCreature(*me, uiWarriorGUID[i]))
                                    temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z);
                            JumpToNextStep(5000);
                            break;

                        // ******* After battle *****************************************************************
                        case 11: // Tirion starts to speak
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN28);
                            JumpToNextStep(21000);
                            break;

                        case 12:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN29);
                            JumpToNextStep(13000);
                            break;

                        case 13:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN30);
                            JumpToNextStep(13000);
                            break;

                        case 14:
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            Talk(SAY_LIGHT_OF_DAWN31);
                            JumpToNextStep(7000);
                            break;

                        case 15: // summon gate
                            if (Creature* temp = me->SummonCreature(NPC_HIGHLORD_ALEXANDROS_MOGRAINE, LightofDawnLoc[22].x, LightofDawnLoc[22].y, LightofDawnLoc[22].z, LightofDawnLoc[22].o, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000))
                            {
                                temp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                temp->CastSpell(temp, SPELL_ALEXANDROS_MOGRAINE_SPAWN, true);
                                temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN06);
                                uiAlexandrosGUID = temp->GetGUID();
                            }
                            JumpToNextStep(4000);
                            break;

                        case 16: // Alexandros out
                            if (Creature* temp = Unit::GetCreature(*me, uiAlexandrosGUID))
                            {
                                temp->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[23].x, LightofDawnLoc[23].y, LightofDawnLoc[23].z);
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN32);
                            }
                            SetHoldState(false); // makes darion turns back
                            JumpToNextStep(5000);
                            break;

                        case 17:
                            me->SetStandState(UNIT_STAND_STATE_KNEEL);
                            Talk(SAY_LIGHT_OF_DAWN34);
                            JumpToNextStep(5000);
                            break;

                        case 18: // Darion's spirit out
                            if (Creature* temp = me->SummonCreature(NPC_DARION_MOGRAINE, LightofDawnLoc[24].x, LightofDawnLoc[24].y, LightofDawnLoc[24].z, LightofDawnLoc[24].o, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000))
                            {
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN35);
                                temp->SetWalk(false);
                                uiDarionGUID = temp->GetGUID();
                            }
                            JumpToNextStep(4000);
                            break;

                        case 19: // runs to father
                            if (Creature* temp = Unit::GetCreature(*me, uiDarionGUID))
                            {
                                temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN07);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[25].x, LightofDawnLoc[25].y, LightofDawnLoc[25].z);
                            }
                            JumpToNextStep(4000);
                            break;

                        case 20:
                            if (Creature* temp = Unit::GetCreature(*me, uiDarionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN36);
                            JumpToNextStep(4000);
                            break;

                        case 21:
                            if (Creature* temp = Unit::GetCreature(*me, uiDarionGUID))
                                temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN08);
                            JumpToNextStep(4000);
                            break;

                        case 22:
                            if (Creature* temp = Unit::GetCreature(*me, uiAlexandrosGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN37);
                            JumpToNextStep(8000);
                            break;

                        case 23:
                            if (Creature* temp = Unit::GetCreature(*me, uiDarionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN38);
                            JumpToNextStep(8000);
                            break;

                        case 24:
                            if (Creature* temp = Unit::GetCreature(*me, uiAlexandrosGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN39);

                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID)) // Tirion moves forward here
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[1].x, LightofDawnLoc[1].y, LightofDawnLoc[1].z);

                            JumpToNextStep(15000);
                            break;

                        case 25:
                            if (Creature* temp = Unit::GetCreature(*me, uiDarionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN40);
                            JumpToNextStep(11000);
                            break;

                        case 26:
                            if (Creature* temp = Unit::GetCreature(*me, uiAlexandrosGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN41);
                            JumpToNextStep(5000);
                            break;

                        case 27:
                            if (Creature* temp = Unit::GetCreature(*me, uiDarionGUID))
                                temp->setDeathState(DeathState::JUST_DIED);
                            JumpToNextStep(24000);
                            break;

                        case 28:
                            if (Creature* temp = Unit::GetCreature(*me, uiAlexandrosGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN42);
                            JumpToNextStep(6000);
                            break;

                        case 29: // lich king spawns
                            if (Creature* temp = me->SummonCreature(NPC_THE_LICH_KING, LightofDawnLoc[26].x, LightofDawnLoc[26].y, LightofDawnLoc[26].z, LightofDawnLoc[26].o, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000))
                            {
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN43);
                                uiLichKingGUID = temp->GetGUID();
                                if (Unit* pAlex = Unit::GetCreature(*me, uiAlexandrosGUID))
                                    temp->CastSpell(pAlex, SPELL_SOUL_FEAST_ALEX, false);
                            }
                            JumpToNextStep(2000);
                            break;

                        case 30:
                            if (Creature* temp = Unit::GetCreature(*me, uiAlexandrosGUID)) // just hide him
                            {
                                temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN09);
                                temp->SetVisible(false);
                            }
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                            {
                                temp->InterruptNonMeleeSpells(false);
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN45);
                            }
                            JumpToNextStep(3000);
                            break;

                        case 31:
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            Talk(EMOTE_LIGHT_OF_DAWN10);
                            Talk(SAY_LIGHT_OF_DAWN44);
                            JumpToNextStep(3000);
                            break;

                        case 32:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[27].x, LightofDawnLoc[27].y, LightofDawnLoc[27].z);
                            JumpToNextStep(6000);
                            break;

                        case 33: // Darion supports to jump to lich king here
                            if (Unit::GetCreature(*me, uiLichKingGUID))
                                DoCast(me, SPELL_MOGRAINE_CHARGE); // jumping charge
                            // doesn't make it looks well, so workarounds, Darion charges, looks better
                            me->SetSpeed(MOVE_RUN, 3.0f);
                            me->SetWalk(false);
                            SetHoldState(false);
                            JumpToNextStep(0);
                            break;

                        case 35: // Lich king counterattacks
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                            {
                                temp->HandleEmoteCommand(EMOTE_ONESHOT_KICK);
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN46);
                            }
                            me->SetSpeed(MOVE_RUN, 6.0f);
                            me->SetStandState(UNIT_STAND_STATE_DEAD);
                            SetHoldState(false); // Darion got kicked by lich king
                            JumpToNextStep(0);
                            break;

                        case 37: // Lich king counterattacks
                            me->SetStandState(UNIT_STAND_STATE_KNEEL);
                            JumpToNextStep(3000);
                            break;

                        case 38:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN47);
                            JumpToNextStep(8000);
                            break;

                        case 39:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN48);
                            JumpToNextStep(15000);
                            break;

                        case 40:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN49);
                            JumpToNextStep(17000);
                            break;

                        case 41: // Lich king - Apocalypse
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                            {
                                temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN11);
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN51);
                                if (Creature* pTirion = Unit::GetCreature(*me, uiTirionGUID))
                                {
                                    pTirion->SetStandState(UNIT_STAND_STATE_KNEEL);
                                    //temp->CastSpell(pTirion, SPELL_APOCALYPSE, false); // not working
                                    temp->CastSpell(pTirion, SPELL_SOUL_FEAST_TIRION, false);
                                    pTirion->AI()->Talk(EMOTE_LIGHT_OF_DAWN12);
                                }
                            }
                            JumpToNextStep(2000);
                            break;

                        case 42: // Maxwell yells for attack
                            {
                                float fLichPositionX = 0,
                                      fLichPositionY = 0,
                                      fLichPositionZ = 0;
                                if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                {
                                    fLichPositionX = temp->GetPositionX();
                                    fLichPositionY = temp->GetPositionY();
                                    fLichPositionZ = temp->GetPositionZ();
                                }

                                if (fLichPositionX && fLichPositionY)
                                {
                                    Unit* temp = me->SummonCreature(NPC_DEFENDER_OF_THE_LIGHT, LightofDawnLoc[0].x+rand()%10, LightofDawnLoc[0].y+rand()%10, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 10000);
                                    temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_ATTACK_UNARMED);
                                    temp->SetWalk(false);
                                    temp->SetSpeed(MOVE_RUN, 2.0f);
                                    temp->setFaction(me->getFaction());
                                    temp->GetMotionMaster()->MovePoint(0, fLichPositionX, fLichPositionY, fLichPositionZ);
                                    uiDefenderGUID[0] = temp->GetGUID();

                                    temp = me->SummonCreature(NPC_RIMBLAT_EARTHSHATTER, LightofDawnLoc[0].x+rand()%10, LightofDawnLoc[0].y+rand()%10, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 10000);
                                    temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_ATTACK_UNARMED);
                                    temp->SetWalk(false);
                                    temp->SetSpeed(MOVE_RUN, 2.0f);
                                    temp->setFaction(me->getFaction());
                                    temp->GetMotionMaster()->MovePoint(0, fLichPositionX, fLichPositionY, fLichPositionZ);
                                    uiEarthshatterGUID[0] = temp->GetGUID();
                                }
                                if (Creature* temp = Unit::GetCreature(*me, uiMaxwellGUID))
                                {
                                    temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_ATTACK_UNARMED);
                                    temp->SetWalk(false);
                                    temp->SetSpeed(MOVE_RUN, 2.0f);
                                    temp->GetMotionMaster()->MovePoint(0, fLichPositionX, fLichPositionY, fLichPositionZ);
                                    temp->AI()->Talk(SAY_LIGHT_OF_DAWN50);
                                }
                                if (Creature* temp = Unit::GetCreature(*me, uiKorfaxGUID))
                                {
                                    temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_ATTACK_UNARMED);
                                    temp->SetWalk(false);
                                    temp->SetSpeed(MOVE_RUN, 2.0f);
                                    temp->HandleEmoteCommand(EMOTE_STATE_ATTACK_UNARMED);
                                    temp->GetMotionMaster()->MovePoint(0, fLichPositionX, fLichPositionY, fLichPositionZ);
                                }
                                if (Creature* temp = Unit::GetCreature(*me, uiEligorGUID))
                                {
                                    temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_ATTACK_UNARMED);
                                    temp->SetWalk(false);
                                    temp->SetSpeed(MOVE_RUN, 2.0f);
                                    temp->GetMotionMaster()->MovePoint(0, fLichPositionX, fLichPositionY, fLichPositionZ);
                                }
                            }
                            JumpToNextStep(4500);
                            break;

                        case 43: // They all got kicked
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN13);

                            if (Creature* temp = Unit::GetCreature(*me, uiMaxwellGUID))
                            {
                                temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                                temp->SetSpeed(MOVE_RUN, 6.0f);
                                temp->SetStandState(UNIT_STAND_STATE_DEAD);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[14].x, LightofDawnLoc[14].y, LightofDawnLoc[14].z);
                            }
                            if (Creature* temp = Unit::GetCreature(*me, uiKorfaxGUID))
                            {
                                temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                                temp->SetSpeed(MOVE_RUN, 6.0f);
                                temp->SetStandState(UNIT_STAND_STATE_DEAD);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[11].x, LightofDawnLoc[11].y, LightofDawnLoc[11].z);
                            }
                            if (Creature* temp = Unit::GetCreature(*me, uiEligorGUID))
                            {
                                temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                                temp->SetSpeed(MOVE_RUN, 6.0f);
                                temp->SetStandState(UNIT_STAND_STATE_DEAD);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[17].x, LightofDawnLoc[17].y, LightofDawnLoc[17].z);
                            }
                            if (Creature* temp = Unit::GetCreature(*me, uiDefenderGUID[0]))
                            {
                                temp->SetSpeed(MOVE_RUN, 6.0f);
                                temp->SetStandState(UNIT_STAND_STATE_DEAD);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%10, LightofDawnLoc[0].y+rand()%10, LightofDawnLoc[0].z);
                            }
                            if (Creature* temp = Unit::GetCreature(*me, uiEarthshatterGUID[0]))
                            {
                                temp->SetSpeed(MOVE_RUN, 6.0f);
                                temp->SetStandState(UNIT_STAND_STATE_DEAD);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[0].x+rand()%10, LightofDawnLoc[0].y+rand()%10, LightofDawnLoc[0].z);
                            }
                            JumpToNextStep(3000);
                            break;

                        case 44: // make them stand up
                            if (Creature* temp = Unit::GetCreature(*me, uiMaxwellGUID))
                                temp->SetStandState(UNIT_STAND_STATE_STAND);
                            if (Creature* temp = Unit::GetCreature(*me, uiKorfaxGUID))
                                temp->SetStandState(UNIT_STAND_STATE_STAND);
                            if (Creature* temp = Unit::GetCreature(*me, uiEligorGUID))
                                temp->SetStandState(UNIT_STAND_STATE_STAND);
                            JumpToNextStep(1000);
                            break;

                        case 45:
                            Talk(SAY_LIGHT_OF_DAWN52);
                            JumpToNextStep(5000);
                            break;

                        case 46: // Darion stand up, "not today"
                            me->SetSpeed(MOVE_RUN, 1.0f);
                            me->SetWalk(true);
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            Talk(SAY_LIGHT_OF_DAWN53);
                            SetHoldState(false); // Darion throws sword
                            JumpToNextStep(7000);
                            break;

                        case 47: // Ashbringer rebirth
                            me->SetStandState(UNIT_STAND_STATE_KNEEL);
                            Talk(EMOTE_LIGHT_OF_DAWN15);
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                            {
                                temp->SetStandState(UNIT_STAND_STATE_STAND);
                                temp->SetUInt32Value(UNIT_FIELD_VIRTUAL_ITEM_ID + 0, uint32(EQUIP_HIGHLORD_TIRION_FORDRING));
                                temp->CastSpell(temp, SPELL_REBIRTH_OF_THE_ASHBRINGER, false);
                            }
                            JumpToNextStep(1000);
                            break;

                        case 48: // Show the cleansing effect (dawn of light)
                            //if (GameObject* go = me->GetMap()->GetGameObject(uiDawnofLightGUID))
                            //    go->SetPhaseMask(128, true);
                            me->SummonGameObject(GO_LIGHT_OF_DAWN, 2283.896f, -5287.914f, 83.066f, 0, 0, 0, 0, 0, 30000);
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                            {
                                if (temp->HasAura(SPELL_REBIRTH_OF_THE_ASHBRINGER, 0))
                                    temp->RemoveAurasDueToSpell(SPELL_REBIRTH_OF_THE_ASHBRINGER);
                                temp->CastSpell(temp, 41542, false); // workarounds, light expoded, makes it cool
                                temp->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
                            }
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->InterruptNonMeleeSpells(false);
                            JumpToNextStep(2500);
                            break;

                        case 49:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN54);
                            JumpToNextStep(4000);
                            break;

                        case 50:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN55);
                            JumpToNextStep(5000);
                            break;

                        case 51:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN56);
                            JumpToNextStep(1000);
                            break;

                        case 52: // Tiron charges
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                            {
                                temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN16);
                                temp->CastSpell(temp, SPELL_TIRION_CHARGE, false); // jumping charge
                                temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_STATE_READY2H);
                                temp->SetSpeed(MOVE_RUN, 3.0f); // workarounds, make Tirion still running
                                temp->SetWalk(false);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[2].x, LightofDawnLoc[2].y, LightofDawnLoc[2].z);
                                if (Creature* lktemp = Unit::GetCreature(*me, uiLichKingGUID))
                                    lktemp->Relocate(LightofDawnLoc[28].x, LightofDawnLoc[28].y, LightofDawnLoc[28].z); // workarounds, he should kick back by Tirion, but here we relocate him
                            }
                            JumpToNextStep(1500);
                            break;

                        case 53:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN57);
                            JumpToNextStep(1000);
                            break;

                        case 54:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                            {
                                temp->SetSpeed(MOVE_RUN, 1.0f);
                                me->SetWalk(true);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[29].x, LightofDawnLoc[29].y, LightofDawnLoc[29].z); // 26
                            }
                            JumpToNextStep(4000);
                            break;

                        case 55:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->SetStandState(UNIT_STAND_STATE_KNEEL);
                            JumpToNextStep(2000);
                            break;

                        case 56:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->SetStandState(UNIT_STAND_STATE_STAND);
                            JumpToNextStep(1500);
                            break;

                        case 57:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN58);
                            JumpToNextStep(10000);
                            break;

                        case 58:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN59);
                            JumpToNextStep(10000);
                            break;

                        case 59:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->CastSpell(temp, SPELL_TELEPORT_VISUAL, false);
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID)) // Tirion runs to Darion
                            {
                                temp->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                                temp->SetSpeed(MOVE_RUN, 1.0f);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[6].x, LightofDawnLoc[6].y, LightofDawnLoc[6].z);
                            }
                            JumpToNextStep(2500);
                            break;

                        case 60:
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID)) // Lich king disappears here
                            {
                                temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN17);
                                temp->Kill(temp);
                            }
                            JumpToNextStep(10000);
                            break;

                        case 61:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN60);
                            JumpToNextStep(3000);
                            break;

                        case 62:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                            {
                                temp->SetWalk(true);
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[7].x, LightofDawnLoc[7].y, LightofDawnLoc[7].z);
                            }
                            JumpToNextStep(5500);
                            break;

                        case 63:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                            {
                                temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[8].x, LightofDawnLoc[8].y, LightofDawnLoc[8].z);
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN61);
                            }
                            JumpToNextStep(15000);
                            break;

                        case 64:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN62);
                            JumpToNextStep(7000);
                            break;

                        case 65:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN63);
                            JumpToNextStep(10000);
                            break;

                        case 66:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN64);
                            JumpToNextStep(11000);
                            break;

                        case 67:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN65);
                            JumpToNextStep(10000);
                            break;

                        case 68:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN66);
                            JumpToNextStep(8000);
                            break;

                        case 69:
                            if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                                temp->AI()->Talk(SAY_LIGHT_OF_DAWN67);
                            JumpToNextStep(10000);
                            break;

                        case 70:
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            Talk(SAY_LIGHT_OF_DAWN68);
                            JumpToNextStep(10000);
                            break;

                        case 71:
                            //if (GameObject* go = me->GetMap()->GetGameObject(uiDawnofLightGUID)) // Turn off dawn of light
                            //    go->SetPhaseMask(0, true);
                            {
                                Map* map = me->GetMap(); // search players with in 50 yards for quest credit
                                Map::PlayerList const &PlayerList = map->GetPlayers();
                                if (!PlayerList.isEmpty())
                                {
                                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                        if (i->GetSource()->IsAlive() && me->IsWithinDistInMap(i->GetSource(), 50))
                                            i->GetSource()->CastSpell(i->GetSource(), SPELL_THE_LIGHT_OF_DAWN_Q, false);
                                }
                            }
                            me->SetVisible(false); // respawns another Darion for quest turn in
                            me->SummonCreature(NPC_HIGHLORD_DARION_MOGRAINE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 180000);
                            JumpToNextStep(1000);
                            break;

                        case 72:
                            SetHoldState(false); // Escort ends
                            JumpToNextStep(25000);
                            break;

                        case 73:
                            if (Creature* temp = Unit::GetCreature(*me, uiKoltiraGUID))
                                temp->DespawnOrUnsummon();
                            if (Creature* temp = Unit::GetCreature(*me, uiOrbazGUID))
                                temp->DespawnOrUnsummon();
                            if (Creature* temp = Unit::GetCreature(*me, uiThassarianGUID))
                                temp->DespawnOrUnsummon();
                            if (Creature* temp = Unit::GetCreature(*me, uiLichKingGUID))
                                temp->DespawnOrUnsummon();
                            me->DespawnOrUnsummon();
                            break;
                    }

                } else uiPhase_timer -= diff;
            }

            // ******* During battle *****************************************************************
            else
            {
                if (uiAnti_magic_zone <= diff)
                {
                    DoCast(me, SPELL_ANTI_MAGIC_ZONE1);
                    uiAnti_magic_zone = std::rand() % 30000 + 25000;
                } else uiAnti_magic_zone -= diff;

                if (uiDeath_strike <= diff)
                {
                    DoCastVictim(SPELL_DEATH_STRIKE);
                    uiDeath_strike = std::rand() % 10000 + 5000;
                } else uiDeath_strike -= diff;

                if (uiDeath_embrace <= diff)
                {
                    DoCastVictim(SPELL_DEATH_EMBRACE);
                    uiDeath_embrace = std::rand() % 10000 + 5000;
                } else uiDeath_embrace -= diff;

                if (uiIcy_touch <= diff)
                {
                    DoCastVictim(SPELL_ICY_TOUCH1);
                    uiIcy_touch = std::rand() % 10000 + 5000;
                } else uiIcy_touch -= diff;

                if (uiUnholy_blight <= diff)
                {
                    DoCastVictim(SPELL_UNHOLY_BLIGHT);
                    uiUnholy_blight = std::rand() % 10000 + 5000;
                } else uiUnholy_blight -= diff;

                if (uiFight_speech <= diff)
                {
                    Talk(SAY_LIGHT_OF_DAWN09);
                    uiFight_speech = std::rand() % 20000 + 15000;
                } else uiFight_speech -= diff;

                // Check spawns
                if (uiSpawncheck <= diff)
                {
                    SpawnNPC();
                    uiSpawncheck = 1000;
                } else uiSpawncheck -= diff;

                // Check targets
                if (uiTargetcheck <= diff)
                {
                    for (uint8 i = 0; i < ENCOUNTER_GHOUL_NUMBER; ++i)
                        NPCChangeTarget(uiGhoulGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_WARRIOR_NUMBER; ++i)
                        NPCChangeTarget(uiWarriorGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_ABOMINATION_NUMBER; ++i)
                        NPCChangeTarget(uiAbominationGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_BEHEMOTH_NUMBER; ++i)
                        NPCChangeTarget(uiBehemothGUID[i]);
                    NPCChangeTarget(uiKoltiraGUID);
                    NPCChangeTarget(uiOrbazGUID);
                    NPCChangeTarget(uiThassarianGUID);

                    uiTargetcheck = 10000;
                } else uiTargetcheck -= diff;

                // Battle end
                if (uiFight_duration <= diff + 5000)
                {
                    if (!uiTirionGUID)
                        if (Creature* temp = me->SummonCreature(NPC_HIGHLORD_TIRION_FORDRING, LightofDawnLoc[0].x, LightofDawnLoc[0].y, LightofDawnLoc[0].z, 1.528f, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 600000))
                        {
                            temp->setFaction(me->getFaction());
                            temp->SetUInt32Value(UNIT_FIELD_VIRTUAL_ITEM_ID + 0, uint32(EQUIP_UNEQUIP));
                            temp->AI()->Talk(SAY_LIGHT_OF_DAWN25);
                            uiTirionGUID = temp->GetGUID();
                        }
                }
                if (uiFight_duration <= diff)
                {
                    bIsBattle = false;
                    uiFight_duration = 300000;

                    if (me->HasAura(SPELL_THE_MIGHT_OF_MOGRAINE, 0))
                        me->RemoveAurasDueToSpell(SPELL_THE_MIGHT_OF_MOGRAINE);
                    me->RemoveAllAuras();
                    me->DeleteThreatList();
                    me->CombatStop(true);
                    me->InterruptNonMeleeSpells(false);
                    me->SetWalk(false);

                    for (uint8 i = 0; i < ENCOUNTER_DEFENDER_NUMBER; ++i)
                        DespawnNPC(uiDefenderGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_EARTHSHATTER_NUMBER; ++i)
                        DespawnNPC(uiEarthshatterGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_ABOMINATION_NUMBER; ++i)
                        DespawnNPC(uiAbominationGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_BEHEMOTH_NUMBER; ++i)
                        DespawnNPC(uiBehemothGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_GHOUL_NUMBER; ++i)
                        DespawnNPC(uiGhoulGUID[i]);
                    for (uint8 i = 0; i < ENCOUNTER_WARRIOR_NUMBER; ++i)
                        DespawnNPC(uiWarriorGUID[i]);

                    if (Creature* temp = Unit::GetCreature(*me, uiKorfaxGUID))
                    {
                        temp->RemoveAllAuras();
                        temp->DeleteThreatList();
                        temp->CombatStop(true);
                        temp->AttackStop();
                        temp->setFaction(me->getFaction());
                        temp->SetWalk(false);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[9].x, LightofDawnLoc[9].y, LightofDawnLoc[9].z);
                    }

                    if (Creature* temp = Unit::GetCreature(*me, uiMaxwellGUID))
                    {
                        temp->RemoveAllAuras();
                        temp->DeleteThreatList();
                        temp->CombatStop(true);
                        temp->AttackStop();
                        temp->setFaction(me->getFaction());
                        temp->SetWalk(false);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[12].x, LightofDawnLoc[12].y, LightofDawnLoc[12].z);
                    }

                    if (Creature* temp = Unit::GetCreature(*me, uiEligorGUID))
                    {
                        temp->RemoveAllAuras();
                        temp->DeleteThreatList();
                        temp->CombatStop(true);
                        temp->AttackStop();
                        temp->setFaction(me->getFaction());
                        temp->SetWalk(false);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[15].x, LightofDawnLoc[15].y, LightofDawnLoc[15].z);
                    }
                    DespawnNPC(uiRayneGUID);

                    if (Creature* temp = Unit::GetCreature(*me, uiKoltiraGUID))
                    {
                        temp->RemoveAllAuras();
                        temp->DeleteThreatList();
                        temp->CombatStop(true);
                        temp->AttackStop();
                        temp->setFaction(me->getFaction());
                        temp->SetWalk(false);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[18].x, LightofDawnLoc[18].y, LightofDawnLoc[18].z);
                        temp->CastSpell(temp, SPELL_THE_LIGHT_OF_DAWN, false);
                    }

                    if (Creature* temp = Unit::GetCreature(*me, uiOrbazGUID))
                        temp->AI()->Talk(EMOTE_LIGHT_OF_DAWN04);

                    if (Creature* temp = Unit::GetCreature(*me, uiThassarianGUID))
                    {
                        temp->RemoveAllAuras();
                        temp->DeleteThreatList();
                        temp->CombatStop(true);
                        temp->AttackStop();
                        temp->setFaction(me->getFaction());
                        temp->SetWalk(false);
                        temp->GetMotionMaster()->MovePoint(0, LightofDawnLoc[20].x, LightofDawnLoc[20].y, LightofDawnLoc[20].z);
                        temp->CastSpell(temp, SPELL_THE_LIGHT_OF_DAWN, false);
                    }

                    if (Creature* temp = Unit::GetCreature(*me, uiTirionGUID))
                        temp->AI()->Talk(SAY_LIGHT_OF_DAWN26);

                    SetHoldState(false);

                } else uiFight_duration -= diff;

                DoMeleeAttackIfReady();
            }
        }

        void JumpToNextStep(uint32 uiTimer)
        {
            uiPhase_timer = uiTimer;
            ++uiStep;
        }

        void NPCChangeTarget(uint64 ui_GUID)
        {
            if (Creature* temp = Unit::GetCreature(*me, ui_GUID))
                if (temp->IsAlive())
                    if (Unit* pTarger = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        if (pTarger->IsAlive())
                        {
                            // temp->DeleteThreatList();
                            temp->AddThreat(pTarger, 0.0f);
                            temp->AI()->AttackStart(pTarger);
                            temp->SetInCombatWith(pTarger);
                            pTarger->SetInCombatWith(temp);
                            // temp->GetMotionMaster()->MoveChase(pTarger, 20.0f);
                        }
        }

        void SpawnNPC()
        {
            Unit* temp = NULL;

            // Death
            for (uint8 i = 0; i < ENCOUNTER_GHOUL_NUMBER; ++i)
            {
                temp = Unit::GetCreature(*me, uiGhoulGUID[i]);
                if (!temp)
                {
                    temp = me->SummonCreature(NPC_ACHERUS_GHOUL, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                    temp->setFaction(2084);
                    uiGhoulGUID[i] = temp->GetGUID();
                }
            }
            for (uint8 i = 0; i < ENCOUNTER_ABOMINATION_NUMBER; ++i)
            {
                temp = Unit::GetCreature(*me, uiAbominationGUID[i]);
                if (!temp)
                {
                    temp = me->SummonCreature(NPC_WARRIOR_OF_THE_FROZEN_WASTES, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                    temp->setFaction(2084);
                    uiAbominationGUID[i] = temp->GetGUID();
                }
            }
            for (uint8 i = 0; i < ENCOUNTER_WARRIOR_NUMBER; ++i)
            {
                temp = Unit::GetCreature(*me, uiWarriorGUID[i]);
                if (!temp)
                {
                    temp = me->SummonCreature(NPC_RAMPAGING_ABOMINATION, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                    temp->setFaction(2084);
                    uiWarriorGUID[i] = temp->GetGUID();
                }
            }
            for (uint8 i = 0; i < ENCOUNTER_BEHEMOTH_NUMBER; ++i)
            {
                temp = Unit::GetCreature(*me, uiBehemothGUID[i]);
                if (!temp)
                {
                    temp = me->SummonCreature(NPC_FLESH_BEHEMOTH, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                    temp->setFaction(2084);
                    uiBehemothGUID[i] = temp->GetGUID();
                }
            }

            // Dawn
            for (uint8 i = 0; i < ENCOUNTER_DEFENDER_NUMBER; ++i)
            {
                temp = Unit::GetCreature(*me, uiDefenderGUID[i]);
                if (!temp)
                {
                    temp = me->SummonCreature(NPC_DEFENDER_OF_THE_LIGHT, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                    temp->setFaction(2089);
                    me->AddThreat(temp, 0.0f);
                    uiDefenderGUID[i] = temp->GetGUID();
                }
            }
            for (uint8 i = 0; i < ENCOUNTER_EARTHSHATTER_NUMBER; ++i)
            {
                temp = Unit::GetCreature(*me, uiEarthshatterGUID[i]);
                if (!temp)
                {
                    temp = me->SummonCreature(NPC_RIMBLAT_EARTHSHATTER, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                    temp->setFaction(2089);
                    me->AddThreat(temp, 0.0f);
                    uiEarthshatterGUID[i] = temp->GetGUID();
                }
            }
            temp = Unit::GetCreature(*me, uiKorfaxGUID);
            if (!temp)
            {
                temp = me->SummonCreature(NPC_KORFAX_CHAMPION_OF_THE_LIGHT, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 600000);
                temp->setFaction(2089);
                me->AddThreat(temp, 0.0f);
                uiKorfaxGUID = temp->GetGUID();
            }
            temp = Unit::GetCreature(*me, uiMaxwellGUID);
            if (!temp)
            {
                temp = me->SummonCreature(NPC_LORD_MAXWELL_TYROSUS, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 600000);
                temp->setFaction(2089);
                me->AddThreat(temp, 0.0f);
                uiMaxwellGUID = temp->GetGUID();
            }
            temp = Unit::GetCreature(*me, uiEligorGUID);
            if (!temp)
            {
                temp = me->SummonCreature(NPC_COMMANDER_ELIGOR_DAWNBRINGER, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 600000);
                temp->setFaction(2089);
                me->AddThreat(temp, 0.0f);
                uiEligorGUID = temp->GetGUID();
            }
            temp = Unit::GetCreature(*me, uiRayneGUID);
            if (!temp)
            {
                temp = me->SummonCreature(NPC_RAYNE, LightofDawnLoc[0].x+rand()%30, LightofDawnLoc[0].y+rand()%30, LightofDawnLoc[0].z, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
                temp->setFaction(2089);
                me->AddThreat(temp, 0.0f);
                uiRayneGUID = temp->GetGUID();
            }
        }

        void DespawnNPC(uint64 pGUID)
        {
            if (Creature* temp = Unit::GetCreature(*me, pGUID))
                if (temp->IsAlive())
                {
                    temp->SetVisible(false);
                    temp->Kill(temp);
                }
        }
    };
};

/*######
## npc the lich king in dawn of light
######*/
class npc_the_lich_king_tirion_dawn : public CreatureScript
{
public:
    npc_the_lich_king_tirion_dawn() : CreatureScript("npc_the_lich_king_tirion_dawn") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_the_lich_king_tirion_dawnAI(creature);
    }

    struct npc_the_lich_king_tirion_dawnAI : public ScriptedAI
    {
        npc_the_lich_king_tirion_dawnAI(Creature* creature) : ScriptedAI(creature) { Reset(); }
        void Reset() OVERRIDE { }
        void AttackStart(Unit* /*who*/) OVERRIDE { } // very sample, just don't make them aggreesive
        void UpdateAI(uint32 /*diff*/) OVERRIDE { }
        void JustDied(Unit* /*killer*/) OVERRIDE { }
    };
};

void AddSC_the_scarlet_enclave_c5()
{
    new npc_highlord_darion_mograine();
    new npc_the_lich_king_tirion_dawn();
}
