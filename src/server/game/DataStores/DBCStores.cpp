/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "DBCfmt.h"
#include "DBCStores.h"
#include "ItemPrototype.h"
#include "Log.h"
#include "ObjectDefines.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"
#include "TransportMgr.h"

#include <map>

typedef std::map<uint16, uint32> AreaFlagByAreaID;
typedef std::map<uint32, uint32> AreaFlagByMapID;

struct WMOAreaTableTripple
{
    WMOAreaTableTripple(int32 r, int32 a, int32 g) : groupId(g), rootId(r), adtId(a)
    {
    }

    bool operator <(const WMOAreaTableTripple& b) const
    {
        return memcmp(this, &b, sizeof(WMOAreaTableTripple)) < 0;
    }

    // ordered by entropy; that way memcmp will have a minimal medium runtime
    int32 groupId;
    int32 rootId;
    int32 adtId;
};

typedef std::map<WMOAreaTableTripple, WMOAreaTableEntry const*> WMOAreaInfoByTripple;

DBCStorage <AreaTableEntry> sAreaStore(AreaTableEntryfmt);
DBCStorage <AreaGroupEntry> sAreaGroupStore(AreaGroupEntryfmt);
//DBCStorage <AreaPOIEntry> sAreaPOIStore(AreaPOIEntryfmt);
static AreaFlagByAreaID sAreaFlagByAreaID;
static AreaFlagByMapID sAreaFlagByMapID;                    // for instances without generated *.map files

static WMOAreaInfoByTripple sWMOAreaInfoByTripple;

DBCStorage <AchievementEntry> sAchievementStore(Achievementfmt);
//DBCStorage <AchievementCriteriaEntry> sAchievementCriteriaStore(AchievementCriteriafmt);
DBCStorage <AreaTriggerEntry> sAreaTriggerStore(AreaTriggerEntryfmt);
DBCStorage <ArmorLocationEntry> sArmorLocationStore(ArmorLocationfmt);
DBCStorage <AuctionHouseEntry> sAuctionHouseStore(AuctionHouseEntryfmt);
DBCStorage <BankBagSlotPricesEntry> sBankBagSlotPricesStore(BankBagSlotPricesEntryfmt);
DBCStorage <BannedAddOnsEntry> sBannedAddOnsStore(BannedAddOnsfmt);
DBCStorage <BattlemasterListEntry> sBattlemasterListStore(BattlemasterListEntryfmt);
DBCStorage <BarberShopStyleEntry> sBarberShopStyleStore(BarberShopStyleEntryfmt);
DBCStorage <CharStartOutfitEntry> sCharStartOutfitStore(CharStartOutfitEntryfmt);
std::map<uint32, CharStartOutfitEntry const*> sCharStartOutfitMap;
DBCStorage <CharTitlesEntry> sCharTitlesStore(CharTitlesEntryfmt);
DBCStorage <ChatChannelsEntry> sChatChannelsStore(ChatChannelsEntryfmt);
DBCStorage <ChrClassesEntry> sChrClassesStore(ChrClassesEntryfmt);
DBCStorage <ChrRacesEntry> sChrRacesStore(ChrRacesEntryfmt);
DBCStorage <ChrPowerTypesEntry> sChrPowerTypesStore(ChrClassesXPowerTypesfmt);
DBCStorage <ChrSpecializationEntry> sChrSpecializationStore(ChrSpecializationfmt);
DBCStorage <CinematicCameraEntry> sCinematicCameraStore(CinematicCameraEntryfmt);
DBCStorage <CinematicSequencesEntry> sCinematicSequencesStore(CinematicSequencesEntryfmt);
DBCStorage <CreatureDisplayInfoEntry> sCreatureDisplayInfoStore(CreatureDisplayInfofmt);
DBCStorage <CreatureFamilyEntry> sCreatureFamilyStore(CreatureFamilyfmt);
DBCStorage <CreatureModelDataEntry> sCreatureModelDataStore(CreatureModelDatafmt);
DBCStorage <CreatureSpellDataEntry> sCreatureSpellDataStore(CreatureSpellDatafmt);
DBCStorage <CreatureTypeEntry> sCreatureTypeStore(CreatureTypefmt);
DBCStorage <CurrencyTypesEntry> sCurrencyTypesStore(CurrencyTypesfmt);
DBCStorage <CriteriaEntry> sCriteriaStore(Criteriafmt);
DBCStorage <CriteriaTreeEntry> sCriteriaTreeStore(CriteriaTreefmt);
uint32 PowersByClass[MAX_CLASSES][MAX_POWERS];

DBCStorage <DestructibleModelDataEntry> sDestructibleModelDataStore(DestructibleModelDatafmt);
DBCStorage <DifficultyEntry> sDifficultyStore(Difficultyfmt);
DBCStorage <DungeonEncounterEntry> sDungeonEncounterStore(DungeonEncounterfmt);
DBCStorage <DurabilityQualityEntry> sDurabilityQualityStore(DurabilityQualityfmt);
DBCStorage <DurabilityCostsEntry> sDurabilityCostsStore(DurabilityCostsfmt);

DBCStorage <EmotesEntry> sEmotesStore(EmotesEntryfmt);
DBCStorage <EmotesTextEntry> sEmotesTextStore(EmotesTextEntryfmt);

typedef std::map<uint32, SimpleFactionsList> FactionTeamMap;
static FactionTeamMap sFactionTeamMap;
DBCStorage <FactionEntry> sFactionStore(FactionEntryfmt);
DBCStorage <FactionTemplateEntry> sFactionTemplateStore(FactionTemplateEntryfmt);

DBCStorage <GameObjectDisplayInfoEntry> sGameObjectDisplayInfoStore(GameObjectDisplayInfofmt);
DBCStorage <GemPropertiesEntry> sGemPropertiesStore(GemPropertiesEntryfmt);
DBCStorage <GlyphPropertiesEntry> sGlyphPropertiesStore(GlyphPropertiesfmt);
DBCStorage <GlyphSlotEntry> sGlyphSlotStore(GlyphSlotfmt);

DBCStorage <GtBarberShopCostBaseEntry>    sGtBarberShopCostBaseStore(GtBarberShopCostBasefmt);
DBCStorage <GtCombatRatingsEntry>         sGtCombatRatingsStore(GtCombatRatingsfmt);
DBCStorage <GtChanceToMeleeCritBaseEntry> sGtChanceToMeleeCritBaseStore(GtChanceToMeleeCritBasefmt);
DBCStorage <GtChanceToMeleeCritEntry>     sGtChanceToMeleeCritStore(GtChanceToMeleeCritfmt);
DBCStorage <GtChanceToSpellCritBaseEntry> sGtChanceToSpellCritBaseStore(GtChanceToSpellCritBasefmt);
DBCStorage <GtChanceToSpellCritEntry>     sGtChanceToSpellCritStore(GtChanceToSpellCritfmt);
DBCStorage <GtNPCManaCostScalerEntry>     sGtNPCManaCostScalerStore(GtNPCManaCostScalerfmt);
DBCStorage <GtOCTClassCombatRatingScalarEntry> sGtOCTClassCombatRatingScalarStore(GtOCTClassCombatRatingScalarfmt);
DBCStorage <gtOCTHpPerStaminaEntry>       sGtOCTHpPerStaminaStore(GtOCTHpPerStaminafmt);
DBCStorage <GtRegenMPPerSptEntry>         sGtRegenMPPerSptStore(GtRegenMPPerSptfmt);
DBCStorage <GtSpellScalingEntry>          sGtSpellScalingStore(GtSpellScalingfmt);
DBCStorage <GtOCTBaseHPByClassEntry>      sGtOCTBaseHPByClassStore(GtOCTBaseHPByClassfmt);
DBCStorage <GtOCTBaseMPByClassEntry>      sGtOCTBaseMPByClassStore(GtOCTBaseMPByClassfmt);
DBCStorage <GuildPerkSpellsEntry>         sGuildPerkSpellsStore(GuildPerkSpellsfmt);

DBCStorage <HolidaysEntry>                sHolidaysStore(Holidaysfmt);

DBCStorage <ImportPriceArmorEntry>        sImportPriceArmorStore(ImportPriceArmorfmt);
DBCStorage <ImportPriceQualityEntry>      sImportPriceQualityStore(ImportPriceQualityfmt);
DBCStorage <ImportPriceShieldEntry>       sImportPriceShieldStore(ImportPriceShieldfmt);
DBCStorage <ImportPriceWeaponEntry>       sImportPriceWeaponStore(ImportPriceWeaponfmt);
DBCStorage <ItemPriceBaseEntry>           sItemPriceBaseStore(ItemPriceBasefmt);
DBCStorage <ItemReforgeEntry>             sItemReforgeStore(ItemReforgefmt);
DBCStorage <ItemArmorQualityEntry>        sItemArmorQualityStore(ItemArmorQualityfmt);
DBCStorage <ItemArmorShieldEntry>         sItemArmorShieldStore(ItemArmorShieldfmt);
DBCStorage <ItemArmorTotalEntry>          sItemArmorTotalStore(ItemArmorTotalfmt);
DBCStorage <ItemClassEntry>               sItemClassStore(ItemClassfmt);
//DBCStorage <ItemBagFamilyEntry>           sItemBagFamilyStore(ItemBagFamilyfmt);
DBCStorage <ItemDamageEntry>              sItemDamageAmmoStore(ItemDamagefmt);
DBCStorage <ItemDamageEntry>              sItemDamageOneHandStore(ItemDamagefmt);
DBCStorage <ItemDamageEntry>              sItemDamageOneHandCasterStore(ItemDamagefmt);
DBCStorage <ItemDamageEntry>              sItemDamageRangedStore(ItemDamagefmt);
DBCStorage <ItemDamageEntry>              sItemDamageThrownStore(ItemDamagefmt);
DBCStorage <ItemDamageEntry>              sItemDamageTwoHandStore(ItemDamagefmt);
DBCStorage <ItemDamageEntry>              sItemDamageTwoHandCasterStore(ItemDamagefmt);
DBCStorage <ItemDamageEntry>              sItemDamageWandStore(ItemDamagefmt);
DBCStorage <ItemDisenchantLootEntry>      sItemDisenchantLootStore(ItemDisenchantLootfmt);
//DBCStorage <ItemDisplayInfoEntry> sItemDisplayInfoStore(ItemDisplayTemplateEntryfmt); -- not used currently
DBCStorage <ItemLimitCategoryEntry> sItemLimitCategoryStore(ItemLimitCategoryEntryfmt);
DBCStorage <ItemRandomPropertiesEntry> sItemRandomPropertiesStore(ItemRandomPropertiesfmt);
DBCStorage <ItemRandomSuffixEntry> sItemRandomSuffixStore(ItemRandomSuffixfmt);
DBCStorage <ItemSetEntry> sItemSetStore(ItemSetEntryfmt);

DBCStorage <LFGDungeonEntry> sLFGDungeonStore(LFGDungeonEntryfmt);
DBCStorage <LiquidTypeEntry> sLiquidTypeStore(LiquidTypefmt);
DBCStorage <LockEntry> sLockStore(LockEntryfmt);

DBCStorage <MailTemplateEntry> sMailTemplateStore(MailTemplateEntryfmt);
DBCStorage <MapEntry> sMapStore(MapEntryfmt);

// DBC used only for initialization sMapDifficultyMap at startup.
DBCStorage <MapDifficultyEntry> sMapDifficultyStore(MapDifficultyEntryfmt); // only for loading
MapDifficultyMap sMapDifficultyMap;

DBCStorage <ModifierTreeEntry> sModifierTreeStore(ModifierTreefmt);
DBCStorage <MovieEntry> sMovieStore(MovieEntryfmt);
DBCStorage <MountCapabilityEntry> sMountCapabilityStore(MountCapabilityfmt);
DBCStorage <MountTypeEntry> sMountTypeStore(MountTypefmt);

DBCStorage <NameGenEntry> sNameGenStore(NameGenfmt);
NameGenVectorArraysMap sGenNameVectoArraysMap;

DBCStorage <OverrideSpellDataEntry> sOverrideSpellDataStore(OverrideSpellDatafmt);

DBCStorage <PvPDifficultyEntry> sPvPDifficultyStore(PvPDifficultyfmt);

DBCStorage <QuestSortEntry> sQuestSortStore(QuestSortEntryfmt);
DBCStorage <QuestXPEntry>   sQuestXPStore(QuestXPfmt);
DBCStorage <QuestFactionRewEntry>  sQuestFactionRewardStore(QuestFactionRewardfmt);
DBCStorage <QuestPOIPointEntry> sQuestPOIPointStore(QuestPOIPointfmt);
DBCStorage <RandomPropertiesPointsEntry> sRandomPropertiesPointsStore(RandomPropertiesPointsfmt);

//DBCStorage <ResearchBranchEntry>  sResearchBranchStore(ResearchBranchfmt);
//DBCStorage <ResearchProjectEntry> sResearchProjectStore(ResearchProjectfmt);
//DBCStorage <ResearchSiteEntry>    sResearchSiteStore(ResearchSitefmt);
//static DigsitePOIPolygonContainer sDigsitePOIPolygons;

DBCStorage <ScalingStatDistributionEntry> sScalingStatDistributionStore(ScalingStatDistributionfmt);
DBCStorage <ScalingStatValuesEntry> sScalingStatValuesStore(ScalingStatValuesfmt);

DBCStorage <SkillLineEntry> sSkillLineStore(SkillLinefmt);
DBCStorage <SkillLineAbilityEntry> sSkillLineAbilityStore(SkillLineAbilityfmt);

DBCStorage <SoundEntriesEntry> sSoundEntriesStore(SoundEntriesfmt);

DBCStorage <SpecializationSpellsEntry> sSpecializationSpellsStore(SpecializationSpellsfmt);
DBCStorage <SpellItemEnchantmentEntry> sSpellItemEnchantmentStore(SpellItemEnchantmentfmt);
DBCStorage <SpellItemEnchantmentConditionEntry> sSpellItemEnchantmentConditionStore(SpellItemEnchantmentConditionfmt);
DBCStorage <SpellEntry> sSpellStore(SpellEntryfmt);
DBCStorage <SpellMiscEntry> sSpellMiscStore(SpellMiscfmt);
DBCStorage <SpellEffectScalingEntry> sSpellEffectScalingStore(SpellEffectScalingfmt);

SpellCategoryStore sSpellsByCategoryStore;
PetFamilySpellsStore sPetFamilySpellsStore;
SpellsPerClassStore sSpellsPerClassStore;
ClassBySkillIdStore sClassBySkillIdStore;
SpellEffectScallingByEffectId sSpellEffectScallingByEffectId;

DBCStorage <SpellScalingEntry> sSpellScalingStore(SpellScalingEntryfmt);
DBCStorage <SpellTotemsEntry> sSpellTotemsStore(SpellTotemsEntryfmt);
DBCStorage <SpellTargetRestrictionsEntry> sSpellTargetRestrictionsStore(SpellTargetRestrictionsEntryfmt);
DBCStorage <SpellPowerEntry> sSpellPowerStore(SpellPowerEntryfmt);
DBCStorage <SpellLevelsEntry> sSpellLevelsStore(SpellLevelsEntryfmt);
DBCStorage <SpellInterruptsEntry> sSpellInterruptsStore(SpellInterruptsEntryfmt);
DBCStorage <SpellEquippedItemsEntry> sSpellEquippedItemsStore(SpellEquippedItemsEntryfmt);
DBCStorage <SpellClassOptionsEntry> sSpellClassOptionsStore(SpellClassOptionsEntryfmt);
DBCStorage <SpellCooldownsEntry> sSpellCooldownsStore(SpellCooldownsEntryfmt);
DBCStorage <SpellAuraOptionsEntry> sSpellAuraOptionsStore(SpellAuraOptionsEntryfmt);
DBCStorage <SpellAuraRestrictionsEntry> sSpellAuraRestrictionsStore(SpellAuraRestrictionsEntryfmt);
DBCStorage <SpellCastingRequirementsEntry> sSpellCastingRequirementsStore(SpellCastingRequirementsEntryfmt);
DBCStorage <SpellCastTimesEntry> sSpellCastTimesStore(SpellCastTimefmt);
DBCStorage <SpellCategoriesEntry> sSpellCategoriesStore(SpellCategoriesEntryfmt);
DBCStorage <SpellCategoryEntry> sSpellCategoryStore(SpellCategoryfmt);
DBCStorage <SpellEffectEntry> sSpellEffectStore(SpellEffectEntryfmt);
DBCStorage <SpellDifficultyEntry> sSpellDifficultyStore(SpellDifficultyfmt);
DBCStorage <SpellDurationEntry> sSpellDurationStore(SpellDurationfmt);
DBCStorage <SpellFocusObjectEntry> sSpellFocusObjectStore(SpellFocusObjectfmt);
DBCStorage <SpellRadiusEntry> sSpellRadiusStore(SpellRadiusfmt);
DBCStorage <SpellRangeEntry> sSpellRangeStore(SpellRangefmt);
DBCStorage <SpellRuneCostEntry> sSpellRuneCostStore(SpellRuneCostfmt);
DBCStorage <SpellShapeshiftEntry> sSpellShapeshiftStore(SpellShapeshiftEntryfmt);
DBCStorage <SpellShapeshiftFormEntry> sSpellShapeshiftFormStore(SpellShapeshiftFormfmt);
DBCStorage <SummonPropertiesEntry> sSummonPropertiesStore(SummonPropertiesfmt);
DBCStorage <TalentEntry> sTalentStore(TalentEntryfmt);
TalentSpellPosMap sTalentSpellPosMap;
typedef std::map<uint32, std::vector<uint32> > SpecializationSpellsMap;

SpecializationSpellsMap sSpecializationSpellsMap;

// store absolute bit position for first rank for talent inspect
static uint32 sSpecializationClassStore[MAX_CLASSES][4];

DBCStorage <TaxiNodesEntry> sTaxiNodesStore(TaxiNodesEntryfmt);
TaxiMask sTaxiNodesMask;
TaxiMask sOldContinentsNodesMask;
TaxiMask sHordeTaxiNodesMask;
TaxiMask sAllianceTaxiNodesMask;
TaxiMask sDeathKnightTaxiNodesMask;

// DBC used only for initialization sTaxiPathSetBySource at startup.
TaxiPathSetBySource sTaxiPathSetBySource;
DBCStorage <TaxiPathEntry> sTaxiPathStore(TaxiPathEntryfmt);

// DBC used only for initialization sTaxiPathNodeStore at startup.
TaxiPathNodesByPath sTaxiPathNodesByPath;
static DBCStorage <TaxiPathNodeEntry> sTaxiPathNodeStore(TaxiPathNodeEntryfmt);

DBCStorage <TotemCategoryEntry> sTotemCategoryStore(TotemCategoryEntryfmt);
DBCStorage <TransportAnimationEntry> sTransportAnimationStore(TransportAnimationfmt);
DBCStorage <TransportRotationEntry> sTransportRotationStore(TransportRotationfmt);
DBCStorage <UnitPowerBarEntry> sUnitPowerBarStore(UnitPowerBarfmt);
DBCStorage <VehicleEntry> sVehicleStore(VehicleEntryfmt);
DBCStorage <VehicleSeatEntry> sVehicleSeatStore(VehicleSeatEntryfmt);
DBCStorage <WMOAreaTableEntry> sWMOAreaTableStore(WMOAreaTableEntryfmt);
DBCStorage <WorldMapAreaEntry> sWorldMapAreaStore(WorldMapAreaEntryfmt);
DBCStorage <WorldMapOverlayEntry> sWorldMapOverlayStore(WorldMapOverlayEntryfmt);
DBCStorage <WorldSafeLocsEntry> sWorldSafeLocsStore(WorldSafeLocsEntryfmt);
DBCStorage <PhaseEntry> sPhaseStore(PhaseEntryfmt);
DBCStorage <PhaseGroupEntry> sPhaseGroupStore(PhaseGroupfmt);

PhaseGroupContainer sPhasesByGroup;

typedef std::list<std::string> StoreProblemList;

uint32 DBCFileCount = 0;

static bool LoadDBC_assert_print(uint32 fsize, uint32 rsize, const std::string& filename)
{
    SF_LOG_ERROR("misc", "Size of '%s' set by format string (%u) not equal size of C++ structure (%u).", filename.c_str(), fsize, rsize);

    // ASSERT must fail after function call
    return false;
}

template<class T>
inline void LoadDBC(uint32& availableDbcLocales, StoreProblemList& errors, DBCStorage<T>& storage, std::string const& dbcPath, std::string const& filename, std::string const* customFormat = NULL, std::string const* customIndexName = NULL)
{
    // compatibility format and C++ structure sizes
    ASSERT(DBCFileLoader::GetFormatRecordSize(storage.GetFormat()) == sizeof(T) || LoadDBC_assert_print(DBCFileLoader::GetFormatRecordSize(storage.GetFormat()), sizeof(T), filename));

    ++DBCFileCount;
    std::string dbcFilename = dbcPath + filename;
    SqlDbc* sql = NULL;
    if (customFormat)
        sql = new SqlDbc(&filename, customFormat, customIndexName, storage.GetFormat());

    if (storage.Load(dbcFilename.c_str(), sql))
    {
        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
        {
            if (!(availableDbcLocales & (1 << i)))
                continue;

            std::string localizedName(dbcPath);
            localizedName.append(localeNames[i]);
            localizedName.push_back('/');
            localizedName.append(filename);

            if (!storage.LoadStringsFrom(localizedName.c_str()))
                availableDbcLocales &= ~(1 << i);             // mark as not available for speedup next checks
        }
    }
    else
    {
        // sort problematic dbc to (1) non compatible and (2) non-existed
        if (FILE* f = fopen(dbcFilename.c_str(), "rb"))
        {
            std::ostringstream stream;
            stream << dbcFilename << " exists, and has " << storage.GetFieldCount() << " field(s) (expected " << strlen(storage.GetFormat()) << "). Extracted file might be from wrong client version or a database-update has been forgotten.";
            std::string buf = stream.str();
            errors.push_back(buf);
            fclose(f);
        }
        else
            errors.push_back(dbcFilename);
    }

    delete sql;
}

void LoadDBCStores(const std::string& dataPath)
{
    uint32 oldMSTime = getMSTime();

    std::string dbcPath = dataPath + "dbc/";

    StoreProblemList bad_dbc_files;
    uint32 availableDbcLocales = 0xFFFFFFFF;

    LoadDBC(availableDbcLocales, bad_dbc_files, sAreaStore, dbcPath, "AreaTable.dbc");

    // must be after sAreaStore loading
    for (uint32 i = 0; i < sAreaStore.GetNumRows(); ++i)           // areaflag numbered from 0
    {
        if (AreaTableEntry const* area = sAreaStore.LookupEntry(i))
        {
            // fill AreaId->DBC records
            sAreaFlagByAreaID.insert(AreaFlagByAreaID::value_type(uint16(area->m_ID), area->m_AreaBit));

            // fill MapId->DBC records (skip sub zones and continents)
            if (area->m_ParentAreaID == 0 && area->m_ContinentID != 0 && area->m_ContinentID != 1 && area->m_ContinentID != 530 && area->m_ContinentID != 571 && area->m_ContinentID != 860 && area->m_ContinentID != 870)
                sAreaFlagByMapID.insert(AreaFlagByMapID::value_type(area->m_ContinentID, area->m_AreaBit));
        }
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sAchievementStore, dbcPath, "Achievement.dbc"/*, &CustomAchievementfmt, &CustomAchievementIndex*/);//15595
    //LoadDBC(availableDbcLocales, bad_dbc_files, sAchievementCriteriaStore,    dbcPath, "Achievement_Criteria.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sAreaTriggerStore, dbcPath, "AreaTrigger.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sAreaGroupStore, dbcPath, "AreaGroup.dbc");//15595
    //LoadDBC(availableDbcLocales, bad_dbc_files, sAreaPOIStore,                dbcPath, "AreaPOI.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sAuctionHouseStore, dbcPath, "AuctionHouse.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sArmorLocationStore, dbcPath, "ArmorLocation.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sBankBagSlotPricesStore, dbcPath, "BankBagSlotPrices.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sBannedAddOnsStore, dbcPath, "BannedAddOns.dbc");
    LoadDBC(availableDbcLocales, bad_dbc_files, sBattlemasterListStore, dbcPath, "BattleMasterList.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sBarberShopStyleStore, dbcPath, "BarberShopStyle.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sCharStartOutfitStore, dbcPath, "CharStartOutfit.dbc");//15595
    for (uint32 i = 0; i < sCharStartOutfitStore.GetNumRows(); ++i)
        if (CharStartOutfitEntry const* outfit = sCharStartOutfitStore.LookupEntry(i))
            sCharStartOutfitMap[outfit->Race | (outfit->Class << 8) | (outfit->Gender << 16)] = outfit;

    LoadDBC(availableDbcLocales, bad_dbc_files, sCharTitlesStore, dbcPath, "CharTitles.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sChatChannelsStore, dbcPath, "ChatChannels.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sChrClassesStore, dbcPath, "ChrClasses.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sChrRacesStore, dbcPath, "ChrRaces.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sChrPowerTypesStore, dbcPath, "ChrClassesXPowerTypes.dbc");//15595
    for (uint32 i = 0; i < MAX_CLASSES; ++i)
        for (uint32 j = 0; j < MAX_POWERS; ++j)
            PowersByClass[i][j] = MAX_POWERS;

    for (uint32 i = 0; i < sChrPowerTypesStore.GetNumRows(); ++i)
    {
        if (ChrPowerTypesEntry const* power = sChrPowerTypesStore.LookupEntry(i))
        {
            uint32 index = 0;
            for (uint32 j = 0; j < MAX_POWERS; ++j)
                if (PowersByClass[power->classId][j] != MAX_POWERS)
                    ++index;

            PowersByClass[power->classId][power->power] = index;
        }
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sCinematicCameraStore, dbcPath, "CinematicCamera.dbc"); // 18414
    LoadDBC(availableDbcLocales, bad_dbc_files, sCinematicSequencesStore, dbcPath, "CinematicSequences.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sCreatureDisplayInfoStore, dbcPath, "CreatureDisplayInfo.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sCreatureFamilyStore, dbcPath, "CreatureFamily.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sCreatureModelDataStore, dbcPath, "CreatureModelData.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sCreatureSpellDataStore, dbcPath, "CreatureSpellData.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sCreatureTypeStore, dbcPath, "CreatureType.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sCurrencyTypesStore, dbcPath, "CurrencyTypes.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sCriteriaStore, dbcPath, "Criteria.dbc");//18414
    LoadDBC(availableDbcLocales, bad_dbc_files, sCriteriaTreeStore, dbcPath, "CriteriaTree.dbc");//18414
    LoadDBC(availableDbcLocales, bad_dbc_files, sDestructibleModelDataStore, dbcPath, "DestructibleModelData.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sDifficultyStore, dbcPath, "Difficulty.dbc"); // 18414
    LoadDBC(availableDbcLocales, bad_dbc_files, sDungeonEncounterStore, dbcPath, "DungeonEncounter.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sDurabilityCostsStore, dbcPath, "DurabilityCosts.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sDurabilityQualityStore, dbcPath, "DurabilityQuality.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sEmotesStore, dbcPath, "Emotes.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sEmotesTextStore, dbcPath, "EmotesText.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sFactionStore, dbcPath, "Faction.dbc");//15595
    for (uint32 i = 0; i < sFactionStore.GetNumRows(); ++i)
    {
        FactionEntry const* faction = sFactionStore.LookupEntry(i);
        if (faction && faction->team)
        {
            SimpleFactionsList& flist = sFactionTeamMap[faction->team];
            flist.push_back(i);
        }
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sFactionTemplateStore, dbcPath, "FactionTemplate.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGameObjectDisplayInfoStore, dbcPath, "GameObjectDisplayInfo.dbc");//15595
    for (uint32 i = 0; i < sGameObjectDisplayInfoStore.GetNumRows(); ++i)
    {
        if (GameObjectDisplayInfoEntry const* info = sGameObjectDisplayInfoStore.LookupEntry(i))
        {
            if (info->maxX < info->minX)
                std::swap(*(float*)(&info->maxX), *(float*)(&info->minX));
            if (info->maxY < info->minY)
                std::swap(*(float*)(&info->maxY), *(float*)(&info->minY));
            if (info->maxZ < info->minZ)
                std::swap(*(float*)(&info->maxZ), *(float*)(&info->minZ));
        }
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sGemPropertiesStore, dbcPath, "GemProperties.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGlyphPropertiesStore, dbcPath, "GlyphProperties.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGlyphSlotStore, dbcPath, "GlyphSlot.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtBarberShopCostBaseStore, dbcPath, "gtBarberShopCostBase.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtCombatRatingsStore, dbcPath, "gtCombatRatings.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtChanceToMeleeCritBaseStore, dbcPath, "gtChanceToMeleeCritBase.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtChanceToMeleeCritStore, dbcPath, "gtChanceToMeleeCrit.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtChanceToSpellCritBaseStore, dbcPath, "gtChanceToSpellCritBase.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtChanceToSpellCritStore, dbcPath, "gtChanceToSpellCrit.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtNPCManaCostScalerStore, dbcPath, "gtNPCManaCostScaler.dbc");
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtOCTClassCombatRatingScalarStore, dbcPath, "gtOCTClassCombatRatingScalar.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtOCTHpPerStaminaStore, dbcPath, "gtOCTHpPerStamina.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtRegenMPPerSptStore, dbcPath, "gtRegenMPPerSpt.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtSpellScalingStore, dbcPath, "gtSpellScaling.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtOCTBaseHPByClassStore, dbcPath, "gtOCTBaseHPByClass.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGtOCTBaseMPByClassStore, dbcPath, "gtOCTBaseMPByClass.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sGuildPerkSpellsStore, dbcPath, "GuildPerkSpells.dbc");//15595

    LoadDBC(availableDbcLocales, bad_dbc_files, sHolidaysStore, dbcPath, "Holidays.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sImportPriceArmorStore, dbcPath, "ImportPriceArmor.dbc"); // 15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sImportPriceQualityStore, dbcPath, "ImportPriceQuality.dbc"); // 15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sImportPriceShieldStore, dbcPath, "ImportPriceShield.dbc"); // 15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sImportPriceWeaponStore, dbcPath, "ImportPriceWeapon.dbc"); // 15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemPriceBaseStore, dbcPath, "ItemPriceBase.dbc"); // 15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemReforgeStore, dbcPath, "ItemReforge.dbc"); // 15595
    //LoadDBC(availableDbcLocales, bad_dbc_files, sItemBagFamilyStore,          dbcPath, "ItemBagFamily.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemClassStore, dbcPath, "ItemClass.dbc"); // 15595
    //LoadDBC(dbcCount, availableDbcLocales, bad_dbc_files, sItemDisplayInfoStore,        dbcPath, "ItemDisplayInfo.dbc");     -- not used currently
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemLimitCategoryStore, dbcPath, "ItemLimitCategory.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemRandomPropertiesStore, dbcPath, "ItemRandomProperties.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemRandomSuffixStore, dbcPath, "ItemRandomSuffix.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemSetStore, dbcPath, "ItemSet.dbc");//15595

    LoadDBC(availableDbcLocales, bad_dbc_files, sItemArmorQualityStore, dbcPath, "ItemArmorQuality.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemArmorShieldStore, dbcPath, "ItemArmorShield.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemArmorTotalStore, dbcPath, "ItemArmorTotal.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDamageAmmoStore, dbcPath, "ItemDamageAmmo.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDamageOneHandStore, dbcPath, "ItemDamageOneHand.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDamageOneHandCasterStore, dbcPath, "ItemDamageOneHandCaster.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDamageRangedStore, dbcPath, "ItemDamageRanged.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDamageThrownStore, dbcPath, "ItemDamageThrown.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDamageTwoHandStore, dbcPath, "ItemDamageTwoHand.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDamageTwoHandCasterStore, dbcPath, "ItemDamageTwoHandCaster.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDamageWandStore, dbcPath, "ItemDamageWand.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sItemDisenchantLootStore, dbcPath, "ItemDisenchantLoot.dbc");

    LoadDBC(availableDbcLocales, bad_dbc_files, sLFGDungeonStore, dbcPath, "LfgDungeons.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sLiquidTypeStore, dbcPath, "LiquidType.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sLockStore, dbcPath, "Lock.dbc");//15595

    LoadDBC(availableDbcLocales, bad_dbc_files, sMailTemplateStore, dbcPath, "MailTemplate.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sMapStore, dbcPath, "Map.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sMapDifficultyStore, dbcPath, "MapDifficulty.dbc");//15595
    // fill data
    sMapDifficultyMap[0][0] = MapDifficulty(DIFFICULTY_NONE, "", 0, 0, false);//map 0 is missingg from MapDifficulty.dbc use this till its ported to sql
    for (uint32 i = 0; i < sMapDifficultyStore.GetNumRows(); ++i)
        if (MapDifficultyEntry const* entry = sMapDifficultyStore.LookupEntry(i))
            sMapDifficultyMap[entry->MapId][entry->Difficulty] = MapDifficulty(entry->Difficulty, entry->areaTriggerText, entry->resetTime, entry->maxPlayers, entry->areaTriggerText[0] > 0);
    sMapDifficultyStore.Clear();

    LoadDBC(availableDbcLocales, bad_dbc_files, sMountCapabilityStore, dbcPath, "MountCapability.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sMountTypeStore, dbcPath, "MountType.dbc");//15595

    LoadDBC(availableDbcLocales, bad_dbc_files, sNameGenStore, dbcPath, "NameGen.dbc");//15595
    for (uint32 i = 0; i < sNameGenStore.GetNumRows(); ++i)
        if (NameGenEntry const* entry = sNameGenStore.LookupEntry(i))
            sGenNameVectoArraysMap[entry->race].stringVectorArray[entry->gender].push_back(std::string(entry->name));
    sNameGenStore.Clear();

    LoadDBC(availableDbcLocales, bad_dbc_files, sModifierTreeStore, dbcPath, "ModifierTree.dbc");//18414
    LoadDBC(availableDbcLocales, bad_dbc_files, sMovieStore, dbcPath, "Movie.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sOverrideSpellDataStore, dbcPath, "OverrideSpellData.dbc");//15595

    LoadDBC(availableDbcLocales, bad_dbc_files, sPhaseStore, dbcPath, "Phase.dbc"); // 18414
    LoadDBC(availableDbcLocales, bad_dbc_files, sPhaseGroupStore, dbcPath, "PhaseXPhaseGroup.dbc"); // 18414
    for (uint32 i = 0; i < sPhaseGroupStore.GetNumRows(); ++i)
        if (PhaseGroupEntry const* group = sPhaseGroupStore.LookupEntry(i))
            if (PhaseEntry const* phase = sPhaseStore.LookupEntry(group->PhaseId))
                sPhasesByGroup[group->GroupId].insert(phase->ID);

    LoadDBC(availableDbcLocales, bad_dbc_files, sPvPDifficultyStore, dbcPath, "PvpDifficulty.dbc");//15595
    for (uint32 i = 0; i < sPvPDifficultyStore.GetNumRows(); ++i)
        if (PvPDifficultyEntry const* entry = sPvPDifficultyStore.LookupEntry(i))
            if (entry->bracketId > MAX_BATTLEGROUND_BRACKETS)
                ASSERT(false && "Need update MAX_BATTLEGROUND_BRACKETS by DBC data");

    LoadDBC(availableDbcLocales, bad_dbc_files, sQuestXPStore, dbcPath, "QuestXP.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sQuestFactionRewardStore, dbcPath, "QuestFactionReward.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sQuestSortStore, dbcPath, "QuestSort.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sQuestPOIPointStore, dbcPath, "QuestPOIPoint.dbc");//15595

    LoadDBC(availableDbcLocales, bad_dbc_files, sRandomPropertiesPointsStore, dbcPath, "RandPropPoints.dbc");//15595
    /*
    LoadDBC(availableDbcLocales, bad_dbc_files, sResearchBranchStore,         dbcPath, "ResearchBranch.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sResearchProjectStore,        dbcPath, "ResearchProject.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sResearchSiteStore,           dbcPath, "ResearchSite.dbc");//15595

    // must be after sQuestPOIPointStore and sResearchSiteStore loading
    for (uint32 i = 0; i < sResearchSiteStore.GetNumRows(); ++i)
    {
        if (ResearchSiteEntry const* siteEntry = sResearchSiteStore.LookupEntry(i))
        {
            for (uint32 j = 0; j < sQuestPOIPointStore.GetNumRows(); ++j)
                if (QuestPOIPointEntry const* pointEntry = sQuestPOIPointStore.LookupEntry(j))
                    if (siteEntry->QuestPOIBlobId == pointEntry->BlobId)
                        sDigsitePOIPolygons [siteEntry->Id].push_back(std::make_pair(pointEntry->X, pointEntry->Y));
        }
    }*/

    LoadDBC(availableDbcLocales, bad_dbc_files, sScalingStatDistributionStore, dbcPath, "ScalingStatDistribution.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sScalingStatValuesStore, dbcPath, "ScalingStatValues.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSkillLineStore, dbcPath, "SkillLine.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSkillLineAbilityStore, dbcPath, "SkillLineAbility.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSoundEntriesStore, dbcPath, "SoundEntries.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellStore, dbcPath, "Spell.dbc"/*, &CustomSpellEntryfmt, &CustomSpellEntryIndex*/);//
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellCategoriesStore, dbcPath, "SpellCategories.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellCategoryStore, dbcPath, "SpellCategory.dbc");
    for (uint32 i = 1; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const* spell = sSpellStore.LookupEntry(i);
        if (!spell)
            continue;

        if (SpellCategoriesEntry const* category = sSpellCategoriesStore.LookupEntry(spell->SpellCategoriesId))
            sSpellsByCategoryStore[category->Category].insert(i);
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellScalingStore, dbcPath, "SpellScaling.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellTotemsStore, dbcPath, "SpellTotems.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellTargetRestrictionsStore, dbcPath, "SpellTargetRestrictions.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellPowerStore, dbcPath, "SpellPower.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellLevelsStore, dbcPath, "SpellLevels.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellInterruptsStore, dbcPath, "SpellInterrupts.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellEquippedItemsStore, dbcPath, "SpellEquippedItems.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellClassOptionsStore, dbcPath, "SpellClassOptions.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellCooldownsStore, dbcPath, "SpellCooldowns.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellAuraOptionsStore, dbcPath, "SpellAuraOptions.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellAuraRestrictionsStore, dbcPath, "SpellAuraRestrictions.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellCastingRequirementsStore, dbcPath, "SpellCastingRequirements.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellEffectStore, dbcPath, "SpellEffect.dbc"/*, &CustomSpellEffectEntryfmt, &CustomSpellEffectEntryIndex*/);//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellCastTimesStore, dbcPath, "SpellCastTimes.dbc");//15595
    // LoadDBC(availableDbcLocales, bad_dbc_files, sSpellDifficultyStore,        dbcPath, "SpellDifficulty.dbc", &CustomSpellDifficultyfmt, &CustomSpellDifficultyIndex);//15595 -- DBC gone, wtf blizzard ...
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellDurationStore, dbcPath, "SpellDuration.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellFocusObjectStore, dbcPath, "SpellFocusObject.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellItemEnchantmentStore, dbcPath, "SpellItemEnchantment.dbc");//18414
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellMiscStore, dbcPath, "SpellMisc.dbc");//17538
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellEffectScalingStore, dbcPath, "SpellEffectScaling.dbc");//17538
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellItemEnchantmentConditionStore, dbcPath, "SpellItemEnchantmentCondition.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellRadiusStore, dbcPath, "SpellRadius.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellRangeStore, dbcPath, "SpellRange.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellRuneCostStore, dbcPath, "SpellRuneCost.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellShapeshiftStore, dbcPath, "SpellShapeshift.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSpellShapeshiftFormStore, dbcPath, "SpellShapeshiftForm.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sSummonPropertiesStore, dbcPath, "SummonProperties.dbc");//15595

    for (uint32 j = 0; j < sSpellEffectScalingStore.GetNumRows(); j++)
    {
        SpellEffectScalingEntry const* spellEffectScaling = sSpellEffectScalingStore.LookupEntry(j);
        if (!spellEffectScaling)
            continue;

        sSpellEffectScallingByEffectId.insert(std::make_pair(spellEffectScaling->SpellEffectId, j));
    }

    std::map<std::string, uint32> classIdByName;
    for (uint32 j = 0; j < sChrClassesStore.GetNumRows(); j++)
    {
        ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(j);
        if (!classEntry)
            continue;

        classIdByName.insert(std::make_pair(std::string(classEntry->name), j));
    }

    for (uint32 j = 0; j < sSkillLineStore.GetNumRows(); j++)
    {
        SkillLineEntry const* skillEntry = sSkillLineStore.LookupEntry(j);
        if (!skillEntry)
            continue;

        if (skillEntry->categoryId != SKILL_CATEGORY_CLASS)
            continue;

        std::map<std::string, uint32> ::const_iterator iter = classIdByName.find(std::string(skillEntry->name));
        if (iter == classIdByName.end())
            continue;

        sClassBySkillIdStore.insert(std::make_pair(j, iter->second));
    }

    for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
    {
        SkillLineAbilityEntry const* skillAbility = sSkillLineAbilityStore.LookupEntry(j);
        if (!skillAbility)
            continue;

        if (skillAbility->learnOnGetSkill != ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL)
            continue;

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(skillAbility->spellId);
        if (!spellInfo)
            continue;

        SpellLevelsEntry const* spellLevels = sSpellLevelsStore.LookupEntry(spellInfo->SpellLevelsId);
        if (!spellLevels || !spellLevels->spellLevel)
            continue;

        uint32 classId = GetClassBySkillId(skillAbility->skillId);

        if (!classId)
            continue;

        if (sSpellsPerClassStore.find(classId) == sSpellsPerClassStore.end())
            sSpellsPerClassStore.insert(make_pair(classId, std::list<SkillLineAbilityEntry const*>()));

        sSpellsPerClassStore[classId].push_back(skillAbility);
    }

    // Must be done when sSkillLineAbilityStore, sSpellStore, sSpellLevelsStore and sCreatureFamilyStore are all loaded
    for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
    {
        SkillLineAbilityEntry const* skillLine = sSkillLineAbilityStore.LookupEntry(j);
        if (!skillLine)
            continue;

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(skillLine->spellId);
        if (!spellInfo)
            continue;

        SpellLevelsEntry const* levels = sSpellLevelsStore.LookupEntry(spellInfo->SpellLevelsId);
        if (spellInfo->SpellLevelsId && (!levels || levels->spellLevel))
            continue;

        if (SpellMiscEntry const* spellMisc = sSpellMiscStore.LookupEntry(spellInfo->SpellMiscId))
        {
            if (spellMisc->Attributes & SPELL_ATTR0_PASSIVE)
            {
                for (uint32 i = 1; i < sCreatureFamilyStore.GetNumRows(); ++i)
                {
                    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(i);
                    if (!cFamily)
                        continue;

                    if (skillLine->skillId != cFamily->skillLine[0] && skillLine->skillId != cFamily->skillLine[1])
                        continue;

                    if (skillLine->learnOnGetSkill != ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL)
                        continue;

                    sPetFamilySpellsStore[i].insert(spellInfo->Id);
                }
            }
        }
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sTalentStore, dbcPath, "Talent.dbc");//15595

    // Create Spelldifficulty searcher
    for (uint32 i = 0; i < sSpellDifficultyStore.GetNumRows(); ++i)
    {
        SpellDifficultyEntry const* spellDiff = sSpellDifficultyStore.LookupEntry(i);
        if (!spellDiff)
            continue;

        SpellDifficultyEntry newEntry;
        memset(newEntry.SpellID, 0, 4 * sizeof(uint32));
        for (uint32 x = 0; x < 4; ++x)
        {
            if (spellDiff->SpellID[x] <= 0 || !sSpellStore.LookupEntry(spellDiff->SpellID[x]))
            {
                if (spellDiff->SpellID[x] > 0)//don't show error if spell is <= 0, not all modes have spells and there are unknown negative values
                    SF_LOG_ERROR("sql.sql", "spelldifficulty_dbc: spell %i at field id:%u at spellid%i does not exist in SpellStore (spell.dbc), loaded as 0", spellDiff->SpellID[x], spellDiff->ID, x);
                newEntry.SpellID[x] = 0;//spell was <= 0 or invalid, set to 0
            }
            else
                newEntry.SpellID[x] = spellDiff->SpellID[x];
        }

        if (newEntry.SpellID[0] <= 0 || newEntry.SpellID[1] <= 0)//id0-1 must be always set!
            continue;

        for (uint32 x = 0; x < 4; ++x)
            sSpellMgr->SetSpellDifficultyId(uint32(newEntry.SpellID[x]), spellDiff->ID);
    }

    // create talent spells set
    for (unsigned int i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
        if (!talentInfo)
            continue;

        for (int j = 0; j < MAX_TALENT_RANK; j++)
            if (talentInfo->SpellId)
                sTalentSpellPosMap[talentInfo->SpellId] = TalentSpellPos(i, j);
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sChrSpecializationStore, dbcPath, "ChrSpecialization.dbc");

    // prepare fast data access to bit pos of talent ranks for use at inspecting
    {
        // now have all max ranks (and then bit amount used for store talent ranks in inspect)
        for (uint32 j = 0; j < sChrSpecializationStore.GetNumRows(); j++)
        {
            ChrSpecializationEntry const* specializationInfo = sChrSpecializationStore.LookupEntry(j);
            if (!specializationInfo)
                continue;

            // prevent memory corruption; otherwise cls will become 12 below
            if (!specializationInfo->classId || specializationInfo->classId >= MAX_CLASSES)
                continue;

            sSpecializationClassStore[specializationInfo->classId][specializationInfo->TabPage] = j;
        }
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sSpecializationSpellsStore, dbcPath, "SpecializationSpells.dbc");
    for (uint32 j = 0; j < sSpecializationSpellsStore.GetNumRows(); j++)
        if (SpecializationSpellsEntry const* specializationSpells = sSpecializationSpellsStore.LookupEntry(j))
            sSpecializationSpellsMap[specializationSpells->SpecializationId].push_back(specializationSpells->SpellId);

    LoadDBC(availableDbcLocales, bad_dbc_files, sTaxiNodesStore, dbcPath, "TaxiNodes.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sTaxiPathStore, dbcPath, "TaxiPath.dbc");//15595
    for (uint32 i = 1; i < sTaxiPathStore.GetNumRows(); ++i)
        if (TaxiPathEntry const* entry = sTaxiPathStore.LookupEntry(i))
            sTaxiPathSetBySource[entry->from][entry->to] = TaxiPathBySourceAndDestination(entry->ID, entry->price);
    uint32 pathCount = sTaxiPathStore.GetNumRows();

    //## TaxiPathNode.dbc ## Loaded only for initialization different structures
    LoadDBC(availableDbcLocales, bad_dbc_files, sTaxiPathNodeStore, dbcPath, "TaxiPathNode.dbc");//15595
    // Calculate path nodes count
    std::vector<uint32> pathLength;
    pathLength.resize(pathCount);                           // 0 and some other indexes not used
    for (uint32 i = 1; i < sTaxiPathNodeStore.GetNumRows(); ++i)
        if (TaxiPathNodeEntry const* entry = sTaxiPathNodeStore.LookupEntry(i))
        {
            if (pathLength[entry->path] < entry->index + 1)
                pathLength[entry->path] = entry->index + 1;
        }
    // Set path length
    sTaxiPathNodesByPath.resize(pathCount);                 // 0 and some other indexes not used
    for (uint32 i = 1; i < sTaxiPathNodesByPath.size(); ++i)
        sTaxiPathNodesByPath[i].resize(pathLength[i]);
    // fill data
    for (uint32 i = 1; i < sTaxiPathNodeStore.GetNumRows(); ++i)
        if (TaxiPathNodeEntry const* entry = sTaxiPathNodeStore.LookupEntry(i))
            sTaxiPathNodesByPath[entry->path].set(entry->index, entry);

    // Initialize global taxinodes mask
    // include existed nodes that have at least single not spell base (scripted) path
    {
        std::set<uint32> spellPaths;
        for (uint32 i = 1; i < sSpellEffectStore.GetNumRows(); ++i)
            if (SpellEffectEntry const* sInfo = sSpellEffectStore.LookupEntry(i))
                if (sInfo->Effect == SPELL_EFFECT_SEND_TAXI)
                    spellPaths.insert(sInfo->EffectMiscValue);

        memset(sTaxiNodesMask, 0, sizeof(sTaxiNodesMask));
        memset(sOldContinentsNodesMask, 0, sizeof(sOldContinentsNodesMask));
        memset(sHordeTaxiNodesMask, 0, sizeof(sHordeTaxiNodesMask));
        memset(sAllianceTaxiNodesMask, 0, sizeof(sAllianceTaxiNodesMask));
        memset(sDeathKnightTaxiNodesMask, 0, sizeof(sDeathKnightTaxiNodesMask));
        for (uint32 i = 1; i < sTaxiNodesStore.GetNumRows(); ++i)
        {
            TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);
            if (!node)
                continue;

            TaxiPathSetBySource::const_iterator src_i = sTaxiPathSetBySource.find(i);
            if (src_i != sTaxiPathSetBySource.end() && !src_i->second.empty())
            {
                bool ok = false;
                for (TaxiPathSetForSource::const_iterator dest_i = src_i->second.begin(); dest_i != src_i->second.end(); ++dest_i)
                {
                    // not spell path
                    if (spellPaths.find(dest_i->second.ID) == spellPaths.end())
                    {
                        ok = true;
                        break;
                    }
                }

                if (!ok)
                    continue;
            }

            // valid taxi network node
            uint8  field = (uint8)((i - 1) / 8);
            uint32 submask = 1 << ((i - 1) % 8);

            sTaxiNodesMask[field] |= submask;
            if (node->MountCreatureID[0] && node->MountCreatureID[0] != 32981)
                sHordeTaxiNodesMask[field] |= submask;
            if (node->MountCreatureID[1] && node->MountCreatureID[1] != 32981)
                sAllianceTaxiNodesMask[field] |= submask;
            if (node->MountCreatureID[0] == 32981 || node->MountCreatureID[1] == 32981)
                sDeathKnightTaxiNodesMask[field] |= submask;

            // old continent node (+ nodes virtually at old continents, check explicitly to avoid loading map files for zone info)
            if (node->map_id < 2 || i == 82 || i == 83 || i == 93 || i == 94)
                sOldContinentsNodesMask[field] |= submask;

            // fix DK node at Ebon Hold and Shadow Vault flight master
            if (i == 315 || i == 333)
                ((TaxiNodesEntry*)node)->MountCreatureID[1] = 32981;
        }
    }

    //LoadDBC(availableDbcLocales, bad_dbc_files, sTeamContributionPointsStore, dbcPath, "TeamContributionPoints.dbc");
    LoadDBC(availableDbcLocales, bad_dbc_files, sTotemCategoryStore, dbcPath, "TotemCategory.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sTransportAnimationStore, dbcPath, "TransportAnimation.dbc");
    for (uint32 i = 0; i < sTransportAnimationStore.GetNumRows(); ++i)
    {
        TransportAnimationEntry const* anim = sTransportAnimationStore.LookupEntry(i);
        if (!anim)
            continue;

        sTransportMgr->AddPathNodeToTransport(anim->TransportEntry, anim->TimeSeg, anim);
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sTransportRotationStore, dbcPath, "TransportRotation.dbc");
    for (uint32 i = 0; i < sTransportRotationStore.GetNumRows(); ++i)
    {
        TransportRotationEntry const* rot = sTransportRotationStore.LookupEntry(i);
        if (!rot)
            continue;

        sTransportMgr->AddPathRotationToTransport(rot->TransportEntry, rot->TimeSeg, rot);
    }

    LoadDBC(availableDbcLocales, bad_dbc_files, sUnitPowerBarStore, dbcPath, "UnitPowerBar.dbc");//15595

    LoadDBC(availableDbcLocales, bad_dbc_files, sVehicleStore, dbcPath, "Vehicle.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sVehicleSeatStore, dbcPath, "VehicleSeat.dbc");//15595

    LoadDBC(availableDbcLocales, bad_dbc_files, sWMOAreaTableStore, dbcPath, "WMOAreaTable.dbc");//15595
    for (uint32 i = 0; i < sWMOAreaTableStore.GetNumRows(); ++i)
        if (WMOAreaTableEntry const* entry = sWMOAreaTableStore.LookupEntry(i))
            sWMOAreaInfoByTripple.insert(WMOAreaInfoByTripple::value_type(WMOAreaTableTripple(entry->rootId, entry->adtId, entry->groupId), entry));
    LoadDBC(availableDbcLocales, bad_dbc_files, sWorldMapAreaStore, dbcPath, "WorldMapArea.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sWorldMapOverlayStore, dbcPath, "WorldMapOverlay.dbc");//15595
    LoadDBC(availableDbcLocales, bad_dbc_files, sWorldSafeLocsStore, dbcPath, "WorldSafeLocs.dbc");//15595

    // error checks
    if (bad_dbc_files.size() >= DBCFileCount)
    {
        SF_LOG_ERROR("misc", "Incorrect DataDir value in worldserver.conf or ALL required *.dbc files (%d) not found by path: %sdbc", DBCFileCount, dataPath.c_str());
        exit(1);
    }
    else if (!bad_dbc_files.empty())
    {
        std::string str;
        for (StoreProblemList::iterator i = bad_dbc_files.begin(); i != bad_dbc_files.end(); ++i)
            str += *i + "\n";

        SF_LOG_ERROR("misc", "Some required *.dbc files (%u from %d) not found or not compatible:\n%s", (uint32)bad_dbc_files.size(), DBCFileCount, str.c_str());
        exit(1);
    }

    // Check loaded DBC files proper version
    if (!sAreaStore.LookupEntry(5491) ||     // last area (areaflag) added in 5.4.8 (18414)
        !sCharTitlesStore.LookupEntry(389) ||     // last char title added in 5.4.8 (18414)
        !sGemPropertiesStore.LookupEntry(2467) ||     // last gem property added in 5.4.8 (18414)
        !sMapStore.LookupEntry(1173) ||     // last map added in 5.4.8 (18414)
        !sSpellStore.LookupEntry(163227))      // last spell added in 5.4.8 (18414)
    {
        SF_LOG_ERROR("misc", "You have _outdated_ DBC files. Please extract correct dbc files from client 5.4.8 18414.");
        exit(1);
    }

    SF_LOG_INFO("server.loading", ">> Initialized %d DBC data stores in %u ms", DBCFileCount, GetMSTimeDiffToNow(oldMSTime));
}

const std::string* GetRandomCharacterName(uint8 race, uint8 gender)
{
    uint32 size = sGenNameVectoArraysMap[race].stringVectorArray[gender].size();
    uint32 randPos = std::rand() % size;

    return &sGenNameVectoArraysMap[race].stringVectorArray[gender][randPos];
}

SimpleFactionsList const* GetFactionTeamList(uint32 faction)
{
    FactionTeamMap::const_iterator itr = sFactionTeamMap.find(faction);
    if (itr != sFactionTeamMap.end())
        return &itr->second;

    return NULL;
}

char const* GetPetName(uint32 petfamily, uint32 /*dbclang*/)
{
    if (!petfamily)
        return NULL;
    CreatureFamilyEntry const* pet_family = sCreatureFamilyStore.LookupEntry(petfamily);
    if (!pet_family)
        return NULL;
    return pet_family->Name ? pet_family->Name : NULL;
}

TalentSpellPos const* GetTalentSpellPos(uint32 spellId)
{
    TalentSpellPosMap::const_iterator itr = sTalentSpellPosMap.find(spellId);
    if (itr == sTalentSpellPosMap.end())
        return NULL;

    return &itr->second;
}

uint32 GetTalentSpellCost(uint32 spellId)
{
    if (TalentSpellPos const* pos = GetTalentSpellPos(spellId))
        return pos->rank + 1;

    return 0;
}

int32 GetAreaFlagByAreaID(uint32 area_id)
{
    AreaFlagByAreaID::iterator i = sAreaFlagByAreaID.find(area_id);
    if (i == sAreaFlagByAreaID.end())
        return -1;

    return i->second;
}

WMOAreaTableEntry const* GetWMOAreaTableEntryByTripple(int32 rootid, int32 adtid, int32 groupid)
{
    WMOAreaInfoByTripple::iterator i = sWMOAreaInfoByTripple.find(WMOAreaTableTripple(rootid, adtid, groupid));
    if (i == sWMOAreaInfoByTripple.end())
        return NULL;
    return i->second;
}

AreaTableEntry const* GetAreaEntryByAreaID(uint32 area_id)
{
    int32 areaflag = GetAreaFlagByAreaID(area_id);
    if (areaflag < 0)
        return NULL;

    return sAreaStore.LookupEntry(areaflag);
}

AreaTableEntry const* GetAreaEntryByAreaFlagAndMap(uint32 area_flag, uint32 map_id)
{
    if (area_flag)
        return sAreaStore.LookupEntry(area_flag);

    if (MapEntry const* mapEntry = sMapStore.LookupEntry(map_id))
        return GetAreaEntryByAreaID(mapEntry->linked_zone);

    return NULL;
}

char const* GetRaceName(uint8 race, uint8 /*locale*/)
{
    ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race);
    return raceEntry ? raceEntry->name : NULL;
}

char const* GetClassName(uint8 class_, uint8 /*locale*/)
{
    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(class_);
    return classEntry ? classEntry->name : NULL;
}

uint32 GetAreaFlagByMapId(uint32 mapid)
{
    AreaFlagByMapID::iterator i = sAreaFlagByMapID.find(mapid);
    if (i == sAreaFlagByMapID.end())
        return 0;
    else
        return i->second;
}

uint32 GetVirtualMapForMapAndZone(uint32 mapid, uint32 zoneId)
{
    if (mapid != 530 && mapid != 571 && mapid != 732 && mapid != 1064)   // speed for most cases
        return mapid;

    if (WorldMapAreaEntry const* wma = sWorldMapAreaStore.LookupEntry(zoneId))
        return wma->virtual_map_id >= 0 ? wma->virtual_map_id : wma->map_id;

    return mapid;
}

uint32 GetMaxLevelForExpansion(uint32 expansion)
{
    switch (expansion)
    {
        case CONTENT_1_60:
            return 60;
        case CONTENT_61_70:
            return 70;
        case CONTENT_71_80:
            return 80;
        case CONTENT_81_85:
            return 85;
        case CONTENT_86_90:
            return 90;
        default:
            break;
    }
    return 0;
}

/*
Used only for calculate xp gain by content lvl.
Calculation on Gilneas and group maps of LostIslands calculated as CONTENT_1_60.
*/
ContentLevels GetContentLevelsForMapAndZone(uint32 mapid, uint32 zoneId)
{
    mapid = GetVirtualMapForMapAndZone(mapid, zoneId);
    if (mapid < 2)
        return CONTENT_1_60;

    MapEntry const* mapEntry = sMapStore.LookupEntry(mapid);
    if (!mapEntry)
        return CONTENT_1_60;

    // no need enum all maps from phasing
    if (mapEntry->rootPhaseMap >= 0)
        mapid = mapEntry->rootPhaseMap;

    switch (mapid)
    {
        case 648:   //LostIslands
        case 654:   //Gilneas
            return CONTENT_1_60;
        default:
            return ContentLevels(mapEntry->Expansion());
    }
}

bool IsTotemCategoryCompatiableWith(uint32 itemTotemCategoryId, uint32 requiredTotemCategoryId)
{
    if (requiredTotemCategoryId == 0)
        return true;
    if (itemTotemCategoryId == 0)
        return false;

    TotemCategoryEntry const* itemEntry = sTotemCategoryStore.LookupEntry(itemTotemCategoryId);
    if (!itemEntry)
        return false;
    TotemCategoryEntry const* reqEntry = sTotemCategoryStore.LookupEntry(requiredTotemCategoryId);
    if (!reqEntry)
        return false;

    if (itemEntry->categoryType != reqEntry->categoryType)
        return false;

    return (itemEntry->categoryMask & reqEntry->categoryMask) == reqEntry->categoryMask;
}

void Zone2MapCoordinates(float& x, float& y, uint32 zone)
{
    WorldMapAreaEntry const* maEntry = sWorldMapAreaStore.LookupEntry(zone);

    // if not listed then map coordinates (instance)
    if (!maEntry)
        return;

    std::swap(x, y);                                         // at client map coords swapped
    x = x * ((maEntry->x2 - maEntry->x1) / 100) + maEntry->x1;
    y = y * ((maEntry->y2 - maEntry->y1) / 100) + maEntry->y1;      // client y coord from top to down
}

void Map2ZoneCoordinates(float& x, float& y, uint32 zone)
{
    WorldMapAreaEntry const* maEntry = sWorldMapAreaStore.LookupEntry(zone);

    // if not listed then map coordinates (instance)
    if (!maEntry)
        return;

    x = (x - maEntry->x1) / ((maEntry->x2 - maEntry->x1) / 100);
    y = (y - maEntry->y1) / ((maEntry->y2 - maEntry->y1) / 100);    // client y coord from top to down
    std::swap(x, y);                                         // client have map coords swapped
}

uint32 GetClassBySkillId(uint32 skillId)
{
    ClassBySkillIdStore::const_iterator iter = sClassBySkillIdStore.find(skillId);
    if (iter != sClassBySkillIdStore.end())
        return iter->second;
    return 0;
}

uint32 GetSkillIdByClass(uint32 classId)
{
    for (ClassBySkillIdStore::const_iterator iter = sClassBySkillIdStore.begin(); iter != sClassBySkillIdStore.end(); ++iter)
        if (iter->second == classId)
            return iter->first;
    return 0;
}

std::list<uint32> GetSpellsForLevels(uint32 classId, uint32 raceMask, uint32 specializationId, uint32 minLevel, uint32 maxLevel)
{
    std::list<uint32> spellList;

    if (classId != 0)
    {
        SpellsPerClassStore::const_iterator classIter = sSpellsPerClassStore.find(classId);
        if (classIter != sSpellsPerClassStore.end())
        {
            const std::list<SkillLineAbilityEntry const*>& learnSpellList = classIter->second;
            for (std::list<SkillLineAbilityEntry const*>::const_iterator iter = learnSpellList.begin(); iter != learnSpellList.end(); ++iter)
            {
                SkillLineAbilityEntry const* skillLine = *iter;
                if (!skillLine)
                    continue;

                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(skillLine->spellId);
                if (!spellInfo)
                    continue;

                if (skillLine->racemask && !(skillLine->racemask & raceMask))
                    continue;

                if (spellInfo->SpellLevel <= minLevel || spellInfo->SpellLevel > maxLevel)
                    continue;

                spellList.push_back(spellInfo->Id);
            }
        }
    }

    if (!specializationId)
        return spellList;

    SpecializationSpellsMap::const_iterator specIter = sSpecializationSpellsMap.find(specializationId);
    if (specIter != sSpecializationSpellsMap.end())
    {
        const std::vector<uint32>& learnSpellList = specIter->second;
        for (int i = 0; i != learnSpellList.size(); i++)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(learnSpellList[i]);
            if (!spellInfo)
                continue;

            if (spellInfo->SpellLevel <= minLevel || spellInfo->SpellLevel > maxLevel)
                continue;

            spellList.push_back(spellInfo->Id);
        }
    }
    return spellList;
}

std::vector<uint32> const* GetSpecializationSpells(uint32 specializationId)
{
    SpecializationSpellsMap::const_iterator specIter = sSpecializationSpellsMap.find(specializationId);
    if (specIter != sSpecializationSpellsMap.end())
        return &specIter->second;

    return NULL;
}

MapDifficulty const* GetDefaultMapDifficulty(uint32 mapId)
{
    auto itr = sMapDifficultyMap.find(mapId);
    if (itr == sMapDifficultyMap.end())
        return nullptr;

    if (itr->second.empty())
        return nullptr;

    for (auto& p : itr->second)
    {
        DifficultyEntry const* difficulty = sDifficultyStore.LookupEntry(p.first);
        if (!difficulty)
            continue;

        if (difficulty->flags & 0x02)
            return &p.second;
    }

    return &itr->second.begin()->second;
}

MapDifficulty const* GetMapDifficultyData(uint32 mapId, DifficultyID difficulty)
{
    auto itr = sMapDifficultyMap.find(mapId);
    if (itr == sMapDifficultyMap.end())
        return nullptr;

    auto diffItr = itr->second.find(difficulty);
    if (diffItr == itr->second.end())
        return nullptr;

    return &diffItr->second;
}

MapDifficulty const* GetDownscaledMapDifficultyData(uint32 mapId, DifficultyID& difficulty)
{
    DifficultyEntry const* diffEntry = sDifficultyStore.LookupEntry(difficulty);
    if (!diffEntry)
        return GetDefaultMapDifficulty(mapId);

    uint32 tmpDiff = difficulty;
    MapDifficulty const* mapDiff = GetMapDifficultyData(mapId, DifficultyID(tmpDiff));
    while (!mapDiff)
    {
        tmpDiff = diffEntry->DownscaleID;
        diffEntry = sDifficultyStore.LookupEntry(tmpDiff);
        if (!diffEntry)
            return GetDefaultMapDifficulty(mapId);

        // pull new data
        mapDiff = GetMapDifficultyData(mapId, DifficultyID(tmpDiff));
    }

    difficulty = DifficultyID(tmpDiff);
    return mapDiff;
}

PvPDifficultyEntry const* GetBattlegroundBracketByLevel(uint32 mapid, uint32 level)
{
    PvPDifficultyEntry const* maxEntry = NULL;              // used for level > max listed level case
    for (uint32 i = 0; i < sPvPDifficultyStore.GetNumRows(); ++i)
    {
        if (PvPDifficultyEntry const* entry = sPvPDifficultyStore.LookupEntry(i))
        {
            // skip unrelated and too-high brackets
            if (entry->mapId != mapid || entry->minLevel > level)
                continue;

            // exactly fit
            if (entry->maxLevel >= level)
                return entry;

            // remember for possible out-of-range case (search higher from existed)
            if (!maxEntry || maxEntry->maxLevel < entry->maxLevel)
                maxEntry = entry;
        }
    }

    return maxEntry;
}

PvPDifficultyEntry const* GetBattlegroundBracketById(uint32 mapid, BattlegroundBracketId id)
{
    for (uint32 i = 0; i < sPvPDifficultyStore.GetNumRows(); ++i)
        if (PvPDifficultyEntry const* entry = sPvPDifficultyStore.LookupEntry(i))
            if (entry->mapId == mapid && entry->GetBracketId() == id)
                return entry;

    return NULL;
}

uint32 const* GetClassSpecializations(uint8 classId)
{
    return sSpecializationClassStore[classId];
}

uint32 GetLiquidFlags(uint32 liquidType)
{
    if (LiquidTypeEntry const* liq = sLiquidTypeStore.LookupEntry(liquidType))
        return 1 << liq->Type;

    return 0;
}

CharStartOutfitEntry const* GetCharStartOutfitEntry(uint8 race, uint8 class_, uint8 gender)
{
    std::map<uint32, CharStartOutfitEntry const*>::const_iterator itr = sCharStartOutfitMap.find(race | (class_ << 8) | (gender << 16));
    if (itr == sCharStartOutfitMap.end())
        return NULL;

    return itr->second;
}

uint32 GetPowerIndexByClass(uint32 powerType, uint32 classId)
{
    return PowersByClass[classId][powerType];
}

uint32 ScalingStatValuesEntry::GetStatMultiplier(uint32 inventoryType) const
{
    if (inventoryType < MAX_INVTYPE)
    {
        switch (inventoryType)
        {
            case INVTYPE_NON_EQUIP:
            case INVTYPE_BODY:
            case INVTYPE_BAG:
            case INVTYPE_TABARD:
            case INVTYPE_AMMO:
            case INVTYPE_QUIVER:
                return 0;
            case INVTYPE_HEAD:
            case INVTYPE_CHEST:
            case INVTYPE_LEGS:
            case INVTYPE_2HWEAPON:
            case INVTYPE_ROBE:
                return StatMultiplier[0];
            case INVTYPE_SHOULDERS:
            case INVTYPE_WAIST:
            case INVTYPE_FEET:
            case INVTYPE_HANDS:
            case INVTYPE_TRINKET:
                return StatMultiplier[1];
            case INVTYPE_NECK:
            case INVTYPE_WRISTS:
            case INVTYPE_FINGER:
            case INVTYPE_SHIELD:
            case INVTYPE_CLOAK:
            case INVTYPE_HOLDABLE:
                return StatMultiplier[2];
            case INVTYPE_RANGED:
            case INVTYPE_THROWN:
            case INVTYPE_RANGEDRIGHT:
            case INVTYPE_RELIC:
                return StatMultiplier[3];
            case INVTYPE_WEAPON:
            case INVTYPE_WEAPONMAINHAND:
            case INVTYPE_WEAPONOFFHAND:
                return StatMultiplier[4];
            default:
                break;
        }
    }
    return 0;
}

uint32 ScalingStatValuesEntry::GetArmor(uint32 inventoryType, uint32 armorType) const
{
    if (inventoryType <= INVTYPE_ROBE && armorType < 4)
    {
        switch (inventoryType)
        {
            case INVTYPE_NON_EQUIP:
            case INVTYPE_NECK:
            case INVTYPE_BODY:
            case INVTYPE_FINGER:
            case INVTYPE_TRINKET:
            case INVTYPE_WEAPON:
            case INVTYPE_RANGED:
            case INVTYPE_2HWEAPON:
            case INVTYPE_BAG:
            case INVTYPE_TABARD:
                break;
            case INVTYPE_SHOULDERS:
                return Armor[0][armorType];
            case INVTYPE_CHEST:
            case INVTYPE_ROBE:
                return Armor[1][armorType];
            case INVTYPE_HEAD:
                return Armor[2][armorType];
            case INVTYPE_LEGS:
                return Armor[3][armorType];
            case INVTYPE_FEET:
                return Armor[4][armorType];
            case INVTYPE_WAIST:
                return Armor[5][armorType];
            case INVTYPE_HANDS:
                return Armor[6][armorType];
            case INVTYPE_WRISTS:
                return Armor[7][armorType];
            case INVTYPE_CLOAK:
                return CloakArmor;
            case INVTYPE_SHIELD:
                return ShieldArmor;
            default:
                break;
        }
    }
    return 0;
}

uint32 ScalingStatValuesEntry::GetDPSAndDamageMultiplier(uint32 subClass, bool isCasterWeapon, float* damageMultiplier) const
{
    if (!isCasterWeapon)
    {
        switch (subClass)
        {
            case ITEM_SUBCLASS_WEAPON_AXE:
            case ITEM_SUBCLASS_WEAPON_MACE:
            case ITEM_SUBCLASS_WEAPON_SWORD:
            case ITEM_SUBCLASS_WEAPON_DAGGER:
            case ITEM_SUBCLASS_WEAPON_THROWN:
                *damageMultiplier = 0.3f;
                return dpsMod[0];
            case ITEM_SUBCLASS_WEAPON_AXE2:
            case ITEM_SUBCLASS_WEAPON_MACE2:
            case ITEM_SUBCLASS_WEAPON_POLEARM:
            case ITEM_SUBCLASS_WEAPON_SWORD2:
            case ITEM_SUBCLASS_WEAPON_STAFF:
            case ITEM_SUBCLASS_WEAPON_FISHING_POLE:
                *damageMultiplier = 0.2f;
                return dpsMod[1];
            case ITEM_SUBCLASS_WEAPON_BOW:
            case ITEM_SUBCLASS_WEAPON_GUN:
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                *damageMultiplier = 0.3f;
                return dpsMod[4];
            case ITEM_SUBCLASS_WEAPON_Obsolete:
            case ITEM_SUBCLASS_WEAPON_EXOTIC:
            case ITEM_SUBCLASS_WEAPON_EXOTIC2:
            case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
            case ITEM_SUBCLASS_WEAPON_MISCELLANEOUS:
            case ITEM_SUBCLASS_WEAPON_SPEAR:
            case ITEM_SUBCLASS_WEAPON_WAND:
                break;
        }
    }
    else
    {
        if (subClass <= ITEM_SUBCLASS_WEAPON_WAND)
        {
            uint32 mask = 1 << subClass;
            // two-handed weapons
            if (mask & 0x562)
            {
                *damageMultiplier = 0.2f;
                return dpsMod[3];
            }

            if (mask & (1 << ITEM_SUBCLASS_WEAPON_WAND))
            {
                *damageMultiplier = 0.3f;
                return dpsMod[5];
            }
        }
        *damageMultiplier = 0.3f;
        return dpsMod[2];
    }
    return 0;
}
/*
DigsitePOIPolygon const* GetDigsitePOIPolygon(uint32 digsiteId)
{
    DigsitePOIPolygonContainer::const_iterator itr = sDigsitePOIPolygons.find(digsiteId);
    if (itr != sDigsitePOIPolygons.end())
        return &itr->second;

    return NULL;
}*/

/// Returns LFGDungeonEntry for a specific map and difficulty. Will return first found entry if multiple dungeons use the same map (such as Scarlet Monastery)
LFGDungeonEntry const* GetLFGDungeon(uint32 mapId, DifficultyID difficulty)
{
    for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
    {
        LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i);
        if (!dungeon)
            continue;

        if (dungeon->m_ContinentID == int32(mapId) && DifficultyID(dungeon->m_DifficultyID) == difficulty)
            return dungeon;
    }

    return NULL;
}

std::set<uint32> const& GetPhasesForGroup(uint32 group)
{
    return sPhasesByGroup[group];
}
