/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Shadowmoon_Valley
SD%Complete: 100
SDComment: Quest support: 10519, 10583, 10601, 10804, 10854, 10458, 10481, 10480, 11082, 10781, 10451. Vendor Drake Dealer Hurlunk.
SDCategory: Shadowmoon Valley
EndScriptData */

/* ContentData
npc_mature_netherwing_drake
npc_enslaved_netherwing_drake
npc_drake_dealer_hurlunk
npcs_flanis_swiftwing_and_kagrosh
npc_murkblood_overseer
npc_karynaku
npc_oronok_tornheart
npc_overlord_morghor
npc_earthmender_wilda
npc_torloth_the_magnificent
npc_illidari_spawn
npc_lord_illidan_stormrage
go_crystal_prison
npc_enraged_spirit
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "Group.h"
#include "SpellScript.h"
#include "Player.h"
#include "WorldSession.h"

/*#####
# npc_mature_netherwing_drake
#####*/

enum MatureNetherwing
{
    SAY_JUST_EATEN              = 0,

    SPELL_PLACE_CARCASS         = 38439,
    SPELL_JUST_EATEN            = 38502,
    SPELL_NETHER_BREATH         = 38467,
    POINT_ID                    = 1,

    GO_CARCASS                  = 185155,

    QUEST_KINDNESS              = 10804,
    NPC_EVENT_PINGER            = 22131
};

class npc_mature_netherwing_drake : public CreatureScript
{
public:
    npc_mature_netherwing_drake() : CreatureScript("npc_mature_netherwing_drake") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_mature_netherwing_drakeAI(creature);
    }

    struct npc_mature_netherwing_drakeAI : public ScriptedAI
    {
        npc_mature_netherwing_drakeAI(Creature* creature) : ScriptedAI(creature) { }

        uint64 uiPlayerGUID;

        bool bCanEat;
        bool bIsEating;

        uint32 EatTimer;
        uint32 CastTimer;

        void Reset() OVERRIDE
        {
            uiPlayerGUID = 0;

            bCanEat = false;
            bIsEating = false;

            EatTimer = 5000;
            CastTimer = 5000;
        }

        void SpellHit(Unit* pCaster, SpellInfo const* spell) OVERRIDE
        {
            if (bCanEat || bIsEating)
                return;

            if (pCaster->GetTypeId() == TypeID::TYPEID_PLAYER && spell->Id == SPELL_PLACE_CARCASS && !me->HasAura(SPELL_JUST_EATEN))
            {
                uiPlayerGUID = pCaster->GetGUID();
                bCanEat = true;
            }
        }

        void MovementInform(uint32 type, uint32 id) OVERRIDE
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == POINT_ID)
            {
                bIsEating = true;
                EatTimer = 7000;
                me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK_UNARMED);
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (bCanEat || bIsEating)
            {
                if (EatTimer <= diff)
                {
                    if (bCanEat && !bIsEating)
                    {
                        if (Unit* unit = Unit::GetUnit(*me, uiPlayerGUID))
                        {
                            if (GameObject* go = unit->FindNearestGameObject(GO_CARCASS, 10))
                            {
                                if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
                                    me->GetMotionMaster()->MovementExpired();

                                me->GetMotionMaster()->MoveIdle();
                                me->StopMoving();

                                me->GetMotionMaster()->MovePoint(POINT_ID, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
                            }
                        }
                        bCanEat = false;
                    }
                    else if (bIsEating)
                    {
                        DoCast(me, SPELL_JUST_EATEN);
                        Talk(SAY_JUST_EATEN);

                        if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayerGUID))
                        {
                            player->KilledMonsterCredit(NPC_EVENT_PINGER, 0);

                            if (GameObject* go = player->FindNearestGameObject(GO_CARCASS, 10))
                                go->Delete();
                        }

                        Reset();
                        me->GetMotionMaster()->Clear();
                    }
                }
                else
                    EatTimer -= diff;

            return;
            }

            if (!UpdateVictim())
                return;

            if (CastTimer <= diff)
            {
                DoCastVictim(SPELL_NETHER_BREATH);
                CastTimer = 5000;
            } else CastTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

/*###
# npc_enslaved_netherwing_drake
####*/

enum EnshlavedNetherwingDrake
{
    // Factions
    FACTION_DEFAULT                 = 62,
    FACTION_FRIENDLY                = 1840, // Not sure if this is correct, it was taken off of Mordenai.

    // Spells
    SPELL_HIT_FORCE_OF_NELTHARAKU   = 38762,
    SPELL_FORCE_OF_NELTHARAKU       = 38775,

    // Creatures
    NPC_DRAGONMAW_SUBJUGATOR        = 21718,
    NPC_ESCAPE_DUMMY                = 22317
};

class npc_enslaved_netherwing_drake : public CreatureScript
{
public:
    npc_enslaved_netherwing_drake() : CreatureScript("npc_enslaved_netherwing_drake") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_enslaved_netherwing_drakeAI(creature);
    }

    struct npc_enslaved_netherwing_drakeAI : public ScriptedAI
    {
        npc_enslaved_netherwing_drakeAI(Creature* creature) : ScriptedAI(creature)
        {
            PlayerGUID = 0;
            Tapped = false;
            Reset();
        }

        uint64 PlayerGUID;
        uint32 FlyTimer;
        bool Tapped;

        void Reset() OVERRIDE
        {
            if (!Tapped)
                me->setFaction(FACTION_DEFAULT);

            FlyTimer = 10000;
            me->SetDisableGravity(false);
            me->SetVisible(true);
        }

        void SpellHit(Unit* caster, const SpellInfo* spell) OVERRIDE
        {
            if (!caster)
                return;

            if (caster->GetTypeId() == TypeID::TYPEID_PLAYER && spell->Id == SPELL_HIT_FORCE_OF_NELTHARAKU && !Tapped)
            {
                Tapped = true;
                PlayerGUID = caster->GetGUID();

                me->setFaction(FACTION_FRIENDLY);
                DoCast(caster, SPELL_FORCE_OF_NELTHARAKU, true);

                Unit* Dragonmaw = me->FindNearestCreature(NPC_DRAGONMAW_SUBJUGATOR, 50);
                if (Dragonmaw)
                {
                    me->AddThreat(Dragonmaw, 100000.0f);
                    AttackStart(Dragonmaw);
                }

                HostileReference* ref = me->getThreatManager().getOnlineContainer().getReferenceByTarget(caster);
                if (ref)
                    ref->removeReference();
            }
        }

        void MovementInform(uint32 type, uint32 id) OVERRIDE
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 1)
            {
                if (PlayerGUID)
                {
                    Unit* player = Unit::GetUnit(*me, PlayerGUID);
                    if (player)
                        DoCast(player, SPELL_FORCE_OF_NELTHARAKU, true);

                    PlayerGUID = 0;
                }
                me->SetVisible(false);
                me->SetDisableGravity(false);
                me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                me->RemoveCorpse();
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
            {
                if (Tapped)
                {
                    if (FlyTimer <= diff)
                    {
                        Tapped = false;
                        if (PlayerGUID)
                        {
                            Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                            if (player && player->GetQuestStatus(10854) == QUEST_STATUS_INCOMPLETE)
                            {
                                DoCast(player, SPELL_FORCE_OF_NELTHARAKU, true);
                                /*
                                float x, y, z;
                                me->GetPosition(x, y, z);

                                float dx, dy, dz;
                                me->GetRandomPoint(x, y, z, 20, dx, dy, dz);
                                dz += 20; // so it's in the air, not ground*/

                                Position pos;
                                if (Unit* EscapeDummy = me->FindNearestCreature(NPC_ESCAPE_DUMMY, 30))
                                    EscapeDummy->GetPosition(&pos);
                                else
                                {
                                    me->GetRandomNearPosition(pos, 20);
                                    pos.m_positionZ += 25;
                                }

                                me->SetDisableGravity(true);
                                me->GetMotionMaster()->MovePoint(1, pos);
                            }
                        }
                    } else FlyTimer -= diff;
                }
                return;
            }

            DoMeleeAttackIfReady();
        }
    };
};

/*#####
# npc_dragonmaw_peon
#####*/

class npc_dragonmaw_peon : public CreatureScript
{
public:
    npc_dragonmaw_peon() : CreatureScript("npc_dragonmaw_peon") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_dragonmaw_peonAI(creature);
    }

    struct npc_dragonmaw_peonAI : public ScriptedAI
    {
        npc_dragonmaw_peonAI(Creature* creature) : ScriptedAI(creature) { }

        uint64 PlayerGUID;
        bool Tapped;
        uint32 PoisonTimer;

        void Reset() OVERRIDE
        {
            PlayerGUID = 0;
            Tapped = false;
            PoisonTimer = 0;
        }

        void SpellHit(Unit* caster, const SpellInfo* spell) OVERRIDE
        {
            if (!caster)
                return;

            if (caster->GetTypeId() == TypeID::TYPEID_PLAYER && spell->Id == 40468 && !Tapped)
            {
                PlayerGUID = caster->GetGUID();

                Tapped = true;
                float x, y, z;
                caster->GetClosePoint(x, y, z, me->GetObjectSize());

                me->SetWalk(false);
                me->GetMotionMaster()->MovePoint(1, x, y, z);
            }
        }

        void MovementInform(uint32 type, uint32 id) OVERRIDE
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id)
            {
                me->SetUInt32Value(UNIT_FIELD_NPC_EMOTESTATE, EMOTE_ONESHOT_EAT);
                PoisonTimer = 15000;
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (PoisonTimer)
            {
                if (PoisonTimer <= diff)
                {
                    if (PlayerGUID)
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        if (player && player->GetQuestStatus(11020) == QUEST_STATUS_INCOMPLETE)
                            player->KilledMonsterCredit(23209, 0);
                    }
                    PoisonTimer = 0;
                    me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                } else PoisonTimer -= diff;
            }
        }
    };
};

/*######
## npc_drake_dealer_hurlunk
######*/

class npc_drake_dealer_hurlunk : public CreatureScript
{
public:
    npc_drake_dealer_hurlunk() : CreatureScript("npc_drake_dealer_hurlunk") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        if (creature->IsVendor() && player->GetReputationRank(1015) == REP_EXALTED)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }
};

/*######
## npc_flanis_swiftwing_and_kagrosh
######*/

#define GOSSIP_HSK1 "Take Flanis's Pack"
#define GOSSIP_HSK2 "Take Kagrosh's Pack"

class npcs_flanis_swiftwing_and_kagrosh : public CreatureScript
{
public:
    npcs_flanis_swiftwing_and_kagrosh() : CreatureScript("npcs_flanis_swiftwing_and_kagrosh") { }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            ItemPosCountVec dest;
            uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 30658, 1, NULL);
            if (msg == EQUIP_ERR_OK)
            {
                player->StoreNewItem(dest, 30658, 1, true);
                player->PlayerTalkClass->ClearMenus();
            }
        }
        if (action == GOSSIP_ACTION_INFO_DEF+2)
        {
            ItemPosCountVec dest;
            uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 30659, 1, NULL);
            if (msg == EQUIP_ERR_OK)
            {
                player->StoreNewItem(dest, 30659, 1, true);
                player->PlayerTalkClass->ClearMenus();
            }
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        if (player->GetQuestStatus(10583) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(30658, 1, true))
            player->ADD_GOSSIP_ITEM(0, GOSSIP_HSK1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        if (player->GetQuestStatus(10601) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(30659, 1, true))
            player->ADD_GOSSIP_ITEM(0, GOSSIP_HSK2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }
};

/*######
## npc_murkblood_overseer
######*/

#define QUEST_11082     11082

#define GOSSIP_HMO "I am here for you, overseer."
#define GOSSIP_SMO1 "How dare you question an overseer of the Dragonmaw!"
#define GOSSIP_SMO2 "Who speaks of me? What are you talking about, broken?"
#define GOSSIP_SMO3 "Continue please."
#define GOSSIP_SMO4 "Who are these bidders?"
#define GOSSIP_SMO5 "Well... yes."

class npc_murkblood_overseer : public CreatureScript
{
public:
    npc_murkblood_overseer() : CreatureScript("npc_murkblood_overseer") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SMO1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                                                                //correct id not known
                player->SEND_GOSSIP_MENU(10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SMO2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                                                                //correct id not known
                player->SEND_GOSSIP_MENU(10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SMO3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                                                                //correct id not known
                player->SEND_GOSSIP_MENU(10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SMO4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                                                                //correct id not known
                player->SEND_GOSSIP_MENU(10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SMO5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                                                                //correct id not known
                player->SEND_GOSSIP_MENU(10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                                                                //correct id not known
                player->SEND_GOSSIP_MENU(10940, creature->GetGUID());
                creature->CastSpell(player, 41121, false);
                player->AreaExploredOrEventHappens(QUEST_11082);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_11082) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(0, GOSSIP_HMO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        player->SEND_GOSSIP_MENU(10940, creature->GetGUID());
        return true;
    }
};

/*######
## npc_oronok
######*/

#define GOSSIP_ORONOK1 "I am ready to hear your story, Oronok."
#define GOSSIP_ORONOK2 "How do I find the cipher?"
#define GOSSIP_ORONOK3 "How do you know all of this?"
#define GOSSIP_ORONOK4 "Yet what? What is it, Oronok?"
#define GOSSIP_ORONOK5 "Continue, please."
#define GOSSIP_ORONOK6 "So what of the cipher now? And your boys?"
#define GOSSIP_ORONOK7 "I will find your boys and the cipher, Oronok."

class npc_oronok_tornheart : public CreatureScript
{
public:
    npc_oronok_tornheart() : CreatureScript("npc_oronok_tornheart") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_TRADE:
                player->GetSession()->SendListInventory(creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF:
                player->ADD_GOSSIP_ITEM(0, GOSSIP_ORONOK2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                player->SEND_GOSSIP_MENU(10313, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+1:
                player->ADD_GOSSIP_ITEM(0, GOSSIP_ORONOK3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                player->SEND_GOSSIP_MENU(10314, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                player->ADD_GOSSIP_ITEM(0, GOSSIP_ORONOK4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                player->SEND_GOSSIP_MENU(10315, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                player->ADD_GOSSIP_ITEM(0, GOSSIP_ORONOK5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                player->SEND_GOSSIP_MENU(10316, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                player->ADD_GOSSIP_ITEM(0, GOSSIP_ORONOK6, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                player->SEND_GOSSIP_MENU(10317, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                player->ADD_GOSSIP_ITEM(0, GOSSIP_ORONOK7, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                player->SEND_GOSSIP_MENU(10318, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                player->CLOSE_GOSSIP_MENU();
                player->AreaExploredOrEventHappens(10519);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());
        if (creature->IsVendor())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        if (player->GetQuestStatus(10519) == QUEST_STATUS_INCOMPLETE)
        {
            player->ADD_GOSSIP_ITEM(0, GOSSIP_ORONOK1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            player->SEND_GOSSIP_MENU(10312, creature->GetGUID());
        }else
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }
};

/*####
# npc_karynaku
####*/

enum Karynaku
{
    QUEST_ALLY_OF_NETHER    = 10870,
    QUEST_ZUHULED_THE_WACK  = 10866,

    NPC_ZUHULED_THE_WACKED  = 11980,

    TAXI_PATH_ID            = 649,
};

class npc_karynaku : public CreatureScript
{
    public:
        npc_karynaku() : CreatureScript("npc_karynaku") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) OVERRIDE
        {
            if (quest->GetQuestId() == QUEST_ALLY_OF_NETHER)
                player->ActivateTaxiPathTo(TAXI_PATH_ID);

            if (quest->GetQuestId() == QUEST_ZUHULED_THE_WACK)
                creature->SummonCreature(NPC_ZUHULED_THE_WACKED, -4204.94f, 316.397f, 122.508f, 1.309f, TempSummonType::TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);

            return true;
        }
};

/*####
# npc_overlord_morghor
# this whole script is wrong and needs a rewrite.even the illidan npc used is the wrong one.npc id 23467 may be the correct one
####*/
enum OverlordData
{
    QUEST_LORD_ILLIDAN_STORMRAGE    = 11108,

    C_ILLIDAN                       = 22083,
    C_YARZILL                       = 23141,

    SPELL_ONE                       = 39990, // Red Lightning Bolt
    SPELL_TWO                       = 41528, // Mark of Stormrage
    SPELL_THREE                     = 40216, // Dragonaw Faction
    SPELL_FOUR                      = 42016, // Dragonaw Trasform

    OVERLORD_SAY_1                  = 0,
    OVERLORD_SAY_2                  = 1,
  //OVERLORD_SAY_3                  = 2,
    OVERLORD_SAY_4                  = 3,
    OVERLORD_SAY_5                  = 4,
    OVERLORD_SAY_6                  = 5,

    OVERLORD_YELL_1                 = 6,
    OVERLORD_YELL_2                 = 7,

    LORD_ILLIDAN_SAY_1              = 0,
    LORD_ILLIDAN_SAY_2              = 1,
    LORD_ILLIDAN_SAY_3              = 2,
    LORD_ILLIDAN_SAY_4              = 3,
    LORD_ILLIDAN_SAY_5              = 4,
    LORD_ILLIDAN_SAY_6              = 5,
    LORD_ILLIDAN_SAY_7              = 6,

    YARZILL_THE_MERC_SAY            = 0
};

class npc_overlord_morghor : public CreatureScript
{
public:
    npc_overlord_morghor() : CreatureScript("npc_overlord_morghor") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest *_Quest) OVERRIDE
    {
        if (_Quest->GetQuestId() == QUEST_LORD_ILLIDAN_STORMRAGE)
        {
            CAST_AI(npc_overlord_morghor::npc_overlord_morghorAI, creature->AI())->PlayerGUID = player->GetGUID();
            CAST_AI(npc_overlord_morghor::npc_overlord_morghorAI, creature->AI())->StartEvent();
            return true;
        }
        return false;
    }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
    return new npc_overlord_morghorAI(creature);
    }

    struct npc_overlord_morghorAI : public ScriptedAI
    {
        npc_overlord_morghorAI(Creature* creature) : ScriptedAI(creature) { }

        uint64 PlayerGUID;
        uint64 IllidanGUID;

        uint32 ConversationTimer;
        uint32 Step;

        bool Event;

        void Reset() OVERRIDE
        {
            PlayerGUID = 0;
            IllidanGUID = 0;

            ConversationTimer = 0;
            Step = 0;

            Event = false;
            me->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 2);
        }

        void StartEvent()
        {
            me->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 0);
            me->SetUInt32Value(UNIT_FIELD_ANIM_TIER, 0);
            Unit* Illidan = me->SummonCreature(C_ILLIDAN, -5107.83f, 602.584f, 85.2393f, 4.92598f, TempSummonType::TEMPSUMMON_CORPSE_DESPAWN, 0);
            if (Illidan)
            {
                IllidanGUID = Illidan->GetGUID();
                Illidan->SetVisible(false);
            }
            if (PlayerGUID)
            {
                Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                if (player)
                    Talk(OVERLORD_SAY_1, player);
            }
            ConversationTimer = 4200;
            Step = 0;
            Event = true;
        }

        uint32 NextStep(uint32 Step)
        {
            Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
            Creature* Illi = ObjectAccessor::GetCreature(*me, IllidanGUID);

            if (!player)
            {
                EnterEvadeMode();
                return 0;
            }

            switch (Step)
            {
                case 0:
                    return 0;
                    break;
                case 1:
                    me->GetMotionMaster()->MovePoint(0, -5104.41f, 595.297f, 85.6838f);
                    return 9000;
                    break;
                case 2:
                    Talk(OVERLORD_YELL_1, player);
                    return 4500;
                    break;
                case 3:
                    me->SetInFront(player);
                    return 3200;
                    break;
                case 4:
                    Talk(OVERLORD_SAY_2, player);
                    return 2000;
                    break;
                case 5:
                    if (Illi)
                    {
                        Illi->SetVisible(true);
                        Illi->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        Illi->SetDisplayId(21526);
                    }
                    return 350;
                    break;
                case 6:
                    if (Illi)
                    {
                        Illi->CastSpell(Illi, SPELL_ONE, true);
                        Illi->SetTarget(me->GetGUID());
                        me->SetTarget(IllidanGUID);
                    }
                    return 2000;
                    break;
                case 7:
                    Talk(OVERLORD_YELL_2);
                    return 4500;
                    break;
                case 8:
                    me->SetUInt32Value(UNIT_FIELD_ANIM_TIER, 8);
                    return 2500;
                    break;
                case 9:
                    // missing text "Lord Illidan, this is the Dragonmaw that I, and others, have told you about. He will lead us to victory!"
                    return 5000;
                    break;
                case 10:
                    if (Illi)
                        Illi->AI()->Talk(LORD_ILLIDAN_SAY_1);
                    return 5000;
                    break;
                case 11:
                    Talk(OVERLORD_SAY_4, player);
                    return 6000;
                    break;
                case 12:
                    if (Illi)
                        Illi->AI()->Talk(LORD_ILLIDAN_SAY_2);
                    return 5500;
                    break;
                case 13:
                    if (Illi)
                        Illi->AI()->Talk(LORD_ILLIDAN_SAY_3);
                    return 4000;
                    break;
                case 14:
                    if (Illi)
                        Illi->SetTarget(PlayerGUID);
                    return 1500;
                    break;
                case 15:
                    if (Illi)
                        Illi->AI()->Talk(LORD_ILLIDAN_SAY_4);
                    return 1500;
                    break;
                case 16:
                    if (Illi)
                        Illi->CastSpell(player, SPELL_TWO, true);
                    player->RemoveAurasDueToSpell(SPELL_THREE);
                    player->RemoveAurasDueToSpell(SPELL_FOUR);
                    return 5000;
                    break;
                case 17:
                    if (Illi)
                        Illi->AI()->Talk(LORD_ILLIDAN_SAY_5);
                    return 5000;
                    break;
                case 18:
                    if (Illi)
                        Illi->AI()->Talk(LORD_ILLIDAN_SAY_6);
                    return 5000;
                    break;
                case 19:
                    if (Illi)
                        Illi->AI()->Talk(LORD_ILLIDAN_SAY_7);
                    return 5000;
                    break;
                case 20:
                    if (Illi)
                    {
                        Illi->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
                        Illi->SetDisableGravity(true);
                    }
                    return 500;
                    break;
                case 21:
                    Talk(OVERLORD_SAY_5);
                    return 500;
                    break;
                case 22:
                    if (Illi)
                    {
                        Illi->SetVisible(false);
                        Illi->setDeathState(DeathState::JUST_DIED);
                    }
                    return 1000;
                    break;
                case 23:
                    me->SetUInt32Value(UNIT_FIELD_ANIM_TIER, 0);
                    return 2000;
                    break;
                case 24:
                    me->SetTarget(PlayerGUID);
                    return 5000;
                    break;
                case 25:
                    Talk(OVERLORD_SAY_6);
                    return 2000;
                    break;
                case 26:
                    player->GroupEventHappens(QUEST_LORD_ILLIDAN_STORMRAGE, me);
                    return 6000;
                    break;
                case 27:
                    {
                        Unit* Yarzill = me->FindNearestCreature(C_YARZILL, 50.0f);
                        if (Yarzill)
                            Yarzill->SetTarget(PlayerGUID);
                        return 500;
                    }
                    break;
                case 28:
                    player->RemoveAurasDueToSpell(SPELL_TWO);
                    player->RemoveAurasDueToSpell(41519);
                    player->CastSpell(player, SPELL_THREE, true);
                    player->CastSpell(player, SPELL_FOUR, true);
                    return 1000;
                    break;
                case 29:
                    {
                        if (Creature* Yarzill = me->FindNearestCreature(C_YARZILL, 50.0f))
                            Yarzill->AI()->Talk(YARZILL_THE_MERC_SAY, player);
                        return 5000;
                    }
                    break;
                case 30:
                    {
                        if (Creature* Yarzill = me->FindNearestCreature(C_YARZILL, 50.0f))
                            Yarzill->SetTarget(0);
                        return 5000;
                    }
                    break;
                case 31:
                    {
                        if (Creature* Yarzill = me->FindNearestCreature(C_YARZILL, 50.0f))
                            Yarzill->CastSpell(player, 41540, true);
                        return 1000;
                    }
                    break;
                case 32:
                    me->GetMotionMaster()->MovePoint(0, -5085.77f, 577.231f, 86.6719f);
                    return 5000;
                    break;
                case 33:
                    me->SetTarget(0);
                    Reset();
                    return 100;
                    break;
                default :
                    return 0;
                    break;
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!ConversationTimer)
                return;

            if (ConversationTimer <= diff)
            {
                if (Event && PlayerGUID)
                    ConversationTimer = NextStep(++Step);
            } else ConversationTimer -= diff;
        }
    };
};

/*####
# npc_earthmender_wilda
####*/

enum Earthmender
{
    SAY_WIL_START               = 0,
    SAY_WIL_AGGRO               = 1,
    SAY_WIL_PROGRESS1           = 2,
    SAY_WIL_PROGRESS2           = 3,
    SAY_WIL_FIND_EXIT           = 4,
    SAY_WIL_JUST_AHEAD          = 5,
    SAY_WIL_END                 = 6,

    SPELL_CHAIN_LIGHTNING       = 16006,
    SPELL_EARTHBING_TOTEM       = 15786,
    SPELL_FROST_SHOCK           = 12548,
    SPELL_HEALING_WAVE          = 12491,

    QUEST_ESCAPE_COILSCAR       = 10451,
    NPC_COILSKAR_ASSASSIN       = 21044,
    FACTION_EARTHEN             = 1726                      //guessed
};

class npc_earthmender_wilda : public CreatureScript
{
public:
    npc_earthmender_wilda() : CreatureScript("npc_earthmender_wilda") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest) OVERRIDE
    {
        if (quest->GetQuestId() == QUEST_ESCAPE_COILSCAR)
        {
            creature->AI()->Talk(SAY_WIL_START, player);
            creature->setFaction(FACTION_EARTHEN);

            if (npc_earthmender_wildaAI* pEscortAI = CAST_AI(npc_earthmender_wilda::npc_earthmender_wildaAI, creature->AI()))
                pEscortAI->Start(false, false, player->GetGUID(), quest);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_earthmender_wildaAI(creature);
    }

    struct npc_earthmender_wildaAI : public npc_escortAI
    {
        npc_earthmender_wildaAI(Creature* creature) : npc_escortAI(creature) { }

        uint32 m_uiHealingTimer;

        void Reset() OVERRIDE
        {
            m_uiHealingTimer = 0;
        }

        void WaypointReached(uint32 waypointId) OVERRIDE
        {
            Player* player = GetPlayerForEscort();
            if (!player)
                return;

            switch (waypointId)
            {
                case 13:
                    Talk(SAY_WIL_PROGRESS1, player);
                    DoSpawnAssassin();
                    break;
                case 14:
                    DoSpawnAssassin();
                    break;
                case 15:
                    Talk(SAY_WIL_FIND_EXIT, player);
                    break;
                case 19:
                    DoRandomSay();
                    break;
                case 20:
                    DoSpawnAssassin();
                    break;
                case 26:
                    DoRandomSay();
                    break;
                case 27:
                    DoSpawnAssassin();
                    break;
                case 33:
                    DoRandomSay();
                    break;
                case 34:
                    DoSpawnAssassin();
                    break;
                case 37:
                    DoRandomSay();
                    break;
                case 38:
                    DoSpawnAssassin();
                    break;
                case 39:
                    Talk(SAY_WIL_JUST_AHEAD, player);
                    break;
                case 43:
                    DoRandomSay();
                    break;
                case 44:
                    DoSpawnAssassin();
                    break;
                case 50:
                    Talk(SAY_WIL_END, player);
                    player->GroupEventHappens(QUEST_ESCAPE_COILSCAR, me);
                    break;
            }
        }

        void JustSummoned(Creature* summoned) OVERRIDE
        {
            if (summoned->GetEntry() == NPC_COILSKAR_ASSASSIN)
                summoned->AI()->AttackStart(me);
        }

        //this is very unclear, random say without no real relevance to script/event
        void DoRandomSay()
        {
            Talk(SAY_WIL_PROGRESS2);
        }

        void DoSpawnAssassin()
        {
            //unknown where they actually appear
            DoSummon(NPC_COILSKAR_ASSASSIN, me, 15.0f, 5000, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT);
        }

        void EnterCombat(Unit* who) OVERRIDE
        {
            //don't always use
            if (rand()%5)
                return;

            //only aggro text if not player
            if (who->GetTypeId() != TypeID::TYPEID_PLAYER)
            {
                //appears to be random
                if (std::rand() % 1)
                    Talk(SAY_WIL_AGGRO);
            }
        }

        void UpdateAI(uint32 uiDiff) OVERRIDE
        {
            npc_escortAI::UpdateAI(uiDiff);

            if (!UpdateVictim())
                return;

            /// @todo add more abilities
            if (!HealthAbovePct(30))
            {
                if (m_uiHealingTimer <= uiDiff)
                {
                    DoCast(me, SPELL_HEALING_WAVE);
                    m_uiHealingTimer = 15000;
                }
                else
                    m_uiHealingTimer -= uiDiff;
            }
        }
    };
};

/*#####
# Quest: Battle of the crimson watch
#####*/

/* ContentData
Battle of the crimson watch - creatures, gameobjects and defines
npc_illidari_spawn : Adds that are summoned in the Crimson Watch battle.
npc_torloth_the_magnificent : Final Creature that players have to face before quest is completed
npc_lord_illidan_stormrage : Creature that controls the event.
go_crystal_prison : GameObject that begins the event and hands out quest
EndContentData */

#define QUEST_BATTLE_OF_THE_CRIMSON_WATCH 10781
#define EVENT_AREA_RADIUS 65 //65yds
#define EVENT_COOLDOWN 30000 //in ms. appear after event completed or failed (should be = Adds despawn time)

struct TorlothCinematic
{
    uint32 creature, Timer;
};

// Creature 0 - Torloth, 1 - Illidan
static TorlothCinematic TorlothAnim[]=
{
    {0, 2000},
    {1, 7000},
    {0, 3000},
    {0, 2000}, // Torloth stand
    {0, 1000},
    {0, 3000},
    {0, 0}
};

struct Location
{
    float x, y, z, o;
};

//Cordinates for Spawns
static Location SpawnLocation[]=
{
    //Cords used for:
    {-4615.8556f, 1342.2532f, 139.9f, 1.612f}, //Illidari Soldier
    {-4598.9365f, 1377.3182f, 139.9f, 3.917f}, //Illidari Soldier
    {-4598.4697f, 1360.8999f, 139.9f, 2.427f}, //Illidari Soldier
    {-4589.3599f, 1369.1061f, 139.9f, 3.165f}, //Illidari Soldier
    {-4608.3477f, 1386.0076f, 139.9f, 4.108f}, //Illidari Soldier
    {-4633.1889f, 1359.8033f, 139.9f, 0.949f}, //Illidari Soldier
    {-4623.5791f, 1351.4574f, 139.9f, 0.971f}, //Illidari Soldier
    {-4607.2988f, 1351.6099f, 139.9f, 2.416f}, //Illidari Soldier
    {-4633.7764f, 1376.0417f, 139.9f, 5.608f}, //Illidari Soldier
    {-4600.2461f, 1369.1240f, 139.9f, 3.056f}, //Illidari Mind Breaker
    {-4631.7808f, 1367.9459f, 139.9f, 0.020f}, //Illidari Mind Breaker
    {-4600.2461f, 1369.1240f, 139.9f, 3.056f}, //Illidari Highlord
    {-4631.7808f, 1367.9459f, 139.9f, 0.020f}, //Illidari Highlord
    {-4615.5586f, 1353.0031f, 139.9f, 1.540f}, //Illidari Highlord
    {-4616.4736f, 1384.2170f, 139.9f, 4.971f}, //Illidari Highlord
    {-4627.1240f, 1378.8752f, 139.9f, 2.544f} //Torloth The Magnificent
};

struct WaveData
{
    uint8 SpawnCount, UsedSpawnPoint;
    uint32 CreatureId, SpawnTimer, YellTimer;
};

static WaveData WavesInfo[]=
{
    {9, 0, 22075, 10000, 7000},   //Illidari Soldier
    {2, 9, 22074, 10000, 7000},   //Illidari Mind Breaker
    {4, 11, 19797, 10000, 7000},  //Illidari Highlord
    {1, 15, 22076, 10000, 7000}   //Torloth The Magnificent
};

struct SpawnSpells
{
 uint32 Timer1, Timer2, SpellId;
};

static SpawnSpells SpawnCast[]=
{
    {10000, 15000, 35871},  // Illidari Soldier Cast - Spellbreaker
    {10000, 10000, 38985},  // Illidari Mind Breake Cast - Focused Bursts
    {35000, 35000, 22884},  // Illidari Mind Breake Cast - Psychic Scream
    {20000, 20000, 17194},  // Illidari Mind Breake Cast - Mind Blast
    {8000, 15000, 38010},   // Illidari Highlord Cast - Curse of Flames
    {12000, 20000, 16102},  // Illidari Highlord Cast - Flamestrike
    {10000, 15000, 15284},  // Torloth the Magnificent Cast - Cleave
    {18000, 20000, 39082},  // Torloth the Magnificent Cast - Shadowfury
    {25000, 28000, 33961}   // Torloth the Magnificent Cast - Spell Reflection
};

/*######
# npc_torloth_the_magnificent
#####*/

class npc_torloth_the_magnificent : public CreatureScript
{
public:
    npc_torloth_the_magnificent() : CreatureScript("npc_torloth_the_magnificent") { }

    CreatureAI* GetAI(Creature* c) const OVERRIDE
    {
        return new npc_torloth_the_magnificentAI(c);
    }

    struct npc_torloth_the_magnificentAI : public ScriptedAI
    {
        npc_torloth_the_magnificentAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 AnimationTimer, SpellTimer1, SpellTimer2, SpellTimer3;

        uint8 AnimationCount;

        uint64 LordIllidanGUID;
        uint64 AggroTargetGUID;

        bool Timers;

        void Reset() OVERRIDE
        {
            AnimationTimer = 4000;
            AnimationCount = 0;
            LordIllidanGUID = 0;
            AggroTargetGUID = 0;
            Timers = false;

            me->AddUnitState(UNIT_STATE_ROOT);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetTarget(0);
        }

        void EnterCombat(Unit* /*who*/)OVERRIDE { }

        void HandleAnimation()
        {
            Creature* creature = me;

            if (TorlothAnim[AnimationCount].creature == 1)
            {
                creature = (Unit::GetCreature(*me, LordIllidanGUID));

                if (!creature)
                    return;
            }

            AnimationTimer = TorlothAnim[AnimationCount].Timer;

            switch (AnimationCount)
            {
            case 0:
                me->SetUInt32Value(UNIT_FIELD_ANIM_TIER, 8);
                break;
            case 3:
                me->RemoveFlag(UNIT_FIELD_ANIM_TIER, 8);
                break;
            case 5:
                if (Player* AggroTarget = ObjectAccessor::GetPlayer(*me, AggroTargetGUID))
                {
                    me->SetTarget(AggroTarget->GetGUID());
                    me->AddThreat(AggroTarget, 1);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                }
                break;
            case 6:
                if (Player* AggroTarget = ObjectAccessor::GetPlayer(*me, AggroTargetGUID))
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->ClearUnitState(UNIT_STATE_ROOT);

                    float x, y, z;
                    AggroTarget->GetPosition(x, y, z);
                    me->GetMotionMaster()->MovePoint(0, x, y, z);
                }
                break;
            }
            ++AnimationCount;
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (AnimationTimer)
            {
                if (AnimationTimer <= diff)
                {
                    HandleAnimation();
                } else AnimationTimer -= diff;
            }

            if (AnimationCount < 6)
            {
                me->CombatStop();
            } else if (!Timers)
            {
                SpellTimer1 = SpawnCast[6].Timer1;
                SpellTimer2 = SpawnCast[7].Timer1;
                SpellTimer3 = SpawnCast[8].Timer1;
                Timers = true;
            }

            if (Timers)
            {
                if (SpellTimer1 <= diff)
                {
                    DoCastVictim(SpawnCast[6].SpellId);//Cleave
                    SpellTimer1 = SpawnCast[6].Timer2 + (rand()%10 * 1000);
                } else SpellTimer1 -= diff;

                if (SpellTimer2 <= diff)
                {
                    DoCastVictim(SpawnCast[7].SpellId);//Shadowfury
                    SpellTimer2 = SpawnCast[7].Timer2 + (rand()%5 * 1000);
                } else SpellTimer2 -= diff;

                if (SpellTimer3 <= diff)
                {
                    DoCast(me, SpawnCast[8].SpellId);
                    SpellTimer3 = SpawnCast[8].Timer2 + (rand()%7 * 1000);//Spell Reflection
                } else SpellTimer3 -= diff;
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer) OVERRIDE
        {
            switch (killer->GetTypeId())
            {
                case TypeID::TYPEID_UNIT:
                    if (Unit* owner = killer->GetOwner())
                        if (Player* player = owner->ToPlayer())
                            player->GroupEventHappens(QUEST_BATTLE_OF_THE_CRIMSON_WATCH, me);
                    break;
                case TypeID::TYPEID_PLAYER:
                    if (Player* player = killer->ToPlayer())
                        player->GroupEventHappens(QUEST_BATTLE_OF_THE_CRIMSON_WATCH, me);
                    break;
                default:
                    break;
            }

            if (Creature* LordIllidan = (Unit::GetCreature(*me, LordIllidanGUID)))
                LordIllidan->AI()->EnterEvadeMode();
        }
    };
};

/*#####
# npc_lord_illidan_stormrage
#####*/

class npc_lord_illidan_stormrage : public CreatureScript
{
public:
    npc_lord_illidan_stormrage() : CreatureScript("npc_lord_illidan_stormrage") { }

    struct npc_lord_illidan_stormrageAI : public ScriptedAI
    {
        npc_lord_illidan_stormrageAI(Creature* creature) : ScriptedAI(creature)
        {
            PlayerGUID = 0;
            WaveTimer = 10000;
            AnnounceTimer = 7000;
            LiveCount = 0;
            WaveCount = 0;
            EventStarted = false;
            Announced = false;
            Failed = false;
        }

        void Reset() OVERRIDE
        {
            PlayerGUID = 0;
            WaveTimer = 10000;
            AnnounceTimer = 7000;
            LiveCount = 0;
            WaveCount = 0;
            EventStarted = false;
            Announced = false;
            Failed = false;

            me->SetVisible(false);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE { }

        void MoveInLineOfSight(Unit* /*who*/) OVERRIDE { }

        void AttackStart(Unit* /*who*/) OVERRIDE { }

        void SummonNextWave();

        void CheckEventFail()
        {
            Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);

            if (!player)
                return;

            if (Group* EventGroup = player->GetGroup())
            {
                uint8 GroupMemberCount = 0;
                uint8 DeadMemberCount = 0;
                uint8 FailedMemberCount = 0;

                Group::MemberSlotList const& members = EventGroup->GetMemberSlots();

                for (Group::member_citerator itr = members.begin(); itr!= members.end(); ++itr)
                {
                    Player* GroupMember = ObjectAccessor::GetPlayer(*me, itr->guid);
                    if (!GroupMember)
                        continue;
                    if (!GroupMember->IsWithinDistInMap(me, EVENT_AREA_RADIUS) && GroupMember->GetQuestStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH) == QUEST_STATUS_INCOMPLETE)
                    {
                        GroupMember->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
                        ++FailedMemberCount;
                    }
                    ++GroupMemberCount;

                    if (GroupMember->isDead())
                        ++DeadMemberCount;
                }

                if (GroupMemberCount == FailedMemberCount)
                {
                    Failed = true;
                }

                if (GroupMemberCount == DeadMemberCount)
                {
                    for (Group::member_citerator itr = members.begin(); itr!= members.end(); ++itr)
                    {
                        if (Player* groupMember = ObjectAccessor::GetPlayer(*me, itr->guid))
                            if (groupMember->GetQuestStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH) == QUEST_STATUS_INCOMPLETE)
                                groupMember->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
                    }
                    Failed = true;
                }
            } else if (player->isDead() || !player->IsWithinDistInMap(me, EVENT_AREA_RADIUS))
            {
                player->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
                Failed = true;
            }
        }

        void LiveCounter()
        {
            --LiveCount;
            if (!LiveCount)
                Announced = false;
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!PlayerGUID || !EventStarted)
                return;

            if (!LiveCount && WaveCount < 4)
            {
                if (!Announced && AnnounceTimer <= diff)
                {
                    Announced = true;
                }
                else
                    AnnounceTimer -= diff;

                if (WaveTimer <= diff)
                {
                    SummonNextWave();
                }
                else
                    WaveTimer -= diff;
            }
            CheckEventFail();

            if (Failed)
                EnterEvadeMode();
        }

        uint64 PlayerGUID;
        uint32 WaveTimer;
        uint32 AnnounceTimer;
        int8 LiveCount;
        uint8 WaveCount;
        bool EventStarted;
        bool Announced;
        bool Failed;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_lord_illidan_stormrageAI(creature);
    }
};

/*######
# npc_illidari_spawn
######*/

class npc_illidari_spawn : public CreatureScript
{
public:
    npc_illidari_spawn() : CreatureScript("npc_illidari_spawn") { }

    CreatureAI* GetAI(Creature* c) const OVERRIDE
    {
        return new npc_illidari_spawnAI(c);
    }

    struct npc_illidari_spawnAI : public ScriptedAI
    {
        npc_illidari_spawnAI(Creature* creature) : ScriptedAI(creature) { }

        uint64 LordIllidanGUID;
        uint32 SpellTimer1, SpellTimer2, SpellTimer3;
        bool Timers;

        void Reset() OVERRIDE
        {
            LordIllidanGUID = 0;
            Timers = false;
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE { }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            me->RemoveCorpse();
            if (Creature* LordIllidan = (Unit::GetCreature(*me, LordIllidanGUID)))
                if (LordIllidan)
                    CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, LordIllidan->AI())->LiveCounter();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            if (!Timers)
            {
                if (me->GetEntry() == 22075)//Illidari Soldier
                {
                    SpellTimer1 = SpawnCast[0].Timer1 + (rand()%4 * 1000);
                }
                if (me->GetEntry() == 22074)//Illidari Mind Breaker
                {
                    SpellTimer1 = SpawnCast[1].Timer1 + (rand()%10 * 1000);
                    SpellTimer2 = SpawnCast[2].Timer1 + (rand()%4 * 1000);
                    SpellTimer3 = SpawnCast[3].Timer1 + (rand()%4 * 1000);
                }
                if (me->GetEntry() == 19797)// Illidari Highlord
                {
                    SpellTimer1 = SpawnCast[4].Timer1 + (rand()%4 * 1000);
                    SpellTimer2 = SpawnCast[5].Timer1 + (rand()%4 * 1000);
                }
                Timers = true;
            }
            //Illidari Soldier
            if (me->GetEntry() == 22075)
            {
                if (SpellTimer1 <= diff)
                {
                    DoCastVictim(SpawnCast[0].SpellId);//Spellbreaker
                    SpellTimer1 = SpawnCast[0].Timer2 + (rand()%5 * 1000);
                } else SpellTimer1 -= diff;
            }
            //Illidari Mind Breaker
            if (me->GetEntry() == 22074)
            {
                if (SpellTimer1 <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    {
                        if (target->GetTypeId() == TypeID::TYPEID_PLAYER)
                        {
                            DoCast(target, SpawnCast[1].SpellId); //Focused Bursts
                            SpellTimer1 = SpawnCast[1].Timer2 + (rand()%5 * 1000);
                        } else SpellTimer1 = 2000;
                    }
                } else SpellTimer1 -= diff;

                if (SpellTimer2 <= diff)
                {
                    DoCastVictim(SpawnCast[2].SpellId);//Psychic Scream
                    SpellTimer2 = SpawnCast[2].Timer2 + (rand()%13 * 1000);
                } else SpellTimer2 -= diff;

                if (SpellTimer3 <= diff)
                {
                    DoCastVictim(SpawnCast[3].SpellId);//Mind Blast
                    SpellTimer3 = SpawnCast[3].Timer2 + (rand()%8 * 1000);
                } else SpellTimer3 -= diff;
            }
            //Illidari Highlord
            if (me->GetEntry() == 19797)
            {
                if (SpellTimer1 <= diff)
                {
                    DoCastVictim(SpawnCast[4].SpellId);//Curse Of Flames
                    SpellTimer1 = SpawnCast[4].Timer2 + (rand()%10 * 1000);
                } else SpellTimer1 -= diff;

                if (SpellTimer2 <= diff)
                {
                    DoCastVictim(SpawnCast[5].SpellId);//Flamestrike
                    SpellTimer2 = SpawnCast[5].Timer2 + (rand()%7 * 13000);
                } else SpellTimer2 -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

void npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI::SummonNextWave()
{
    uint8 count = WavesInfo[WaveCount].SpawnCount;
    uint8 locIndex = WavesInfo[WaveCount].UsedSpawnPoint;
    uint8 FelguardCount = 0;
    uint8 DreadlordCount = 0;

    for (uint8 i = 0; i < count; ++i)
    {
        Creature* Spawn = NULL;
        float X = SpawnLocation[locIndex + i].x;
        float Y = SpawnLocation[locIndex + i].y;
        float Z = SpawnLocation[locIndex + i].z;
        float O = SpawnLocation[locIndex + i].o;
        Spawn = me->SummonCreature(WavesInfo[WaveCount].CreatureId, X, Y, Z, O, TempSummonType::TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000);
        ++LiveCount;

        if (Spawn)
        {
            Spawn->LoadCreaturesAddon();

            if (WaveCount == 0)//1 Wave
            {
                if (rand()%3 == 1 && FelguardCount<2)
                {
                    Spawn->SetDisplayId(18654);
                    ++FelguardCount;
                }
                else if (DreadlordCount < 3)
                {
                    Spawn->SetDisplayId(19991);
                    ++DreadlordCount;
                }
                else if (FelguardCount<2)
                {
                    Spawn->SetDisplayId(18654);
                    ++FelguardCount;
                }
            }

            if (WaveCount < 3)//1-3 Wave
            {
                if (PlayerGUID)
                {
                    if (Player* target = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                    {
                        float x, y, z;
                        target->GetPosition(x, y, z);
                        Spawn->GetMotionMaster()->MovePoint(0, x, y, z);
                    }
                }
                CAST_AI(npc_illidari_spawn::npc_illidari_spawnAI, Spawn->AI())->LordIllidanGUID = me->GetGUID();
            }

            if (WavesInfo[WaveCount].CreatureId == 22076) // Torloth
            {
                CAST_AI(npc_torloth_the_magnificent::npc_torloth_the_magnificentAI, Spawn->AI())->LordIllidanGUID = me->GetGUID();
                if (PlayerGUID)
                    CAST_AI(npc_torloth_the_magnificent::npc_torloth_the_magnificentAI, Spawn->AI())->AggroTargetGUID = PlayerGUID;
            }
        }
    }
    ++WaveCount;
    WaveTimer = WavesInfo[WaveCount].SpawnTimer;
    AnnounceTimer = WavesInfo[WaveCount].YellTimer;
}

/*#####
# go_crystal_prison
######*/

class go_crystal_prison : public GameObjectScript
{
public:
    go_crystal_prison() : GameObjectScript("go_crystal_prison") { }

    bool OnQuestAccept(Player* player, GameObject* /*go*/, Quest const* quest) OVERRIDE
    {
        if (quest->GetQuestId() == QUEST_BATTLE_OF_THE_CRIMSON_WATCH)
        {
            Creature* Illidan = player->FindNearestCreature(22083, 50);

            if (Illidan && !CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, Illidan->AI())->EventStarted)
            {
                CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, Illidan->AI())->PlayerGUID = player->GetGUID();
                CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, Illidan->AI())->LiveCount = 0;
                CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, Illidan->AI())->EventStarted=true;
            }
        }
     return true;
    }
};

/*####
# npc_enraged_spirits
####*/

enum Enraged_Dpirits
{
// QUESTS
    QUEST_ENRAGED_SPIRITS_FIRE_EARTH        = 10458,
    QUEST_ENRAGED_SPIRITS_AIR               = 10481,
    QUEST_ENRAGED_SPIRITS_WATER             = 10480,

    // Totem
    ENTRY_TOTEM_OF_SPIRITS                  = 21071,
    RADIUS_TOTEM_OF_SPIRITS                 = 15,

    // SPIRITS
    NPC_ENRAGED_EARTH_SPIRIT                = 21050,
    NPC_ENRAGED_FIRE_SPIRIT                 = 21061,
    NPC_ENRAGED_AIR_SPIRIT                  = 21060,
    NPC_ENRAGED_WATER_SPIRIT                = 21059,

    // SOULS
    NPC_EARTHEN_SOUL                        = 21073,
    NPC_FIERY_SOUL                          = 21097,
    NPC_ENRAGED_AIRY_SOUL                   = 21116,
    NPC_ENRAGED_WATERY_SOUL                 = 21109, // wrong model

    // SPELL KILLCREDIT - not working!?! - using KilledMonsterCredit
    SPELL_EARTHEN_SOUL_CAPTURED_CREDIT      = 36108,
    SPELL_FIERY_SOUL_CAPTURED_CREDIT        = 36117,
    SPELL_AIRY_SOUL_CAPTURED_CREDIT         = 36182,
    SPELL_WATERY_SOUL_CAPTURED_CREDIT       = 36171,

    // KilledMonsterCredit Workaround
    NPC_CREDIT_FIRE                         = 21094,
    NPC_CREDIT_WATER                        = 21095,
    NPC_CREDIT_AIR                          = 21096,
    NPC_CREDIT_EARTH                        = 21092,

    // Captured Spell / Buff
    SPELL_SOUL_CAPTURED                     = 36115,

    // Factions
    FACTION_ENRAGED_SOUL_FRIENDLY           = 35,
    FACTION_ENRAGED_SOUL_HOSTILE            = 14
};

class npc_enraged_spirit : public CreatureScript
{
public:
    npc_enraged_spirit() : CreatureScript("npc_enraged_spirit") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
    return new npc_enraged_spiritAI(creature);
    }

    struct npc_enraged_spiritAI : public ScriptedAI
    {
        npc_enraged_spiritAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() OVERRIDE { }

        void EnterCombat(Unit* /*who*/)OVERRIDE { }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            // always spawn spirit on death
            // if totem around
            // move spirit to totem and cast kill count
            uint32 entry = 0;
            uint32 credit = 0;

            switch (me->GetEntry())
            {
                  case NPC_ENRAGED_FIRE_SPIRIT:
                    entry  = NPC_FIERY_SOUL;
                    //credit = SPELL_FIERY_SOUL_CAPTURED_CREDIT;
                    credit = NPC_CREDIT_FIRE;
                    break;
                  case NPC_ENRAGED_EARTH_SPIRIT:
                    entry  = NPC_EARTHEN_SOUL;
                    //credit = SPELL_EARTHEN_SOUL_CAPTURED_CREDIT;
                    credit = NPC_CREDIT_EARTH;
                    break;
                  case NPC_ENRAGED_AIR_SPIRIT:
                    entry  = NPC_ENRAGED_AIRY_SOUL;
                    //credit = SPELL_AIRY_SOUL_CAPTURED_CREDIT;
                    credit = NPC_CREDIT_AIR;
                    break;
                  case NPC_ENRAGED_WATER_SPIRIT:
                    entry  = NPC_ENRAGED_WATERY_SOUL;
                    //credit = SPELL_WATERY_SOUL_CAPTURED_CREDIT;
                    credit = NPC_CREDIT_WATER;
                    break;
                default:
                    break;
            }

            // Spawn Soul on Kill ALWAYS!
            Creature* Summoned = NULL;
            Unit* totemOspirits = NULL;

            if (entry != 0)
                Summoned = DoSpawnCreature(entry, 0, 0, 1, 0, TempSummonType::TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 5000);

            // FIND TOTEM, PROCESS QUEST
            if (Summoned)
            {
                 totemOspirits = me->FindNearestCreature(ENTRY_TOTEM_OF_SPIRITS, RADIUS_TOTEM_OF_SPIRITS);
                 if (totemOspirits)
                 {
                     Summoned->setFaction(FACTION_ENRAGED_SOUL_FRIENDLY);
                     Summoned->GetMotionMaster()->MovePoint(0, totemOspirits->GetPositionX(), totemOspirits->GetPositionY(), Summoned->GetPositionZ());

                     if (Unit* owner = totemOspirits->GetOwner())
                         if (Player* player = owner->ToPlayer())
                             player->KilledMonsterCredit(credit, 0);
                     DoCast(totemOspirits, SPELL_SOUL_CAPTURED);
                 }
            }
        }
    };
};

enum ZuluhedChains
{
    QUEST_ZULUHED   = 10866,
    NPC_KARYNAKU    = 22112,
};

class spell_unlocking_zuluheds_chains : public SpellScriptLoader
{
    public:
        spell_unlocking_zuluheds_chains() : SpellScriptLoader("spell_unlocking_zuluheds_chains") { }

        class spell_unlocking_zuluheds_chains_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_unlocking_zuluheds_chains_SpellScript);

            void HandleAfterHit()
            {
                if (GetCaster()->GetTypeId() == TypeID::TYPEID_PLAYER)
                    if (Creature* karynaku = GetCaster()->FindNearestCreature(NPC_KARYNAKU, 15.0f))
                        GetCaster()->ToPlayer()->KilledMonsterCredit(NPC_KARYNAKU, karynaku->GetGUID());
            }

            void Register() OVERRIDE
            {
                AfterHit += SpellHitFn(spell_unlocking_zuluheds_chains_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_unlocking_zuluheds_chains_SpellScript();
        }
};

enum ShadowMoonTuberEnum
{
    SPELL_WHISTLE               = 36652,
    SPELL_SHADOWMOON_TUBER      = 36462,

    NPC_BOAR_ENTRY              = 21195,
    GO_SHADOWMOON_TUBER_MOUND   = 184701,

    POINT_TUBER                 = 1,
    TYPE_BOAR                   = 1,
    DATA_BOAR                   = 1
};

class npc_shadowmoon_tuber_node : public CreatureScript
{
public:
    npc_shadowmoon_tuber_node() : CreatureScript("npc_shadowmoon_tuber_node") { }

    struct npc_shadowmoon_tuber_nodeAI : public ScriptedAI
    {
        npc_shadowmoon_tuber_nodeAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() OVERRIDE
        {
            tapped = false;
            tuberGUID = 0;
            resetTimer = 60000;
        }

        void SetData(uint32 id, uint32 data) OVERRIDE
        {
            if (id == TYPE_BOAR && data == DATA_BOAR)
            {
                // Spawn chest GO
                DoCast(SPELL_SHADOWMOON_TUBER);

                // Despawn the tuber
                if (GameObject* tuber = me->FindNearestGameObject(GO_SHADOWMOON_TUBER_MOUND, 5.0f))
                {
                    tuberGUID = tuber->GetGUID();
                    // @Workaround: find how to properly despawn the GO
                    tuber->SetPhaseMask(2, true);
                }
            }
        }

        void SpellHit(Unit* /*caster*/, const SpellInfo* spell) OVERRIDE
        {
            if (!tapped && spell->Id == SPELL_WHISTLE)
            {
                if (Creature* boar = me->FindNearestCreature(NPC_BOAR_ENTRY, 30.0f))
                {
                    // Disable trigger and force nearest boar to walk to him
                    tapped = true;
                    boar->SetWalk(false);
                    boar->GetMotionMaster()->MovePoint(POINT_TUBER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                }
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (tapped)
            {
                if (resetTimer <= diff)
                {
                    // Respawn the tuber
                    if (tuberGUID)
                        if (GameObject* tuber = GameObject::GetGameObject(*me, tuberGUID))
                        // @Workaround: find how to properly respawn the GO
                            tuber->SetPhaseMask(1, true);

                    Reset();
                }
                else
                    resetTimer -= diff;
            }
        }
    private:
        bool tapped;
        uint64 tuberGUID;
        uint32 resetTimer;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_shadowmoon_tuber_nodeAI(creature);
    }
};

void AddSC_shadowmoon_valley()
{
    new npc_mature_netherwing_drake();
    new npc_enslaved_netherwing_drake();
    new npc_dragonmaw_peon();
    new npc_drake_dealer_hurlunk();
    new npcs_flanis_swiftwing_and_kagrosh();
    new npc_murkblood_overseer();
    new npc_karynaku();
    new npc_oronok_tornheart();
    new npc_overlord_morghor();
    new npc_earthmender_wilda();
    new npc_lord_illidan_stormrage();
    new go_crystal_prison();
    new npc_illidari_spawn();
    new npc_torloth_the_magnificent();
    new npc_enraged_spirit();
    new spell_unlocking_zuluheds_chains();
    new npc_shadowmoon_tuber_node();
}
