/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Prince_Malchezzar
SD%Complete: 100
SDComment:
SDCategory: Karazhan
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "karazhan.h"
#include "SpellInfo.h"

// 18 Coordinates for Infernal spawns
struct InfernalPoint
{
    float x, y;
};

#define INFERNAL_Z  275.5f

static InfernalPoint InfernalPoints[] =
{
    {-10922.8f, -1985.2f},
    {-10916.2f, -1996.2f},
    {-10932.2f, -2008.1f},
    {-10948.8f, -2022.1f},
    {-10958.7f, -1997.7f},
    {-10971.5f, -1997.5f},
    {-10990.8f, -1995.1f},
    {-10989.8f, -1976.5f},
    {-10971.6f, -1973.0f},
    {-10955.5f, -1974.0f},
    {-10939.6f, -1969.8f},
    {-10958.0f, -1952.2f},
    {-10941.7f, -1954.8f},
    {-10943.1f, -1988.5f},
    {-10948.8f, -2005.1f},
    {-10984.0f, -2019.3f},
    {-10932.8f, -1979.6f},
    {-10935.7f, -1996.0f}
};

//Enfeeble is supposed to reduce hp to 1 and then heal player back to full when it ends
//Along with reducing healing and regen while enfeebled to 0%
//This spell effect will only reduce healing
enum PrinceMalchezaar
{
    SAY_AGGRO                   = 0,
    SAY_AXE_TOSS1               = 1,
    SAY_AXE_TOSS2               = 2,
//  SAY_SPECIAL1                = 3, Not used, needs to be implemented, but I don't know where it should be used.
//  SAY_SPECIAL2                = 4, Not used, needs to be implemented, but I don't know where it should be used.
//  SAY_SPECIAL3                = 5, Not used, needs to be implemented, but I don't know where it should be used.
    SAY_SLAY                    = 6,
    SAY_SUMMON                  = 7,
    SAY_DEATH                   = 8,

    TOTAL_INFERNAL_POINTS       = 18,

    SPELL_ENFEEBLE              = 30843,                       //Enfeeble during phase 1 and 2
    SPELL_ENFEEBLE_EFFECT       = 41624,

    SPELL_SHADOWNOVA            = 30852,                       //Shadownova used during all phases
    SPELL_SW_PAIN               = 30854,                       //Shadow word pain during phase 1 and 3 (different targeting rules though)
    SPELL_THRASH_PASSIVE        = 12787,                       //Extra attack chance during phase 2
    SPELL_SUNDER_ARMOR          = 30901,                       //Sunder armor during phase 2
    SPELL_THRASH_AURA           = 12787,                       //Passive proc chance for thrash
    SPELL_EQUIP_AXES            = 30857,                       //Visual for axe equiping
    SPELL_AMPLIFY_DAMAGE        = 39095,                       //Amplifiy during phase 3
    SPELL_CLEAVE                = 30131,                       //Same as Nightbane.
    SPELL_HELLFIRE              = 30859,                       //Infenals' hellfire aura
    NETHERSPITE_INFERNAL        = 17646,                       //The netherspite infernal creature
    MALCHEZARS_AXE              = 17650,                       //Malchezar's axes (creatures), summoned during phase 3

    INFERNAL_MODEL_INVISIBLE    = 11686,                       //Infernal Effects
    SPELL_INFERNAL_RELAY        = 30834,

    EQUIP_ID_AXE                = 33542                        //Axes info
};

//---------Infernal code first
class netherspite_infernal : public CreatureScript
{
public:
    netherspite_infernal() : CreatureScript("netherspite_infernal") { }

    struct netherspite_infernalAI : public ScriptedAI
    {
        netherspite_infernalAI(Creature* creature) : ScriptedAI(creature),
            HellfireTimer(0), CleanupTimer(0), malchezaar(0), point(NULL) { }

        void Reset() OVERRIDE { }
        void EnterCombat(Unit* /*who*/) OVERRIDE { }
        void MoveInLineOfSight(Unit* /*who*/) OVERRIDE { }


        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (HellfireTimer)
            {
                if (HellfireTimer <= diff)
                {
                    DoCast(me, SPELL_HELLFIRE);
                    HellfireTimer = 0;
                }
                else HellfireTimer -= diff;
            }

            if (CleanupTimer)
            {
                if (CleanupTimer <= diff)
                {
                    Cleanup();
                    CleanupTimer = 0;
                } else CleanupTimer -= diff;
            }
        }

        void KilledUnit(Unit* who) OVERRIDE
        {
            if (Unit* unit = Unit::GetUnit(*me, malchezaar))
                if (Creature* creature = unit->ToCreature())
                    creature->AI()->KilledUnit(who);
        }

        void SpellHit(Unit* /*who*/, const SpellInfo* spell) OVERRIDE
        {
            if (spell->Id == SPELL_INFERNAL_RELAY)
            {
                me->SetDisplayId(me->GetUInt32Value(UNIT_FIELD_NATIVE_DISPLAY_ID));
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                HellfireTimer = 4000;
                CleanupTimer = 170000;
            }
        }

        void DamageTaken(Unit* done_by, uint32 &damage) OVERRIDE
        {
            if (done_by->GetGUID() != malchezaar)
                damage = 0;
        }

        void Cleanup();

    public:
        InfernalPoint* point;
        uint64 malchezaar;
    private:
        uint32 HellfireTimer;
        uint32 CleanupTimer;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new netherspite_infernalAI(creature);
    }
};

class boss_malchezaar : public CreatureScript
{
public:
    boss_malchezaar() : CreatureScript("boss_malchezaar") { }

    struct boss_malchezaarAI : public ScriptedAI
    {
        boss_malchezaarAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            memset(axes, 0, sizeof(axes));

            EnfeebleTimer = 0;
            EnfeebleResetTimer = 0;
            ShadowNovaTimer = 0;
            SWPainTimer = 0;
            SunderArmorTimer = 0;
            AmplifyDamageTimer = 0;
            Cleave_Timer = 0;
            InfernalTimer = 0;
            AxesTargetSwitchTimer = 0;
            InfernalCleanupTimer = 0;

            infernals = { };
            positions = { };

            for (uint8 i = 0; i < 2; i++)
            {
                axes[i] = 0;
            }

            for (uint8 j = 0; j < 5; j++)
            {
                enfeeble_targets[j] = 0;
                enfeeble_health[j] = 0;
            }

            phase = 0;
        }

        void Reset() OVERRIDE
        {
            AxesCleanup();
            ClearWeapons();
            InfernalCleanup();
            positions.clear();

            for (uint8 i = 0; i < 5; ++i)
            {
                enfeeble_targets[i] = 0;
                enfeeble_health[i] = 0;
            }

            for (uint8 i = 0; i < TOTAL_INFERNAL_POINTS; ++i)
                positions.push_back(&InfernalPoints[i]);

            EnfeebleTimer = 30000;
            EnfeebleResetTimer = 38000;
            ShadowNovaTimer = 35500;
            SWPainTimer = 20000;
            AmplifyDamageTimer = 5000;
            Cleave_Timer = 8000;
            InfernalTimer = 40000;
            InfernalCleanupTimer = 47000;
            AxesTargetSwitchTimer = std::rand() % 20000 + 7500;
            SunderArmorTimer = std::rand() % 10000 + 5000;
            phase = 1;

            if (instance)
                instance->HandleGameObject(instance->GetData64(DATA_GO_NETHER_DOOR), true);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_SLAY);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);

            AxesCleanup();
            ClearWeapons();
            InfernalCleanup();
            positions.clear();

            for (uint8 i = 0; i < TOTAL_INFERNAL_POINTS; ++i)
                positions.push_back(&InfernalPoints[i]);

            if (instance)
                instance->HandleGameObject(instance->GetData64(DATA_GO_NETHER_DOOR), true);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);

            if (instance)
                instance->HandleGameObject(instance->GetData64(DATA_GO_NETHER_DOOR), false); // Open the door leading further in
        }

        void InfernalCleanup()
        {
            //Infernal Cleanup
            for (std::vector<uint64>::const_iterator itr = infernals.begin(); itr != infernals.end(); ++itr)
                if (Unit* pInfernal = Unit::GetUnit(*me, *itr))
                    if (pInfernal->IsAlive())
                    {
                        pInfernal->SetVisible(false);
                        pInfernal->setDeathState(DeathState::JUST_DIED);
                    }

            infernals.clear();
        }

        void AxesCleanup()
        {
            for (uint8 i = 0; i < 2; ++i)
            {
                Unit* axe = Unit::GetUnit(*me, axes[i]);
                if (axe && axe->IsAlive())
                    axe->Kill(axe);
                axes[i] = 0;
            }
        }

        void ClearWeapons()
        {
            SetEquipmentSlots(false, EQUIP_UNEQUIP, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);

            //damage
            const CreatureTemplate* cinfo = me->GetCreatureTemplate();
            me->SetBaseWeaponDamage(WeaponAttackType::BASE_ATTACK, WeaponDamageRange::MINDAMAGE, cinfo->mindmg);
            me->SetBaseWeaponDamage(WeaponAttackType::BASE_ATTACK, WeaponDamageRange::MAXDAMAGE, cinfo->maxdmg);
            me->UpdateDamagePhysical(WeaponAttackType::BASE_ATTACK);
        }

        void EnfeebleHealthEffect()
        {
            const SpellInfo* info = sSpellMgr->GetSpellInfo(SPELL_ENFEEBLE_EFFECT);
            if (!info)
                return;

            ThreatContainer::StorageType const &t_list = me->getThreatManager().getThreatList();
            std::vector<Unit*> targets;

            if (t_list.empty())
                return;

            //begin + 1, so we don't target the one with the highest threat
            ThreatContainer::StorageType::const_iterator itr = t_list.begin();
            std::advance(itr, 1);
            for (; itr != t_list.end(); ++itr) //store the threat list in a different container
                if (Unit* target = Unit::GetUnit(*me, (*itr)->getUnitGuid()))
                    if (target->IsAlive() && target->GetTypeId() == TypeID::TYPEID_PLAYER)
                        targets.push_back(target);

            //cut down to size if we have more than 5 targets
            while (targets.size() > 5)
                targets.erase(targets.begin()+rand()%targets.size());

            uint32 i = 0;
            for (std::vector<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter, ++i)
                if (Unit* target = *iter)
                {
                    enfeeble_targets[i] = target->GetGUID();
                    enfeeble_health[i] = target->GetHealth();

                    target->CastSpell(target, SPELL_ENFEEBLE, true, 0, 0, me->GetGUID());
                    target->SetHealth(1);
                }
        }

        void EnfeebleResetHealth()
        {
            for (uint8 i = 0; i < 5; ++i)
            {
                Unit* target = Unit::GetUnit(*me, enfeeble_targets[i]);
                if (target && target->IsAlive())
                    target->SetHealth(enfeeble_health[i]);
                enfeeble_targets[i] = 0;
                enfeeble_health[i] = 0;
            }
        }

        void SummonInfernal(const uint32 /*diff*/)
        {
            InfernalPoint *point = NULL;
            Position pos;
            if ((me->GetMapId() != 532) || positions.empty())
                me->GetRandomNearPosition(pos, 60);
            else
            {
                point = Skyfire::Containers::SelectRandomContainerElement(positions);
                pos.Relocate(point->x, point->y, INFERNAL_Z, frand(0.0f, float(M_PI * 2)));
            }

            Creature* infernal = me->SummonCreature(NETHERSPITE_INFERNAL, pos, TempSummonType::TEMPSUMMON_TIMED_DESPAWN, 180000);

            if (infernal)
            {
                infernal->SetDisplayId(INFERNAL_MODEL_INVISIBLE);
                infernal->setFaction(me->getFaction());
                if (point)
                    CAST_AI(netherspite_infernal::netherspite_infernalAI, infernal->AI())->point=point;
                CAST_AI(netherspite_infernal::netherspite_infernalAI, infernal->AI())->malchezaar=me->GetGUID();

                infernals.push_back(infernal->GetGUID());
                DoCast(infernal, SPELL_INFERNAL_RELAY);
            }

            Talk(SAY_SUMMON);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            if (EnfeebleResetTimer && EnfeebleResetTimer <= diff) // Let's not forget to reset that
            {
                EnfeebleResetHealth();
                EnfeebleResetTimer = 0;
            } else EnfeebleResetTimer -= diff;

            if (me->HasUnitState(UNIT_STATE_STUNNED))      // While shifting to phase 2 malchezaar stuns himself
                return;

            if (me->GetUInt64Value(UNIT_FIELD_TARGET) != me->GetVictim()->GetGUID())
                me->SetTarget(me->GetVictim()->GetGUID());

            if (phase == 1)
            {
                if (HealthBelowPct(60))
                {
                    me->InterruptNonMeleeSpells(false);

                    phase = 2;

                    //animation
                    DoCast(me, SPELL_EQUIP_AXES);

                    //text
                    Talk(SAY_AXE_TOSS1);

                    //passive thrash aura
                    DoCast(me, SPELL_THRASH_AURA, true);

                    //models
                    SetEquipmentSlots(false, EQUIP_ID_AXE, EQUIP_ID_AXE, EQUIP_NO_CHANGE);

                    //damage
                    const CreatureTemplate* cinfo = me->GetCreatureTemplate();
                    me->SetBaseWeaponDamage(WeaponAttackType::BASE_ATTACK, WeaponDamageRange::MINDAMAGE, 2*cinfo->mindmg);
                    me->SetBaseWeaponDamage(WeaponAttackType::BASE_ATTACK, WeaponDamageRange::MAXDAMAGE, 2*cinfo->maxdmg);
                    me->UpdateDamagePhysical(WeaponAttackType::BASE_ATTACK);

                    me->SetBaseWeaponDamage(WeaponAttackType::OFF_ATTACK, WeaponDamageRange::MINDAMAGE, cinfo->mindmg);
                    me->SetBaseWeaponDamage(WeaponAttackType::OFF_ATTACK, WeaponDamageRange::MAXDAMAGE, cinfo->maxdmg);
                    //Sigh, updating only works on main attack, do it manually ....
                    me->SetFloatValue(UNIT_FIELD_MIN_OFF_HAND_DAMAGE, cinfo->mindmg);
                    me->SetFloatValue(UNIT_FIELD_MAX_OFF_HAND_DAMAGE, cinfo->maxdmg);

                    me->SetAttackTime(WeaponAttackType::OFF_ATTACK, (me->GetAttackTime(WeaponAttackType::BASE_ATTACK)*150)/100);
                }
            }
            else if (phase == 2)
            {
                if (HealthBelowPct(30))
                {
                    InfernalTimer = 15000;

                    phase = 3;

                    ClearWeapons();

                    //remove thrash
                    me->RemoveAurasDueToSpell(SPELL_THRASH_AURA);

                    Talk(SAY_AXE_TOSS2);

                    Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true);
                    for (uint8 i = 0; i < 2; ++i)
                    {
                        Creature* axe = me->SummonCreature(MALCHEZARS_AXE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 1000);
                        if (axe)
                        {
                            axe->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            axe->setFaction(me->getFaction());
                            axes[i] = axe->GetGUID();
                            if (target)
                            {
                                axe->AI()->AttackStart(target);
                                //axe->getThreatManager().tauntApply(target); //Taunt Apply and fade out does not work properly
                                                                // So we'll use a hack to add a lot of threat to our target
                                axe->AddThreat(target, 10000000.0f);
                            }
                        }
                    }

                    if (ShadowNovaTimer > 35000)
                        ShadowNovaTimer = EnfeebleTimer + 5000;

                    return;
                }

                if (SunderArmorTimer <= diff)
                {
                    DoCastVictim(SPELL_SUNDER_ARMOR);
                    SunderArmorTimer = std::rand() % 18000 + 10000;
                } else SunderArmorTimer -= diff;

                if (Cleave_Timer <= diff)
                {
                    DoCastVictim(SPELL_CLEAVE);
                    Cleave_Timer = std::rand() % 12000 + 6000;
                } else Cleave_Timer -= diff;
            }
            else
            {
                if (AxesTargetSwitchTimer <= diff)
                {
                    AxesTargetSwitchTimer = std::rand() % 20000 + 7500;

                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    {
                        for (uint8 i = 0; i < 2; ++i)
                        {
                            if (Unit* axe = Unit::GetUnit(*me, axes[i]))
                            {
                                if (axe->GetVictim())
                                    DoModifyThreatPercent(axe->GetVictim(), -100);
                                if (target)
                                    axe->AddThreat(target, 1000000.0f);
                                //axe->getThreatManager().tauntFadeOut(axe->GetVictim());
                                //axe->getThreatManager().tauntApply(target);
                            }
                        }
                    }
                } else AxesTargetSwitchTimer -= diff;

                if (AmplifyDamageTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        DoCast(target, SPELL_AMPLIFY_DAMAGE);
                    AmplifyDamageTimer = std::rand() % 30000 + 20000;
                } else AmplifyDamageTimer -= diff;
            }

            //Time for global and double timers
            if (InfernalTimer <= diff)
            {
                SummonInfernal(diff);
                InfernalTimer = phase == 3 ? 14500 : 44500;    // 15 secs in phase 3, 45 otherwise
            } else InfernalTimer -= diff;

            if (ShadowNovaTimer <= diff)
            {
                DoCastVictim(SPELL_SHADOWNOVA);
                ShadowNovaTimer = phase == 3 ? 31000 : uint32(-1);
            } else ShadowNovaTimer -= diff;

            if (phase != 2)
            {
                if (SWPainTimer <= diff)
                {
                    Unit* target = NULL;
                    if (phase == 1)
                        target = me->GetVictim();        // the tank
                    else                                          // anyone but the tank
                        target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true);

                    if (target)
                        DoCast(target, SPELL_SW_PAIN);

                    SWPainTimer = 20000;
                } else SWPainTimer -= diff;
            }

            if (phase != 3)
            {
                if (EnfeebleTimer <= diff)
                {
                    EnfeebleHealthEffect();
                    EnfeebleTimer = 30000;
                    ShadowNovaTimer = 5000;
                    EnfeebleResetTimer = 9000;
                } else EnfeebleTimer -= diff;
            }

            if (phase == 2)
                DoMeleeAttacksIfReady();
            else
                DoMeleeAttackIfReady();
        }

        void DoMeleeAttacksIfReady()
        {
            if (me->IsWithinMeleeRange(me->GetVictim()) && !me->IsNonMeleeSpellCasted(false))
            {
                //Check for base attack
                if (me->isAttackReady() && me->GetVictim())
                {
                    me->AttackerStateUpdate(me->GetVictim());
                    me->resetAttackTimer();
                }
                //Check for offhand attack
                if (me->isAttackReady(WeaponAttackType::OFF_ATTACK) && me->GetVictim())
                {
                    me->AttackerStateUpdate(me->GetVictim(), WeaponAttackType::OFF_ATTACK);
                    me->resetAttackTimer(WeaponAttackType::OFF_ATTACK);
                }
            }
        }

        void Cleanup(Creature* infernal, InfernalPoint *point)
        {
            for (std::vector<uint64>::iterator itr = infernals.begin(); itr!= infernals.end(); ++itr)
                if (*itr == infernal->GetGUID())
            {
                infernals.erase(itr);
                break;
            }

            positions.push_back(point);
        }

    private:
        InstanceScript* instance;
        uint32 EnfeebleTimer;
        uint32 EnfeebleResetTimer;
        uint32 ShadowNovaTimer;
        uint32 SWPainTimer;
        uint32 SunderArmorTimer;
        uint32 AmplifyDamageTimer;
        uint32 Cleave_Timer;
        uint32 InfernalTimer;
        uint32 AxesTargetSwitchTimer;
        uint32 InfernalCleanupTimer;

        std::vector<uint64> infernals;
        std::vector<InfernalPoint*> positions;

        uint64 axes[2];
        uint64 enfeeble_targets[5];
        uint32 enfeeble_health[5];

        uint32 phase;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_malchezaarAI(creature);
    }
};

void netherspite_infernal::netherspite_infernalAI::Cleanup()
{
    Creature* pMalchezaar = Unit::GetCreature(*me, malchezaar);

    if (pMalchezaar && pMalchezaar->IsAlive())
        CAST_AI(boss_malchezaar::boss_malchezaarAI, pMalchezaar->AI())->Cleanup(me, point);
}

void AddSC_boss_malchezaar()
{
    new boss_malchezaar();
    new netherspite_infernal();
}
