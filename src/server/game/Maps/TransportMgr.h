/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef TRANSPORTMGR_H
#define TRANSPORTMGR_H

#include "DBCStores.h"
#include "Spline.h"
#include <ace/Singleton.h>
#include <G3D/Quat.h>

struct KeyFrame;
struct GameObjectTemplate;
struct TransportTemplate;
class Transport;
class Map;

typedef Movement::Spline<double>                 TransportSpline;
typedef std::vector<KeyFrame>                    KeyFrameVec;
typedef UNORDERED_MAP<uint32, TransportTemplate> TransportTemplates;
typedef std::set<Transport*>                     TransportSet;
typedef UNORDERED_MAP<uint32, TransportSet>      TransportMap;
typedef UNORDERED_MAP<uint32, std::set<uint32> > TransportInstanceMap;

struct KeyFrame
{
    explicit KeyFrame(TaxiPathNodeEntry const& _node) : Index(0), Node(&_node),
        DistSinceStop(-1.0f), DistUntilStop(-1.0f), DistFromPrev(-1.0f), TimeFrom(0.0f), TimeTo(0.0f),
        Teleport(false), ArriveTime(0), DepartureTime(0), Spline(NULL), NextDistFromPrev(0.0f), NextArriveTime(0)
    {
    }

    uint32 Index;
    TaxiPathNodeEntry const* Node;
    float DistSinceStop;
    float DistUntilStop;
    float DistFromPrev;
    float TimeFrom;
    float TimeTo;
    bool Teleport;
    uint32 ArriveTime;
    uint32 DepartureTime;
    TransportSpline* Spline;

    // Data needed for next frame
    float NextDistFromPrev;
    uint32 NextArriveTime;

    bool IsTeleportFrame() const { return Teleport; }
    bool IsStopFrame() const { return Node->actionFlag == 2; }
};

struct TransportTemplate
{
    TransportTemplate() : inInstance(false), pathTime(0), accelTime(0.0f), accelDist(0.0f), entry(0) { }
    ~TransportTemplate();

    std::set<uint32> mapsUsed;
    bool inInstance;
    uint32 pathTime;
    KeyFrameVec keyFrames;
    float accelTime;
    float accelDist;
    uint32 entry;
};

typedef std::map<uint32, TransportAnimationEntry const*> TransportPathContainer;
typedef std::map<uint32, TransportRotationEntry const*> TransportPathRotationContainer;

struct TransportAnimation
{
    TransportAnimation() : TotalTime(0) { }

    TransportPathContainer Path;
    TransportPathRotationContainer Rotations;
    uint32 TotalTime;

    TransportAnimationEntry const* GetAnimNode(uint32 time) const;
    G3D::Quat GetAnimRotation(uint32 time) const;
};

typedef std::map<uint32, TransportAnimation> TransportAnimationContainer;

class TransportMgr
{
    friend class ACE_Singleton<TransportMgr, ACE_Thread_Mutex>;
    friend void LoadDBCStores(std::string const&);

public:
    void Unload();

    void LoadTransportTemplates();

    // Creates a transport using given GameObject template entry
    Transport* CreateTransport(uint32 entry, uint32 guid = 0, Map* map = NULL);

    // Spawns all continent transports, used at core startup
    void SpawnContinentTransports();

    // creates all transports for instance
    void CreateInstanceTransports(Map* map);

    TransportTemplate const* GetTransportTemplate(uint32 entry) const
    {
        TransportTemplates::const_iterator itr = _transportTemplates.find(entry);
        if (itr != _transportTemplates.end())
            return &itr->second;
        return NULL;
    }

    TransportAnimation const* GetTransportAnimInfo(uint32 entry) const
    {
        TransportAnimationContainer::const_iterator itr = _transportAnimations.find(entry);
        if (itr != _transportAnimations.end())
            return &itr->second;

        return NULL;
    }

private:
    TransportMgr();
    ~TransportMgr();
    TransportMgr(TransportMgr const&);
    TransportMgr& operator=(TransportMgr const&);

    // Generates and precaches a path for transport to avoid generation each time transport instance is created
    void GeneratePath(GameObjectTemplate const* goInfo, TransportTemplate* transport);

    void AddPathNodeToTransport(uint32 transportEntry, uint32 timeSeg, TransportAnimationEntry const* node);

    void AddPathRotationToTransport(uint32 transportEntry, uint32 timeSeg, TransportRotationEntry const* node)
    {
        _transportAnimations[transportEntry].Rotations[timeSeg] = node;
    }

    // Container storing transport templates
    TransportTemplates _transportTemplates;

    // Container storing transport entries to create for instanced maps
    TransportInstanceMap _instanceTransports;

    TransportAnimationContainer _transportAnimations;
};

#define sTransportMgr ACE_Singleton<TransportMgr, ACE_Thread_Mutex>::instance()

#endif // TRANSPORTMGR_H
