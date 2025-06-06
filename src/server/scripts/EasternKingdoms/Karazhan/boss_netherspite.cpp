/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Netherspite
SD%Complete: 90
SDComment: Not sure about timing and portals placing
SDCategory: Karazhan
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "karazhan.h"
#include "Player.h"

enum Netherspite
{
    EMOTE_PHASE_PORTAL          = 0,
    EMOTE_PHASE_BANISH          = 1,

    SPELL_NETHERBURN_AURA       = 30522,
    SPELL_VOIDZONE              = 37063,
    SPELL_NETHER_INFUSION       = 38688,
    SPELL_NETHERBREATH          = 38523,
    SPELL_BANISH_VISUAL         = 39833,
    SPELL_BANISH_ROOT           = 42716,
    SPELL_EMPOWERMENT           = 38549,
    SPELL_NETHERSPITE_ROAR      = 38684,
};


const float PortalCoord[3][3] =
{
    {-11195.353516f, -1613.237183f, 278.237258f}, // Left side
    {-11137.846680f, -1685.607422f, 278.239258f}, // Right side
    {-11094.493164f, -1591.969238f, 279.949188f}  // Back side
};

enum Netherspite_Portal{
    RED_PORTAL = 0, // Perseverence
    GREEN_PORTAL = 1, // Serenity
    BLUE_PORTAL = 2 // Dominance
};

const uint32 PortalID[3] = {17369, 17367, 17368};
const uint32 PortalVisual[3] = {30487, 30490, 30491};
const uint32 PortalBeam[3] = {30465, 30464, 30463};
const uint32 PlayerBuff[3] = {30421, 30422, 30423};
const uint32 NetherBuff[3] = {30466, 30467, 30468};
const uint32 PlayerDebuff[3] = {38637, 38638, 38639};

class boss_netherspite : public CreatureScript
{
public:
    boss_netherspite() : CreatureScript("boss_netherspite") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_netherspiteAI(creature);
    }

    struct boss_netherspiteAI : public ScriptedAI
    {
        boss_netherspiteAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            for (int i=0; i<3; ++i)
            {
                PortalGUID[i] = 0;
                BeamTarget[i] = 0;
                BeamerGUID[i] = 0;
            }
        }

        InstanceScript* instance;

        bool PortalPhase;
        bool Berserk;
        uint32 PhaseTimer; // timer for phase switching
        uint32 VoidZoneTimer;
        uint32 NetherInfusionTimer; // berserking timer
        uint32 NetherbreathTimer;
        uint32 EmpowermentTimer;
        uint32 PortalTimer; // timer for beam checking
        uint64 PortalGUID[3]; // guid's of portals
        uint64 BeamerGUID[3]; // guid's of auxiliary beaming portals
        uint64 BeamTarget[3]; // guid's of portals' current targets

        bool IsBetween(WorldObject* u1, WorldObject* target, WorldObject* u2) // the in-line checker
        {
            if (!u1 || !u2 || !target)
                return false;

            float xn, yn, xp, yp, xh, yh;
            xn = u1->GetPositionX();
            yn = u1->GetPositionY();
            xp = u2->GetPositionX();
            yp = u2->GetPositionY();
            xh = target->GetPositionX();
            yh = target->GetPositionY();

            // check if target is between (not checking distance from the beam yet)
            if (dist(xn, yn, xh, yh) >= dist(xn, yn, xp, yp) || dist(xp, yp, xh, yh) >= dist(xn, yn, xp, yp))
                return false;
            // check  distance from the beam
            return (abs((xn-xp)*yh+(yp-yn)*xh-xn*yp+xp*yn)/dist(xn, yn, xp, yp) < 1.5f);
        }

        float dist(float xa, float ya, float xb, float yb) // auxiliary method for distance
        {
            return sqrt((xa-xb)*(xa-xb) + (ya-yb)*(ya-yb));
        }

        void Reset() OVERRIDE
        {
            Berserk = false;
            NetherInfusionTimer = 540000;
            VoidZoneTimer = 15000;
            NetherbreathTimer = 3000;

            HandleDoors(true);
            DestroyPortals();
        }

        void SummonPortals()
        {
            uint8 r = rand()%4;
            uint8 pos[3];
            pos[RED_PORTAL] = ((r % 2) ? (r > 1 ? 2 : 1) : 0);
            pos[GREEN_PORTAL] = ((r % 2) ? 0 : (r > 1 ? 2 : 1));
            pos[BLUE_PORTAL] = (r > 1 ? 1 : 2); // Blue Portal not on the left side (0)

            for (int i=0; i<3; ++i)
                if (Creature* portal = me->SummonCreature(PortalID[i], PortalCoord[pos[i]][0], PortalCoord[pos[i]][1], PortalCoord[pos[i]][2], 0, TempSummonType::TEMPSUMMON_TIMED_DESPAWN, 60000))
                {
                    PortalGUID[i] = portal->GetGUID();
                    portal->AddAura(PortalVisual[i], portal);
                }
        }

        void DestroyPortals()
        {
            for (int i=0; i<3; ++i)
            {
                if (Creature* portal = Unit::GetCreature(*me, PortalGUID[i]))
                    portal->DisappearAndDie();
                if (Creature* portal = Unit::GetCreature(*me, BeamerGUID[i]))
                    portal->DisappearAndDie();
                PortalGUID[i] = 0;
                BeamTarget[i] = 0;
            }
        }

        void UpdatePortals() // Here we handle the beams' behavior
        {
            for (int j=0; j<3; ++j) // j = color
                if (Creature* portal = Unit::GetCreature(*me, PortalGUID[j]))
                {
                    // the one who's been casted upon before
                    Unit* current = Unit::GetUnit(*portal, BeamTarget[j]);
                    // temporary store for the best suitable beam reciever
                    Unit* target = me;

                    if (Map* map = me->GetMap())
                    {
                        Map::PlayerList const& players = map->GetPlayers();

                        // get the best suitable target
                        for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                        {
                            Player* p = i->GetSource();
                            if (p && p->IsAlive() // alive
                                && (!target || target->GetDistance2d(portal)>p->GetDistance2d(portal)) // closer than current best
                                && !p->HasAura(PlayerDebuff[j], 0) // not exhausted
                                && !p->HasAura(PlayerBuff[(j+1)%3], 0) // not on another beam
                                && !p->HasAura(PlayerBuff[(j+2)%3], 0)
                                && IsBetween(me, p, portal)) // on the beam
                                target = p;
                        }
                    }
                    // buff the target
                    if (target->GetTypeId() == TypeID::TYPEID_PLAYER)
                        target->AddAura(PlayerBuff[j], target);
                    else
                        target->AddAura(NetherBuff[j], target);
                    // cast visual beam on the chosen target if switched
                    // simple target switching isn't working -> using BeamerGUID to cast (workaround)
                    if (!current || target != current)
                    {
                        BeamTarget[j] = target->GetGUID();
                        // remove currently beaming portal
                        if (Creature* beamer = Unit::GetCreature(*portal, BeamerGUID[j]))
                        {
                            beamer->CastSpell(target, PortalBeam[j], false);
                            beamer->DisappearAndDie();
                            BeamerGUID[j] = 0;
                        }
                        // create new one and start beaming on the target
                        if (Creature* beamer = portal->SummonCreature(PortalID[j], portal->GetPositionX(), portal->GetPositionY(), portal->GetPositionZ(), portal->GetOrientation(), TempSummonType::TEMPSUMMON_TIMED_DESPAWN, 60000))
                        {
                            beamer->CastSpell(target, PortalBeam[j], false);
                            BeamerGUID[j] = beamer->GetGUID();
                        }
                    }
                    // aggro target if Red Beam
                    if (j == RED_PORTAL && me->GetVictim() != target && target->GetTypeId() == TypeID::TYPEID_PLAYER)
                        me->getThreatManager().addThreat(target, 100000.0f+DoGetThreat(me->GetVictim()));
                }
        }

        void SwitchToPortalPhase()
        {
            me->RemoveAurasDueToSpell(SPELL_BANISH_ROOT);
            me->RemoveAurasDueToSpell(SPELL_BANISH_VISUAL);
            SummonPortals();
            PhaseTimer = 60000;
            PortalPhase = true;
            PortalTimer = 10000;
            EmpowermentTimer = 10000;
            Talk(EMOTE_PHASE_PORTAL);
        }

        void SwitchToBanishPhase()
        {
            me->RemoveAurasDueToSpell(SPELL_EMPOWERMENT);
            me->RemoveAurasDueToSpell(SPELL_NETHERBURN_AURA);
            DoCast(me, SPELL_BANISH_VISUAL, true);
            DoCast(me, SPELL_BANISH_ROOT, true);
            DestroyPortals();
            PhaseTimer = 30000;
            PortalPhase = false;
            Talk(EMOTE_PHASE_BANISH);

            for (int i=0; i<3; ++i)
                me->RemoveAurasDueToSpell(NetherBuff[i]);
        }

        void HandleDoors(bool open) // Massive Door switcher
        {
            if (GameObject* Door = GameObject::GetGameObject(*me, instance ? instance->GetData64(DATA_GO_MASSIVE_DOOR) : 0))
                Door->SetGoState(open ? GOState::GO_STATE_ACTIVE : GOState::GO_STATE_READY);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            HandleDoors(false);
            SwitchToPortalPhase();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            HandleDoors(true);
            DestroyPortals();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            // Void Zone
            if (VoidZoneTimer <= diff)
            {
                DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 45, true), SPELL_VOIDZONE, true);
                VoidZoneTimer = 15000;
            } else VoidZoneTimer -= diff;

            // NetherInfusion Berserk
            if (!Berserk && NetherInfusionTimer <= diff)
            {
                me->AddAura(SPELL_NETHER_INFUSION, me);
                DoCast(me, SPELL_NETHERSPITE_ROAR);
                Berserk = true;
            } else NetherInfusionTimer -= diff;

            if (PortalPhase) // PORTAL PHASE
            {
                // Distribute beams and buffs
                if (PortalTimer <= diff)
                {
                    UpdatePortals();
                    PortalTimer = 1000;
                } else PortalTimer -= diff;

                // Empowerment & Nether Burn
                if (EmpowermentTimer <= diff)
                {
                    DoCast(me, SPELL_EMPOWERMENT);
                    me->AddAura(SPELL_NETHERBURN_AURA, me);
                    EmpowermentTimer = 90000;
                } else EmpowermentTimer -= diff;

                if (PhaseTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        SwitchToBanishPhase();
                        return;
                    }
                } else PhaseTimer -= diff;
            }
            else // BANISH PHASE
            {
                // Netherbreath
                if (NetherbreathTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                        DoCast(target, SPELL_NETHERBREATH);
                    NetherbreathTimer = std::rand() % 7000 + 5000;
                } else NetherbreathTimer -= diff;

                if (PhaseTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        SwitchToPortalPhase();
                        return;
                    }
                } else PhaseTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_netherspite()
{
    new boss_netherspite();
}
