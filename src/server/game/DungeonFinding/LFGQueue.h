/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SF_LFGQUEUE_H
#define SF_LFGQUEUE_H

#include "LFG.h"

namespace lfg
{

    enum LfgCompatibility
    {
        LFG_COMPATIBILITY_PENDING,
        LFG_INCOMPATIBLES_WRONG_GROUP_SIZE,
        LFG_INCOMPATIBLES_TOO_MUCH_PLAYERS,
        LFG_INCOMPATIBLES_MULTIPLE_LFG_GROUPS,
        LFG_INCOMPATIBLES_HAS_IGNORES,
        LFG_INCOMPATIBLES_NO_ROLES,
        LFG_INCOMPATIBLES_NO_DUNGEONS,
        LFG_COMPATIBLES_WITH_LESS_PLAYERS,                     // Values under this = not compatible (do not modify order)
        LFG_COMPATIBLES_BAD_STATES,
        LFG_COMPATIBLES_MATCH                                  // Must be the last one
    };

    struct LfgCompatibilityData
    {
        LfgCompatibilityData() : compatibility(LFG_COMPATIBILITY_PENDING) { }
        LfgCompatibilityData(LfgCompatibility _compatibility) : compatibility(_compatibility) { }
        LfgCompatibilityData(LfgCompatibility _compatibility, LfgRolesMap const& _roles) :
            compatibility(_compatibility), roles(_roles) { }

        LfgCompatibility compatibility;
        LfgRolesMap roles;
    };

    /// Stores player or group queue info
    struct LfgQueueData
    {
        LfgQueueData() : joinTime(time_t(time(NULL))), tanks(LFG_TANKS_NEEDED),
            healers(LFG_HEALERS_NEEDED), dps(LFG_DPS_NEEDED)
        { }

        LfgQueueData(time_t _joinTime, LfgDungeonSet const& _dungeons, LfgRolesMap const& _roles) :
            joinTime(_joinTime), tanks(LFG_TANKS_NEEDED), healers(LFG_HEALERS_NEEDED),
            dps(LFG_DPS_NEEDED), dungeons(_dungeons), roles(_roles)
        { }

        time_t joinTime;                                       ///< Player queue join time (to calculate wait times)
        uint8 tanks;                                           ///< Tanks needed
        uint8 healers;                                         ///< Healers needed
        uint8 dps;                                             ///< Dps needed
        LfgDungeonSet dungeons;                                ///< Selected Player/Group Dungeon/s
        LfgRolesMap roles;                                     ///< Selected Player Role/s
        std::string bestCompatible;                            ///< Best compatible combination of people queued
    };

    struct LfgWaitTime
    {
        LfgWaitTime() : time(-1), number(0) { }
        int32 time;                                            ///< Wait time
        uint32 number;                                         ///< Number of people used to get that wait time
    };

    typedef std::map<uint32, LfgWaitTime> LfgWaitTimesContainer;
    typedef std::map<std::string, LfgCompatibilityData> LfgCompatibleContainer;
    typedef std::map<uint64, LfgQueueData> LfgQueueDataContainer;

    /**
        Stores all data related to queue
    */
    class LFGQueue
    {
    public:
        // Add/Remove from queue
        void AddToQueue(uint64 guid, bool reQueue = false);
        void RemoveFromQueue(uint64 guid);
        void AddQueueData(uint64 guid, time_t joinTime, LfgDungeonSet const& dungeons, LfgRolesMap const& rolesMap);
        void RemoveQueueData(uint64 guid);

        // Update Timers (when proposal success)
        void UpdateWaitTimeAvg(int32 waitTime, uint32 dungeonId);
        void UpdateWaitTimeTank(int32 waitTime, uint32 dungeonId);
        void UpdateWaitTimeHealer(int32 waitTime, uint32 dungeonId);
        void UpdateWaitTimeDps(int32 waitTime, uint32 dungeonId);

        // Update Queue timers
        void UpdateQueueTimers(uint8 queueId, time_t currTime);
        time_t GetJoinTime(uint64 guid) const;

        // Find new group
        uint8 FindGroups();

        // Just for debugging purposes
        std::string DumpQueueInfo() const;
        std::string DumpCompatibleInfo(bool full = false) const;

    private:
        void SetQueueUpdateData(std::string const& strGuids, LfgRolesMap const& proposalRoles);
        LfgRolesMap const& RemoveFromQueueUpdateData(uint64 guid);

        void AddToNewQueue(uint64 guid);
        void AddToCurrentQueue(uint64 guid);
        void RemoveFromNewQueue(uint64 guid);
        void RemoveFromCurrentQueue(uint64 guid);

        void SetCompatibles(std::string const& key, LfgCompatibility compatibles);
        LfgCompatibility GetCompatibles(std::string const& key);
        void RemoveFromCompatibles(uint64 guid);

        void SetCompatibilityData(std::string const& key, LfgCompatibilityData const& compatibles);
        LfgCompatibilityData* GetCompatibilityData(std::string const& key);
        void FindBestCompatibleInQueue(LfgQueueDataContainer::iterator itrQueue);
        void UpdateBestCompatibleInQueue(LfgQueueDataContainer::iterator itrQueue, std::string const& key, LfgRolesMap const& roles);

        LfgCompatibility FindNewGroups(LfgGuidList& check, LfgGuidList& all);
        LfgCompatibility CheckCompatibility(LfgGuidList check);

        // Queue
        LfgQueueDataContainer QueueDataStore;              ///< Queued groups
        LfgCompatibleContainer CompatibleMapStore;         ///< Compatible dungeons

        LfgWaitTimesContainer waitTimesAvgStore;           ///< Average wait time to find a group queuing as multiple roles
        LfgWaitTimesContainer waitTimesTankStore;          ///< Average wait time to find a group queuing as tank
        LfgWaitTimesContainer waitTimesHealerStore;        ///< Average wait time to find a group queuing as healer
        LfgWaitTimesContainer waitTimesDpsStore;           ///< Average wait time to find a group queuing as dps
        LfgGuidList currentQueueStore;                     ///< Ordered list. Used to find groups
        LfgGuidList newToQueueStore;                       ///< New groups to add to queue
    };

} // namespace lfg

#endif
