/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SF_UNIT_H
#define SF_UNIT_H

#include "DBCStructure.h"
#include "EventProcessor.h"
#include "FollowerReference.h"
#include "FollowerRefManager.h"
#include "HostileRefManager.h"
#include "MotionMaster.h"
#include "MoveSplineInit.h"
#include "Object.h"
#include "SpellAuraDefines.h"
#include "ThreatManager.h"

#define WORLD_TRIGGER   12999

enum SpellInterruptFlags
{
    SPELL_INTERRUPT_FLAG_MOVEMENT = 0x01, // why need this for instant?
    SPELL_INTERRUPT_FLAG_PUSH_BACK = 0x02, // push back
    SPELL_INTERRUPT_FLAG_UNK3 = 0x04, // any info?
    SPELL_INTERRUPT_FLAG_INTERRUPT = 0x08, // interrupt
    SPELL_INTERRUPT_FLAG_ABORT_ON_DMG = 0x10  // _complete_ interrupt on direct damage
    //SPELL_INTERRUPT_UNK             = 0x20                // unk, 564 of 727 spells having this spell start with "Glyph"
};

// See SpellAuraInterruptFlags for other values definitions
enum SpellChannelInterruptFlags
{
    CHANNEL_INTERRUPT_FLAG_INTERRUPT = 0x08,  // interrupt
    CHANNEL_FLAG_DELAY = 0x4000
};

enum SpellAuraInterruptFlags
{
    AURA_INTERRUPT_FLAG_HITBYSPELL = 0x00000001,   // 0    removed when getting hit by a negative spell?
    AURA_INTERRUPT_FLAG_TAKE_DAMAGE = 0x00000002,   // 1    removed by any damage
    AURA_INTERRUPT_FLAG_CAST = 0x00000004,   // 2    cast any spells
    AURA_INTERRUPT_FLAG_MOVE = 0x00000008,   // 3    removed by any movement
    AURA_INTERRUPT_FLAG_TURNING = 0x00000010,   // 4    removed by any turning
    AURA_INTERRUPT_FLAG_JUMP = 0x00000020,   // 5    removed by entering combat
    AURA_INTERRUPT_FLAG_NOT_MOUNTED = 0x00000040,   // 6    removed by dismounting
    AURA_INTERRUPT_FLAG_NOT_ABOVEWATER = 0x00000080,   // 7    removed by entering water
    AURA_INTERRUPT_FLAG_NOT_UNDERWATER = 0x00000100,   // 8    removed by leaving water
    AURA_INTERRUPT_FLAG_NOT_SHEATHED = 0x00000200,   // 9    removed by unsheathing
    AURA_INTERRUPT_FLAG_TALK = 0x00000400,   // 10   talk to npc / loot? action on creature
    AURA_INTERRUPT_FLAG_USE = 0x00000800,   // 11   mine/use/open action on gameobject
    AURA_INTERRUPT_FLAG_MELEE_ATTACK = 0x00001000,   // 12   removed by attacking
    AURA_INTERRUPT_FLAG_SPELL_ATTACK = 0x00002000,   // 13   ???
    AURA_INTERRUPT_FLAG_UNK14 = 0x00004000,   // 14
    AURA_INTERRUPT_FLAG_TRANSFORM = 0x00008000,   // 15   removed by transform?
    AURA_INTERRUPT_FLAG_UNK16 = 0x00010000,   // 16
    AURA_INTERRUPT_FLAG_MOUNT = 0x00020000,   // 17   misdirect, aspect, swim speed
    AURA_INTERRUPT_FLAG_NOT_SEATED = 0x00040000,   // 18   removed by standing up (used by food and drink mostly and sleep/Fake Death like)
    AURA_INTERRUPT_FLAG_CHANGE_MAP = 0x00080000,   // 19   leaving map/getting teleported
    AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION = 0x00100000,   // 20   removed by auras that make you invulnerable, or make other to lose selection on you
    AURA_INTERRUPT_FLAG_UNK21 = 0x00200000,   // 21
    AURA_INTERRUPT_FLAG_TELEPORTED = 0x00400000,   // 22
    AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT = 0x00800000,   // 23   removed by entering pvp combat
    AURA_INTERRUPT_FLAG_DIRECT_DAMAGE = 0x01000000,   // 24   removed by any direct damage
    AURA_INTERRUPT_FLAG_LANDING = 0x02000000,   // 25   removed by hitting the ground

    AURA_INTERRUPT_FLAG_NOT_VICTIM = (AURA_INTERRUPT_FLAG_HITBYSPELL | AURA_INTERRUPT_FLAG_TAKE_DAMAGE | AURA_INTERRUPT_FLAG_DIRECT_DAMAGE)
};

enum SpellModOp
{
    SPELLMOD_DAMAGE = 0,
    SPELLMOD_DURATION = 1,
    SPELLMOD_THREAT = 2,
    SPELLMOD_EFFECT1 = 3,
    SPELLMOD_CHARGES = 4,
    SPELLMOD_RANGE = 5,
    SPELLMOD_RADIUS = 6,
    SPELLMOD_CRITICAL_CHANCE = 7,
    SPELLMOD_ALL_EFFECTS = 8,
    SPELLMOD_NOT_LOSE_CASTING_TIME = 9,
    SPELLMOD_CASTING_TIME = 10,
    SPELLMOD_COOLDOWN = 11,
    SPELLMOD_EFFECT2 = 12,
    SPELLMOD_IGNORE_ARMOR = 13,
    SPELLMOD_COST = 14,
    SPELLMOD_CRIT_DAMAGE_BONUS = 15,
    SPELLMOD_RESIST_MISS_CHANCE = 16,
    SPELLMOD_JUMP_TARGETS = 17,
    SPELLMOD_CHANCE_OF_SUCCESS = 18,
    SPELLMOD_ACTIVATION_TIME = 19,
    SPELLMOD_DAMAGE_MULTIPLIER = 20,
    SPELLMOD_GLOBAL_COOLDOWN = 21,
    SPELLMOD_DOT = 22,
    SPELLMOD_EFFECT3 = 23,
    SPELLMOD_BONUS_MULTIPLIER = 24,
    // spellmod 25
    SPELLMOD_PROC_PER_MINUTE = 26,
    SPELLMOD_VALUE_MULTIPLIER = 27,
    SPELLMOD_RESIST_DISPEL_CHANCE = 28,
    SPELLMOD_CRIT_DAMAGE_BONUS_2 = 29, //one not used spell
    SPELLMOD_SPELL_COST_REFUND_ON_FAIL = 30,
    SPELLMOD_CHARGES2 = 31,
    SPELLMOD_EFFECT4 = 32,
    SPELLMOD_EFFECT5 = 33,
    SPELLMOD_UNK_35 = 35 // Bodyguard Visual
};

#define MAX_SPELLMOD 36

enum SpellValueMod
{
    SPELLVALUE_BASE_POINT0,
    SPELLVALUE_BASE_POINT1,
    SPELLVALUE_BASE_POINT2,
    SPELLVALUE_RADIUS_MOD,
    SPELLVALUE_MAX_TARGETS,
    SPELLVALUE_AURA_STACK
};

class CustomSpellValues
{
    typedef std::pair<SpellValueMod, int32> CustomSpellValueMod;
    typedef std::vector<CustomSpellValueMod> StorageType;

public:
    typedef StorageType::const_iterator const_iterator;

public:
    void AddSpellMod(SpellValueMod mod, int32 value)
    {
        storage_.push_back(CustomSpellValueMod(mod, value));
    }

    const_iterator begin() const
    {
        return storage_.begin();
    }

    const_iterator end() const
    {
        return storage_.end();
    }

private:
    StorageType storage_;
};

enum SpellFacingFlags
{
    SPELL_FACING_FLAG_INFRONT = 0x0001
};

#define BASE_MINDAMAGE 1.0f
#define BASE_MAXDAMAGE 2.0f
#define BASE_ATTACK_TIME 2000

// byte value (UNIT_FIELD_ANIM_TIER, 0)
enum UnitStandStateType
{
    UNIT_STAND_STATE_STAND = 0,
    UNIT_STAND_STATE_SIT = 1,
    UNIT_STAND_STATE_SIT_CHAIR = 2,
    UNIT_STAND_STATE_SLEEP = 3,
    UNIT_STAND_STATE_SIT_LOW_CHAIR = 4,
    UNIT_STAND_STATE_SIT_MEDIUM_CHAIR = 5,
    UNIT_STAND_STATE_SIT_HIGH_CHAIR = 6,
    UNIT_STAND_STATE_DEAD = 7,
    UNIT_STAND_STATE_KNEEL = 8,
    UNIT_STAND_STATE_SUBMERGED = 9
};

// byte flag value (UNIT_FIELD_ANIM_TIER, 2)
enum UnitStandFlags
{
    UNIT_STAND_FLAGS_UNK1 = 0x01,
    UNIT_STAND_FLAGS_CREEP = 0x02,
    UNIT_STAND_FLAGS_UNTRACKABLE = 0x04,
    UNIT_STAND_FLAGS_UNK4 = 0x08,
    UNIT_STAND_FLAGS_UNK5 = 0x10,
    UNIT_STAND_FLAGS_ALL = 0xFF
};

// byte flags value (UNIT_FIELD_ANIM_TIER, 3)
enum UnitBytes1_Flags
{
    UNIT_BYTE1_FLAG_ALWAYS_STAND = 0x01,
    UNIT_BYTE1_FLAG_HOVER = 0x02,
    UNIT_BYTE1_FLAG_UNK_3 = 0x04,
    UNIT_BYTE1_FLAG_ALL = 0xFF
};

// high byte (3 from 0..3) of UNIT_FIELD_SHAPESHIFT_FORM
enum ShapeshiftForm
{
    FORM_NONE = 0x00,
    FORM_CAT = 0x01,
    FORM_TREE = 0x02,
    FORM_TRAVEL = 0x03,
    FORM_AQUA = 0x04,
    FORM_BEAR = 0x05,
    FORM_AMBIENT = 0x06,
    FORM_GHOUL = 0x07,
    FORM_DIREBEAR = 0x08, // Removed in 4.0.1
    FORM_STEVES_GHOUL = 0x09,
    FORM_THARONJA_SKELETON = 0x0A,
    FORM_TEST_OF_STRENGTH = 0x0B,
    FORM_BLB_PLAYER = 0x0C,
    FORM_SHADOW_DANCE = 0x0D,
    FORM_CREATUREBEAR = 0x0E,
    FORM_CREATURECAT = 0x0F,
    FORM_GHOSTWOLF = 0x10,
    FORM_BATTLESTANCE = 0x11,
    FORM_DEFENSIVESTANCE = 0x12,
    FORM_BERSERKERSTANCE = 0x13,
    FORM_WISE_SERPENT = 0x14,
    FORM_ZOMBIE = 0x15,
    FORM_METAMORPHOSIS = 0x16,
    FORM_STURDY_OX = 0x17,
    FORM_FIERCE_TIGER = 0x18,
    FORM_UNDEAD = 0x19,
    FORM_MASTER_ANGLER = 0x1A,
    FORM_FLIGHT_EPIC = 0x1B,
    FORM_SHADOW = 0x1C,
    FORM_FLIGHT = 0x1D,
    FORM_STEALTH = 0x1E,
    FORM_MOONKIN = 0x1F,
    FORM_SPIRITOFREDEMPTION = 0x20
};

// low byte (0 from 0..3) of UNIT_FIELD_SHAPESHIFT_FORM
enum SheathState
{
    SHEATH_STATE_UNARMED = 0,                              // non prepared weapon
    SHEATH_STATE_MELEE = 1,                              // prepared melee weapon
    SHEATH_STATE_RANGED = 2                               // prepared ranged weapon
};

#define MAX_SHEATH_STATE    3

// byte (1 from 0..3) of UNIT_FIELD_SHAPESHIFT_FORM
enum UnitPVPStateFlags
{
    UNIT_BYTE2_FLAG_PVP = 0x01,
    UNIT_BYTE2_FLAG_UNK1 = 0x02,
    UNIT_BYTE2_FLAG_FFA_PVP = 0x04,
    UNIT_BYTE2_FLAG_SANCTUARY = 0x08,
    UNIT_BYTE2_FLAG_UNK4 = 0x10,
    UNIT_BYTE2_FLAG_UNK5 = 0x20,
    UNIT_BYTE2_FLAG_UNK6 = 0x40,
    UNIT_BYTE2_FLAG_UNK7 = 0x80
};

// byte (2 from 0..3) of UNIT_FIELD_SHAPESHIFT_FORM
enum UnitRename
{
    UNIT_CAN_BE_RENAMED = 0x01,
    UNIT_CAN_BE_ABANDONED = 0x02
};

#define CREATURE_MAX_SPELLS     8
#define MAX_SPELL_CHARM         4
#define MAX_SPELL_VEHICLE       6
#define MAX_SPELL_POSSESS       8
#define MAX_SPELL_CONTROL_BAR   10

#define MAX_AGGRO_RESET_TIME 10 // in seconds
#define MAX_AGGRO_RADIUS 45.0f  // yards

enum Swing
{
    NOSWING = 0,
    SINGLEHANDEDSWING = 1,
    TWOHANDEDSWING = 2
};

enum class VictimState
{
    VICTIMSTATE_MISS = 0, // set when attacker misses
    VICTIMSTATE_WOUND = 1, // victim got clear/blocked hit
    VICTIMSTATE_DODGE = 2,
    VICTIMSTATE_PARRY = 3,
    VICTIMSTATE_INTERRUPT = 4,
    VICTIMSTATE_BLOCK = 5, // unused? not set when blocked, even on full block
    VICTIMSTATE_EVADE = 6,
    VICTIMSTATE_IMMUNE = 7,
    VICTIMSTATE_DEFLECT = 8
};

enum HitInfo
{
    HITINFO_NORMALSWING = 0x00000000,
    HITINFO_UNK1 = 0x00000001,               // req correct packet structure
    HITINFO_AFFECTS_VICTIM = 0x00000002,
    HITINFO_OFFHAND = 0x00000004,
    HITINFO_UNK2 = 0x00000008,
    HITINFO_MISS = 0x00000010,
    HITINFO_FULL_ABSORB = 0x00000020,
    HITINFO_PARTIAL_ABSORB = 0x00000040,
    HITINFO_FULL_RESIST = 0x00000080,
    HITINFO_PARTIAL_RESIST = 0x00000100,
    HITINFO_CRITICALHIT = 0x00000200,               // critical hit
    // 0x00000400
    // 0x00000800
    HITINFO_UNK12 = 0x00001000,
    HITINFO_BLOCK = 0x00002000,               // blocked damage
    // 0x00004000                                           // Hides worldtext for 0 damage
    // 0x00008000                                           // Related to blood visual
    HITINFO_GLANCING = 0x00010000,
    HITINFO_CRUSHING = 0x00020000,
    HITINFO_NO_ANIMATION = 0x00040000,
    // 0x00080000
    // 0x00100000
    HITINFO_SWINGNOHITSOUND = 0x00200000,               // unused?
    // 0x00400000
    HITINFO_RAGE_GAIN = 0x00800000,
    HITINFO_FAKE_DAMAGE = 0x01000000, // enables damage animation even if no damage done, set only if no damage
    HITINFO_UNK25 = 0x02000000,
    HITINFO_UNK26 = 0x04000000,
};

//i would like to remove this: (it is defined in item.h
enum InventorySlot
{
    NULL_BAG = 0,
    NULL_SLOT = 255
};

struct FactionTemplateEntry;
struct SpellValue;

class AuraApplication;
class Aura;
class UnitAura;
class AuraEffect;
class Creature;
class Spell;
class SpellInfo;
class DynamicObject;
class GameObject;
class Item;
class Pet;
class PetAura;
class Minion;
class Guardian;
class UnitAI;
class Totem;
class Transport;
class Vehicle;
class VehicleJoinEvent;
class TransportBase;
class SpellCastTargets;
namespace Movement
{
    class ExtraMovementStatusElement;
    class MoveSpline;
}

typedef std::list<Unit*> UnitList;
typedef std::list< std::pair<Aura*, uint8> > DispelChargesList;

struct SpellImmune
{
    SpellImmune() : type(0), spellId(0) { }
    uint32 type;
    uint32 spellId;
};

typedef std::list<SpellImmune> SpellImmuneList;

enum UnitModifierType
{
    BASE_VALUE = 0,
    BASE_PCT = 1,
    TOTAL_VALUE = 2,
    TOTAL_PCT = 3,
    MODIFIER_TYPE_END = 4
};

enum class WeaponDamageRange
{
    MINDAMAGE,
    MAXDAMAGE
};

enum DamageTypeToSchool
{
    RESISTANCE,
    DAMAGE_DEALT,
    DAMAGE_TAKEN
};

enum AuraRemoveMode
{
    AURA_REMOVE_NONE = 0,
    AURA_REMOVE_BY_DEFAULT = 1,       // scripted remove, remove by stack with aura with different ids and sc aura remove
    AURA_REMOVE_BY_CANCEL,
    AURA_REMOVE_BY_ENEMY_SPELL,       // dispel and absorb aura destroy
    AURA_REMOVE_BY_EXPIRE,            // aura duration has ended
    AURA_REMOVE_BY_DEATH
};

enum TriggerCastFlags
{
    TRIGGERED_NONE = 0x00000000,   //! Not triggered
    TRIGGERED_IGNORE_GCD = 0x00000001,   //! Will ignore GCD
    TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD = 0x00000002,   //! Will ignore Spell and Category cooldowns
    TRIGGERED_IGNORE_POWER_AND_REAGENT_COST = 0x00000004,   //! Will ignore power and reagent cost
    TRIGGERED_IGNORE_CAST_ITEM = 0x00000008,   //! Will not take away cast item or update related achievement criteria
    TRIGGERED_IGNORE_AURA_SCALING = 0x00000010,   //! Will ignore aura scaling
    TRIGGERED_IGNORE_CAST_IN_PROGRESS = 0x00000020,   //! Will not check if a current cast is in progress
    TRIGGERED_IGNORE_COMBO_POINTS = 0x00000040,   //! Will ignore combo point requirement
    TRIGGERED_CAST_DIRECTLY = 0x00000080,   //! In Spell::prepare, will be cast directly without setting containers for executed spell
    TRIGGERED_IGNORE_AURA_INTERRUPT_FLAGS = 0x00000100,   //! Will ignore interruptible aura's at cast
    TRIGGERED_IGNORE_SET_FACING = 0x00000200,   //! Will not adjust facing to target (if any)
    TRIGGERED_IGNORE_SHAPESHIFT = 0x00000400,   //! Will ignore shapeshift checks
    TRIGGERED_IGNORE_CASTER_AURASTATE = 0x00000800,   //! Will ignore caster aura states including combat requirements and death state
    TRIGGERED_IGNORE_CASTER_MOUNTED_OR_ON_VEHICLE = 0x00002000,   //! Will ignore mounted/on vehicle restrictions
    TRIGGERED_IGNORE_CASTER_AURAS = 0x00010000,   //! Will ignore caster aura restrictions or requirements
    TRIGGERED_DISALLOW_PROC_EVENTS = 0x00020000,   //! Disallows proc events from triggered spell (default)
    TRIGGERED_DONT_REPORT_CAST_ERROR = 0x00040000,   //! Will return SPELL_FAILED_DONT_REPORT in CheckCast functions
    TRIGGERED_IGNORE_EQUIPPED_ITEM_REQUIREMENT = 0x00080000,   //! Will ignore equipped item requirements
    TRIGGERED_IGNORE_TARGET_CHECK = 0x00100000,   //! Will ignore most target checks (mostly DBC target checks)
    TRIGGERED_FULL_MASK = 0xFFFFFFFF
};

enum UnitMods
{
    UNIT_MOD_STAT_STRENGTH,                                 // UNIT_MOD_STAT_STRENGTH..UNIT_MOD_STAT_SPIRIT must be in existed order, it's accessed by index values of Stats enum.
    UNIT_MOD_STAT_AGILITY,
    UNIT_MOD_STAT_STAMINA,
    UNIT_MOD_STAT_INTELLECT,
    UNIT_MOD_STAT_SPIRIT,
    UNIT_MOD_HEALTH,
    UNIT_MOD_MANA,                                          // UNIT_MOD_MANA..UNIT_MOD_RUNIC_POWER must be in existed order, it's accessed by index values of Powers enum.
    UNIT_MOD_RAGE,
    UNIT_MOD_FOCUS,
    UNIT_MOD_ENERGY,
    UNIT_MOD_UNUSED,                                        // Old UNIT_MOD_HAPPINESS
    UNIT_MOD_RUNE,
    UNIT_MOD_RUNIC_POWER,
    UNIT_MOD_SOUL_SHARDS,
    UNIT_MOD_ECLIPSE,
    UNIT_MOD_HOLY_POWER,
    UNIT_MOD_ALTERNATE_POWER,
    UNIT_MOD_DARK_FORCE,
    UNIT_MOD_CHI,
    UNIT_MOD_SHADOW_ORBS,
    UNIT_MOD_BURNING_EMBERS,
    UNIT_MOD_DEMONIC_FURY,
    UNIT_MOD_ARCANE_CHARGES,
    UNIT_MOD_ARMOR,                                         // UNIT_MOD_ARMOR..UNIT_MOD_RESISTANCE_ARCANE must be in existed order, it's accessed by index values of SpellSchools enum.
    UNIT_MOD_RESISTANCE_HOLY,
    UNIT_MOD_RESISTANCE_FIRE,
    UNIT_MOD_RESISTANCE_NATURE,
    UNIT_MOD_RESISTANCE_FROST,
    UNIT_MOD_RESISTANCE_SHADOW,
    UNIT_MOD_RESISTANCE_ARCANE,
    UNIT_MOD_ATTACK_POWER,
    UNIT_MOD_ATTACK_POWER_RANGED,
    UNIT_MOD_DAMAGE_MAINHAND,
    UNIT_MOD_DAMAGE_OFFHAND,
    UNIT_MOD_DAMAGE_RANGED,
    UNIT_MOD_END,
    // synonyms
    UNIT_MOD_STAT_START = UNIT_MOD_STAT_STRENGTH,
    UNIT_MOD_STAT_END = UNIT_MOD_STAT_SPIRIT + 1,
    UNIT_MOD_RESISTANCE_START = UNIT_MOD_ARMOR,
    UNIT_MOD_RESISTANCE_END = UNIT_MOD_RESISTANCE_ARCANE + 1,
    UNIT_MOD_POWER_START = UNIT_MOD_MANA,
    UNIT_MOD_POWER_END = UNIT_MOD_ARCANE_CHARGES + 1
};

enum BaseModGroup
{
    CRIT_PERCENTAGE,
    RANGED_CRIT_PERCENTAGE,
    OFFHAND_CRIT_PERCENTAGE,
    SHIELD_BLOCK_VALUE,
    BASEMOD_END
};

enum BaseModType
{
    FLAT_MOD,
    PCT_MOD,
    MOD_END
};

enum class DeathState
{
    ALIVE = 0,
    JUST_DIED = 1,
    CORPSE = 2,
    DEAD = 3,
    JUST_RESPAWNED = 4
};

enum UnitState
{
    UNIT_STATE_DIED = 0x00000001,                     // player has fake death aura
    UNIT_STATE_MELEE_ATTACKING = 0x00000002,                     // player is melee attacking someone
    //UNIT_STATE_MELEE_ATTACK_BY = 0x00000004,                     // player is melee attack by someone
    UNIT_STATE_STUNNED = 0x00000008,
    UNIT_STATE_ROAMING = 0x00000010,
    UNIT_STATE_CHASE = 0x00000020,
    //UNIT_STATE_SEARCHING       = 0x00000040,
    UNIT_STATE_FLEEING = 0x00000080,
    UNIT_STATE_IN_FLIGHT = 0x00000100,                     // player is in flight mode
    UNIT_STATE_FOLLOW = 0x00000200,
    UNIT_STATE_ROOT = 0x00000400,
    UNIT_STATE_CONFUSED = 0x00000800,
    UNIT_STATE_DISTRACTED = 0x00001000,
    UNIT_STATE_ISOLATED = 0x00002000,                     // area auras do not affect other players
    UNIT_STATE_ATTACK_PLAYER = 0x00004000,
    UNIT_STATE_CASTING = 0x00008000,
    UNIT_STATE_POSSESSED = 0x00010000,
    UNIT_STATE_CHARGING = 0x00020000,
    UNIT_STATE_JUMPING = 0x00040000,
    UNIT_STATE_MOVE = 0x00100000,
    UNIT_STATE_ROTATING = 0x00200000,
    UNIT_STATE_EVADE = 0x00400000,
    UNIT_STATE_ROAMING_MOVE = 0x00800000,
    UNIT_STATE_CONFUSED_MOVE = 0x01000000,
    UNIT_STATE_FLEEING_MOVE = 0x02000000,
    UNIT_STATE_CHASE_MOVE = 0x04000000,
    UNIT_STATE_FOLLOW_MOVE = 0x08000000,
    UNIT_STATE_IGNORE_PATHFINDING = 0x10000000,                 // do not use pathfinding in any MovementGenerator
    UNIT_STATE_UNATTACKABLE = UNIT_STATE_IN_FLIGHT,
    // for real move using movegen check and stop (except unstoppable flight)
    UNIT_STATE_MOVING = UNIT_STATE_ROAMING_MOVE | UNIT_STATE_CONFUSED_MOVE | UNIT_STATE_FLEEING_MOVE | UNIT_STATE_CHASE_MOVE | UNIT_STATE_FOLLOW_MOVE,
    UNIT_STATE_CONTROLLED = (UNIT_STATE_CONFUSED | UNIT_STATE_STUNNED | UNIT_STATE_FLEEING),
    UNIT_STATE_LOST_CONTROL = (UNIT_STATE_CONTROLLED | UNIT_STATE_JUMPING | UNIT_STATE_CHARGING),
    UNIT_STATE_SIGHTLESS = (UNIT_STATE_LOST_CONTROL | UNIT_STATE_EVADE),
    UNIT_STATE_CANNOT_AUTOATTACK = (UNIT_STATE_LOST_CONTROL | UNIT_STATE_CASTING),
    UNIT_STATE_CANNOT_TURN = (UNIT_STATE_LOST_CONTROL | UNIT_STATE_ROTATING),
    // stay by different reasons
    UNIT_STATE_NOT_MOVE = UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DIED | UNIT_STATE_DISTRACTED,
    UNIT_STATE_ALL_STATE = 0xffffffff                      //(UNIT_STATE_STOPPED | UNIT_STATE_MOVING | UNIT_STATE_IN_COMBAT | UNIT_STATE_IN_FLIGHT)
};

enum UnitMoveType
{
    MOVE_WALK = 0,
    MOVE_RUN = 1,
    MOVE_RUN_BACK = 2,
    MOVE_SWIM = 3,
    MOVE_SWIM_BACK = 4,
    MOVE_TURN_RATE = 5,
    MOVE_FLIGHT = 6,
    MOVE_FLIGHT_BACK = 7,
    MOVE_PITCH_RATE = 8
};

#define MAX_MOVE_TYPE     9

extern float baseMoveSpeed[MAX_MOVE_TYPE];
extern float playerBaseMoveSpeed[MAX_MOVE_TYPE];

enum class WeaponAttackType
{
    BASE_ATTACK = 0,
    OFF_ATTACK = 1,
    RANGED_ATTACK = 2,
    MAX_ATTACK = 3
};

enum class CombatRating
{
    CR_WEAPON_SKILL = 0,
    CR_DEFENSE_SKILL = 1, // Removed in 4.0.1
    CR_DODGE = 2,
    CR_PARRY = 3,
    CR_BLOCK = 4,
    CR_HIT_MELEE = 5,
    CR_HIT_RANGED = 6,
    CR_HIT_SPELL = 7,
    CR_CRIT_MELEE = 8,
    CR_CRIT_RANGED = 9,
    CR_CRIT_SPELL = 10,
    CR_HIT_TAKEN_MELEE = 11, // Deprecated since Cataclysm
    CR_HIT_TAKEN_RANGED = 12, // Deprecated since Cataclysm
    CR_HIT_TAKEN_SPELL = 13, // Deprecated since Cataclysm
    CR_RESILIENCE_CRIT_TAKEN = 14,
    CR_RESILIENCE_PLAYER_DAMAGE_TAKEN = 15,
    CR_CRIT_TAKEN_SPELL = 16, // Deprecated since Cataclysm
    CR_HASTE_MELEE = 17,
    CR_HASTE_RANGED = 18,
    CR_HASTE_SPELL = 19,
    CR_WEAPON_SKILL_MAINHAND = 20,
    CR_WEAPON_SKILL_OFFHAND = 21,
    CR_WEAPON_SKILL_RANGED = 22,
    CR_EXPERTISE = 23,
    CR_ARMOR_PENETRATION = 24,
    CR_MASTERY = 25,
    CR_PVP_POWER = 26
};

#define MAX_COMBAT_RATING         27

enum DamageEffectType
{
    DIRECT_DAMAGE = 0,                            // used for normal weapon damage (not for class abilities or spells)
    SPELL_DIRECT_DAMAGE = 1,                            // spell/class abilities damage
    DOT = 2,
    HEAL = 3,
    NODAMAGE = 4,                            // used also in case when damage applied to health but not applied to spell channelInterruptFlags/etc
    SELF_DAMAGE = 5
};

// Value masks for UNIT_FIELD_FLAGS
enum UnitFlags
{
    UNIT_FLAG_SERVER_CONTROLLED = 0x00000001,           // set only when unit movement is controlled by server - by SPLINE/MONSTER_MOVE packets, together with UNIT_FLAG_STUNNED; only set to units controlled by client; client function CGUnit_C::IsClientControlled returns false when set for owner
    UNIT_FLAG_NON_ATTACKABLE = 0x00000002,           // not attackable
    UNIT_FLAG_DISABLE_MOVE = 0x00000004,
    UNIT_FLAG_PVP_ATTACKABLE = 0x00000008,           // allow apply pvp rules to attackable state in addition to faction dependent state
    UNIT_FLAG_RENAME = 0x00000010,
    UNIT_FLAG_PREPARATION = 0x00000020,           // don't take reagents for spells with SPELL_ATTR5_NO_REAGENT_WHILE_PREP
    UNIT_FLAG_UNK_6 = 0x00000040,
    UNIT_FLAG_NOT_ATTACKABLE_1 = 0x00000080,           // ?? (UNIT_FLAG_PVP_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1) is NON_PVP_ATTACKABLE
    UNIT_FLAG_IMMUNE_TO_PC = 0x00000100,           // disables combat/assistance with PlayerCharacters (PC) - see Unit::_IsValidAttackTarget, Unit::_IsValidAssistTarget
    UNIT_FLAG_IMMUNE_TO_NPC = 0x00000200,           // disables combat/assistance with NonPlayerCharacters (NPC) - see Unit::_IsValidAttackTarget, Unit::_IsValidAssistTarget
    UNIT_FLAG_LOOTING = 0x00000400,           // loot animation
    UNIT_FLAG_PET_IN_COMBAT = 0x00000800,           // in combat?, 2.0.8
    UNIT_FLAG_PVP = 0x00001000,           // changed in 3.0.3
    UNIT_FLAG_SILENCED = 0x00002000,           // silenced, 2.1.1
    UNIT_FLAG_UNK_14 = 0x00004000,           // 2.0.8
    UNIT_FLAG_UNK_15 = 0x00008000,
    UNIT_FLAG_UNK_16 = 0x00010000,
    UNIT_FLAG_PACIFIED = 0x00020000,           // 3.0.3 ok
    UNIT_FLAG_STUNNED = 0x00040000,           // 3.0.3 ok
    UNIT_FLAG_IN_COMBAT = 0x00080000,
    UNIT_FLAG_TAXI_FLIGHT = 0x00100000,           // disable casting at client side spell not allowed by taxi flight (mounted?), probably used with 0x4 flag
    UNIT_FLAG_DISARMED = 0x00200000,           // 3.0.3, disable melee spells casting..., "Required melee weapon" added to melee spells tooltip.
    UNIT_FLAG_CONFUSED = 0x00400000,
    UNIT_FLAG_FLEEING = 0x00800000,
    UNIT_FLAG_PLAYER_CONTROLLED = 0x01000000,           // used in spell Eyes of the Beast for pet... let attack by controlled creature
    UNIT_FLAG_NOT_SELECTABLE = 0x02000000,
    UNIT_FLAG_SKINNABLE = 0x04000000,
    UNIT_FLAG_MOUNT = 0x08000000,
    UNIT_FLAG_UNK_28 = 0x10000000,
    UNIT_FLAG_DISABLE_POWERS = 0x20000000,           // used in Feing Death spell
    UNIT_FLAG_SHEATHE = 0x40000000,
    UNIT_FLAG_UNK_31 = 0x80000000,
    MAX_UNIT_FLAGS = 33
};

// Value masks for UNIT_FIELD_FLAGS2
enum UnitFlags2
{
    UNIT_FLAG2_FEIGN_DEATH = 0x00000001,
    UNIT_FLAG2_UNK1 = 0x00000002,   // Hide unit model (show only player equip)
    UNIT_FLAG2_IGNORE_REPUTATION = 0x00000004,
    UNIT_FLAG2_COMPREHEND_LANG = 0x00000008,
    UNIT_FLAG2_MIRROR_IMAGE = 0x00000010,
    UNIT_FLAG2_INSTANTLY_APPEAR_MODEL = 0x00000020,   // Unit model instantly appears when summoned (does not fade in)
    UNIT_FLAG2_FORCE_MOVEMENT = 0x00000040,
    UNIT_FLAG2_DISARM_OFFHAND = 0x00000080,
    UNIT_FLAG2_DISABLE_PRED_STATS = 0x00000100,   // Player has disabled predicted stats (Used by raid frames)
    UNIT_FLAG2_DISARM_RANGED = 0x00000400,   // this does not disable ranged weapon display (maybe additional flag needed?)
    UNIT_FLAG2_REGENERATE_POWER = 0x00000800,
    UNIT_FLAG2_RESTRICT_PARTY_INTERACTION = 0x00001000,   // Restrict interaction to party or raid
    UNIT_FLAG2_PREVENT_SPELL_CLICK = 0x00002000,   // Prevent spellclick
    UNIT_FLAG2_ALLOW_ENEMY_INTERACT = 0x00004000,
    UNIT_FLAG2_DISABLE_TURN = 0x00008000,
    UNIT_FLAG2_UNK2 = 0x00010000,
    UNIT_FLAG2_PLAY_DEATH_ANIM = 0x00020000,   // Plays special death animation upon death
    UNIT_FLAG2_ALLOW_CHEAT_SPELLS = 0x00040000    // Allows casting spells with AttributesEx7 & SPELL_ATTR7_IS_CHEAT_SPELL
};

/// Non Player Character flags
enum NPCFlags
{
    UNIT_NPC_FLAG_NONE = 0x00000000,
    UNIT_NPC_FLAG_GOSSIP = 0x00000001,       // 100%
    UNIT_NPC_FLAG_QUESTGIVER = 0x00000002,       // 100%
    UNIT_NPC_FLAG_UNK1 = 0x00000004,
    UNIT_NPC_FLAG_UNK2 = 0x00000008,
    UNIT_NPC_FLAG_TRAINER = 0x00000010,       // 100%
    UNIT_NPC_FLAG_TRAINER_CLASS = 0x00000020,       // 100%
    UNIT_NPC_FLAG_TRAINER_PROFESSION = 0x00000040,       // 100%
    UNIT_NPC_FLAG_VENDOR = 0x00000080,       // 100%
    UNIT_NPC_FLAG_VENDOR_AMMO = 0x00000100,       // 100%, general goods vendor
    UNIT_NPC_FLAG_VENDOR_FOOD = 0x00000200,       // 100%
    UNIT_NPC_FLAG_VENDOR_POISON = 0x00000400,       // guessed
    UNIT_NPC_FLAG_VENDOR_REAGENT = 0x00000800,       // 100%
    UNIT_NPC_FLAG_REPAIR = 0x00001000,       // 100%
    UNIT_NPC_FLAG_FLIGHTMASTER = 0x00002000,       // 100%
    UNIT_NPC_FLAG_SPIRITHEALER = 0x00004000,       // guessed
    UNIT_NPC_FLAG_SPIRITGUIDE = 0x00008000,       // guessed
    UNIT_NPC_FLAG_INNKEEPER = 0x00010000,       // 100%
    UNIT_NPC_FLAG_BANKER = 0x00020000,       // 100%
    UNIT_NPC_FLAG_PETITIONER = 0x00040000,       // 100% 0xC0000 = guild petitions, 0x40000 = arena team petitions
    UNIT_NPC_FLAG_TABARDDESIGNER = 0x00080000,       // 100%
    UNIT_NPC_FLAG_BATTLEMASTER = 0x00100000,       // 100%
    UNIT_NPC_FLAG_AUCTIONEER = 0x00200000,       // 100%
    UNIT_NPC_FLAG_STABLEMASTER = 0x00400000,       // 100%
    UNIT_NPC_FLAG_GUILD_BANKER = 0x00800000,       // cause client to send 997 opcode
    UNIT_NPC_FLAG_SPELLCLICK = 0x01000000,       // cause client to send 1015 opcode (spell click)
    UNIT_NPC_FLAG_PLAYER_VEHICLE = 0x02000000,       // players with mounts that have vehicle data should have it set
    UNIT_NPC_FLAG_REFORGER = 0x08000000,       // reforging
    UNIT_NPC_FLAG_TRANSMOGRIFIER = 0x10000000,       // transmogrification
    UNIT_NPC_FLAG_VAULTKEEPER = 0x20000000,       // void storage
    UNIT_NPC_FLAG_WILDPET_CAPTURABLE = 0x40000000,        // wild pet
    UNIT_NPC_FLAG_BMAH = 0x80000000,        // bmah goya (-2147483648)
};

enum MovementFlags
{
    MOVEMENTFLAG_NONE = 0x00000000,
    MOVEMENTFLAG_FORWARD = 0x00000001,
    MOVEMENTFLAG_BACKWARD = 0x00000002,
    MOVEMENTFLAG_STRAFE_LEFT = 0x00000004,
    MOVEMENTFLAG_STRAFE_RIGHT = 0x00000008,
    MOVEMENTFLAG_LEFT = 0x00000010,
    MOVEMENTFLAG_RIGHT = 0x00000020,
    MOVEMENTFLAG_PITCH_UP = 0x00000040,
    MOVEMENTFLAG_PITCH_DOWN = 0x00000080,
    MOVEMENTFLAG_WALKING = 0x00000100,               // Walking
    MOVEMENTFLAG_DISABLE_GRAVITY = 0x00000200,               // Former MOVEMENTFLAG_LEVITATING. This is used when walking is not possible.
    MOVEMENTFLAG_ROOT = 0x00000400,               // Must not be set along with MOVEMENTFLAG_MASK_MOVING
    MOVEMENTFLAG_FALLING = 0x00000800,               // damage dealt on that type of falling
    MOVEMENTFLAG_FALLING_FAR = 0x00001000,
    MOVEMENTFLAG_PENDING_STOP = 0x00002000,
    MOVEMENTFLAG_PENDING_STRAFE_STOP = 0x00004000,
    MOVEMENTFLAG_PENDING_FORWARD = 0x00008000,
    MOVEMENTFLAG_PENDING_BACKWARD = 0x00010000,
    MOVEMENTFLAG_PENDING_STRAFE_LEFT = 0x00020000,
    MOVEMENTFLAG_PENDING_STRAFE_RIGHT = 0x00040000,
    MOVEMENTFLAG_PENDING_ROOT = 0x00080000,
    MOVEMENTFLAG_SWIMMING = 0x00100000,               // appears with fly flag also
    MOVEMENTFLAG_ASCENDING = 0x00200000,               // press "space" when flying
    MOVEMENTFLAG_DESCENDING = 0x00400000,
    MOVEMENTFLAG_CAN_FLY = 0x00800000,               // Appears when unit can fly AND also walk
    MOVEMENTFLAG_FLYING = 0x01000000,               // unit is actually flying. pretty sure this is only used for players. creatures use disable_gravity
    MOVEMENTFLAG_SPLINE_ELEVATION = 0x02000000,               // used for flight paths
    MOVEMENTFLAG_WATERWALKING = 0x04000000,               // prevent unit from falling through water
    MOVEMENTFLAG_FALLING_SLOW = 0x08000000,               // active rogue safe fall spell (passive)
    MOVEMENTFLAG_HOVER = 0x10000000,               // hover, cannot jump
    MOVEMENTFLAG_DISABLE_COLLISION = 0x20000000,

    /// @todo Check if PITCH_UP and PITCH_DOWN really belong here..
    MOVEMENTFLAG_MASK_MOVING =
    MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT |
    MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING |
    MOVEMENTFLAG_SPLINE_ELEVATION,

    MOVEMENTFLAG_MASK_TURNING =
    MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT,

    MOVEMENTFLAG_MASK_MOVING_FLY =
    MOVEMENTFLAG_FLYING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING,

    // Movement flags allowed for creature in CreateObject - we need to keep all other enabled serverside
    // to properly calculate all movement
    MOVEMENTFLAG_MASK_CREATURE_ALLOWED =
    MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ROOT | MOVEMENTFLAG_SWIMMING |
    MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_WATERWALKING | MOVEMENTFLAG_FALLING_SLOW | MOVEMENTFLAG_HOVER,

    /// @todo if needed: add more flags to this masks that are exclusive to players
    MOVEMENTFLAG_MASK_PLAYER_ONLY =
    MOVEMENTFLAG_FLYING,

    /// Movement flags that have change status opcodes associated for players
    MOVEMENTFLAG_MASK_HAS_PLAYER_STATUS_OPCODE = MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ROOT |
    MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_WATERWALKING | MOVEMENTFLAG_FALLING_SLOW | MOVEMENTFLAG_HOVER
};

enum MovementFlags2
{
    MOVEMENTFLAG2_NONE = 0x00000000,
    MOVEMENTFLAG2_NO_STRAFE = 0x00000001,
    MOVEMENTFLAG2_NO_JUMPING = 0x00000002,
    MOVEMENTFLAG2_FULL_SPEED_TURNING = 0x00000004,
    MOVEMENTFLAG2_FULL_SPEED_PITCHING = 0x00000008,
    MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING = 0x00000010,
    MOVEMENTFLAG2_UNK5 = 0x00000020,
    MOVEMENTFLAG2_UNK6 = 0x00000040,
    MOVEMENTFLAG2_UNK7 = 0x00000080,
    MOVEMENTFLAG2_UNK8 = 0x00000100,
    MOVEMENTFLAG2_UNK9 = 0x00000200,
    MOVEMENTFLAG2_CAN_SWIM_TO_FLY_TRANS = 0x00000400,
    MOVEMENTFLAG2_UNK11 = 0x00000800,
    MOVEMENTFLAG2_UNK12 = 0x00001000,
    MOVEMENTFLAG2_INTERPOLATED_MOVEMENT = 0x00002000,
    MOVEMENTFLAG2_INTERPOLATED_TURNING = 0x00004000,
    MOVEMENTFLAG2_INTERPOLATED_PITCHING = 0x00008000
};

enum UnitTypeMask
{
    UNIT_MASK_NONE = 0x00000000,
    UNIT_MASK_SUMMON = 0x00000001,
    UNIT_MASK_MINION = 0x00000002,
    UNIT_MASK_GUARDIAN = 0x00000004,
    UNIT_MASK_TOTEM = 0x00000008,
    UNIT_MASK_PET = 0x00000010,
    UNIT_MASK_VEHICLE = 0x00000020,
    UNIT_MASK_PUPPET = 0x00000040,
    UNIT_MASK_HUNTER_PET = 0x00000080,
    UNIT_MASK_CONTROLABLE_GUARDIAN = 0x00000100,
    UNIT_MASK_ACCESSORY = 0x00000200
};

struct DiminishingReturn
{
    DiminishingReturn(DiminishingGroup group, uint32 t, uint32 count) : DRGroup(group), stack(0), hitTime(t), hitCount(count) { }

    DiminishingGroup        DRGroup : 16;
    uint16                  stack : 16;
    uint32                  hitTime;
    uint32                  hitCount;
};

enum class MeleeHitOutcome
{
    MELEE_HIT_EVADE, MELEE_HIT_MISS, MELEE_HIT_DODGE, MELEE_HIT_BLOCK, MELEE_HIT_PARRY,
    MELEE_HIT_GLANCING, MELEE_HIT_CRIT, MELEE_HIT_CRUSHING, MELEE_HIT_NORMAL
};

class DispelInfo
{
public:
    explicit DispelInfo(Unit* dispeller, uint32 dispellerSpellId, uint8 chargesRemoved) :
        _dispellerUnit(dispeller), _dispellerSpell(dispellerSpellId), _chargesRemoved(chargesRemoved)
    { }

    Unit* GetDispeller() const
    {
        return _dispellerUnit;
    }
    uint32 GetDispellerSpellId() const
    {
        return _dispellerSpell;
    }
    uint8 GetRemovedCharges() const
    {
        return _chargesRemoved;
    }
    void SetRemovedCharges(uint8 amount)
    {
        _chargesRemoved = amount;
    }
private:
    Unit* _dispellerUnit;
    uint32 _dispellerSpell;
    uint8 _chargesRemoved;
};

struct CleanDamage
{
    CleanDamage(uint32 mitigated, uint32 absorbed, WeaponAttackType _attackType, MeleeHitOutcome _hitOutCome) :
        absorbed_damage(absorbed), mitigated_damage(mitigated), attackType(_attackType), hitOutCome(_hitOutCome)
    { }

    uint32 absorbed_damage;
    uint32 mitigated_damage;

    WeaponAttackType attackType;
    MeleeHitOutcome hitOutCome;
};

struct CalcDamageInfo;

class DamageInfo
{
private:
    Unit* const m_attacker;
    Unit* const m_victim;
    uint32 m_damage;
    SpellInfo const* const m_spellInfo;
    SpellSchoolMask const m_schoolMask;
    DamageEffectType const m_damageType;
    WeaponAttackType m_attackType;
    uint32 m_absorb;
    uint32 m_resist;
    uint32 m_block;
public:
    explicit DamageInfo(Unit* _attacker, Unit* _victim, uint32 _damage, SpellInfo const* _spellInfo, SpellSchoolMask _schoolMask, DamageEffectType _damageType);
    explicit DamageInfo(CalcDamageInfo& dmgInfo);

    void ModifyDamage(int32 amount);
    void AbsorbDamage(uint32 amount);
    void ResistDamage(uint32 amount);
    void BlockDamage(uint32 amount);

    Unit* GetAttacker() const
    {
        return m_attacker;
    }
    Unit* GetVictim() const
    {
        return m_victim;
    }
    SpellInfo const* GetSpellInfo() const
    {
        return m_spellInfo;
    }
    SpellSchoolMask GetSchoolMask() const
    {
        return m_schoolMask;
    }
    DamageEffectType GetDamageType() const
    {
        return m_damageType;
    }
    WeaponAttackType GetAttackType() const
    {
        return m_attackType;
    }
    uint32 GetDamage() const
    {
        return m_damage;
    }
    uint32 GetAbsorb() const
    {
        return m_absorb;
    }
    uint32 GetResist() const
    {
        return m_resist;
    }
    uint32 GetBlock() const
    {
        return m_block;
    }
};

class HealInfo
{
private:
    uint32 m_heal;
    uint32 m_absorb;
public:
    explicit HealInfo(uint32 heal)
        : m_heal(heal)
    {
        m_absorb = 0;
    }
    void AbsorbHeal(uint32 amount)
    {
        amount = std::min(amount, GetHeal());
        m_absorb += amount;
        m_heal -= amount;
    }

    uint32 GetHeal() const
    {
        return m_heal;
    }
};

class ProcEventInfo
{
public:
    ProcEventInfo(Unit* actor, Unit* actionTarget, Unit* procTarget, uint32 typeMask,
        uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask,
        Spell* spell, DamageInfo* damageInfo, HealInfo* healInfo);

    Unit* GetActor() const { return _actor; }
    Unit* GetActionTarget() const { return _actionTarget; }
    Unit* GetProcTarget() const { return _procTarget; }

    uint32 GetTypeMask() const { return _typeMask; }
    uint32 GetSpellTypeMask() const { return _spellTypeMask; }
    uint32 GetSpellPhaseMask() const { return _spellPhaseMask; }
    uint32 GetHitMask() const { return _hitMask; }


    SpellInfo const* GetSpellInfo() const { return NULL; }
    SpellSchoolMask GetSchoolMask() const { return SPELL_SCHOOL_MASK_NONE; }

    DamageInfo* GetDamageInfo() const { return _damageInfo; }
    HealInfo* GetHealInfo() const { return _healInfo; }

private:
    Unit* const _actor;
    Unit* const _actionTarget;
    Unit* const _procTarget;
    uint32 _typeMask;
    uint32 _spellTypeMask;
    uint32 _spellPhaseMask;
    uint32 _hitMask;
    Spell* _spell;
    DamageInfo* _damageInfo;
    HealInfo* _healInfo;
};

// Struct for use in Unit::CalculateMeleeDamage
// Need create structure like in SMSG_ATTACKER_STATE_UPDATE opcode
struct CalcDamageInfo
{
    CalcDamageInfo() : attacker(NULL), target(NULL), damageSchoolMask(0), damage(0), absorb(0), resist(0), blocked_amount(0),
        HitInfo(0), TargetState(VictimState::VICTIMSTATE_MISS), attackType(WeaponAttackType::BASE_ATTACK), procAttacker(0),
        procVictim(0), procEx(0), cleanDamage(0), hitOutCome(MeleeHitOutcome::MELEE_HIT_EVADE) { }

    Unit* attacker;             // Attacker
    Unit* target;               // Target for damage
    uint32 damageSchoolMask;
    uint32 damage;
    uint32 absorb;
    uint32 resist;
    uint32 blocked_amount;
    uint32 HitInfo;
    VictimState TargetState;
    // Helper
    WeaponAttackType attackType; //
    uint32 procAttacker;
    uint32 procVictim;
    uint32 procEx;
    uint32 cleanDamage;          // Used only for rage calculation
    MeleeHitOutcome hitOutCome;  /// @todo remove this field (need use TargetState)
};

// Spell damage info structure based on structure sending in SMSG_SPELL_NON_MELEE_DAMAGE_LOG opcode
struct SpellNonMeleeDamage
{
    SpellNonMeleeDamage(Unit* _attacker, Unit* _target, uint32 _SpellID, uint32 _schoolMask)
        : target(_target), attacker(_attacker), SpellID(_SpellID), damage(0), overkill(0), schoolMask(_schoolMask),
        absorb(0), resist(0), physicalLog(false), unused(false), blocked(0), HitInfo(0), cleanDamage(0)
    { }

    Unit* target;
    Unit* attacker;
    uint32 SpellID;
    uint32 damage;
    uint32 overkill;
    uint32 schoolMask;
    uint32 absorb;
    uint32 resist;
    bool   physicalLog;
    bool   unused;
    uint32 blocked;
    uint32 HitInfo;
    // Used for help
    uint32 cleanDamage;
};

struct SpellPeriodicAuraLogInfo
{
    SpellPeriodicAuraLogInfo(AuraEffect const* _auraEff, uint32 _damage, uint32 _overDamage, uint32 _absorb, uint32 _resist, uint32 _power, float _multiplier, bool _critical)
        : auraEff(_auraEff), damage(_damage), overDamage(_overDamage), absorb(_absorb), resist(_resist), power(_power), multiplier(_multiplier), critical(_critical)
    { }

    AuraEffect const* auraEff;
    uint32 damage;
    uint32 overDamage;                                      // overkill/overheal
    uint32 absorb;
    uint32 resist;
    uint32 power;
    float  multiplier;
    bool   critical;
};

uint32 createProcExtendMask(SpellNonMeleeDamage* damageInfo, SpellMissInfo missCondition);

struct RedirectThreatInfo
{
    RedirectThreatInfo() : _targetGUID(0), _threatPct(0)
    { }
    uint64 _targetGUID;
    uint32 _threatPct;

    uint64 GetTargetGUID() const
    {
        return _targetGUID;
    }
    uint32 GetThreatPct() const
    {
        return _threatPct;
    }

    void Set(uint64 guid, uint32 pct)
    {
        _targetGUID = guid;
        _threatPct = pct;
    }

    void ModifyThreatPct(int32 amount)
    {
        amount += _threatPct;
        _threatPct = uint32(std::max(0, amount));
    }
};

#define MAX_DECLINED_NAME_CASES 5

struct DeclinedName
{
    std::string name[MAX_DECLINED_NAME_CASES];
    uint32 nameLength[MAX_DECLINED_NAME_CASES];
};

enum CurrentSpellTypes
{
    CURRENT_MELEE_SPELL = 0,
    CURRENT_GENERIC_SPELL = 1,
    CURRENT_CHANNELED_SPELL = 2,
    CURRENT_AUTOREPEAT_SPELL = 3
};

#define CURRENT_FIRST_NON_MELEE_SPELL 1
#define CURRENT_MAX_SPELL             4

struct GlobalCooldown
{
    explicit GlobalCooldown(uint32 _dur = 0, uint32 _time = 0) : duration(_dur), cast_time(_time)
    { }

    uint32 duration;
    uint32 cast_time;
};

typedef UNORDERED_MAP<uint32 /*category*/, GlobalCooldown> GlobalCooldownList;

class GlobalCooldownMgr                                     // Shared by Player and CharmInfo
{
public:
    GlobalCooldownMgr()
    { }

public:
    bool HasGlobalCooldown(SpellInfo const* spellInfo) const;
    void AddGlobalCooldown(SpellInfo const* spellInfo, uint32 gcd);
    void CancelGlobalCooldown(SpellInfo const* spellInfo);

private:
    GlobalCooldownList m_GlobalCooldowns;
};

enum ActiveStates
{
    ACT_PASSIVE = 0x01,                                    // 0x01 - passive
    ACT_ASSIST = 0x03,                                    // 0x03 - assist
    ACT_DISABLED = 0x81,                                    // 0x80 - castable
    ACT_ENABLED = 0xC1,                                    // 0x40 | 0x80 - auto cast + castable
    ACT_COMMAND = 0x07,                                    // 0x01 | 0x02 | 0x04
    ACT_REACTION = 0x06,                                    // 0x02 | 0x04
    ACT_DECIDE = 0x00                                     // custom
};

enum ReactStates
{
    REACT_PASSIVE = 0,
    REACT_DEFENSIVE = 1,
    REACT_AGGRESSIVE = 2,
    REACT_ASSIST = 3
};

enum CommandStates
{
    COMMAND_STAY = 0,
    COMMAND_FOLLOW = 1,
    COMMAND_ATTACK = 2,
    COMMAND_ABANDON = 3,
    COMMAND_MOVE_TO = 4
};

#define UNIT_ACTION_BUTTON_ACTION(X) (uint32(X) & 0x00FFFFFF)
#define UNIT_ACTION_BUTTON_TYPE(X)   ((uint32(X) & 0xFF000000) >> 24)
#define MAKE_UNIT_ACTION_BUTTON(A, T) (uint32(A) | (uint32(T) << 24))

struct UnitActionBarEntry
{
    UnitActionBarEntry() : packedData(uint32(ACT_DISABLED) << 24)
    { }

    uint32 packedData;

    // helper
    ActiveStates GetType() const
    {
        return ActiveStates(UNIT_ACTION_BUTTON_TYPE(packedData));
    }
    uint32 GetAction() const
    {
        return UNIT_ACTION_BUTTON_ACTION(packedData);
    }
    bool IsActionBarForSpell() const
    {
        ActiveStates Type = GetType();
        return Type == ACT_DISABLED || Type == ACT_ENABLED || Type == ACT_PASSIVE;
    }

    void SetActionAndType(uint32 action, ActiveStates type)
    {
        packedData = MAKE_UNIT_ACTION_BUTTON(action, type);
    }

    void SetType(ActiveStates type)
    {
        packedData = MAKE_UNIT_ACTION_BUTTON(UNIT_ACTION_BUTTON_ACTION(packedData), type);
    }

    void SetAction(uint32 action)
    {
        packedData = (packedData & 0xFF000000) | UNIT_ACTION_BUTTON_ACTION(action);
    }
};

typedef std::list<Player*> SharedVisionList;

enum CharmType
{
    CHARM_TYPE_CHARM,
    CHARM_TYPE_POSSESS,
    CHARM_TYPE_VEHICLE,
    CHARM_TYPE_CONVERT
};

typedef UnitActionBarEntry CharmSpellInfo;

enum ActionBarIndex
{
    ACTION_BAR_INDEX_START = 0,
    ACTION_BAR_INDEX_PET_SPELL_START = 3,
    ACTION_BAR_INDEX_PET_SPELL_END = 7,
    ACTION_BAR_INDEX_END = 10
};

#define MAX_UNIT_ACTION_BAR_INDEX (ACTION_BAR_INDEX_END-ACTION_BAR_INDEX_START)

struct CharmInfo
{
public:
    explicit CharmInfo(Unit* unit);
    ~CharmInfo();
    void RestoreState();
    uint32 GetPetNumber() const
    {
        return _petnumber;
    }
    void SetPetNumber(uint32 petnumber, bool statwindow);

    void SetCommandState(CommandStates st)
    {
        _CommandState = st;
    }
    CommandStates GetCommandState() const
    {
        return _CommandState;
    }
    bool HasCommandState(CommandStates state) const
    {
        return (_CommandState == state);
    }

    void InitPossessCreateSpells();
    void InitCharmCreateSpells();
    void InitPetActionBar();
    void InitEmptyActionBar(bool withAttack = true);

    //return true if successful
    bool AddSpellToActionBar(SpellInfo const* spellInfo, ActiveStates newstate = ACT_DECIDE, uint8 preferredSlot = 0);
    bool RemoveSpellFromActionBar(uint32 spell_id);
    void LoadPetActionBar(const std::string& data);
    void BuildActionBar(WorldPacket* data) const;
    void SetSpellAutocast(SpellInfo const* spellInfo, bool state);
    void SetActionBar(uint8 index, uint32 spellOrAction, ActiveStates type)
    {
        PetActionBar[index].SetActionAndType(spellOrAction, type);
    }
    UnitActionBarEntry const* GetActionBarEntry(uint8 index) const
    {
        return &(PetActionBar[index]);
    }

    void ToggleCreatureAutocast(SpellInfo const* spellInfo, bool apply);

    CharmSpellInfo* GetCharmSpell(uint8 index)
    {
        return &(_charmspells[index]);
    }

    GlobalCooldownMgr& GetGlobalCooldownMgr()
    {
        return m_GlobalCooldownMgr;
    }

    void SetIsCommandAttack(bool val);
    bool IsCommandAttack() const { return _isCommandAttack; }
    void SetIsCommandFollow(bool val);
    bool IsCommandFollow() const { return _isCommandFollow; }
    void SetIsAtStay(bool val);
    bool IsAtStay() const { return _isAtStay; }
    void SetIsFollowing(bool val);
    bool IsFollowing() const { return _isFollowing; }
    void SetIsReturning(bool val);
    bool IsReturning() const { return _isReturning; }
    void SaveStayPosition();
    void GetStayPosition(float& x, float& y, float& z) const;

private:
    Unit* _unit;
    UnitActionBarEntry PetActionBar[MAX_UNIT_ACTION_BAR_INDEX];
    CharmSpellInfo _charmspells[4];
    CommandStates _CommandState;
    uint32 _petnumber;
    bool _barInit;

    //for restoration after charmed
    ReactStates     _oldReactState;

    bool _isCommandAttack;
    bool _isCommandFollow;
    bool _isAtStay;
    bool _isFollowing;
    bool _isReturning;
    float _stayX;
    float _stayY;
    float _stayZ;

    GlobalCooldownMgr m_GlobalCooldownMgr;
};

// for clearing special attacks
#define REACTIVE_TIMER_START 4000

enum ReactiveType
{
    REACTIVE_DEFENSE = 0,
    REACTIVE_HUNTER_PARRY = 1,
    REACTIVE_OVERPOWER = 2
};

#define MAX_REACTIVE 3
#define SUMMON_SLOT_PET     0
#define SUMMON_SLOT_TOTEM   1
#define MAX_TOTEM_SLOT      5
#define SUMMON_SLOT_MINIPET 5
#define SUMMON_SLOT_QUEST   6
#define MAX_SUMMON_SLOT     7

#define MAX_GAMEOBJECT_SLOT 4

enum PlayerTotemType
{
    SUMMON_TYPE_TOTEM_FIRE = 63,
    SUMMON_TYPE_TOTEM_EARTH = 81,
    SUMMON_TYPE_TOTEM_WATER = 82,
    SUMMON_TYPE_TOTEM_AIR = 83,

    SUMMON_TYPE_LIGHTSPRING = 1141,
    SUMMON_TYPE_STATUE_JADE = 3216,
    SUMMON_TYPE_STATUE_OX = 3223
};

// delay time next attack to prevent client attack animation problems
#define ATTACK_DISPLAY_DELAY 200
#define MAX_PLAYER_STEALTH_DETECT_RANGE 30.0f               // max distance for detection targets by player

struct SpellProcEventEntry;                                 // used only privately

class Unit : public WorldObject
{
public:
    typedef std::set<Unit*> AttackerSet;
    typedef std::set<Unit*> ControlList;

    typedef std::multimap<uint32, Aura*> AuraMap;
    typedef std::pair<AuraMap::const_iterator, AuraMap::const_iterator> AuraMapBounds;
    typedef std::pair<AuraMap::iterator, AuraMap::iterator> AuraMapBoundsNonConst;

    typedef std::multimap<uint32, AuraApplication*> AuraApplicationMap;
    typedef std::pair<AuraApplicationMap::const_iterator, AuraApplicationMap::const_iterator> AuraApplicationMapBounds;
    typedef std::pair<AuraApplicationMap::iterator, AuraApplicationMap::iterator> AuraApplicationMapBoundsNonConst;

    typedef std::multimap<AuraStateType, AuraApplication*> AuraStateAurasMap;
    typedef std::pair<AuraStateAurasMap::const_iterator, AuraStateAurasMap::const_iterator> AuraStateAurasMapBounds;

    typedef std::list<AuraEffect*> AuraEffectList;
    typedef std::list<Aura*> AuraList;
    typedef std::list<AuraApplication*> AuraApplicationList;
    typedef std::list<DiminishingReturn> Diminishing;
    typedef std::set<uint32> ComboPointHolderSet;

    typedef std::map<uint8, AuraApplication*> VisibleAuraMap;

    virtual ~Unit();

    UnitAI* GetAI()
    {
        return i_AI;
    }
    void SetAI(UnitAI* newAI)
    {
        i_AI = newAI;
    }

    void AddToWorld() override;
    void RemoveFromWorld() override;

    void CleanupBeforeRemoveFromMap(bool finalCleanup);
    void CleanupsBeforeDelete(bool finalCleanup = true) override;                        // used in ~Creature/~Player (or before mass creature delete to remove cross-references to already deleted units)

    DiminishingLevels GetDiminishing(DiminishingGroup  group);
    void IncrDiminishing(DiminishingGroup group);
    float ApplyDiminishingToDuration(DiminishingGroup  group, int32& duration, Unit* caster, DiminishingLevels Level, int32 limitduration);
    void ApplyDiminishingAura(DiminishingGroup  group, bool apply);
    void ClearDiminishings()
    {
        m_Diminishing.clear();
    }

    // target dependent range checks
    float GetSpellMaxRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const;
    float GetSpellMinRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const;

    void Update(uint32 time) OVERRIDE;

    void setAttackTimer(WeaponAttackType type, uint32 time)
    {
        m_attackTimer[uint8(type)] = time;
    }
    void resetAttackTimer(WeaponAttackType type = WeaponAttackType::BASE_ATTACK);
    uint32 getAttackTimer(WeaponAttackType type) const
    {
        return m_attackTimer[uint8(type)];
    }
    bool isAttackReady(WeaponAttackType type = WeaponAttackType::BASE_ATTACK) const
    {
        return m_attackTimer[uint8(type)] == 0;
    }
    bool haveOffhandWeapon() const;
    bool CanDualWield() const
    {
        return m_canDualWield;
    }
    void SetCanDualWield(bool value)
    {
        m_canDualWield = value;
    }
    float GetCombatReach() const
    {
        return m_floatValues[UNIT_FIELD_COMBAT_REACH];
    }
    float GetMeleeReach() const;
    bool IsWithinCombatRange(const Unit* obj, float dist2compare) const;
    bool IsWithinMeleeRange(const Unit* obj, float dist = MELEE_RANGE) const;
    void GetRandomContactPoint(const Unit* target, float& x, float& y, float& z, float distance2dMin, float distance2dMax) const;
    uint32 m_extraAttacks;
    bool m_canDualWield;

    void _addAttacker(Unit* pAttacker);                  // must be called only from Unit::Attack(Unit*)
    void _removeAttacker(Unit* pAttacker);               // must be called only from Unit::AttackStop()
    Unit* getAttackerForHelper() const;                 // If someone wants to help, who to give them
    bool Attack(Unit* victim, bool meleeAttack);
    void CastStop(uint32 except_spellid = 0);
    bool AttackStop();
    void RemoveAllAttackers();
    AttackerSet const& getAttackers() const
    {
        return m_attackers;
    }
    bool isAttackingPlayer() const;
    Unit* GetVictim() const
    {
        return m_attacking;
    }

    void CombatStop(bool includingCast = false);
    void CombatStopWithPets(bool includingCast = false);
    void StopAttackFaction(uint32 faction_id);
    void GetAttackableUnitListInRange(std::list<Unit*>& list, float fMaxSearchRange) const;
    Unit* SelectNearbyTarget(Unit* exclude = NULL, float dist = NOMINAL_MELEE_RANGE) const;
    void SendMeleeAttackStop(Unit* victim = NULL);
    void SendMeleeAttackStart(Unit* victim);

    void AddUnitState(uint32 f)
    {
        m_state |= f;
    }
    bool HasUnitState(const uint32 f) const
    {
        return (m_state & f);
    }
    void ClearUnitState(uint32 f)
    {
        m_state &= ~f;
    }
    bool CanFreeMove() const;

    uint32 HasUnitTypeMask(uint32 mask) const
    {
        return mask & m_unitTypeMask;
    }
    void AddUnitTypeMask(uint32 mask)
    {
        m_unitTypeMask |= mask;
    }
    bool IsSummon() const
    {
        return m_unitTypeMask & UNIT_MASK_SUMMON;
    }
    bool IsGuardian() const
    {
        return m_unitTypeMask & UNIT_MASK_GUARDIAN;
    }
    bool IsPet() const
    {
        return m_unitTypeMask & UNIT_MASK_PET;
    }
    bool IsHunterPet() const
    {
        return m_unitTypeMask & UNIT_MASK_HUNTER_PET;
    }
    bool IsTotem() const
    {
        return m_unitTypeMask & UNIT_MASK_TOTEM;
    }
    bool IsVehicle() const
    {
        return m_unitTypeMask & UNIT_MASK_VEHICLE;
    }

    uint8 getLevel() const
    {
        return uint8(GetUInt32Value(UNIT_FIELD_LEVEL));
    }
    uint8 getLevelForTarget(WorldObject const* /*target*/) const override { return getLevel(); }
    void SetLevel(uint8 lvl);

    uint8 getRace() const
    {
        return GetByteValue(UNIT_FIELD_SEX, 0);
    }
    uint32 getRaceMask() const
    {
        return 1 << (getRace() - 1);
    }
    uint8 getClass() const
    {
        return GetByteValue(UNIT_FIELD_SEX, 1);
    }
    uint32 getClassMask() const
    {
        return 1 << (getClass() - 1);
    }
    uint8 getGender() const
    {
        return GetByteValue(UNIT_FIELD_SEX, 3);
    }
    uint32 getVirtualRealm() const
    {
        return uint8(GetUInt32Value(PLAYER_FIELD_VIRTUAL_PLAYER_REALM));
    }

    void SetRace(uint8 race)
    {
        SetByteValue(UNIT_FIELD_SEX, 0, race);
    }
    void SetClass(uint8 newClass)
    {
        SetByteValue(UNIT_FIELD_SEX, 1, newClass);
    }
    void SetGender(uint8 gender)
    {
        SetByteValue(UNIT_FIELD_SEX, 3, gender);
    }

    float GetStat(Stats stat) const
    {
        return float(GetUInt32Value(UNIT_FIELD_STATS + EUnitFields(stat)));
    }
    void SetStat(Stats stat, int32 val)
    {
        SetStatInt32Value(UNIT_FIELD_STATS + EUnitFields(stat), val);
    }
    uint32 GetArmor() const
    {
        return GetResistance(SPELL_SCHOOL_NORMAL);
    }
    void SetArmor(int32 val)
    {
        SetResistance(SPELL_SCHOOL_NORMAL, val);
    }

    uint32 GetResistance(SpellSchools school) const
    {
        return GetUInt32Value(UNIT_FIELD_RESISTANCES + EUnitFields(school));
    }
    uint32 GetResistance(SpellSchoolMask mask) const;
    void SetResistance(SpellSchools school, int32 val)
    {
        SetStatInt32Value(UNIT_FIELD_RESISTANCES + EUnitFields(school), val);
    }

    uint32 GetHealth()    const
    {
        return GetUInt32Value(UNIT_FIELD_HEALTH);
    }
    uint32 GetMaxHealth() const
    {
        return GetUInt32Value(UNIT_FIELD_MAX_HEALTH);
    }

    bool IsFullHealth() const
    {
        return GetHealth() == GetMaxHealth();
    }
    bool HealthBelowPct(int32 pct) const
    {
        return GetHealth() < CountPctFromMaxHealth(pct);
    }
    bool HealthBelowPctDamaged(int32 pct, uint32 damage) const
    {
        return int64(GetHealth()) - int64(damage) < int64(CountPctFromMaxHealth(pct));
    }
    bool HealthAbovePct(int32 pct) const
    {
        return GetHealth() > CountPctFromMaxHealth(pct);
    }
    bool HealthAbovePctHealed(int32 pct, uint32 heal) const
    {
        return uint64(GetHealth()) + uint64(heal) > CountPctFromMaxHealth(pct);
    }
    float GetHealthPct() const
    {
        return GetMaxHealth() ? 100.f * GetHealth() / GetMaxHealth() : 0.0f;
    }
    uint32 CountPctFromMaxHealth(int32 pct) const
    {
        return CalculatePct(GetMaxHealth(), pct);
    }
    uint32 CountPctFromCurHealth(int32 pct) const
    {
        return CalculatePct(GetHealth(), pct);
    }

    void SetHealth(uint32 val);
    void SetMaxHealth(uint32 val);
    inline void SetFullHealth()
    {
        SetHealth(GetMaxHealth());
    }
    int32 ModifyHealth(int32 val);
    int32 GetHealthGain(int32 dVal);

    Powers getPowerType() const
    {
        return Powers(GetUInt32Value(UNIT_FIELD_DISPLAY_POWER));
    }
    void SetFieldPowerType(uint32 powerType)
    {
        SetUInt32Value(UNIT_FIELD_DISPLAY_POWER, powerType);
    }

    void setPowerType(Powers power);
    int32 GetPower(Powers power) const;
    int32 GetMinPower(Powers power) const
    {
        return power == POWER_ECLIPSE ? -100 : 0;
    }
    int32 GetMaxPower(Powers power) const;
    int32 CountPctFromMaxPower(Powers power, int32 pct) const
    {
        return CalculatePct(GetMaxPower(power), pct);
    }
    void SetPower(Powers power, int32 val);
    void SetMaxPower(Powers power, int32 val);
    // returns the change in power
    int32 ModifyPower(Powers power, int32 val);
    int32 ModifyPowerPct(Powers power, float pct, bool apply = true);

    uint32 GetAttackTime(WeaponAttackType att) const;
    void SetAttackTime(WeaponAttackType att, uint32 val)
    {
        SetFloatValue(UNIT_FIELD_ATTACK_ROUND_BASE_TIME + uint8(att), val * m_modAttackSpeedPct[uint8(att)]);
    }
    void ApplyAttackTimePercentMod(WeaponAttackType att, float val, bool apply);
    void ApplyCastTimePercentMod(float val, bool apply);

    SheathState GetSheath() const
    {
        return SheathState(GetByteValue(UNIT_FIELD_SHAPESHIFT_FORM, 0));
    }
    virtual void SetSheath(SheathState sheathed)
    {
        SetByteValue(UNIT_FIELD_SHAPESHIFT_FORM, 0, sheathed);
    }

    // faction template id
    uint32 getFaction() const
    {
        return GetUInt32Value(UNIT_FIELD_FACTION_TEMPLATE);
    }
    void setFaction(uint32 faction)
    {
        SetUInt32Value(UNIT_FIELD_FACTION_TEMPLATE, faction);
    }
    FactionTemplateEntry const* GetFactionTemplateEntry() const;

    ReputationRank GetReactionTo(Unit const* target) const;
    ReputationRank static GetFactionReactionTo(FactionTemplateEntry const* factionTemplateEntry, Unit const* target);

    bool IsHostileTo(Unit const* unit) const;
    bool IsHostileToPlayers() const;
    bool IsFriendlyTo(Unit const* unit) const;
    bool IsNeutralToAll() const;
    bool IsInPartyWith(Unit const* unit) const;
    bool IsInRaidWith(Unit const* unit) const;
    void GetPartyMembers(std::list<Unit*>& units);
    bool IsContestedGuard() const;
    bool IsPvP() const
    {
        return HasByteFlag(UNIT_FIELD_SHAPESHIFT_FORM, 1, UNIT_BYTE2_FLAG_PVP);
    }
    void SetPvP(bool state);
    uint32 GetCreatureType() const;
    uint32 GetCreatureTypeMask() const;

    uint8 getStandState() const
    {
        return GetByteValue(UNIT_FIELD_ANIM_TIER, 0);
    }
    bool IsSitState() const;
    bool IsStandState() const;
    void SetStandState(uint8 state);

    void  SetStandFlags(uint8 flags)
    {
        SetByteFlag(UNIT_FIELD_ANIM_TIER, 2, flags);
    }
    void  RemoveStandFlags(uint8 flags)
    {
        RemoveByteFlag(UNIT_FIELD_ANIM_TIER, 2, flags);
    }

    bool IsMounted() const
    {
        return HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_MOUNT);
    }
    uint32 GetMountID() const
    {
        return GetUInt32Value(UNIT_FIELD_MOUNT_DISPLAY_ID);
    }
    void Mount(uint32 mount, uint32 vehicleId = 0, uint32 creatureEntry = 0);
    void Dismount();
    MountCapabilityEntry const* GetMountCapability(uint32 mountType) const;

    void SendDurabilityLoss(Player* receiver, uint32 percent);
    void PlayOneShotAnimKit(uint32 id);

    uint16 GetMaxSkillValueForLevel(Unit const* target = NULL) const
    {
        return (target ? getLevelForTarget(target) : getLevel()) * 5;
    }
    void DealDamageMods(Unit* victim, uint32& damage, uint32* absorb);
    uint32 DealDamage(Unit* victim, uint32 damage, CleanDamage const* cleanDamage = NULL, DamageEffectType damagetype = DIRECT_DAMAGE, SpellSchoolMask damageSchoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* spellProto = NULL, bool durabilityLoss = true);
    void Kill(Unit* victim, bool durabilityLoss = true);
    int32 DealHeal(Unit* victim, uint32 addhealth);

    void ProcDamageAndSpell(Unit* victim, uint32 procAttacker, uint32 procVictim, uint32 procEx, uint32 amount, WeaponAttackType attType = WeaponAttackType::BASE_ATTACK, SpellInfo const* procSpell = NULL, SpellInfo const* procAura = NULL);
    void ProcDamageAndSpellFor(bool isVictim, Unit* target, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, SpellInfo const* procSpell, uint32 damage, SpellInfo const* procAura = NULL);

    void GetProcAurasTriggeredOnEvent(AuraApplicationList& aurasTriggeringProc, AuraApplicationList* procAuras, ProcEventInfo eventInfo);
    void TriggerAurasProcOnEvent(CalcDamageInfo& damageInfo);
    void TriggerAurasProcOnEvent(AuraApplicationList* myProcAuras, AuraApplicationList* targetProcAuras,
        Unit* actionTarget, uint32 typeMaskActor, uint32 typeMaskActionTarget,
        uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask, Spell* spell,
        DamageInfo* damageInfo, HealInfo* healInfo);
    void TriggerAurasProcOnEvent(ProcEventInfo& eventInfo, AuraApplicationList& procAuras);

    void HandleEmoteCommand(uint32 anim_id);
    void AttackerStateUpdate(Unit* victim, WeaponAttackType attType = WeaponAttackType::BASE_ATTACK, bool extra = false);

    void CalculateMeleeDamage(Unit* victim, uint32 damage, CalcDamageInfo* damageInfo, WeaponAttackType attackType = WeaponAttackType::BASE_ATTACK);
    void DealMeleeDamage(CalcDamageInfo* damageInfo, bool durabilityLoss);
    void HandleProcExtraAttackFor(Unit* victim);

    void CalculateSpellDamageTaken(SpellNonMeleeDamage* damageInfo, int32 damage, SpellInfo const* spellInfo, WeaponAttackType attackType = WeaponAttackType::BASE_ATTACK, bool crit = false);
    void DealSpellDamage(SpellNonMeleeDamage* damageInfo, bool durabilityLoss);

    // player or player's pet resilience (-1%)
    uint32 GetCritDamageReduction(uint32 damage) const
    {
        return GetCombatRatingDamageReduction(CombatRating::CR_RESILIENCE_CRIT_TAKEN, 2.2f, 33.0f, damage);
    }
    uint32 GetDamageReduction(uint32 damage) const
    {
        return GetCombatRatingDamageReduction(CombatRating::CR_RESILIENCE_PLAYER_DAMAGE_TAKEN, 2.0f, 100.0f, damage);
    }

    void ApplyResilience(Unit const* victim, int32* damage, bool isCrit) const;

    float MeleeSpellMissChance(Unit const* victim, WeaponAttackType attType, uint32 spellId) const;
    SpellMissInfo MeleeSpellHitResult(Unit* victim, SpellInfo const* spellInfo);
    SpellMissInfo MagicSpellHitResult(Unit* victim, SpellInfo const* spellInfo);
    SpellMissInfo SpellHitResult(Unit* victim, SpellInfo const* spellInfo, bool canReflect = false);

    float GetUnitDodgeChance() const;
    float GetUnitParryChance() const;
    float GetUnitBlockChance() const;
    float GetUnitMissChance(WeaponAttackType attType) const;
    float GetUnitCriticalChance(WeaponAttackType attackType, const Unit* victim) const;
    int32 GetMechanicResistChance(SpellInfo const* spellInfo) const;
    bool CanUseAttackType(WeaponAttackType attacktype) const;

    virtual uint32 GetBlockPercent() const
    {
        return 30;
    }

    uint32 GetUnitMeleeSkill(Unit const* target = NULL) const;

    float GetWeaponProcChance() const;
    float GetPPMProcChance(uint32 WeaponSpeed, float PPM, const SpellInfo* spellProto) const;

    MeleeHitOutcome RollMeleeOutcomeAgainst(const Unit* victim, WeaponAttackType attType) const;
    MeleeHitOutcome RollMeleeOutcomeAgainst(const Unit* victim, WeaponAttackType attType, int32 crit_chance, int32 miss_chance, int32 dodge_chance, int32 parry_chance, int32 block_chance) const;

    bool IsVendor()       const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_VENDOR);
    }
    bool IsTrainer()      const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_TRAINER);
    }
    bool IsQuestGiver()   const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
    }
    bool IsGossip()       const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
    }
    bool IsTaxi()         const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_FLIGHTMASTER);
    }
    bool IsGuildMaster()  const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_PETITIONER);
    }
    bool IsBattleMaster() const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_BATTLEMASTER);
    }
    bool IsBanker()       const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_BANKER);
    }
    bool IsInnkeeper()    const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_INNKEEPER);
    }
    bool IsSpiritHealer() const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPIRITHEALER);
    }
    bool IsSpiritGuide()  const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPIRITGUIDE);
    }
    bool IsTabardDesigner()const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_TABARDDESIGNER);
    }
    bool IsAuctioner()    const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_AUCTIONEER);
    }
    bool IsArmorer()      const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_REPAIR);
    }
    bool IsServiceProvider() const;
    bool IsSpiritService() const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPIRITHEALER | UNIT_NPC_FLAG_SPIRITGUIDE);
    }
    bool IsBMAuctioner() const
    {
        return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_BMAH);
    }

    bool IsInFlight()  const
    {
        return HasUnitState(UNIT_STATE_IN_FLIGHT);
    }

    bool IsInCombat()  const
    {
        return HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
    }
    void CombatStart(Unit* target, bool initialAggro = true);
    void SetInCombatState(bool PvP, Unit* enemy = NULL);
    void SetInCombatWith(Unit* enemy);
    void ClearInCombat();
    uint32 GetCombatTimer() const
    {
        return m_CombatTimer;
    }

    bool HasAuraTypeWithFamilyFlags(AuraType auraType, uint32 familyName, uint32 familyFlags) const;
    bool virtual HasSpell(uint32 /*spellID*/) const
    {
        return false;
    }
    bool HasBreakableByDamageAuraType(AuraType type, uint32 excludeAura = 0) const;
    bool HasBreakableByDamageCrowdControlAura(Unit* excludeCasterChannel = NULL) const;

    bool HasStealthAura()      const
    {
        return HasAuraType(SPELL_AURA_MOD_STEALTH);
    }
    bool HasInvisibilityAura() const
    {
        return HasAuraType(SPELL_AURA_MOD_INVISIBILITY);
    }
    bool isFeared()  const
    {
        return HasAuraType(SPELL_AURA_MOD_FEAR);
    }
    bool isInRoots() const
    {
        return HasAuraType(SPELL_AURA_MOD_ROOT);
    }
    bool IsPolymorphed() const;

    bool isFrozen() const;

    bool isTargetableForAttack(bool checkFakeDeath = true) const;

    bool IsValidAttackTarget(Unit const* target) const;
    bool _IsValidAttackTarget(Unit const* target, SpellInfo const* bySpell, WorldObject const* obj = NULL) const;

    bool IsValidAssistTarget(Unit const* target) const;
    bool _IsValidAssistTarget(Unit const* target, SpellInfo const* bySpell) const;

    virtual bool IsInWater() const;
    virtual bool IsUnderWater() const;
    virtual void UpdateUnderwaterState(Map* m, float x, float y, float z);
    bool isInAccessiblePlaceFor(Creature const* c) const;

    void SendHealSpellLog(ObjectGuid CasterGUID, ObjectGuid TargetGUID, uint32 SpellID, uint32 Damage, uint32 OverHeal, uint32 Absorb, bool critical = false);
    int32 HealBySpell(Unit* victim, SpellInfo const* spellInfo, uint32 addHealth, bool critical = false);
    void SendEnergizeSpellLog(Unit* victim, uint32 spellID, int32 damage, Powers powertype);
    void EnergizeBySpell(Unit* victim, uint32 SpellID, int32 Damage, Powers powertype);
    uint32 SpellNonMeleeDamageLog(Unit* victim, uint32 spellID, uint32 damage);

    void CastSpell(SpellCastTargets const& targets, SpellInfo const* spellInfo, CustomSpellValues const* value, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastSpell(Unit* victim, uint32 spellId, bool triggered, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastSpell(Unit* victim, uint32 spellId, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastSpell(Unit* victim, SpellInfo const* spellInfo, bool triggered, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastSpell(Unit* victim, SpellInfo const* spellInfo, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastSpell(float x, float y, float z, uint32 spellId, bool triggered, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastSpell(GameObject* go, uint32 spellId, bool triggered, Item* castItem = NULL, AuraEffect* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastCustomSpell(Unit* victim, uint32 spellId, int32 const* bp0, int32 const* bp1, int32 const* bp2, bool triggered, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastCustomSpell(uint32 spellId, SpellValueMod mod, int32 value, Unit* victim, bool triggered, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastCustomSpell(uint32 spellId, SpellValueMod mod, int32 value, Unit* victim = NULL, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    void CastCustomSpell(uint32 spellId, CustomSpellValues const& value, Unit* victim = NULL, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = NULL, AuraEffect const* triggeredByAura = NULL, uint64 originalCaster = 0);
    Aura* AddAura(uint32 spellId, Unit* target);
    Aura* AddAura(SpellInfo const* spellInfo, uint32 effMask, Unit* target);
    void SetAuraStack(uint32 spellId, Unit* target, uint32 stack);
    void SendPlaySpellVisualKit(uint32 SpellVisualId, uint32 Duration, int32 Type);
    void SendPlaySpellVisual(uint32 SpellVisualId, float x, float y, float z, float orientation, uint8 SpeedTime, uint16 MissReason, uint16 ReflectStatus);

    void DeMorph();

    void SendAttackStateUpdate(CalcDamageInfo* damageInfo);
    void SendAttackStateUpdate(uint32 HitInfo, Unit* target, uint8 SwingType, SpellSchoolMask damageSchoolMask, uint32 Damage, uint32 AbsorbDamage, uint32 Resist, VictimState TargetState, uint32 BlockedAmount);
    void SendSpellNonMeleeDamageLog(SpellNonMeleeDamage* log);
    void SendSpellNonMeleeDamageLog(Unit* target, uint32 SpellID, uint32 Damage, SpellSchoolMask damageSchoolMask, uint32 AbsorbedDamage, uint32 Resist, bool PhysicalDamage, uint32 Blocked, bool CriticalHit = false);
    void SendPeriodicAuraLog(SpellPeriodicAuraLogInfo* pInfo);
    void SendSpellMiss(Unit* target, uint32 spellID, SpellMissInfo missInfo);
    void SendSpellDamageResist(Unit* target, uint32 spellId);
    void SendSpellDamageImmune(Unit* target, uint32 spellId);

    void NearTeleportTo(float x, float y, float z, float orientation, bool casting = false);
    void SendTeleportPacket(Position& pos);
    virtual bool UpdatePosition(float x, float y, float z, float ang, bool teleport = false);
    // returns true if unit's position really changed
    bool UpdatePosition(const Position& pos, bool teleport = false);
    void UpdateOrientation(float orientation);
    void UpdateHeight(float newZ);

    void SendMoveKnockBack(Player* player, float speedXY, float speedZ, float vcos, float vsin);
    void KnockbackFrom(float x, float y, float speedXY, float speedZ);
    void JumpTo(float speedXY, float speedZ, bool forward = true);
    void JumpTo(WorldObject* obj, float speedZ);

    void MonsterMoveWithSpeed(float x, float y, float z, float speed, bool generatePath = false, bool forceDestination = false);


    //void SendSetPlayHoverAnim(bool PlayHoverAnim);
    //void SendMovementSetSplineAnim(Movement::AnimType anim);

    bool IsLevitating() const
    {
        return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
    }
    bool IsWalking() const
    {
        return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING);
    }
    bool SetWalk(bool enable);
    bool SetDisableGravity(bool disable, bool packetOnly = false);
    bool SetFall(bool enable);
    bool SetSwim(bool enable);
    bool SetCanFly(bool enable);
    bool SetWaterWalking(bool enable, bool packetOnly = false);
    bool SetFeatherFall(bool enable, bool packetOnly = false);
    bool SetHover(bool enable, bool packetOnly = false);
    void SendSetVehicleRecId(uint32 vehicleId);

    void SetInFront(WorldObject const* target);
    void SetFacingTo(float ori);
    void SetFacingToObject(WorldObject* object);

    void SendChangeCurrentVictimOpcode(HostileReference* pHostileReference);
    void SendClearThreatListOpcode(ObjectGuid UnitGUID);
    void SendRemoveFromThreatListOpcode(HostileReference* pHostileReference);
    void SendThreatListUpdate();

    void SendClearTarget();

    bool IsAlive() const
    {
        return (m_deathState == DeathState::ALIVE);
    }
    bool isDying() const
    {
        return (m_deathState == DeathState::JUST_DIED);
    }
    bool isDead() const
    {
        return (m_deathState == DeathState::DEAD || m_deathState == DeathState::CORPSE);
    }
    DeathState getDeathState() const
    {
        return m_deathState;
    }
    virtual void setDeathState(DeathState s);           // overwrited in Creature/Player/Pet

    uint64 GetOwnerGUID() const
    {
        return GetUInt64Value(UNIT_FIELD_SUMMONED_BY);
    }
    void SetOwnerGUID(uint64 owner);
    uint64 GetCreatorGUID() const
    {
        return GetUInt64Value(UNIT_FIELD_CREATED_BY);
    }
    void SetCreatorGUID(uint64 creator)
    {
        SetUInt64Value(UNIT_FIELD_CREATED_BY, creator);
    }
    uint64 GetMinionGUID() const
    {
        return GetUInt64Value(UNIT_FIELD_SUMMON);
    }
    void SetMinionGUID(uint64 guid)
    {
        SetUInt64Value(UNIT_FIELD_SUMMON, guid);
    }
    uint64 GetCharmerGUID() const
    {
        return GetUInt64Value(UNIT_FIELD_CHARMED_BY);
    }
    void SetCharmerGUID(uint64 owner)
    {
        SetUInt64Value(UNIT_FIELD_CHARMED_BY, owner);
    }
    uint64 GetCharmGUID() const
    {
        return GetUInt64Value(UNIT_FIELD_CHARM);
    }
    void SetPetGUID(uint64 guid)
    {
        m_SummonSlot[SUMMON_SLOT_PET] = guid;
    }
    uint64 GetPetGUID() const
    {
        return m_SummonSlot[SUMMON_SLOT_PET];
    }
    void SetCritterGUID(uint64 guid)
    {
        SetUInt64Value(UNIT_FIELD_CRITTER, guid);
    }
    uint64 GetCritterGUID() const
    {
        return GetUInt64Value(UNIT_FIELD_CRITTER);
    }

    bool IsControlledByPlayer() const
    {
        return m_ControlledByPlayer;
    }
    uint64 GetCharmerOrOwnerGUID() const;
    uint64 GetCharmerOrOwnerOrOwnGUID() const;
    bool IsCharmedOwnedByPlayerOrPlayer() const
    {
        return IS_PLAYER_GUID(GetCharmerOrOwnerOrOwnGUID());
    }

    Player* GetSpellModOwner() const;

    Unit* GetOwner() const;
    Guardian* GetGuardianPet() const;
    Minion* GetFirstMinion() const;
    Unit* GetCharmer() const;
    Unit* GetCharm() const;
    Unit* GetCharmerOrOwner() const;
    Unit* GetCharmerOrOwnerOrSelf() const;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    Player* GetAffectingPlayer() const;

    void SetMinion(Minion* minion, bool apply);
    void GetAllMinionsByEntry(std::list<Creature*>& Minions, uint32 entry);
    void RemoveAllMinionsByEntry(uint32 entry);
    void SetCharm(Unit* target, bool apply);
    Unit* GetNextRandomRaidMemberOrPet(float radius);
    bool SetCharmedBy(Unit* charmer, CharmType type, AuraApplication const* aurApp = NULL);
    void RemoveCharmedBy(Unit* charmer);
    void RestoreFaction();

    ControlList m_Controlled;
    Unit* GetFirstControlled() const;
    void RemoveAllControlled();

    bool IsCharmed() const
    {
        return GetCharmerGUID() != 0;
    }
    bool isPossessed() const
    {
        return HasUnitState(UNIT_STATE_POSSESSED);
    }
    bool isPossessedByPlayer() const;
    bool isPossessing() const;
    bool isPossessing(Unit* u) const;

    CharmInfo* GetCharmInfo()
    {
        return m_charmInfo;
    }
    CharmInfo* InitCharmInfo();
    void DeleteCharmInfo();
    void UpdateCharmAI();
    //Player* GetMoverSource() const;
    Player* m_movedPlayer;
    SharedVisionList const& GetSharedVisionList()
    {
        return m_sharedVision;
    }
    void AddPlayerToVision(Player* player);
    void RemovePlayerFromVision(Player* player);
    bool HasSharedVision() const
    {
        return !m_sharedVision.empty();
    }
    void RemoveBindSightAuras();
    void RemoveCharmAuras();

    Pet* CreateTamedPetFrom(Creature* creatureTarget, uint32 spell_id = 0);
    Pet* CreateTamedPetFrom(uint32 creatureEntry, uint32 spell_id = 0);
    bool InitTamedPet(Pet* pet, uint8 level, uint32 spell_id) const;

    // aura apply/remove helpers - you should better not use these
    Aura* _TryStackingOrRefreshingExistingAura(SpellInfo const* newAura, uint32 effMask, Unit* caster, int32* baseAmount = NULL, Item* castItem = NULL, uint64 casterGUID = 0);
    void _AddAura(UnitAura* aura, Unit* caster);
    AuraApplication* _CreateAuraApplication(Aura* aura, uint32 effMask);
    void _ApplyAuraEffect(Aura* aura, uint8 effIndex);
    void _ApplyAura(AuraApplication* aurApp, uint32 effMask);
    void _UnapplyAura(AuraApplicationMap::iterator& i, AuraRemoveMode removeMode);
    void _UnapplyAura(AuraApplication* aurApp, AuraRemoveMode removeMode);

    void _RemoveNoStackAurasDueToAura(Aura* aura);

    void _RegisterAuraEffect(AuraEffect* aurEff, bool apply);

    // m_ownedAuras container management
    AuraMap& GetOwnedAuras()
    {
        return m_ownedAuras;
    }
    AuraMap const& GetOwnedAuras() const
    {
        return m_ownedAuras;
    }

    void RemoveOwnedAura(AuraMap::iterator& i, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    void RemoveOwnedAura(uint32 spellId, uint64 casterGUID = 0, uint32 reqEffMask = 0, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    void RemoveOwnedAura(Aura* aura, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);

    Aura* GetOwnedAura(uint32 spellId, uint64 casterGUID = 0, uint64 itemCasterGUID = 0, uint32 reqEffMask = 0, Aura* except = NULL) const;

    // m_appliedAuras container management
    AuraApplicationMap& GetAppliedAuras()
    {
        return m_appliedAuras;
    }
    AuraApplicationMap const& GetAppliedAuras() const
    {
        return m_appliedAuras;
    }

    void RemoveAura(AuraApplicationMap::iterator& i, AuraRemoveMode mode = AURA_REMOVE_BY_DEFAULT);
    void RemoveAura(uint32 spellId, uint64 casterGUID = 0, uint32 reqEffMask = 0, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    void RemoveAura(AuraApplication* aurApp, AuraRemoveMode mode = AURA_REMOVE_BY_DEFAULT);
    void RemoveAura(Aura* aur, AuraRemoveMode mode = AURA_REMOVE_BY_DEFAULT);

    void RemoveAurasDueToSpell(uint32 spellId, uint64 casterGUID = 0, uint32 reqEffMask = 0, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    void RemoveAuraFromStack(uint32 spellId, uint64 casterGUID = 0, AuraRemoveMode removeMode = AURA_REMOVE_BY_DEFAULT);
    void RemoveAurasDueToSpellByDispel(uint32 spellId, uint32 dispellerSpellId, uint64 casterGUID, Unit* dispeller, uint8 chargesRemoved = 1);
    void RemoveAurasDueToSpellBySteal(uint32 spellId, uint64 casterGUID, Unit* stealer);
    void RemoveAurasDueToItemSpell(uint32 spellId, uint64 castItemGuid);
    void RemoveAurasByType(AuraType auraType, uint64 casterGUID = 0, Aura* except = NULL, bool negative = true, bool positive = true);
    void RemoveNotOwnSingleTargetAuras(uint32 newPhase = 0, bool phaseid = false);
    void RemoveAurasWithInterruptFlags(uint32 flag, uint32 except = 0);
    void RemoveAurasWithAttribute(uint32 flags);
    void RemoveAurasWithFamily(SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, uint64 casterGUID);
    void RemoveAurasWithMechanic(uint32 mechanic_mask, AuraRemoveMode removemode = AURA_REMOVE_BY_DEFAULT, uint32 except = 0);
    void RemoveMovementImpairingAuras();

    void RemoveAreaAurasDueToLeaveWorld();
    void RemoveAllAuras();
    void RemoveArenaAuras();
    void RemoveAllAurasOnDeath();
    void RemoveAllAurasRequiringDeadTarget();
    void RemoveAllAurasExceptType(AuraType type);
    void RemoveAllAurasExceptType(AuraType type1, AuraType type2); /// @todo: once we support variadic templates use them here
    void DelayOwnedAuras(uint32 spellId, uint64 caster, int32 delaytime);

    void _RemoveAllAuraStatMods();
    void _ApplyAllAuraStatMods();

    AuraEffectList const& GetAuraEffectsByType(AuraType type) const
    {
        return m_modAuras[type];
    }
    AuraList& GetSingleCastAuras()
    {
        return m_scAuras;
    }
    AuraList const& GetSingleCastAuras() const
    {
        return m_scAuras;
    }

    AuraEffect* GetAuraEffect(uint32 spellId, uint8 effIndex, uint64 casterGUID = 0) const;
    AuraEffect* GetAuraEffectOfRankedSpell(uint32 spellId, uint8 effIndex, uint64 casterGUID = 0) const;
    AuraEffect* GetAuraEffect(AuraType type, SpellFamilyNames name, uint32 iconId, uint8 effIndex) const; // spell mustn't have familyflags
    AuraEffect* GetAuraEffect(AuraType type, SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, uint64 casterGUID = 0) const;
    AuraEffect* GetDummyAuraEffect(SpellFamilyNames name, uint32 iconId, uint8 effIndex) const;

    AuraApplication* GetAuraApplication(uint32 spellId, uint64 casterGUID = 0, uint64 itemCasterGUID = 0, uint32 reqEffMask = 0, AuraApplication* except = NULL) const;
    Aura* GetAura(uint32 spellId, uint64 casterGUID = 0, uint64 itemCasterGUID = 0, uint32 reqEffMask = 0) const;

    AuraApplication* GetAuraApplicationOfRankedSpell(uint32 spellId, uint64 casterGUID = 0, uint64 itemCasterGUID = 0, uint32 reqEffMask = 0, AuraApplication* except = NULL) const;
    Aura* GetAuraOfRankedSpell(uint32 spellId, uint64 casterGUID = 0, uint64 itemCasterGUID = 0, uint32 reqEffMask = 0) const;

    void GetDispellableAuraList(Unit* caster, uint32 dispelMask, DispelChargesList& dispelList);

    bool HasAuraEffect(uint32 spellId, uint8 effIndex, uint64 caster = 0) const;
    uint32 GetAuraCount(uint32 spellId) const;
    bool HasAura(uint32 spellId, uint64 casterGUID = 0, uint64 itemCasterGUID = 0, uint32 reqEffMask = 0) const;
    bool HasAuraType(AuraType auraType) const;
    bool HasAuraTypeWithCaster(AuraType auratype, uint64 caster) const;
    bool HasAuraTypeWithMiscvalue(AuraType auratype, int32 miscvalue) const;
    bool HasAuraTypeWithAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    bool HasAuraTypeWithValue(AuraType auratype, int32 value) const;
    bool HasNegativeAuraWithInterruptFlag(uint32 flag, uint64 guid = 0) const;
    bool HasNegativeAuraWithAttribute(uint32 flag, uint64 guid = 0) const;
    bool HasAuraWithMechanic(uint32 mechanicMask) const;

    AuraEffect* IsScriptOverriden(SpellInfo const* spell, int32 script) const;
    uint32 GetDiseasesByCaster(uint64 casterGUID, bool remove = false);
    uint32 GetDoTsByCaster(uint64 casterGUID) const;

    int32 GetTotalAuraModifier(AuraType auratype) const;
    float GetTotalAuraMultiplier(AuraType auratype) const;
    int32 GetMaxPositiveAuraModifier(AuraType auratype) const;
    int32 GetMaxNegativeAuraModifier(AuraType auratype) const;

    int32 GetTotalAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const;
    float GetTotalAuraMultiplierByMiscMask(AuraType auratype, uint32 misc_mask) const;
    int32 GetMaxPositiveAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask, const AuraEffect* except = NULL) const;
    int32 GetMaxNegativeAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const;

    int32 GetTotalAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;
    float GetTotalAuraMultiplierByMiscValue(AuraType auratype, int32 misc_value) const;
    int32 GetMaxPositiveAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;
    int32 GetMaxNegativeAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;

    int32 GetTotalAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    float GetTotalAuraMultiplierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    int32 GetMaxPositiveAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    int32 GetMaxNegativeAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;

    float GetResistanceBuffMods(SpellSchools school, bool positive) const;
    void SetResistanceBuffMods(SpellSchools school, bool positive, float val);
    void ApplyResistanceBuffModsMod(SpellSchools school, bool positive, float val, bool apply);
    void ApplyResistanceBuffModsPercentMod(SpellSchools school, bool positive, float val, bool apply);
    void InitStatBuffMods();
    void ApplyStatBuffMod(Stats stat, float val, bool apply);
    void ApplyStatPercentBuffMod(Stats stat, float val, bool apply);
    void SetCreateStat(Stats stat, float val)
    {
        m_createStats[stat] = val;
    }
    void SetCreateHealth(uint32 val)
    {
        SetUInt32Value(UNIT_FIELD_BASE_HEALTH, val);
    }
    uint32 GetCreateHealth() const
    {
        return GetUInt32Value(UNIT_FIELD_BASE_HEALTH);
    }
    void SetCreateMana(uint32 val)
    {
        SetUInt32Value(UNIT_FIELD_BASE_MANA, val);
    }
    uint32 GetCreateMana() const
    {
        return GetUInt32Value(UNIT_FIELD_BASE_MANA);
    }
    uint32 GetPowerIndex(uint32 powerType) const;
    int32 GetCreatePowers(Powers power) const;
    float GetPosStat(Stats stat) const
    {
        return GetFloatValue(UNIT_FIELD_STAT_POS_BUFF + EUnitFields(stat));
    }
    float GetNegStat(Stats stat) const
    {
        return GetFloatValue(UNIT_FIELD_STAT_NEG_BUFF + EUnitFields(stat));
    }
    float GetCreateStat(Stats stat) const
    {
        return m_createStats[stat];
    }

    void SetCurrentCastedSpell(Spell* pSpell);
    virtual void ProhibitSpellSchool(SpellSchoolMask /*idSchoolMask*/, uint32 /*unTimeMs*/)
    { }
    void InterruptSpell(CurrentSpellTypes spellType, bool withDelayed = true, bool withInstant = true);
    void FinishSpell(CurrentSpellTypes spellType, bool ok = true);

    // set withDelayed to true to account delayed spells as casted
    // delayed+channeled spells are always accounted as casted
    // we can skip channeled or delayed checks using flags
    bool IsNonMeleeSpellCasted(bool withDelayed, bool skipChanneled = false, bool skipAutorepeat = false, bool isAutoshoot = false, bool skipInstant = true) const;

    // set withDelayed to true to interrupt delayed spells too
    // delayed+channeled spells are always interrupted
    void InterruptNonMeleeSpells(bool withDelayed, uint32 spellid = 0, bool withInstant = true);

    Spell* GetCurrentSpell(CurrentSpellTypes spellType) const
    {
        return m_currentSpells[spellType];
    }
    Spell* GetCurrentSpell(uint32 spellType) const
    {
        return m_currentSpells[spellType];
    }
    Spell* FindCurrentSpellBySpellId(uint32 spell_id) const;
    int32 GetCurrentSpellCastTime(uint32 spell_id) const;

    uint64 m_SummonSlot[MAX_SUMMON_SLOT];
    uint64 m_ObjectSlot[MAX_GAMEOBJECT_SLOT];

    ShapeshiftForm GetShapeshiftForm() const
    {
        return ShapeshiftForm(GetByteValue(UNIT_FIELD_SHAPESHIFT_FORM, 3));
    }
    void SetShapeshiftForm(ShapeshiftForm form);

    bool IsInFeralForm() const;

    bool IsInDisallowedMountForm() const;

    float m_modMeleeHitChance;
    float m_modRangedHitChance;
    float m_modSpellHitChance;
    int32 m_baseSpellCritChance;

    float m_threatModifier[MAX_SPELL_SCHOOL];
    float m_modAttackSpeedPct[3];

    // Event handler
    EventProcessor m_Events;

    // stat system
    bool HandleStatModifier(UnitMods unitMod, UnitModifierType modifierType, float amount, bool apply);
    void SetModifierValue(UnitMods unitMod, UnitModifierType modifierType, float value)
    {
        m_auraModifiersGroup[unitMod][modifierType] = value;
    }
    float GetModifierValue(UnitMods unitMod, UnitModifierType modifierType) const;
    float GetTotalStatValue(Stats stat) const;
    float GetTotalAuraModValue(UnitMods unitMod) const;
    SpellSchools GetSpellSchoolByAuraGroup(UnitMods unitMod) const;
    Stats GetStatByAuraGroup(UnitMods unitMod) const;
    Powers GetPowerTypeByAuraGroup(UnitMods unitMod) const;
    bool CanModifyStats() const
    {
        return m_canModifyStats;
    }
    void SetCanModifyStats(bool modifyStats)
    {
        m_canModifyStats = modifyStats;
    }
    virtual bool UpdateStats(Stats stat) = 0;
    virtual bool UpdateAllStats() = 0;
    virtual void UpdateResistances(uint32 school) = 0;
    virtual void UpdateArmor() = 0;
    virtual void UpdateMaxHealth() = 0;
    virtual void UpdateMaxPower(Powers power) = 0;
    virtual void UpdateAttackPowerAndDamage(bool ranged = false) = 0;
    virtual void UpdateDamagePhysical(WeaponAttackType attType) = 0;
    float GetTotalAttackPowerValue(WeaponAttackType attType) const;
    float GetWeaponDamageRange(WeaponAttackType attType, WeaponDamageRange type) const;
    void SetBaseWeaponDamage(WeaponAttackType attType, WeaponDamageRange damageRange, float value)
    {
        m_weaponDamage[uint8(attType)][uint8(damageRange)] = value;
    }

    bool isInFrontInMap(Unit const* target, float distance, float arc = M_PI) const;
    bool isInBackInMap(Unit const* target, float distance, float arc = M_PI) const;

    // Visibility system
    bool IsVisible() const;
    void SetVisible(bool x);
    void ClearPhases(bool update = false);
    bool SetPhased(uint32 id, bool update, bool apply) OVERRIDE;
    // common function for visibility checks for player/creatures with detection code
    void SetPhaseMask(uint32 newPhaseMask, bool update) override;// overwrite WorldObject::SetPhaseMask
    void UpdateObjectVisibility(bool forced = true) override;

    SpellImmuneList m_spellImmune[MAX_SPELL_IMMUNITY];
    uint32 m_lastSanctuaryTime;

    // Threat related methods
    bool CanHaveThreatList() const;
    void AddThreat(Unit* victim, float fThreat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* threatSpell = NULL);
    float ApplyTotalThreatModifier(float fThreat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL);
    void DeleteThreatList();
    void TauntApply(Unit* victim);
    void TauntFadeOut(Unit* taunter);
    ThreatManager& getThreatManager()
    {
        return m_ThreatManager;
    }
    void addHatedBy(HostileReference* pHostileReference)
    {
        m_HostileRefManager.insertFirst(pHostileReference);
    };
    void removeHatedBy(HostileReference* /*pHostileReference*/)
    { /* nothing to do yet */
    }
    HostileRefManager& getHostileRefManager()
    {
        return m_HostileRefManager;
    }

    VisibleAuraMap const* GetVisibleAuras()
    {
        return &m_visibleAuras;
    }
    AuraApplication* GetVisibleAura(uint8 slot) const;
    void SetVisibleAura(uint8 slot, AuraApplication* aur);
    void RemoveVisibleAura(uint8 slot);

    uint32 GetInterruptMask() const
    {
        return m_interruptMask;
    }
    void AddInterruptMask(uint32 mask)
    {
        m_interruptMask |= mask;
    }
    void UpdateInterruptMask();
    bool HasVisionObscured(Unit const* target) const;

    uint32 GetDisplayId() const
    {
        return GetUInt32Value(UNIT_FIELD_DISPLAY_ID);
    }
    virtual void SetDisplayId(uint32 modelId);
    uint32 GetNativeDisplayId() const
    {
        return GetUInt32Value(UNIT_FIELD_NATIVE_DISPLAY_ID);
    }
    void RestoreDisplayId();
    void SetNativeDisplayId(uint32 modelId)
    {
        SetUInt32Value(UNIT_FIELD_NATIVE_DISPLAY_ID, modelId);
    }
    void setTransForm(uint32 spellid)
    {
        m_transform = spellid;
    }
    uint32 getTransForm() const
    {
        return m_transform;
    }

    // DynamicObject management
    void _RegisterDynObject(DynamicObject* dynObj);
    void _UnregisterDynObject(DynamicObject* dynObj);
    DynamicObject* GetDynObject(uint32 spellId);
    void RemoveDynObject(uint32 spellId);
    void RemoveAllDynObjects();

    GameObject* GetGameObject(uint32 spellId) const;
    void AddGameObject(GameObject* gameObj);
    void RemoveGameObject(GameObject* gameObj, bool del);
    void RemoveGameObject(uint32 spellid, bool del);
    void RemoveAllGameObjects();

    uint32 CalculateDamage(WeaponAttackType attType, bool normalized, bool addTotalPct);
    float GetAPMultiplier(WeaponAttackType attType, bool normalized);
    void ModifyAuraState(AuraStateType flag, bool apply);
    uint32 BuildAuraStateUpdateForTarget(Unit* target) const;
    bool HasAuraState(AuraStateType flag, SpellInfo const* spellProto = NULL, Unit const* Caster = NULL) const;
    void UnsummonAllTotems();
    Unit* GetMagicHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo);
    Unit* GetMeleeHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo = NULL);

    int32 SpellBaseDamageBonusDone(SpellSchoolMask schoolMask) const;
    int32 SpellBaseDamageBonusTaken(SpellSchoolMask schoolMask) const;
    uint32 SpellDamageBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 pdamage, DamageEffectType damagetype, uint32 stack = 1) const;
    uint32 SpellDamageBonusTaken(Unit* caster, SpellInfo const* spellProto, uint32 pdamage, DamageEffectType damagetype, uint32 stack = 1) const;
    int32 SpellBaseHealingBonusDone(SpellSchoolMask schoolMask) const;
    int32 SpellBaseHealingBonusTaken(SpellSchoolMask schoolMask) const;
    uint32 SpellHealingBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, uint32 stack = 1) const;
    uint32 SpellHealingBonusTaken(Unit* caster, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, uint32 stack = 1) const;

    uint32 MeleeDamageBonusDone(Unit* pVictim, uint32 damage, WeaponAttackType attType, SpellInfo const* spellProto = NULL);
    uint32 MeleeDamageBonusTaken(Unit* attacker, uint32 pdamage, WeaponAttackType attType, SpellInfo const* spellProto = NULL);


    bool   isSpellBlocked(Unit* victim, SpellInfo const* spellProto, WeaponAttackType attackType = WeaponAttackType::BASE_ATTACK);
    bool   isBlockCritical();
    bool   isSpellCrit(Unit* victim, SpellInfo const* spellProto, SpellSchoolMask schoolMask, WeaponAttackType attackType = WeaponAttackType::BASE_ATTACK) const;
    uint32 SpellCriticalDamageBonus(SpellInfo const* spellProto, uint32 damage, Unit* victim);
    uint32 SpellCriticalHealingBonus(SpellInfo const* spellProto, uint32 damage, Unit* victim);

    void SetContestedPvP(Player* attackedPlayer = NULL);

    uint32 GetCastingTimeForBonus(SpellInfo const* spellProto, DamageEffectType damagetype, uint32 CastingTime) const;
    float CalculateDefaultCoefficient(SpellInfo const* spellInfo, DamageEffectType damagetype) const;

    uint32 GetRemainingPeriodicAmount(uint64 caster, uint32 spellId, AuraType auraType, uint8 effectIndex = 0) const;

    void ApplySpellImmune(uint32 spellId, uint32 op, uint32 type, bool apply);
    void ApplySpellDispelImmunity(const SpellInfo* spellProto, DispelType type, bool apply);
    virtual bool IsImmunedToSpell(SpellInfo const* spellInfo) const; // redefined in Creature

    uint32 GetSchoolImmunityMask() const;
    uint32 GetMechanicImmunityMask() const;

    bool IsImmunedToDamage(SpellSchoolMask meleeSchoolMask) const;
    bool IsImmunedToDamage(SpellInfo const* spellInfo) const;
    virtual bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index) const; // redefined in Creature

    static bool IsDamageReducedByArmor(SpellSchoolMask damageSchoolMask, SpellInfo const* spellInfo = NULL, uint8 effIndex = MAX_SPELL_EFFECTS);
    uint32 CalcArmorReducedDamage(Unit* victim, const uint32 damage, SpellInfo const* spellInfo, WeaponAttackType attackType = WeaponAttackType::MAX_ATTACK);
    uint32 CalcSpellResistance(Unit* victim, SpellSchoolMask schoolMask, SpellInfo const* spellInfo) const;
    void CalcAbsorbResist(Unit* victim, SpellSchoolMask schoolMask, DamageEffectType damagetype, uint32 const damage, uint32* absorb, uint32* resist, SpellInfo const* spellInfo = NULL);
    void CalcHealAbsorb(Unit* victim, SpellInfo const* spellInfo, uint32& healAmount, uint32& absorb);

    void  UpdateSpeed(UnitMoveType mtype, bool forced);
    float GetSpeed(UnitMoveType mtype) const;
    float GetSpeedRate(UnitMoveType mtype) const
    {
        return m_speed_rate[mtype];
    }
    void SetSpeed(UnitMoveType mtype, float rate, bool forced = false);

    float ApplyEffectModifiers(SpellInfo const* spellProto, uint8 effect_index, float value) const;
    int32 CalculateSpellDamage(Unit const* target, SpellInfo const* spellProto, uint8 effect_index, int32 const* basePoints = NULL) const;
    int32 CalcSpellDuration(SpellInfo const* spellProto);
    int32 ModSpellDuration(SpellInfo const* spellProto, Unit const* target, int32 duration, bool positive, uint32 effectMask);
    void  ModSpellCastTime(SpellInfo const* spellProto, int32& castTime, Spell* spell = NULL);
    float CalculateLevelPenalty(SpellInfo const* spellProto) const;

    void addFollower(FollowerReference* pRef)
    {
        m_FollowingRefManager.insertFirst(pRef);
    }
    void removeFollower(FollowerReference* /*pRef*/)
    { /* nothing to do yet */
    }
    static Unit* GetUnit(WorldObject& object, uint64 guid);
    static Player* GetPlayer(WorldObject& object, uint64 guid);
    static Creature* GetCreature(WorldObject& object, uint64 guid);

    MotionMaster* GetMotionMaster()
    {
        return &i_motionMaster;
    }
    const MotionMaster* GetMotionMaster() const
    {
        return &i_motionMaster;
    }

    bool IsStopped() const
    {
        return !(HasUnitState(UNIT_STATE_MOVING));
    }
    void StopMoving();

    void AddUnitMovementFlag(uint32 f)
    {
        m_movementInfo.AddMovementFlag(f);
    }
    void RemoveUnitMovementFlag(uint32 f)
    {
        m_movementInfo.RemoveMovementFlag(f);
    }
    bool HasUnitMovementFlag(uint32 f) const
    {
        return m_movementInfo.HasMovementFlag(f);
    }
    uint32 GetUnitMovementFlags() const
    {
        return m_movementInfo.GetMovementFlags();
    }
    void SetUnitMovementFlags(uint32 f)
    {
        m_movementInfo.SetMovementFlags(f);
    }

    void AddExtraUnitMovementFlag(uint16 f)
    {
        m_movementInfo.AddExtraMovementFlag(f);
    }
    void RemoveExtraUnitMovementFlag(uint16 f)
    {
        m_movementInfo.RemoveExtraMovementFlag(f);
    }
    uint16 HasExtraUnitMovementFlag(uint16 f) const
    {
        return m_movementInfo.HasExtraMovementFlag(f);
    }
    uint16 GetExtraUnitMovementFlags() const
    {
        return m_movementInfo.GetExtraMovementFlags();
    }
    void SetExtraUnitMovementFlags(uint16 f)
    {
        m_movementInfo.SetExtraMovementFlags(f);
    }
    bool IsSplineEnabled() const;

    float GetPositionZMinusOffset() const;

    void SetControlled(bool apply, UnitState state);

    void AddComboPointHolder(uint32 lowguid)
    {
        m_ComboPointHolders.insert(lowguid);
    }
    void RemoveComboPointHolder(uint32 lowguid)
    {
        m_ComboPointHolders.erase(lowguid);
    }
    void ClearComboPointHolders();

    ///----------Pet responses methods-----------------
    void SendPetActionFeedback(uint8 msg) const;
    void SendPetTalk(uint32 pettalk) const;
    void SendPetAIReaction(ObjectGuid UnitGUID) const;
    ///----------End of Pet responses methods----------

    void propagateSpeedChange()
    {
        GetMotionMaster()->propagateSpeedChange();
    }

    // reactive attacks
    void ClearAllReactives();
    void StartReactiveTimer(ReactiveType reactive)
    {
        m_reactiveTimer[reactive] = REACTIVE_TIMER_START;
    }
    void UpdateReactives(uint32 p_time);

    // group updates
    void UpdateAuraForGroup(uint8 slot);

    // proc trigger system
    bool CanProc() const
    {
        return !m_procDeep;
    }
    void SetCantProc(bool apply);

    // pet auras
    typedef std::set<PetAura const*> PetAuraSet;
    PetAuraSet m_petAuras;
    void AddPetAura(PetAura const* petSpell);
    void RemovePetAura(PetAura const* petSpell);

    uint32 GetModelForForm(ShapeshiftForm form) const;
    uint32 GetModelForTotem(uint32 totemType) const;

    // Redirect Threat
    void SetRedirectThreat(uint64 guid, uint32 pct)
    {
        _redirectThreadInfo.Set(guid, pct);
    }
    void ResetRedirectThreat()
    {
        SetRedirectThreat(0, 0);
    }
    void ModifyRedirectThreat(int32 amount)
    {
        _redirectThreadInfo.ModifyThreatPct(amount);
    }
    uint32 GetRedirectThreatPercent() const
    {
        return _redirectThreadInfo.GetThreatPct();
    }
    Unit* GetRedirectThreatTarget();

    friend class VehicleJoinEvent;
    bool IsAIEnabled, NeedChangeAI;
    uint64 LastCharmerGUID;
    bool CreateVehicleKit(uint32 id, uint32 creatureEntry, bool loading = false);
    void RemoveVehicleKit(bool remove = false);
    Vehicle* GetVehicleKit()const
    {
        return m_vehicleKit;
    }
    Vehicle* GetVehicle()   const
    {
        return m_vehicle;
    }
    void SetVehicle(Vehicle* vehicle)
    {
        m_vehicle = vehicle;
    }
    bool IsOnVehicle(const Unit* vehicle) const;
    Unit* GetVehicleBase()  const;
    Creature* GetVehicleCreatureBase() const;
    uint64 GetTransGUID() const override;
    /// Returns the transport this unit is on directly (if on vehicle and transport, return vehicle)
    TransportBase* GetDirectTransport() const;

    bool m_ControlledByPlayer;

    bool HandleSpellClick(Unit* clicker, int8 seatId = -1);
    void EnterVehicle(Unit* base, int8 seatId = -1);
    void ExitVehicle(Position const* exitPosition = NULL);
    void ChangeSeat(int8 seatId, bool next = true);

    // Should only be called by AuraEffect::HandleAuraControlVehicle(AuraApplication const* auraApp, uint8 mode, bool apply) const;
    void _ExitVehicle(Position const* exitPosition = NULL);
    void _EnterVehicle(Vehicle* vehicle, int8 seatId, AuraApplication const* aurApp = NULL);

    void WriteMovementInfo(WorldPacket& data, Movement::ExtraMovementStatusElement* extras = NULL);

    bool isMoving() const
    {
        return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_MOVING);
    }
    bool isTurning() const
    {
        return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_TURNING);
    }
    virtual bool CanFly() const = 0;
    bool IsFlying() const
    {
        return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_DISABLE_GRAVITY);
    }
    bool IsFalling() const;

    void RewardRage(uint32 baseRage, bool attacker);

    virtual float GetFollowAngle() const
    {
        return static_cast<float>(M_PI / 2);
    }

    void OutDebugInfo() const;
    virtual bool isBeingLoaded() const
    {
        return false;
    }
    bool IsDuringRemoveFromWorld() const
    {
        return m_duringRemoveFromWorld;
    }

    Pet* ToPet()
    {
        if (IsPet()) return reinterpret_cast<Pet*>(this); else return NULL;
    }
    Pet const* ToPet() const
    {
        if (IsPet()) return reinterpret_cast<Pet const*>(this); else return NULL;
    }

    Totem* ToTotem()
    {
        if (IsTotem()) return reinterpret_cast<Totem*>(this); else return NULL;
    }
    Totem const* ToTotem() const
    {
        if (IsTotem()) return reinterpret_cast<Totem const*>(this); else return NULL;
    }

    TempSummon* ToTempSummon()
    {
        if (IsSummon()) return reinterpret_cast<TempSummon*>(this); else return NULL;
    }
    TempSummon const* ToTempSummon() const
    {
        if (IsSummon()) return reinterpret_cast<TempSummon const*>(this); else return NULL;
    }

    uint64 GetTarget() const
    {
        return GetUInt64Value(UNIT_FIELD_TARGET);
    }
    virtual void SetTarget(uint64 /*guid*/) = 0;

    // Movement info
    std::unique_ptr<Movement::MoveSpline> movespline;

    // Part of Evade mechanics
    time_t GetLastDamagedTime() const
    {
        return _lastDamagedTime;
    }
    void SetLastDamagedTime(time_t val)
    {
        _lastDamagedTime = val;
    }

    uint32 GetMovementCounter() const
    {
        return m_movementCounter;
    }
    void SetAutoattackOverrideSpell(SpellInfo const* spellInfo)
    {
        m_overrideAutoattackSpellInfo = spellInfo;
    }
    void SetAutoattackOverrideRange(uint32 range)
    {
        m_overrideAutoattackRange = range;
    }

protected:
    explicit Unit(bool isWorldObject);

    void BuildValuesUpdate(uint8 updatetype, ByteBuffer* data, Player* target) const override;

    UnitAI* i_AI, * i_disabledAI;

    void _UpdateSpells(uint32 time);
    void _DeleteRemovedAuras();

    void _UpdateAutoRepeatSpell();

    bool m_AutoRepeatFirstCast;

    uint32 m_attackTimer[uint8(WeaponAttackType::MAX_ATTACK)];

    float m_createStats[MAX_STATS];

    AttackerSet m_attackers;
    Unit* m_attacking;

    DeathState m_deathState;

    int32 m_procDeep;

    typedef std::list<DynamicObject*> DynObjectList;
    DynObjectList m_dynObj;

    typedef std::list<GameObject*> GameObjectList;
    GameObjectList m_gameObj;
    bool m_isSorted;
    uint32 m_transform;

    Spell* m_currentSpells[CURRENT_MAX_SPELL];
    SpellInfo const* m_overrideAutoattackSpellInfo;
    uint32 m_overrideAutoattackRange;

    AuraMap m_ownedAuras;
    AuraApplicationMap m_appliedAuras;
    AuraList m_removedAuras;
    AuraMap::iterator m_auraUpdateIterator;
    uint32 m_removedAurasCount;

    AuraEffectList m_modAuras[TOTAL_AURAS];
    AuraList m_scAuras;                        // casted singlecast auras
    AuraApplicationList m_interruptableAuras;             // auras which have interrupt mask applied on unit
    AuraStateAurasMap m_auraStateAuras;        // Used for improve performance of aura state checks on aura apply/remove
    uint32 m_interruptMask;

    float m_auraModifiersGroup[UNIT_MOD_END][MODIFIER_TYPE_END];
    float m_weaponDamage[uint8(WeaponAttackType::MAX_ATTACK)][2];
    bool m_canModifyStats;
    VisibleAuraMap m_visibleAuras;

    float m_speed_rate[MAX_MOVE_TYPE];

    CharmInfo* m_charmInfo;
    SharedVisionList m_sharedVision;

    void SendMissileCancel(uint32 spellId, bool cancel = true);

    virtual SpellSchoolMask GetMeleeDamageSchoolMask() const;

    MotionMaster i_motionMaster;

    uint32 m_reactiveTimer[MAX_REACTIVE];
    uint32 m_regenTimer;

    ThreatManager m_ThreatManager;

    Vehicle* m_vehicle;
    Vehicle* m_vehicleKit;

    uint32 m_unitTypeMask;
    LiquidTypeEntry const* _lastLiquid;

    bool IsAlwaysVisibleFor(WorldObject const* seer) const override;
    bool IsAlwaysDetectableFor(WorldObject const* seer) const override;

    void DisableSpline();

private:
    bool IsTriggeredAtSpellProcEvent(Unit* victim, Aura* aura, SpellInfo const* procSpell, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, bool isVictim, bool active, SpellProcEventEntry const*& spellProcEvent);
    bool HandleAuraProcOnPowerAmount(Unit* victim, uint32 damage, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown);
    bool HandleDummyAuraProc(Unit* victim, uint32 damage, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown);
    bool HandleAuraProc(Unit* victim, uint32 damage, Aura* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown, bool* handled);
    bool HandleProcTriggerSpell(Unit* victim, uint32 damage, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown);
    bool HandleOverrideClassScriptAuraProc(Unit* victim, uint32 damage, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 cooldown);
    bool HandleAuraRaidProcFromChargeWithValue(AuraEffect* triggeredByAura);
    bool HandleAuraRaidProcFromCharge(AuraEffect* triggeredByAura);

    void UpdateSplineMovement(uint32 t_diff);
    void UpdateSplinePosition();

    // player or player's pet
    float GetCombatRatingReduction(CombatRating cr) const;
    uint32 GetCombatRatingDamageReduction(CombatRating cr, float rate, float cap, uint32 damage) const;

protected:
    void SetFeared(bool apply);
    void SetConfused(bool apply);
    void SetStunned(bool apply);
    void SetRooted(bool apply, bool packetOnly = false);

    uint32 m_movementCounter;       ///< Incrementing counter used in movement packets

private:
    uint32 m_state;                                     // Even derived shouldn't modify
    uint32 m_CombatTimer;
    TimeTrackerSmall m_movesplineTimer;

    Diminishing m_Diminishing;
    // Manage all Units that are threatened by us
    HostileRefManager m_HostileRefManager;

    FollowerRefManager m_FollowingRefManager;

    ComboPointHolderSet m_ComboPointHolders;

    RedirectThreatInfo _redirectThreadInfo;

    bool m_cleanupDone; // lock made to not add stuff after cleanup before delete
    bool m_duringRemoveFromWorld; // lock made to not add stuff after begining removing from world

    bool _isWalkingBeforeCharm; // Are we walking before we were charmed?

    time_t _lastDamagedTime; // Part of Evade mechanics
};

namespace Skyfire
{
    // Binary predicate for sorting Units based on percent value of a power
    class PowerPctOrderPred
    {
    public:
        PowerPctOrderPred(Powers power, bool ascending = true) : m_power(power), m_ascending(ascending)
        { }
        bool operator() (const Unit* a, const Unit* b) const
        {
            float rA = a->GetMaxPower(m_power) ? float(a->GetPower(m_power)) / float(a->GetMaxPower(m_power)) : 0.0f;
            float rB = b->GetMaxPower(m_power) ? float(b->GetPower(m_power)) / float(b->GetMaxPower(m_power)) : 0.0f;
            return m_ascending ? rA < rB : rA > rB;
        }
    private:
        const Powers m_power;
        const bool m_ascending;
    };

    // Binary predicate for sorting Units based on percent value of health
    class HealthPctOrderPred
    {
    public:
        HealthPctOrderPred(bool ascending = true) : m_ascending(ascending)
        { }
        bool operator() (const Unit* a, const Unit* b) const
        {
            float rA = a->GetMaxHealth() ? float(a->GetHealth()) / float(a->GetMaxHealth()) : 0.0f;
            float rB = b->GetMaxHealth() ? float(b->GetHealth()) / float(b->GetMaxHealth()) : 0.0f;
            return m_ascending ? rA < rB : rA > rB;
        }
    private:
        const bool m_ascending;
    };
}
#endif
