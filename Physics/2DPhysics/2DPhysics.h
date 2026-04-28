#ifndef __2D_PHYSICS_H__
#define __2D_PHYSICS_H__

#define MAX_ROWS 4                    // Most number of rows an individual constraint can have
#define PENALTY_MIN 1.0f              // Minimum penalty parameter
#define PENALTY_MAX 1000000000.0f     // Maximum penalty parameter
#define COLLISION_MARGIN 0.0005f      // Margin for collision detection to avoid flickering contacts
#define STICK_THRESH 0.01f            // Position threshold for sticking contacts (ie static friction)
#define RESTITUTION_REST_THRESH 1.0f  // Min relative normal velocity (world units/s) before restitution kicks in
#define SLEEP_LIN_THRESH 0.02f        // Below this linear speed (units/s) post-damping, velocity snaps to zero
#define SLEEP_ANG_THRESH 0.05f        // Below this angular speed (rad/s) post-damping, angular velocity snaps to zero
#define SLEEP_FRAMES 30               // Consecutive near-rest substeps before a body fully sleeps (skipped by solver)
#define MAX_SUBSTEPS 16               // Safety cap on per-frame physics substeps (prevents death-spiral on huge dt)

#include "Bodies.h"
#include "Forces.h"

// Core solver: owns every rigid body and force in the 2D world, and steps the
// simulation forward using a VBD-style primal/dual update with optional post
// stabilization. Exposed as a static singleton so bodies/forces can register
// themselves from their own constructors.
class Physics2DSolver
{
public:

    enum EWorldBoundsType
    {
        e_Default,
        e_WrapHorizontally,
        e_WrapVertically,
        e_WrapAll
    };

    struct Params
    {
        float Gravity;       // Gravity (acceleration along Y)
        int   Iterations;    // Solver iterations

        float Alpha;         // Stabilization parameter
        float Beta;          // Penalty ramping parameter
        float Gamma;         // Warmstarting decay parameter

        bool  PostStabilize; // Apply an extra post-stabilization iteration
    };

    static void Init();
    static void Terminate();

    static void Clear();
    static void DefaultParams();
    static void Step(float dt);

    // Set an axis-aligned world region and opt into per-axis position wrapping.
    // When enabled on an axis, bodies that leave the region on that axis
    // reappear on the opposite side after each substep. By default no wrapping
    // is applied. Note: cross-seam body-body collision is NOT yet handled —
    // bodies on opposite sides of a seam won't collide until they cross fully.
    static void SetWorldBounds(float2 minXY, float2 maxXY, EWorldBoundsType wrapType);

    static float2 GetWorldMin() { return s_WorldMin; }
    static float2 GetWorldMax() { return s_WorldMax; }
    static bool   IsWrapX()     { return s_WrapX; }
    static bool   IsWrapY()     { return s_WrapY; }

    // Minimum-image wrapped offset: returns `dp` shifted by ±worldSize on each
    // wrapped axis so its magnitude is at most half the world size. Used to
    // measure inter-body distance across the seam (e.g. broadphase, contacts).
    static float2 MinimumImage(float2 dp);

    // Find the body under a world-space point; writes the body-local hit point to `local`.
    static PhysicsBody2D* Pick(float2 at, float2& local);

    static void AddBody(PhysicsBody2D* body);
    static void RemoveBody(PhysicsBody2D* body);

    static void AddForce(Force2D* force);
    static void RemoveForce(Force2D* force);

    static Params& GetParams() { return s_Params; }

    static PhysicsBody2D* GetBodies() { return s_Bodies; }
    static Force2D*       GetForces() { return s_Forces; }

    // dt of the current/last Step(). Forces can read this during Initialize /
    // ComputeConstraint to scale coefficients (e.g. viscous drag F = -c*v).
    static float          GetStepDt() { return s_Dt; }

private:

    static void StepOnce(float dt);

    static PhysicsBody2D* s_Bodies;
    static Force2D*       s_Forces;
    static Params         s_Params;
    static float          s_Dt;

    static float2         s_WorldMin;
    static float2         s_WorldMax;
    static bool           s_WrapX;
    static bool           s_WrapY;
};

#endif
