/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SKYFIRESERVER_MOVEPLINE_H
#define SKYFIRESERVER_MOVEPLINE_H

#include "MoveSplineInitArgs.h"
#include "Spline.h"

namespace Movement
{
    struct Location : public Vector3
    {
        Location() : orientation(0) { }
        Location(float x, float y, float z, float o) : Vector3(x, y, z), orientation(o) { }
        Location(const Vector3& v) : Vector3(v), orientation(0) { }
        Location(const Vector3& v, float o) : Vector3(v), orientation(o) { }

        float orientation;
    };

    // MoveSpline represents smooth catmullrom or linear curve and point that moves belong it
    // curve can be cyclic - in this case movement will be cyclic
    // point can have vertical acceleration motion componemt(used in fall, parabolic movement)
    class MoveSpline
    {
    public:
        typedef Spline<int32> MySpline;
        enum UpdateResult
        {
            Result_None = 0x01,
            Result_Arrived = 0x02,
            Result_NextCycle = 0x04,
            Result_NextSegment = 0x08
        };
        friend class PacketBuilder;

    protected:
        MySpline        spline;

        FacingInfo      facing;

        uint32          m_Id;

        MoveSplineFlag  splineflags;

        int32           time_passed;
        // currently duration mods are unused, but its _currently_
        //float           duration_mod;
        //float           duration_mod_next;
        float           vertical_acceleration;
        float           initialOrientation;
        int32           effect_start_time;
        int32           point_Idx;
        int32           point_Idx_offset;

        void init_spline(const MoveSplineInitArgs& args);

    protected:
        MySpline::ControlArray const& getPath() const { return spline.getPoints(); }
        void computeParabolicElevation(float& el) const;
        void computeFallElevation(float& el) const;

        UpdateResult _updateState(int32& ms_time_diff);
        int32 next_timestamp() const { return spline.length(point_Idx + 1); }
        int32 segment_time_elapsed() const { return next_timestamp() - time_passed; }
        int32 timeElapsed() const { return Duration() - time_passed; }
        int32 timePassed() const { return time_passed; }

    public:
        int32 Duration() const { return spline.length(); }
        MySpline const& _Spline() const { return spline; }
        int32 _currentSplineIdx() const { return point_Idx; }
        void _Finalize();
        void _Interrupt() { splineflags.done = true; }

    public:
        void Initialize(const MoveSplineInitArgs&);
        bool Initialized() const { return !spline.empty(); }

        MoveSpline();

        template<class UpdateHandler>
        void updateState(int32 difftime, UpdateHandler& handler)
        {
            ASSERT(Initialized());
            do
                handler(_updateState(difftime));
            while (difftime > 0);
        }

        void updateState(int32 difftime)
        {
            ASSERT(Initialized());
            do _updateState(difftime);
            while (difftime > 0);
        }

        Location ComputePosition() const;

        uint32 GetId() const { return m_Id; }
        bool Finalized() const { return splineflags.done; }
        bool isCyclic() const { return splineflags.cyclic; }
        bool isFalling() const { return splineflags.falling; }
        Vector3 FinalDestination() const { return Initialized() ? spline.getPoint(spline.last()) : Vector3(); }
        Vector3 CurrentDestination() const { return Initialized() ? spline.getPoint(point_Idx + 1) : Vector3(); }
        int32 currentPathIdx() const;

        bool onTransport;
        std::string ToString() const;
    };
}
#endif // SKYFIRESERVER_MOVEPLINE_H
