/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* Script Data Start
SDName: Boss ymiron
SDAuthor: LordVanMartin
SD%Complete:
SDComment:
SDCategory:
Script Data End */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "utgarde_pinnacle.h"
#include "SpellInfo.h"

enum Spells
{
    SPELL_BANE                                = 48294,
    H_SPELL_BANE                              = 59301,
    SPELL_DARK_SLASH                          = 48292,
    SPELL_FETID_ROT                           = 48291,
    H_SPELL_FETID_ROT                         = 59300,
    SPELL_SCREAMS_OF_THE_DEAD                 = 51750,
    SPELL_SPIRIT_BURST                        = 48529,
    H_SPELL_SPIRIT_BURST                      = 59305,
    SPELL_SPIRIT_STRIKE                       = 48423,
    H_SPELL_SPIRIT_STRIKE                     = 59304,
    SPELL_ANCESTORS_VENGEANCE                 = 16939,

    SPELL_SUMMON_AVENGING_SPIRIT              = 48592,
    SPELL_SUMMON_SPIRIT_FOUNT                 = 48386,

    SPELL_CHANNEL_SPIRIT_TO_YMIRON            = 48316,
    SPELL_CHANNEL_YMIRON_TO_SPIRIT            = 48307,

    SPELL_SPIRIT_FOUNT                        = 48380,
    H_SPELL_SPIRIT_FOUNT                      = 59320
};

//not in db
enum Yells
{
    SAY_AGGRO                               = 0,
    SAY_SLAY                                = 1,
    SAY_DEATH                               = 2,
    SAY_SUMMON_BJORN                        = 3,
    SAY_SUMMON_HALDOR                       = 4,
    SAY_SUMMON_RANULF                       = 5,
    SAY_SUMMON_TORGYN                       = 6
};

enum Creatures
{
    NPC_BJORN                          = 27303,
    NPC_BJORN_VISUAL                   = 27304,
    NPC_HALDOR                         = 27307,
    NPC_HALDOR_VISUAL                  = 27310,
    NPC_RANULF                         = 27308,
    NPC_RANULF_VISUAL                  = 27311,
    NPC_TORGYN                         = 27309,
    NPC_TORGYN_VISUAL                  = 27312,
    NPC_SPIRIT_FOUNT                   = 27339,
    NPC_AVENGING_SPIRIT                = 27386
};

struct ActiveBoatStruct
{
    uint32 npc;
    int32 say;
    float MoveX, MoveY, MoveZ, SpawnX, SpawnY, SpawnZ, SpawnO;
};

static ActiveBoatStruct ActiveBoat[4] =
{
    {NPC_BJORN_VISUAL,  SAY_SUMMON_BJORN,  404.379f, -335.335f, 104.756f, 413.594f, -335.408f, 107.995f, 3.157f},
    {NPC_HALDOR_VISUAL, SAY_SUMMON_HALDOR, 380.813f, -335.069f, 104.756f, 369.994f, -334.771f, 107.995f, 6.232f},
    {NPC_RANULF_VISUAL, SAY_SUMMON_RANULF, 381.546f, -314.362f, 104.756f, 370.841f, -314.426f, 107.995f, 6.232f},
    {NPC_TORGYN_VISUAL, SAY_SUMMON_TORGYN, 404.310f, -314.761f, 104.756f, 413.992f, -314.703f, 107.995f, 3.157f}
};

enum Misc
{
    DATA_KINGS_BANE                 = 2157
};

class boss_ymiron : public CreatureScript
{
public:
    boss_ymiron() : CreatureScript("boss_ymiron") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_ymironAI(creature);
    }

    struct boss_ymironAI : public ScriptedAI
    {
        boss_ymironAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            for (int i = 0; i < 4; ++i)
                m_uiActiveOrder[i] = i;
            for (int i = 0; i < 3; ++i)
            {
                int r = i + (rand() % (4 - i));
                int temp = m_uiActiveOrder[i];
                m_uiActiveOrder[i] = m_uiActiveOrder[r];
                m_uiActiveOrder[r] = temp;
            }

            m_uiActivedCreatureGUID = 0;
            m_uiOrbGUID = 0;
        }

        bool m_bIsWalking;
        bool m_bIsPause;
        bool m_bIsActiveWithBJORN;
        bool m_bIsActiveWithHALDOR;
        bool m_bIsActiveWithRANULF;
        bool m_bIsActiveWithTORGYN;
        bool kingsBane; // Achievement King's Bane

        uint8 m_uiActiveOrder[4];
        uint8 m_uiActivedNumber;

        uint32 m_uiFetidRot_Timer;
        uint32 m_uiBane_Timer;
        uint32 m_uiDarkSlash_Timer;
        uint32 m_uiAncestors_Vengeance_Timer;

        uint32 m_uiAbility_BJORN_Timer;
        uint32 m_uiAbility_HALDOR_Timer;
        uint32 m_uiAbility_RANULF_Timer;
        uint32 m_uiAbility_TORGYN_Timer;

        uint32 m_uiPause_Timer;
        uint32 m_uiHealthAmountModifier;
        uint32 m_uiHealthAmountMultipler;

        uint64 m_uiActivedCreatureGUID;
        uint64 m_uiOrbGUID;

        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            m_bIsWalking = false;
            m_bIsPause = false;
            m_bIsActiveWithBJORN = false;
            m_bIsActiveWithHALDOR = false;
            m_bIsActiveWithRANULF = false;
            m_bIsActiveWithTORGYN = false;
            kingsBane = true;

            m_uiFetidRot_Timer = std::rand() % 13000 + 8000;
            m_uiBane_Timer = std::rand() % 23000 + 18000;
            m_uiDarkSlash_Timer = std::rand() % 33000 + 28000;
            m_uiAncestors_Vengeance_Timer = DUNGEON_MODE(60000, 45000);
            m_uiPause_Timer               = 0;

            m_uiAbility_BJORN_Timer  = 0;
            m_uiAbility_HALDOR_Timer = 0;
            m_uiAbility_RANULF_Timer = 0;
            m_uiAbility_TORGYN_Timer = 0;

            m_uiActivedNumber        = 0;
            m_uiHealthAmountModifier = 1;
            m_uiHealthAmountMultipler = DUNGEON_MODE(20, 25);

            DespawnBoatGhosts(m_uiActivedCreatureGUID);
            DespawnBoatGhosts(m_uiOrbGUID);

            if (instance)
                instance->SetData(DATA_KING_YMIRON_EVENT, NOT_STARTED);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);

            if (instance)
                instance->SetData(DATA_KING_YMIRON_EVENT, IN_PROGRESS);
        }

        void SpellHitTarget(Unit* who, SpellInfo const* spell) OVERRIDE
        {
            if (who && who->GetTypeId() == TypeID::TYPEID_PLAYER && spell->Id == 59302)
                kingsBane = false;
        }

        uint32 GetData(uint32 type) const OVERRIDE
        {
            if (type == DATA_KINGS_BANE)
                return kingsBane ? 1 : 0;

            return 0;
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (m_bIsWalking)
            {
                if (m_uiPause_Timer <= diff)
                {
                    Talk(ActiveBoat[m_uiActiveOrder[m_uiActivedNumber]].say);
                    DoCast(me, SPELL_CHANNEL_YMIRON_TO_SPIRIT); // should be on spirit
                    if (Creature* temp = me->SummonCreature(ActiveBoat[m_uiActiveOrder[m_uiActivedNumber]].npc, ActiveBoat[m_uiActiveOrder[m_uiActivedNumber]].SpawnX, ActiveBoat[m_uiActiveOrder[m_uiActivedNumber]].SpawnY, ActiveBoat[m_uiActiveOrder[m_uiActivedNumber]].SpawnZ, ActiveBoat[m_uiActiveOrder[m_uiActivedNumber]].SpawnO, TempSummonType::TEMPSUMMON_CORPSE_DESPAWN, 0))
                    {
                        m_uiActivedCreatureGUID = temp->GetGUID();
                        temp->CastSpell(me, SPELL_CHANNEL_SPIRIT_TO_YMIRON, true);
                        temp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        temp->SetDisableGravity(true);
                        switch (m_uiActiveOrder[m_uiActivedNumber])
                        {
                            case 0: m_bIsActiveWithBJORN  = true; break;
                            case 1: m_bIsActiveWithHALDOR = true; break;
                            case 2: m_bIsActiveWithRANULF = true; break;
                            case 3: m_bIsActiveWithTORGYN = true; break;
                        }
                    }

                    m_bIsPause = true;
                    m_bIsWalking = false;
                    m_uiPause_Timer = 3000;
                } else m_uiPause_Timer -= diff;
                return;
            }
            else if (m_bIsPause)
            {
                if (m_uiPause_Timer <= diff)
                {
                    m_uiAbility_BJORN_Timer = 5000;
                    m_uiAbility_HALDOR_Timer = 5000;
                    m_uiAbility_RANULF_Timer = 5000;
                    m_uiAbility_TORGYN_Timer = 5000;

                    m_bIsPause = false;
                    m_uiPause_Timer = 0;
                } else m_uiPause_Timer -= diff;
                return;
            }

            //Return since we have no target
             if (!UpdateVictim())
                 return;

            if (!m_bIsPause)
            {
                // Normal spells ------------------------------------------------------------------------
                if (m_uiBane_Timer <= diff)
                {
                    DoCast(me, SPELL_BANE);
                    m_uiBane_Timer = std::rand() % 25000 + 20000;
                } else m_uiBane_Timer -= diff;

                if (m_uiFetidRot_Timer <= diff)
                {
                    DoCastVictim(SPELL_FETID_ROT);
                    m_uiFetidRot_Timer = std::rand() % 15000 + 10000;
                } else m_uiFetidRot_Timer -= diff;

                if (m_uiDarkSlash_Timer <= diff)
                {
                    DoCastVictim(SPELL_DARK_SLASH);
                    m_uiDarkSlash_Timer = std::rand() % 35000 + 30000;
                } else m_uiDarkSlash_Timer -= diff;

                if (m_uiAncestors_Vengeance_Timer <= diff)
                {
                    DoCast(me, SPELL_ANCESTORS_VENGEANCE);
                    m_uiAncestors_Vengeance_Timer = DUNGEON_MODE(std::rand() % 65000 + 60000, std::rand() % 50000 + 45000);
                } else m_uiAncestors_Vengeance_Timer -= diff;

                // Abilities ------------------------------------------------------------------------------
                if (m_bIsActiveWithBJORN && m_uiAbility_BJORN_Timer <= diff)
                {
                    //DoCast(me, SPELL_SUMMON_SPIRIT_FOUNT); // works fine, but using summon has better control
                    if (Creature* temp = me->SummonCreature(NPC_SPIRIT_FOUNT, 385.0f + rand() % 10, -330.0f + rand() % 10, 104.756f, 0, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 180000))
                    {
                        temp->SetSpeed(MOVE_RUN, 0.4f);
                        temp->CastSpell(temp, DUNGEON_MODE(SPELL_SPIRIT_FOUNT, H_SPELL_SPIRIT_FOUNT), true);
                        temp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        temp->SetDisplayId(11686);
                        m_uiOrbGUID = temp->GetGUID();
                    }
                    m_bIsActiveWithBJORN = false; // only one orb
                } else m_uiAbility_BJORN_Timer -= diff;

                if (m_bIsActiveWithHALDOR && m_uiAbility_HALDOR_Timer <= diff)
                {
                    DoCastVictim(SPELL_SPIRIT_STRIKE);
                    m_uiAbility_HALDOR_Timer = 5000; // overtime
                } else m_uiAbility_HALDOR_Timer -= diff;

                if (m_bIsActiveWithRANULF && m_uiAbility_RANULF_Timer <= diff)
                {
                    DoCast(me, SPELL_SPIRIT_BURST);
                    m_uiAbility_RANULF_Timer = 10000; // overtime
                } else m_uiAbility_RANULF_Timer -= diff;

                if (m_bIsActiveWithTORGYN && m_uiAbility_TORGYN_Timer <= diff)
                {
                    float x, y, z;
                    x = me->GetPositionX()-5;
                    y = me->GetPositionY()-5;
                    z = me->GetPositionZ();
                    for (uint8 i = 0; i < 4; ++i)
                    {
                        //DoCast(me, SPELL_SUMMON_AVENGING_SPIRIT); // works fine, but using summon has better control
                        if (Creature* temp = me->SummonCreature(NPC_AVENGING_SPIRIT, x + rand() % 10, y + rand() % 10, z, 0, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000))
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            {
                                temp->AddThreat(target, 0.0f);
                                temp->AI()->AttackStart(target);
                            }
                        }
                    }
                    m_uiAbility_TORGYN_Timer = 15000; // overtime
                } else m_uiAbility_TORGYN_Timer -= diff;

                // Health check -----------------------------------------------------------------------------
                if (me->HealthBelowPct(100 - m_uiHealthAmountMultipler * m_uiHealthAmountModifier))
                {
                    uint8 m_uiOrder = m_uiHealthAmountModifier - 1;
                    ++m_uiHealthAmountModifier;

                    me->InterruptNonMeleeSpells(true);
                    DoCast(me, SPELL_SCREAMS_OF_THE_DEAD);
                    me->GetMotionMaster()->Clear();
                    me->StopMoving();
                    me->AttackStop();
                    me->GetMotionMaster()->MovePoint(0, ActiveBoat[m_uiActiveOrder[m_uiOrder]].MoveX, ActiveBoat[m_uiActiveOrder[m_uiOrder]].MoveY, ActiveBoat[m_uiActiveOrder[m_uiOrder]].MoveZ);

                    DespawnBoatGhosts(m_uiActivedCreatureGUID);
                    DespawnBoatGhosts(m_uiOrbGUID);

                    m_bIsActiveWithBJORN  = false;
                    m_bIsActiveWithHALDOR = false;
                    m_bIsActiveWithRANULF = false;
                    m_bIsActiveWithTORGYN = false;

                    m_uiBane_Timer                += 8000;
                    m_uiFetidRot_Timer            += 8000;
                    m_uiDarkSlash_Timer           += 8000;
                    m_uiAncestors_Vengeance_Timer += 8000;

                    m_uiActivedNumber = m_uiOrder;
                    m_bIsWalking = true;
                    m_uiPause_Timer = 2000;
                    return;
                }
                DoMeleeAttackIfReady();
            }
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);

            DespawnBoatGhosts(m_uiActivedCreatureGUID);
            DespawnBoatGhosts(m_uiOrbGUID);

            if (instance)
                instance->SetData(DATA_KING_YMIRON_EVENT, DONE);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_SLAY);
        }

        void DespawnBoatGhosts(uint64 m_uiCreatureGUID)
        {
            if (m_uiCreatureGUID)
                if (Creature* temp = Unit::GetCreature(*me, m_uiCreatureGUID))
                    temp->DisappearAndDie();

            m_uiCreatureGUID = 0;
        }
    };
};

class achievement_kings_bane : public AchievementCriteriaScript
{
    public:
        achievement_kings_bane() : AchievementCriteriaScript("achievement_kings_bane")
        {
        }

        bool OnCheck(Player* /*player*/, Unit* target) OVERRIDE
        {
            if (!target)
                return false;

            if (Creature* Ymiron = target->ToCreature())
                if (Ymiron->AI()->GetData(DATA_KINGS_BANE))
                    return true;

            return false;
        }
};

void AddSC_boss_ymiron()
{
    new boss_ymiron();
    new achievement_kings_bane();
}
