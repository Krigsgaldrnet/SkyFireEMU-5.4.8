/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SF_SPELLINFO_H
#define SF_SPELLINFO_H

#include "DB2Structure.h"
#include "DBCStructure.h"
#include "Object.h"
#include "SharedDefines.h"
#include "SpellAuraDefines.h"
#include "Util.h"

class Unit;
class Player;
class Item;
class Spell;
class SpellInfo;
class WorldObject;
struct SpellChainNode;
struct SpellTargetPosition;
struct SpellDurationEntry;
struct SpellModifier;
struct SpellRangeEntry;
struct SpellRadiusEntry;
struct SpellEntry;
struct SpellCastTimesEntry;
struct Condition;

enum SpellCastTargetFlags
{
    TARGET_FLAG_NONE = 0x00000000,
    TARGET_FLAG_UNUSED_1 = 0x00000001,               // not used
    TARGET_FLAG_UNIT = 0x00000002,               // pguid
    TARGET_FLAG_UNIT_RAID = 0x00000004,               // not sent, used to validate target (if raid member)
    TARGET_FLAG_UNIT_PARTY = 0x00000008,               // not sent, used to validate target (if party member)
    TARGET_FLAG_ITEM = 0x00000010,               // pguid
    TARGET_FLAG_SOURCE_LOCATION = 0x00000020,               // pguid, 3 float
    TARGET_FLAG_DEST_LOCATION = 0x00000040,               // pguid, 3 float
    TARGET_FLAG_UNIT_ENEMY = 0x00000080,               // not sent, used to validate target (if enemy)
    TARGET_FLAG_UNIT_ALLY = 0x00000100,               // not sent, used to validate target (if ally)
    TARGET_FLAG_CORPSE_ENEMY = 0x00000200,               // pguid
    TARGET_FLAG_UNIT_DEAD = 0x00000400,               // not sent, used to validate target (if dead creature)
    TARGET_FLAG_GAMEOBJECT = 0x00000800,               // pguid, used with TARGET_GAMEOBJECT_TARGET
    TARGET_FLAG_TRADE_ITEM = 0x00001000,               // pguid
    TARGET_FLAG_STRING = 0x00002000,               // string
    TARGET_FLAG_GAMEOBJECT_ITEM = 0x00004000,               // not sent, used with TARGET_GAMEOBJECT_ITEM_TARGET
    TARGET_FLAG_CORPSE_ALLY = 0x00008000,               // pguid
    TARGET_FLAG_UNIT_MINIPET = 0x00010000,               // pguid, used to validate target (if non combat pet)
    TARGET_FLAG_GLYPH_SLOT = 0x00020000,               // used in glyph spells
    TARGET_FLAG_DEST_TARGET = 0x00040000,               // sometimes appears with DEST_TARGET spells (may appear or not for a given spell)
    TARGET_FLAG_EXTRA_TARGETS = 0x00080000,               // uint32 counter, loop { vec3 - screen position (?), guid }, not used so far
    TARGET_FLAG_UNIT_PASSENGER = 0x00100000,               // guessed, used to validate target (if vehicle passenger)

    TARGET_FLAG_UNIT_MASK = TARGET_FLAG_UNIT | TARGET_FLAG_UNIT_RAID | TARGET_FLAG_UNIT_PARTY
    | TARGET_FLAG_UNIT_ENEMY | TARGET_FLAG_UNIT_ALLY | TARGET_FLAG_UNIT_DEAD | TARGET_FLAG_UNIT_MINIPET | TARGET_FLAG_UNIT_PASSENGER,
    TARGET_FLAG_GAMEOBJECT_MASK = TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_GAMEOBJECT_ITEM,
    TARGET_FLAG_CORPSE_MASK = TARGET_FLAG_CORPSE_ALLY | TARGET_FLAG_CORPSE_ENEMY,
    TARGET_FLAG_ITEM_MASK = TARGET_FLAG_TRADE_ITEM | TARGET_FLAG_ITEM | TARGET_FLAG_GAMEOBJECT_ITEM
};

enum SpellTargetSelectionCategories
{
    TARGET_SELECT_CATEGORY_NYI,
    TARGET_SELECT_CATEGORY_DEFAULT,
    TARGET_SELECT_CATEGORY_CHANNEL,
    TARGET_SELECT_CATEGORY_NEARBY,
    TARGET_SELECT_CATEGORY_CONE,
    TARGET_SELECT_CATEGORY_AREA
};

enum SpellTargetReferenceTypes
{
    TARGET_REFERENCE_TYPE_NONE,
    TARGET_REFERENCE_TYPE_CASTER,
    TARGET_REFERENCE_TYPE_TARGET,
    TARGET_REFERENCE_TYPE_LAST,
    TARGET_REFERENCE_TYPE_SRC,
    TARGET_REFERENCE_TYPE_DEST
};

enum SpellTargetObjectTypes
{
    TARGET_OBJECT_TYPE_NONE = 0,
    TARGET_OBJECT_TYPE_SRC,
    TARGET_OBJECT_TYPE_DEST,
    TARGET_OBJECT_TYPE_UNIT,
    TARGET_OBJECT_TYPE_UNIT_AND_DEST,
    TARGET_OBJECT_TYPE_GOBJ,
    TARGET_OBJECT_TYPE_GOBJ_ITEM,
    TARGET_OBJECT_TYPE_ITEM,
    TARGET_OBJECT_TYPE_CORPSE,
    // only for effect target type
    TARGET_OBJECT_TYPE_CORPSE_ENEMY,
    TARGET_OBJECT_TYPE_CORPSE_ALLY
};

enum SpellTargetCheckTypes
{
    TARGET_CHECK_DEFAULT,
    TARGET_CHECK_ENTRY,
    TARGET_CHECK_ENEMY,
    TARGET_CHECK_ALLY,
    TARGET_CHECK_PARTY,
    TARGET_CHECK_RAID,
    TARGET_CHECK_RAID_CLASS,
    TARGET_CHECK_PASSENGER
};

enum SpellTargetDirectionTypes
{
    TARGET_DIR_NONE,
    TARGET_DIR_FRONT,
    TARGET_DIR_BACK,
    TARGET_DIR_RIGHT,
    TARGET_DIR_LEFT,
    TARGET_DIR_FRONT_RIGHT,
    TARGET_DIR_BACK_RIGHT,
    TARGET_DIR_BACK_LEFT,
    TARGET_DIR_FRONT_LEFT,
    TARGET_DIR_RANDOM,
    TARGET_DIR_ENTRY
};

enum SpellEffectImplicitTargetTypes
{
    EFFECT_IMPLICIT_TARGET_NONE = 0,
    EFFECT_IMPLICIT_TARGET_EXPLICIT,
    EFFECT_IMPLICIT_TARGET_CASTER
};

// Spell clasification
enum SpellSpecificType
{
    SPELL_SPECIFIC_NORMAL = 0,
    SPELL_SPECIFIC_SEAL = 1,
    SPELL_SPECIFIC_AURA = 3,
    SPELL_SPECIFIC_STING = 4,
    SPELL_SPECIFIC_CURSE = 5,
    SPELL_SPECIFIC_ASPECT = 6,
    SPELL_SPECIFIC_TRACKER = 7,
    SPELL_SPECIFIC_WARLOCK_ARMOR = 8,
    SPELL_SPECIFIC_MAGE_ARMOR = 9,
    SPELL_SPECIFIC_ELEMENTAL_SHIELD = 10,
    SPELL_SPECIFIC_MAGE_POLYMORPH = 11,
    SPELL_SPECIFIC_JUDGEMENT = 13,
    SPELL_SPECIFIC_WARLOCK_CORRUPTION = 17,
    SPELL_SPECIFIC_FOOD = 19,
    SPELL_SPECIFIC_DRINK = 20,
    SPELL_SPECIFIC_FOOD_AND_DRINK = 21,
    SPELL_SPECIFIC_PRESENCE = 22,
    SPELL_SPECIFIC_CHARM = 23,
    SPELL_SPECIFIC_SCROLL = 24,
    SPELL_SPECIFIC_MAGE_ARCANE_BRILLANCE = 25,
    SPELL_SPECIFIC_WARRIOR_ENRAGE = 26,
    SPELL_SPECIFIC_PRIEST_DIVINE_SPIRIT = 27,
    SPELL_SPECIFIC_HAND = 28,
    SPELL_SPECIFIC_PHASE = 29,
    SPELL_SPECIFIC_BANE = 30
};

enum SpellCustomAttributes
{
    SPELL_ATTR0_CU_ENCHANT_PROC = 0x00000001,
    SPELL_ATTR0_CU_CONE_BACK = 0x00000002,
    SPELL_ATTR0_CU_CONE_LINE = 0x00000004,
    SPELL_ATTR0_CU_SHARE_DAMAGE = 0x00000008,
    SPELL_ATTR0_CU_NO_INITIAL_THREAT = 0x00000010,
    SPELL_ATTR0_CU_AURA_CC = 0x00000040,
    SPELL_ATTR0_CU_DIRECT_DAMAGE = 0x00000100,
    SPELL_ATTR0_CU_CHARGE = 0x00000200,
    SPELL_ATTR0_CU_PICKPOCKET = 0x00000400,
    SPELL_ATTR0_CU_NEGATIVE_EFF0 = 0x00001000,
    SPELL_ATTR0_CU_NEGATIVE_EFF1 = 0x00002000,
    SPELL_ATTR0_CU_NEGATIVE_EFF2 = 0x00004000,
    SPELL_ATTR0_CU_IGNORE_ARMOR = 0x00008000,
    SPELL_ATTR0_CU_REQ_TARGET_FACING_CASTER = 0x00010000,
    SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET = 0x00020000,

    SPELL_ATTR0_CU_NEGATIVE = SPELL_ATTR0_CU_NEGATIVE_EFF0 | SPELL_ATTR0_CU_NEGATIVE_EFF1 | SPELL_ATTR0_CU_NEGATIVE_EFF2
};

uint32 GetTargetFlagMask(SpellTargetObjectTypes objType);

class SpellImplicitTargetInfo
{
private:
    Targets _target;
public:
    SpellImplicitTargetInfo() : _target(Targets(0)) { }
    SpellImplicitTargetInfo(uint32 target) : _target(Targets(target)) { }

    bool IsArea() const;
    SpellTargetSelectionCategories GetSelectionCategory() const;
    SpellTargetReferenceTypes GetReferenceType() const;
    SpellTargetObjectTypes GetObjectType() const;
    SpellTargetCheckTypes GetCheckType() const;
    SpellTargetDirectionTypes GetDirectionType() const;
    float CalcDirectionAngle() const;

    Targets GetTarget() const;
    uint32 GetExplicitTargetMask(bool& srcSet, bool& dstSet) const;

private:
    struct StaticData
    {
        SpellTargetObjectTypes ObjectType;    // type of object returned by target type
        SpellTargetReferenceTypes ReferenceType; // defines which object is used as a reference when selecting target
        SpellTargetSelectionCategories SelectionCategory;
        SpellTargetCheckTypes SelectionCheckType; // defines selection criteria
        SpellTargetDirectionTypes DirectionType; // direction for cone and dest targets
    };
    static StaticData _data[TOTAL_SPELL_TARGETS];
};

class SpellEffectInfo
{
    SpellInfo const* _spellInfo;
    uint8 _effIndex;
public:
    uint32    Effect;
    uint32    ApplyAuraName;
    uint32    ApplyAuraTickCount;
    int32     DieSides;
    float     RealPointsPerLevel;
    int32     BasePoints;
    float     PointsPerComboPoint;
    float     ValueMultiplier;
    float     DamageMultiplier;
    float     BonusMultiplier;
    int32     MiscValue;
    int32     MiscValueB;
    Mechanics Mechanic;
    SpellImplicitTargetInfo TargetA;
    SpellImplicitTargetInfo TargetB;
    SpellRadiusEntry const* RadiusEntry = { };
    SpellRadiusEntry const* MaxRadiusEntry = { };
    uint32    ChainTarget;
    uint32    ItemType;
    uint32    TriggerSpell;
    flag128    SpellClassMask;
    std::list<Condition*>* ImplicitTargetConditions = { };
    // SpellScalingEntry
    float     ScalingMultiplier;
    float     DeltaScalingMultiplier;
    float     ComboScalingMultiplier;

    SpellEffectInfo() : _spellInfo(NULL), _effIndex(0), Effect(0), ApplyAuraName(0), ApplyAuraTickCount(0), DieSides(0), RealPointsPerLevel(0.0f), BasePoints(0), PointsPerComboPoint(0.0f),
        ValueMultiplier(0.0f), DamageMultiplier(0.0f), BonusMultiplier(0), MiscValue(0), MiscValueB(0), Mechanic(MECHANIC_NONE), ChainTarget(0), ItemType(0),
        TriggerSpell(0), ScalingMultiplier(0.0f), DeltaScalingMultiplier(0.0f), ComboScalingMultiplier(0.0f) { }

    SpellEffectInfo(SpellEntry const* spellEntry, SpellInfo const* spellInfo, uint8 effIndex, SpellEffectEntry const* effect);

    bool IsEffect() const;
    bool IsEffect(SpellEffects effectName) const;
    bool IsAura() const;
    bool IsAura(AuraType aura) const;
    bool IsTargetingArea() const;
    bool IsAreaAuraEffect() const;
    bool IsFarUnitTargetEffect() const;
    bool IsFarDestTargetEffect() const;
    bool IsUnitOwnedAuraEffect() const;

    int32 CalcValue(Unit const* caster = NULL, int32 const* basePoints = NULL, Unit const* target = NULL) const;
    int32 CalcBaseValue(int32 value) const;
    float CalcBonusMultiplier(Unit* caster, Spell* spell = NULL) const;
    float CalcValueMultiplier(Unit* caster, Spell* spell = NULL) const;
    float CalcDamageMultiplier(Unit* caster, Spell* spell = NULL) const;

    bool HasRadius() const;
    bool HasMaxRadius() const;
    float CalcRadius(Unit* caster = NULL, Spell* = NULL) const;

    uint32 GetProvidedTargetMask() const;
    uint32 GetMissingTargetMask(bool srcSet = false, bool destSet = false, uint32 mask = 0) const;

    SpellEffectImplicitTargetTypes GetImplicitTargetType() const;
    SpellTargetObjectTypes GetUsedTargetObjectType() const;

private:
    struct StaticData
    {
        SpellEffectImplicitTargetTypes ImplicitTargetType; // defines what target can be added to effect target list if there's no valid target type provided for effect
        SpellTargetObjectTypes UsedTargetObjectType; // defines valid target object type for spell effect
    };
    static StaticData _data[TOTAL_SPELL_EFFECTS];
};

class SpellInfo
{
public:
    uint32 Id;
    SpellCategoryEntry const* CategoryEntry;
    uint32 Dispel;
    uint32 Mechanic;
    uint32 Attributes;
    uint32 AttributesEx;
    uint32 AttributesEx2;
    uint32 AttributesEx3;
    uint32 AttributesEx4;
    uint32 AttributesEx5;
    uint32 AttributesEx6;
    uint32 AttributesEx7;
    uint32 AttributesEx8;
    uint32 AttributesEx9;
    //uint32 AttributesEx10;
    //uint32 AttributesEx11;
    //uint32 AttributesEx12;
    //uint32 AttributesEx13;
    uint32 AttributesCu;
    uint32 Stances;
    uint32 StancesNot;
    uint32 Targets;
    uint32 TargetCreatureType;
    uint32 RequiresSpellFocus;
    uint32 FacingCasterFlags;
    uint32 CasterAuraState;
    uint32 TargetAuraState;
    uint32 CasterAuraStateNot;
    uint32 TargetAuraStateNot;
    uint32 CasterAuraSpell;
    uint32 TargetAuraSpell;
    uint32 ExcludeCasterAuraSpell;
    uint32 ExcludeTargetAuraSpell;
    SpellCastTimesEntry const* CastTimeEntry;
    uint32 RecoveryTime;
    uint32 CategoryRecoveryTime;
    uint32 StartRecoveryCategory;
    uint32 StartRecoveryTime;
    uint32 InterruptFlags;
    uint32 AuraInterruptFlags;
    uint32 ChannelInterruptFlags;
    uint32 ProcFlags;
    uint32 ProcChance;
    uint32 ProcCharges;
    uint32 MaxLevel;
    uint32 BaseLevel;
    uint32 SpellLevel;
    SpellDurationEntry const* DurationEntry;
    uint32 RuneCostID;
    SpellRangeEntry const* RangeEntry;
    float  Speed;
    uint32 StackAmount;
    uint32 Totem[2];
    int32  Reagent[MAX_SPELL_REAGENTS];
    uint32 ReagentCount[MAX_SPELL_REAGENTS];
    int32  EquippedItemClass;
    int32  EquippedItemSubClassMask;
    int32  EquippedItemInventoryTypeMask;
    uint32 TotemCategory[2];
    uint32 SpellVisual[2];
    uint32 SpellIconID;
    uint32 ActiveIconID;
    char* SpellName;
    //char* Rank;
    //uint32 MaxTargetLevel;
    uint32 MaxAffectedTargets;
    uint32 SpellFamilyName;
    flag128 SpellFamilyFlags;
    uint32 DmgClass;
    uint32 PreventionType;
    int32  AreaGroupId;
    uint32 SchoolMask;
    //uint32 SpellDifficultyId;
    uint32 SpellScalingId;
    uint32 SpellAuraOptionsId;
    uint32 SpellAuraRestrictionsId;
    uint32 SpellCastingRequirementsId;
    uint32 SpellCategoriesId;
    uint32 SpellClassOptionsId;
    uint32 SpellCooldownsId;
    uint32 SpellEquippedItemsId;
    uint32 SpellInterruptsId;
    uint32 SpellLevelsId;
    uint32 SpellReagentsId;
    uint32 SpellShapeshiftId;
    uint32 SpellTargetRestrictionsId;
    uint32 SpellTotemsId;
    //uint32 ResearchProject;
    // SpellScalingEntry
    int32  CastTimeMin;
    int32  CastTimeMax;
    int32  CastTimeMaxLevel;
    int32  ScalingClass;
    float  CoefBase;
    int32  CoefLevelBase;
    SpellEffectInfo Effects[MAX_SPELL_EFFECTS];
    uint32 ExplicitTargetMask;
    SpellChainNode const* ChainEntry;

    // SpecializationSpellsEntry
    std::list<uint32> SpecializationIdList;
    std::list<uint32> OverrideSpellList;

    uint32 talentId;

    // struct access functions
    SpellTargetRestrictionsEntry const* GetSpellTargetRestrictions() const;
    SpellAuraOptionsEntry const* GetSpellAuraOptions() const;
    SpellAuraRestrictionsEntry const* GetSpellAuraRestrictions() const;
    SpellCastingRequirementsEntry const* GetSpellCastingRequirements() const;
    SpellCategoriesEntry const* GetSpellCategories() const;
    SpellClassOptionsEntry const* GetSpellClassOptions() const;
    SpellCooldownsEntry const* GetSpellCooldowns() const;
    SpellEquippedItemsEntry const* GetSpellEquippedItems() const;
    SpellInterruptsEntry const* GetSpellInterrupts() const;
    SpellLevelsEntry const* GetSpellLevels() const;
    SpellReagentsEntry const* GetSpellReagents() const;
    SpellScalingEntry const* GetSpellScaling() const;
    SpellShapeshiftEntry const* GetSpellShapeshift() const;
    SpellTotemsEntry const* GetSpellTotems() const;

    SpellInfo(SpellEntry const* spellEntry, SpellEffectEntry const** effects);
    ~SpellInfo();

    uint32 GetCategory() const;
    bool HasEffect(SpellEffects effect) const;
    bool HasAura(AuraType aura) const;
    bool HasAreaAuraEffect() const;

    bool IsExplicitDiscovery() const;
    bool IsLootCrafting() const;
    bool IsQuestTame() const;
    bool IsProfessionOrRiding() const;
    bool IsProfession() const;
    bool IsPrimaryProfession() const;
    bool IsPrimaryProfessionFirstRank() const;
    bool IsAbilityLearnedWithProfession() const;
    bool IsAbilityOfSkillType(uint32 skillType) const;

    bool IsAffectingArea() const;
    bool IsTargetingArea() const;
    bool NeedsExplicitUnitTarget() const;
    bool NeedsToBeTriggeredByCaster(SpellInfo const* triggeringSpell) const;

    bool IsPassive() const;
    bool IsAutocastable() const;
    bool IsStackableWithRanks() const;
    bool IsPassiveStackableWithRanks() const;
    bool IsMultiSlotAura() const;
    bool IsStackableOnOneSlotWithDifferentCasters() const;
    bool IsCooldownStartedOnEvent() const;
    bool IsDeathPersistent() const;
    bool IsRequiringDeadTarget() const;
    bool IsAllowingDeadTarget() const;
    bool CanBeUsedInCombat() const;
    bool IsPositive() const;
    bool IsPositiveEffect(uint8 effIndex) const;
    bool IsChanneled() const;
    bool NeedsComboPoints() const;
    bool IsBreakingStealth() const;
    bool IsRangedWeaponSpell() const;
    bool IsAutoRepeatRangedSpell() const;

    bool IsAffectedBySpellMods() const;
    bool IsAffectedBySpellMod(SpellModifier const* mod) const;

    bool CanPierceImmuneAura(SpellInfo const* aura) const;
    bool CanDispelAura(SpellInfo const* aura) const;

    bool IsSingleTarget() const;
    bool IsAuraExclusiveBySpecificWith(SpellInfo const* spellInfo) const;
    bool IsAuraExclusiveBySpecificPerCasterWith(SpellInfo const* spellInfo) const;

    SpellCastResult CheckShapeshift(uint32 form) const;
    SpellCastResult CheckLocation(uint32 map_id, uint32 zone_id, uint32 area_id, Player const* player = NULL) const;
    SpellCastResult CheckTarget(Unit const* caster, WorldObject const* target, bool implicit = true) const;
    SpellCastResult CheckExplicitTarget(Unit const* caster, WorldObject const* target, Item const* itemTarget = NULL) const;
    SpellCastResult CheckVehicle(Unit const* caster) const;
    bool CheckTargetCreatureType(Unit const* target) const;

    SpellSchoolMask GetSchoolMask() const;
    uint32 GetAllEffectsMechanicMask() const;
    uint32 GetEffectMechanicMask(uint8 effIndex) const;
    uint32 GetSpellMechanicMaskByEffectMask(uint32 effectMask) const;
    Mechanics GetEffectMechanic(uint8 effIndex) const;
    bool HasAnyEffectMechanic() const;
    uint32 GetDispelMask() const;
    static uint32 GetDispelMask(DispelType type);
    uint32 GetExplicitTargetMask() const;

    AuraStateType GetAuraState() const;
    SpellSpecificType GetSpellSpecific() const;

    float GetMinRange(bool positive = false) const;
    float GetMaxRange(bool positive = false, Unit* caster = NULL, Spell* spell = NULL) const;

    int32 GetDuration() const;
    int32 GetMaxDuration() const;

    uint32 GetMaxTicks() const;

    uint32 CalcCastTime(uint8 level = 0, Spell* spell = NULL) const;
    uint32 GetRecoveryTime() const;

    //int32 CalcPowerCost(Unit const* caster, SpellSchoolMask schoolMask) const;

    bool IsRanked() const;
    uint8 GetRank() const;
    SpellInfo const* GetFirstRankSpell() const;
    SpellInfo const* GetLastRankSpell() const;
    SpellInfo const* GetNextRankSpell() const;
    SpellInfo const* GetPrevRankSpell() const;
    SpellInfo const* GetAuraRankForLevel(uint8 level) const;
    bool IsRankOf(SpellInfo const* spellInfo) const;
    bool IsDifferentRankOf(SpellInfo const* spellInfo) const;
    bool IsHighRankOf(SpellInfo const* spellInfo) const;

    // loading helpers
    void _InitializeExplicitTargetMask();
    bool _IsPositiveEffect(uint8 effIndex, bool deep) const;
    bool _IsPositiveSpell() const;
    static bool _IsPositiveTarget(uint32 targetA, uint32 targetB);

    // unloading helpers
    void _UnloadImplicitTargetConditionLists();
};

#endif // _SPELLINFO_H
