/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "ScriptedEscortAI.h"
#include "CombatAI.h"
#include "PassiveAI.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "CreatureTextMgr.h"

/*######
##Quest 12848
######*/

#define GCD_CAST    1

enum UnworthyInitiate
{
    SPELL_SOUL_PRISON_CHAIN_SELF    = 54612,
    SPELL_SOUL_PRISON_CHAIN         = 54613,
    SPELL_DK_INITIATE_VISUAL        = 51519,

    SPELL_ICY_TOUCH                 = 52372,
    SPELL_PLAGUE_STRIKE             = 52373,
    SPELL_BLOOD_STRIKE              = 52374,
    SPELL_DEATH_COIL                = 52375,

    SAY_EVENT_START                 = 0,
    SAY_EVENT_ATTACK                = 1,

    EVENT_ICY_TOUCH                 = 1,
    EVENT_PLAGUE_STRIKE             = 2,
    EVENT_BLOOD_STRIKE              = 3,
    EVENT_DEATH_COIL                = 4
};

enum UnworthyInitiatePhase
{
    PHASE_CHAINED,
    PHASE_TO_EQUIP,
    PHASE_EQUIPING,
    PHASE_TO_ATTACK,
    PHASE_ATTACKING,
};

uint32 acherus_soul_prison[12] =
{
    191577,
    191580,
    191581,
    191582,
    191583,
    191584,
    191585,
    191586,
    191587,
    191588,
    191589,
    191590
};

uint32 acherus_unworthy_initiate[5] =
{
    29519,
    29520,
    29565,
    29566,
    29567
};

class npc_unworthy_initiate : public CreatureScript
{
public:
    npc_unworthy_initiate() : CreatureScript("npc_unworthy_initiate") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_unworthy_initiateAI(creature);
    }

    struct npc_unworthy_initiateAI : public ScriptedAI
    {
        npc_unworthy_initiateAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            if (!me->GetCurrentEquipmentId())
                me->SetCurrentEquipmentId(me->GetOriginalEquipmentId());
        }

        uint64 playerGUID;
        UnworthyInitiatePhase phase;
        uint32 wait_timer;
        float anchorX, anchorY;
        uint64 anchorGUID;

        EventMap events;

        void Reset() OVERRIDE
        {
            anchorGUID = 0;
            phase = PHASE_CHAINED;
            events.Reset();
            me->setFaction(7);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->SetUInt32Value(UNIT_FIELD_ANIM_TIER, 8);
            me->LoadEquipment(0, true);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            events.ScheduleEvent(EVENT_ICY_TOUCH, 1000, GCD_CAST);
            events.ScheduleEvent(EVENT_PLAGUE_STRIKE, 3000, GCD_CAST);
            events.ScheduleEvent(EVENT_BLOOD_STRIKE, 2000, GCD_CAST);
            events.ScheduleEvent(EVENT_DEATH_COIL, 5000, GCD_CAST);
        }

        void MovementInform(uint32 type, uint32 id) OVERRIDE
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 1)
            {
                wait_timer = 5000;
                me->CastSpell(me, SPELL_DK_INITIATE_VISUAL, true);

                if (Player* starter = ObjectAccessor::GetPlayer(*me, playerGUID))
                    sCreatureTextMgr->SendChat(me, SAY_EVENT_ATTACK, 0, ChatMsg::CHAT_MSG_ADDON, Language::LANG_ADDON, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, starter);

                phase = PHASE_TO_ATTACK;
            }
        }

        void EventStart(Creature* anchor, Player* target)
        {
            wait_timer = 5000;
            phase = PHASE_TO_EQUIP;

            me->SetUInt32Value(UNIT_FIELD_ANIM_TIER, 0);
            me->RemoveAurasDueToSpell(SPELL_SOUL_PRISON_CHAIN_SELF);
            me->RemoveAurasDueToSpell(SPELL_SOUL_PRISON_CHAIN);

            float z;
            anchor->GetContactPoint(me, anchorX, anchorY, z, 1.0f);

            playerGUID = target->GetGUID();
            Talk(SAY_EVENT_START);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            switch (phase)
            {
            case PHASE_CHAINED:
                if (!anchorGUID)
                {
                    if (Creature* anchor = me->FindNearestCreature(29521, 30))
                    {
                        anchor->AI()->SetGUID(me->GetGUID());
                        anchor->CastSpell(me, SPELL_SOUL_PRISON_CHAIN, true);
                        anchorGUID = anchor->GetGUID();
                    }
                    else
                        SF_LOG_ERROR("scripts", "npc_unworthy_initiateAI: unable to find anchor!");

                    float dist = 99.0f;
                    GameObject* prison = NULL;

                    for (uint8 i = 0; i < 12; ++i)
                    {
                        if (GameObject* temp_prison = me->FindNearestGameObject(acherus_soul_prison[i], 30))
                        {
                            if (me->IsWithinDist(temp_prison, dist, false))
                            {
                                dist = me->GetDistance2d(temp_prison);
                                prison = temp_prison;
                            }
                        }
                    }

                    if (prison)
                        prison->ResetDoorOrButton();
                    else
                        SF_LOG_ERROR("scripts", "npc_unworthy_initiateAI: unable to find prison!");
                }
                break;
            case PHASE_TO_EQUIP:
                if (wait_timer)
                {
                    if (wait_timer > diff)
                        wait_timer -= diff;
                    else
                    {
                        me->GetMotionMaster()->MovePoint(1, anchorX, anchorY, me->GetPositionZ());
                        //SF_LOG_DEBUG("scripts", "npc_unworthy_initiateAI: move to %f %f %f", anchorX, anchorY, me->GetPositionZ());
                        phase = PHASE_EQUIPING;
                        wait_timer = 0;
                    }
                }
                break;
            case PHASE_TO_ATTACK:
                if (wait_timer)
                {
                    if (wait_timer > diff)
                        wait_timer -= diff;
                    else
                    {
                        me->setFaction(14);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                        phase = PHASE_ATTACKING;

                        if (Player* target = ObjectAccessor::GetPlayer(*me, playerGUID))
                            AttackStart(target);
                        wait_timer = 0;
                    }
                }
                break;
            case PHASE_ATTACKING:
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_ICY_TOUCH:
                        DoCastVictim(SPELL_ICY_TOUCH);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_ICY_TOUCH, 5000, GCD_CAST);
                        break;
                    case EVENT_PLAGUE_STRIKE:
                        DoCastVictim(SPELL_PLAGUE_STRIKE);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_PLAGUE_STRIKE, 5000, GCD_CAST);
                        break;
                    case EVENT_BLOOD_STRIKE:
                        DoCastVictim(SPELL_BLOOD_STRIKE);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_BLOOD_STRIKE, 5000, GCD_CAST);
                        break;
                    case EVENT_DEATH_COIL:
                        DoCastVictim(SPELL_DEATH_COIL);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_DEATH_COIL, 5000, GCD_CAST);
                        break;
                    }
                }

                DoMeleeAttackIfReady();
                break;
            default:
                break;
            }
        }
    };
};

class npc_unworthy_initiate_anchor : public CreatureScript
{
public:
    npc_unworthy_initiate_anchor() : CreatureScript("npc_unworthy_initiate_anchor") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_unworthy_initiate_anchorAI(creature);
    }

    struct npc_unworthy_initiate_anchorAI : public PassiveAI
    {
        npc_unworthy_initiate_anchorAI(Creature* creature) : PassiveAI(creature), prisonerGUID(0) { }

        uint64 prisonerGUID;

        void SetGUID(uint64 guid, int32 /*id*/) OVERRIDE
        {
            if (!prisonerGUID)
                prisonerGUID = guid;
        }

        uint64 GetGUID(int32 /*id*/) const OVERRIDE
        {
            return prisonerGUID;
        }
    };
};

class go_acherus_soul_prison : public GameObjectScript
{
public:
    go_acherus_soul_prison() : GameObjectScript("go_acherus_soul_prison") { }

    bool OnGossipHello(Player* player, GameObject* go) OVERRIDE
    {
        if (Creature* anchor = go->FindNearestCreature(29521, 15))
            if (uint64 prisonerGUID = anchor->AI()->GetGUID())
                if (Creature* prisoner = Creature::GetCreature(*player, prisonerGUID))
                    CAST_AI(npc_unworthy_initiate::npc_unworthy_initiateAI, prisoner->AI())->EventStart(anchor, player);

        return false;
    }
};

/*######
## npc_death_knight_initiate
######*/

#define GOSSIP_ACCEPT_DUEL      "I challenge you, death knight!"

enum Spells_DKI
{
    SPELL_DUEL                  = 52996,
    //SPELL_DUEL_TRIGGERED        = 52990,
    SPELL_DUEL_VICTORY          = 52994,
    SPELL_DUEL_FLAG             = 52991,
};

enum Says_VBM
{
    SAY_DUEL                    = 0,
};

enum Misc_VBN
{
    QUEST_DEATH_CHALLENGE       = 12733,
    FACTION_HOSTILE             = 2068
};

class npc_death_knight_initiate : public CreatureScript
{
public:
    npc_death_knight_initiate() : CreatureScript("npc_death_knight_initiate") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF)
        {
            player->CLOSE_GOSSIP_MENU();

            if (player->IsInCombat() || creature->IsInCombat())
                return true;

            if (npc_death_knight_initiateAI* pInitiateAI = CAST_AI(npc_death_knight_initiate::npc_death_knight_initiateAI, creature->AI()))
            {
                if (pInitiateAI->m_bIsDuelInProgress)
                    return true;
            }

            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_15);

            sCreatureTextMgr->SendChat(creature, SAY_DUEL, 0, ChatMsg::CHAT_MSG_ADDON, Language::LANG_ADDON, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);

            player->CastSpell(creature, SPELL_DUEL, false);
            player->CastSpell(player, SPELL_DUEL_FLAG, true);
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        if (player->GetQuestStatus(QUEST_DEATH_CHALLENGE) == QUEST_STATUS_INCOMPLETE && creature->IsFullHealth())
        {
            if (player->HealthBelowPct(10))
                return true;

            if (player->IsInCombat() || creature->IsInCombat())
                return true;

            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ACCEPT_DUEL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_death_knight_initiateAI(creature);
    }

    struct npc_death_knight_initiateAI : public CombatAI
    {
        npc_death_knight_initiateAI(Creature* creature) : CombatAI(creature)
        {
            m_bIsDuelInProgress = false;
        }

        bool lose;
        uint64 m_uiDuelerGUID;
        uint32 m_uiDuelTimer;
        bool m_bIsDuelInProgress;

        void Reset() OVERRIDE
        {
            lose = false;
            me->RestoreFaction();
            CombatAI::Reset();

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_15);

            m_uiDuelerGUID = 0;
            m_uiDuelTimer = 5000;
            m_bIsDuelInProgress = false;
        }

        void SpellHit(Unit* pCaster, const SpellInfo* pSpell) OVERRIDE
        {
            if (!m_bIsDuelInProgress && pSpell->Id == SPELL_DUEL)
            {
                m_uiDuelerGUID = pCaster->GetGUID();
                m_bIsDuelInProgress = true;
            }
        }

       void DamageTaken(Unit* pDoneBy, uint32 &uiDamage) OVERRIDE
        {
            if (m_bIsDuelInProgress && pDoneBy->IsControlledByPlayer())
            {
                if (pDoneBy->GetGUID() != m_uiDuelerGUID && pDoneBy->GetOwnerGUID() != m_uiDuelerGUID) // other players cannot help
                    uiDamage = 0;
                else if (uiDamage >= me->GetHealth())
                {
                    uiDamage = 0;

                    if (!lose)
                    {
                        pDoneBy->RemoveGameObject(SPELL_DUEL_FLAG, true);
                        pDoneBy->AttackStop();
                        me->CastSpell(pDoneBy, SPELL_DUEL_VICTORY, true);
                        lose = true;
                        me->CastSpell(me, 7267, true);
                        me->RestoreFaction();
                    }
                }
            }
        }

        void UpdateAI(uint32 uiDiff) OVERRIDE
        {
            if (!UpdateVictim())
            {
                if (m_bIsDuelInProgress)
                {
                    if (m_uiDuelTimer <= uiDiff)
                    {
                        me->setFaction(FACTION_HOSTILE);

                        if (Unit* unit = Unit::GetUnit(*me, m_uiDuelerGUID))
                            AttackStart(unit);
                    }
                    else
                        m_uiDuelTimer -= uiDiff;
                }
                return;
            }

            if (m_bIsDuelInProgress)
            {
                if (lose)
                {
                    if (!me->HasAura(7267))
                        EnterEvadeMode();
                    return;
                }
                else if (me->GetVictim()->GetTypeId() == TypeID::TYPEID_PLAYER && me->GetVictim()->HealthBelowPct(10))
                {
                    me->GetVictim()->CastSpell(me->GetVictim(), 7267, true); // beg
                    me->GetVictim()->RemoveGameObject(SPELL_DUEL_FLAG, true);
                    EnterEvadeMode();
                    return;
                }
            }

            /// @todo spells

            CombatAI::UpdateAI(uiDiff);
        }
    };
};

/*######
## npc_dark_rider_of_acherus
######*/

enum DarkRiderOfAcherus
{
    SAY_DARK_RIDER              = 0,
    SPELL_DESPAWN_HORSE         = 51918
};

class npc_dark_rider_of_acherus : public CreatureScript
{
    public:
        npc_dark_rider_of_acherus() : CreatureScript("npc_dark_rider_of_acherus") { }

        struct npc_dark_rider_of_acherusAI : public ScriptedAI
        {
            npc_dark_rider_of_acherusAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() OVERRIDE
            {
                PhaseTimer = 4000;
                Phase = 0;
                Intro = false;
                TargetGUID = 0;
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!Intro || !TargetGUID)
                    return;

                if (PhaseTimer <= diff)
                {
                    switch (Phase)
                    {
                       case 0:
                            Talk(SAY_DARK_RIDER);
                            PhaseTimer = 5000;
                            Phase = 1;
                            break;
                        case 1:
                            if (Unit* target = ObjectAccessor::GetUnit(*me, TargetGUID))
                                DoCast(target, SPELL_DESPAWN_HORSE, true);
                            PhaseTimer = 3000;
                            Phase = 2;
                            break;
                        case 2:
                            me->SetVisible(false);
                            PhaseTimer = 2000;
                            Phase = 3;
                            break;
                        case 3:
                            me->DespawnOrUnsummon();
                            break;
                        default:
                            break;
                    }
                }
                else
                    PhaseTimer -= diff;
            }

            void InitDespawnHorse(Unit* who)
            {
                if (!who)
                    return;

                TargetGUID = who->GetGUID();
                me->SetWalk(true);
                me->SetSpeed(MOVE_RUN, 0.4f);
                me->GetMotionMaster()->MoveChase(who);
                me->SetTarget(TargetGUID);
                Intro = true;
            }

        private:
            uint32 PhaseTimer;
            uint32 Phase;
            bool Intro;
            uint64 TargetGUID;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_dark_rider_of_acherusAI(creature);
        }
};

/*######
## npc_salanar_the_horseman
######*/

enum Spells_Salanar
{
    SPELL_REALM_OF_SHADOWS            = 52693,
    SPELL_EFFECT_STOLEN_HORSE         = 52263,
    SPELL_DELIVER_STOLEN_HORSE        = 52264,
    SPELL_CALL_DARK_RIDER             = 52266,
    SPELL_EFFECT_OVERTAKE             = 52349
};

class npc_salanar_the_horseman : public CreatureScript
{
public:
    npc_salanar_the_horseman() : CreatureScript("npc_salanar_the_horseman") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_salanar_the_horsemanAI(creature);
    }

    struct npc_salanar_the_horsemanAI : public ScriptedAI
    {
        npc_salanar_the_horsemanAI(Creature* creature) : ScriptedAI(creature) { }

        void SpellHit(Unit* caster, const SpellInfo* spell) OVERRIDE
        {
            if (spell->Id == SPELL_DELIVER_STOLEN_HORSE)
            {
                if (caster->GetTypeId() == TypeID::TYPEID_UNIT && caster->IsVehicle())
                {
                    if (Unit* charmer = caster->GetCharmer())
                    {
                        if (charmer->HasAura(SPELL_EFFECT_STOLEN_HORSE))
                        {
                            charmer->RemoveAurasDueToSpell(SPELL_EFFECT_STOLEN_HORSE);
                            caster->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                            caster->setFaction(35);
                            DoCast(caster, SPELL_CALL_DARK_RIDER, true);
                            if (Creature* Dark_Rider = me->FindNearestCreature(28654, 15))
                                CAST_AI(npc_dark_rider_of_acherus::npc_dark_rider_of_acherusAI, Dark_Rider->AI())->InitDespawnHorse(caster);
                        }
                    }
                }
            }
        }

        void MoveInLineOfSight(Unit* who) OVERRIDE
        {
            ScriptedAI::MoveInLineOfSight(who);

            if (who->GetTypeId() == TypeID::TYPEID_UNIT && who->IsVehicle() && me->IsWithinDistInMap(who, 5.0f))
            {
                if (Unit* charmer = who->GetCharmer())
                {
                    if (Player* player = charmer->ToPlayer())
                    {
                        // for quest Into the Realm of Shadows(12687)
                        if (me->GetEntry() == 28788 && player->GetQuestStatus(12687) == QUEST_STATUS_INCOMPLETE)
                        {
                            player->GroupEventHappens(12687, me);
                            charmer->RemoveAurasDueToSpell(SPELL_EFFECT_OVERTAKE);
                            if (Creature* creature = who->ToCreature())
                            {
                                creature->DespawnOrUnsummon();
                                //creature->Respawn(true);
                            }
                        }

                        if (player->HasAura(SPELL_REALM_OF_SHADOWS))
                            player->RemoveAurasDueToSpell(SPELL_REALM_OF_SHADOWS);
                    }
                }
            }
        }
    };
};

/*######
## npc_ros_dark_rider
######*/

class npc_ros_dark_rider : public CreatureScript
{
public:
    npc_ros_dark_rider() : CreatureScript("npc_ros_dark_rider") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_ros_dark_riderAI(creature);
    }

    struct npc_ros_dark_riderAI : public ScriptedAI
    {
        npc_ros_dark_riderAI(Creature* creature) : ScriptedAI(creature) { }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            me->ExitVehicle();
        }

        void Reset() OVERRIDE
        {
            Creature* deathcharger = me->FindNearestCreature(28782, 30);
            if (!deathcharger)
                return;

            deathcharger->RestoreFaction();
            deathcharger->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            deathcharger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            if (!me->GetVehicle() && deathcharger->IsVehicle() && deathcharger->GetVehicleKit()->HasEmptySeat(0))
                me->EnterVehicle(deathcharger);
        }

        void JustDied(Unit* killer) OVERRIDE
        {
            Creature* deathcharger = me->FindNearestCreature(28782, 30);
            if (!deathcharger)
                return;

            if (killer->GetTypeId() == TypeID::TYPEID_PLAYER && deathcharger->GetTypeId() == TypeID::TYPEID_UNIT && deathcharger->IsVehicle())
            {
                deathcharger->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                deathcharger->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                deathcharger->setFaction(2096);
            }
        }
    };
};

// correct way: 52312 52314 52555 ...
enum Creatures_SG
{
    NPC_GHOULS = 28845,
    NPC_GHOSTS = 28846,
};
class npc_dkc1_gothik : public CreatureScript
{
public:
    npc_dkc1_gothik() : CreatureScript("npc_dkc1_gothik") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_dkc1_gothikAI(creature);
    }

    struct npc_dkc1_gothikAI : public ScriptedAI
    {
        npc_dkc1_gothikAI(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) OVERRIDE

        {
            ScriptedAI::MoveInLineOfSight(who);

            if (who->GetEntry() == NPC_GHOULS && me->IsWithinDistInMap(who, 10.0f))
            {
                if (Unit* owner = who->GetOwner())
                {
                    if (Player* player = owner->ToPlayer())
                    {
                        Creature* creature = who->ToCreature();
                        if (player->GetQuestStatus(12698) == QUEST_STATUS_INCOMPLETE)
                            creature->CastSpell(owner, 52517, true);

                        /// @todo Creatures must not be removed, but, must instead
                        //      stand next to Gothik and be commanded into the pit
                        //      and dig into the ground.
                        creature->DespawnOrUnsummon();

                        if (player->GetQuestStatus(12698) == QUEST_STATUS_COMPLETE)
                            owner->RemoveAllMinionsByEntry(NPC_GHOSTS);
                    }
                }
            }
        }
    };
};

class npc_scarlet_ghoul : public CreatureScript
{
public:
    npc_scarlet_ghoul() : CreatureScript("npc_scarlet_ghoul") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_scarlet_ghoulAI(creature);
    }

    struct npc_scarlet_ghoulAI : public ScriptedAI
    {
        npc_scarlet_ghoulAI(Creature* creature) : ScriptedAI(creature)
        {
            // Ghouls should display their Birth Animation
            // Crawling out of the ground
            //DoCast(me, 35177, true);
            //me->MonsterSay("Mommy?", LANG_UNIVERSAL, 0);
            me->SetReactState(REACT_DEFENSIVE);
        }

        void FindMinions(Unit* owner)
        {
            std::list<Creature*> MinionList;
            owner->GetAllMinionsByEntry(MinionList, NPC_GHOULS);

            if (!MinionList.empty())
            {
                for (std::list<Creature*>::const_iterator itr = MinionList.begin(); itr != MinionList.end(); ++itr)
                {
                    if ((*itr)->GetOwner()->GetGUID() == me->GetOwner()->GetGUID())
                    {
                        if ((*itr)->IsInCombat() && (*itr)->getAttackerForHelper())
                        {
                            AttackStart((*itr)->getAttackerForHelper());
                        }
                    }
                }
            }
        }

        void UpdateAI(uint32 /*diff*/) OVERRIDE
        {
            if (!me->IsInCombat())
            {
                if (Unit* owner = me->GetOwner())
                {
                    Player* plrOwner = owner->ToPlayer();
                    if (plrOwner && plrOwner->IsInCombat())
                    {
                        if (plrOwner->getAttackerForHelper() && plrOwner->getAttackerForHelper()->GetEntry() == NPC_GHOSTS)
                            AttackStart(plrOwner->getAttackerForHelper());
                        else
                            FindMinions(owner);
                    }
                }
            }

            if (!UpdateVictim())
                return;

            //ScriptedAI::UpdateAI(diff);
            //Check if we have a current target
            if (me->GetVictim()->GetEntry() == NPC_GHOSTS)
            {
                if (me->isAttackReady())
                {
                    //If we are within range melee the target
                    if (me->IsWithinMeleeRange(me->GetVictim()))
                    {
                        me->AttackerStateUpdate(me->GetVictim());
                        me->resetAttackTimer();
                    }
                }
            }
        }
    };
};

/*####
## npc_scarlet_miner_cart
####*/

enum ScarletMinerCart
{
    SPELL_CART_CHECK        = 54173,
    SPELL_SUMMON_CART       = 52463,
    SPELL_SUMMON_MINER      = 52464,
    SPELL_CART_DRAG         = 52465,

    NPC_MINER               = 28841
};

class npc_scarlet_miner_cart : public CreatureScript
{
    public:
        npc_scarlet_miner_cart() : CreatureScript("npc_scarlet_miner_cart") { }

        struct npc_scarlet_miner_cartAI : public PassiveAI
        {
            npc_scarlet_miner_cartAI(Creature* creature) : PassiveAI(creature), _minerGUID(0), _playerGUID(0)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid1); // Modelid2 is a horse.
            }

            void JustSummoned(Creature* summon) OVERRIDE
            {
                if (summon->GetEntry() == NPC_MINER)
                {
                    _minerGUID = summon->GetGUID();
                    summon->AI()->SetGUID(_playerGUID);
                }
            }

            void SummonedCreatureDespawn(Creature* summon) OVERRIDE
            {
                if (summon->GetEntry() == NPC_MINER)
                    _minerGUID = 0;
            }

            void DoAction(int32 /*param*/) OVERRIDE
            {
                if (Creature* miner = ObjectAccessor::GetCreature(*me, _minerGUID))
                {
                    me->SetWalk(false);

                    // Not 100% correct, but movement is smooth. Sometimes miner walks faster
                    // than normal, this speed is fast enough to keep up at those times.
                    me->SetSpeed(MOVE_RUN, 1.25f);

                    me->GetMotionMaster()->MoveFollow(miner, 1.0f, 0);
                }
            }

            void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply) OVERRIDE
            {
                if (apply)
                {
                    _playerGUID = who->GetGUID();
                    me->CastSpell((Unit*)NULL, SPELL_SUMMON_MINER, true);
                }
                else
                {
                    _playerGUID = 0;
                    if (Creature* miner = ObjectAccessor::GetCreature(*me, _minerGUID))
                        miner->DespawnOrUnsummon();
                }
            }

        private:
            uint64 _minerGUID;
            uint64 _playerGUID;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_scarlet_miner_cartAI(creature);
        }
};

/*####
## npc_scarlet_miner
####*/

enum Says_SM
{
    SAY_SCARLET_MINER_0         = 0,
    SAY_SCARLET_MINER_1         = 1
};

class npc_scarlet_miner : public CreatureScript
{
    public:
        npc_scarlet_miner() : CreatureScript("npc_scarlet_miner") { }

        struct npc_scarlet_minerAI : public npc_escortAI
        {
            npc_scarlet_minerAI(Creature* creature) : npc_escortAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            uint32 IntroTimer;
            uint32 IntroPhase;
            uint64 carGUID;

            void Reset() OVERRIDE
            {
                carGUID = 0;
                IntroTimer = 0;
                IntroPhase = 0;
            }

            void IsSummonedBy(Unit* summoner) OVERRIDE
            {
                carGUID = summoner->GetGUID();
            }

            void InitWaypoint()
            {
                AddWaypoint(1, 2389.03f,     -5902.74f,     109.014f, 5000);
                AddWaypoint(2, 2341.812012f, -5900.484863f, 102.619743f);
                AddWaypoint(3, 2306.561279f, -5901.738281f, 91.792419f);
                AddWaypoint(4, 2300.098389f, -5912.618652f, 86.014885f);
                AddWaypoint(5, 2294.142090f, -5927.274414f, 75.316849f);
                AddWaypoint(6, 2286.984375f, -5944.955566f, 63.714966f);
                AddWaypoint(7, 2280.001709f, -5961.186035f, 54.228283f);
                AddWaypoint(8, 2259.389648f, -5974.197754f, 42.359348f);
                AddWaypoint(9, 2242.882812f, -5984.642578f, 32.827850f);
                AddWaypoint(10, 2217.265625f, -6028.959473f, 7.675705f);
                AddWaypoint(11, 2202.595947f, -6061.325684f, 5.882018f);
                AddWaypoint(12, 2188.974609f, -6080.866699f, 3.370027f);

                if (std::rand() % 1)
                {
                    AddWaypoint(13, 2176.483887f, -6110.407227f, 1.855181f);
                    AddWaypoint(14, 2172.516602f, -6146.752441f, 1.074235f);
                    AddWaypoint(15, 2138.918457f, -6158.920898f, 1.342926f);
                    AddWaypoint(16, 2129.866699f, -6174.107910f, 4.380779f);
                    AddWaypoint(17, 2117.709473f, -6193.830078f, 13.3542f, 10000);
                }
                else
                {
                    AddWaypoint(13, 2184.190186f, -6166.447266f, 0.968877f);
                    AddWaypoint(14, 2234.265625f, -6163.741211f, 0.916021f);
                    AddWaypoint(15, 2268.071777f, -6158.750977f, 1.822252f);
                    AddWaypoint(16, 2270.028320f, -6176.505859f, 6.340538f);
                    AddWaypoint(17, 2271.739014f, -6195.401855f, 13.3542f, 10000);
                }
            }

            void SetGUID(uint64 guid, int32 /*id = 0*/) OVERRIDE
            {
                InitWaypoint();
                Start(false, false, guid);
                SetDespawnAtFar(false);
            }

            void WaypointReached(uint32 waypointId) OVERRIDE
            {
                switch (waypointId)
                {
                    case 1:
                        if (Unit* car = ObjectAccessor::GetCreature(*me, carGUID))
                            me->SetFacingToObject(car);
                        Talk(SAY_SCARLET_MINER_0);
                        SetRun(true);
                        IntroTimer = 4000;
                        IntroPhase = 1;
                        break;
                    case 17:
                        if (Unit* car = ObjectAccessor::GetCreature(*me, carGUID))
                        {
                            me->SetFacingToObject(car);
                            car->Relocate(car->GetPositionX(), car->GetPositionY(), me->GetPositionZ() + 1);
                            car->StopMoving();
                            car->RemoveAura(SPELL_CART_DRAG);
                        }
                        Talk(SAY_SCARLET_MINER_1);
                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (IntroPhase)
                {
                    if (IntroTimer <= diff)
                    {
                        if (IntroPhase == 1)
                        {
                            if (Creature* car = Unit::GetCreature(*me, carGUID))
                                DoCast(car, SPELL_CART_DRAG);
                            IntroTimer = 800;
                            IntroPhase = 2;
                        }
                        else
                        {
                            if (Creature* car = Unit::GetCreature(*me, carGUID))
                                car->AI()->DoAction(0);
                            IntroPhase = 0;
                        }
                    }
                    else
                        IntroTimer -= diff;
                }
                npc_escortAI::UpdateAI(diff);
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_scarlet_minerAI(creature);
        }
};

enum eyeOfAcherus
{
    EVENT_REMOVE_CONTROL = 1,
    EVENT_SPEAK_1 = 2,
    EVENT_LAUNCH = 3,
    EVENT_REGAIN_CONTROL = 4,

    EYE_TEXT_LAUNCH = 0,
    EYE_TEXT_CONTROL = 1,

    EYE_POINT_DESTINATION_1 = 0,
    EYE_POINT_DESTINATION_2 = 1,

    SPELL_THE_EYE_OF_ACHERUS = 51852,
    SPELL_EYE_VISUAL = 51892,
    SPELL_EYE_FLIGHT_BOOST = 51923,
    SPELL_EYE_FLIGHT = 51890,
};

class npc_eye_of_acherus : public CreatureScript
{
public:
    npc_eye_of_acherus() : CreatureScript("npc_eye_of_acherus") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_eye_of_acherusAI(creature);
    }

    struct npc_eye_of_acherusAI : public NullCreatureAI
    {
        npc_eye_of_acherusAI(Creature* creature) : NullCreatureAI(creature)
        {
            creature->SetDisplayId(creature->GetCreatureTemplate()->Modelid1);

            if (Player* owner = me->GetCharmerOrOwner()->ToPlayer())
                me->GetCharmInfo()->InitPossessCreateSpells();

            creature->SetReactState(REACT_PASSIVE);
        }

        EventMap events;

        void InitializeAI() OVERRIDE
        {
            events.Reset();
            events.ScheduleEvent(EVENT_REMOVE_CONTROL, 500);
            events.ScheduleEvent(EVENT_SPEAK_1, 4000);
            events.ScheduleEvent(EVENT_LAUNCH, 7000);

            DoCast(SPELL_EYE_VISUAL);
            DoCast(SPELL_EYE_FLIGHT);
        }

        void OnCharmed(bool apply) OVERRIDE
        {
            if (!apply)
            {
                me->GetCharmerOrOwner()->RemoveAurasDueToSpell(SPELL_THE_EYE_OF_ACHERUS);
                me->GetCharmerOrOwner()->RemoveAurasDueToSpell(SPELL_EYE_FLIGHT_BOOST);
            }
        }

        void MovementInform(uint32 type, uint32 point) OVERRIDE
        {
            if (type == POINT_MOTION_TYPE && point == EYE_POINT_DESTINATION_2)
                events.ScheduleEvent(EVENT_REGAIN_CONTROL, 1000);
        }

        void JustSummoned(Creature* creature) OVERRIDE
        {
            if (Unit* target = creature->SelectNearbyTarget())
                creature->AI()->AttackStart(target);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            events.Update(diff);
            switch (events.ExecuteEvent())
            {
            case EVENT_REMOVE_CONTROL:
                DoCast(SPELL_EYE_FLIGHT_BOOST);
                if (Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                {
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                    player->SetClientControl(me, 0);
                }
                break;
            case EVENT_SPEAK_1:
                Talk(EYE_TEXT_LAUNCH, me->GetCharmerOrOwner());
                break;
            case EVENT_LAUNCH:
            {
                me->SetSpeed(MOVE_FLIGHT, 5.0f, true);

                const Position EYE_DESTINATION_1 = { me->GetPositionX() - 40.0f, me->GetPositionY(), me->GetPositionZ() + 10.0f, 0.0f };
                const Position EYE_DESTINATION_2 = { 1768.0f, -5876.0f, 153.0f, 0.0f };

                me->GetMotionMaster()->MovePoint(EYE_POINT_DESTINATION_1, EYE_DESTINATION_1);
                me->GetMotionMaster()->MovePoint(EYE_POINT_DESTINATION_2, EYE_DESTINATION_2);
                break;
            }
            case EVENT_REGAIN_CONTROL:
                if (Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                    me->SetSpeed(MOVE_FLIGHT, 3.3f, true);

                    me->RemoveAurasDueToSpell(SPELL_EYE_FLIGHT_BOOST);
                    player->SetClientControl(me, 1);
                    Talk(EYE_TEXT_CONTROL, player);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE);
                    me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
                }
                break;
            }
        }
    };
};

class spell_q12641_death_comes_from_on_high_summon_ghouls : public SpellScriptLoader
{
public:
    spell_q12641_death_comes_from_on_high_summon_ghouls() : SpellScriptLoader("spell_q12641_death_comes_from_on_high_summon_ghouls") { }

    class spell_q12641_death_comes_from_on_high_summon_ghouls_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q12641_death_comes_from_on_high_summon_ghouls_SpellScript);

        void HandleScriptEffect(SpellEffIndex effIndex)
        {
            PreventHitEffect(effIndex);
            if (Unit* target = GetHitUnit())
                GetCaster()->CastSpell(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 54522, true);
        }

        void Register() OVERRIDE
        {
            OnEffectHitTarget += SpellEffectFn(spell_q12641_death_comes_from_on_high_summon_ghouls_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_q12641_death_comes_from_on_high_summon_ghouls_SpellScript();
    }
};

// npc 28912 quest 17217 boss 29001 mob 29007 go 191092

void AddSC_the_scarlet_enclave_c1()
{
    new npc_unworthy_initiate();
    new npc_unworthy_initiate_anchor();
    new go_acherus_soul_prison();
    new npc_death_knight_initiate();
    new npc_salanar_the_horseman();
    new npc_dark_rider_of_acherus();
    new npc_ros_dark_rider();
    new npc_dkc1_gothik();
    new npc_scarlet_ghoul();
    new npc_scarlet_miner();
    new npc_scarlet_miner_cart();
    new npc_eye_of_acherus();
    new spell_q12641_death_comes_from_on_high_summon_ghouls();
}
