#include <math.h>
#include <stdio.h>

#include "2DPhysics.h"

#include "Engine/Timer/Timer.h"

// Flip to 1 to print a diagnostic whenever a body displaces more than its
// bounding radius in a single substep (excluding legitimate end-of-step wrap).
// Catches the rare sphere-sphere teleport so we can see position before/after,
// velocity, and contact partners at the moment it happens.
#define DEBUG_TELEPORT 0

// ---------------------------------------------------------------------------
// Local bridges to the engine Maths API. These fill in small gaps (no free
// dot/abs/transpose/col, no unary minus on float2, no diagonal/outer for
// float3x3) without polluting Maths.h.
// ---------------------------------------------------------------------------
static inline float2 Neg(const float2& v)                  { return float2(-v.x, -v.y); }
static inline float  Dot(const float2& a, const float2& b) { return float2::dotproduct(a, b); }
static inline float3 Neg(const float3& v)                  { return float3(-v.x, -v.y, -v.z); }
static inline float2 XY(const float3& v)                   { return float2(v.x, v.y); }
static inline float  Length(float3 v)                      { return v.length(); }

static inline float3x3 Diag3(float a, float b, float c)
{
    return float3x3(float3(a, 0, 0), float3(0, b, 0), float3(0, 0, c));
}

static inline float3x3 Outer3(const float3& a, const float3& b)
{
    return float3x3(b * a.x, b * a.y, b * a.z);
}

static inline float3 Col3(const float3x3& m, int i)
{
    if (i == 0) return float3(m.m00, m.m10, m.m20);
    if (i == 1) return float3(m.m01, m.m11, m.m21);
    return              float3(m.m02, m.m12, m.m22);
}

// LDL^T solve for a 3x3 SPD system A x = b. Matches the reference's `solve()`.
static inline float3 Solve3(const float3x3& A, const float3& b)
{
    // A = L D L^T, L unit lower triangular, D diagonal
    float d0  = A.m00;
    float l10 = A.m10 / d0;
    float l20 = A.m20 / d0;

    float d1  = A.m11 - l10 * l10 * d0;
    float l21 = (A.m21 - l20 * l10 * d0) / d1;

    float d2  = A.m22 - l20 * l20 * d0 - l21 * l21 * d1;

    // Forward: L y = b
    float y0 = b.x;
    float y1 = b.y - l10 * y0;
    float y2 = b.z - l20 * y0 - l21 * y1;

    // Diagonal: D z = y
    float z0 = y0 / d0;
    float z1 = y1 / d1;
    float z2 = y2 / d2;

    // Back: L^T x = z
    float x2 = z2;
    float x1 = z1 - l21 * x2;
    float x0 = z0 - l10 * x1 - l20 * x2;

    return float3(x0, x1, x2);
}

// ---------------------------------------------------------------------------
// Static state
// ---------------------------------------------------------------------------
PhysicsBody2D*              Physics2DSolver::s_Bodies = nullptr;
Force2D*                    Physics2DSolver::s_Forces = nullptr;
Physics2DSolver::Params     Physics2DSolver::s_Params = {};
float                       Physics2DSolver::s_Dt = 0.0f;
float2                      Physics2DSolver::s_WorldMin = float2(0.f, 0.f);
float2                      Physics2DSolver::s_WorldMax = float2(0.f, 0.f);
bool                        Physics2DSolver::s_WrapX = false;
bool                        Physics2DSolver::s_WrapY = false;

void Physics2DSolver::SetWorldBounds(float2 minXY, float2 maxXY, EWorldBoundsType wrapType)
{
    bool wrapX = (wrapType == EWorldBoundsType::e_WrapHorizontally) || (wrapType == EWorldBoundsType::e_WrapAll);
    bool wrapY = (wrapType == EWorldBoundsType::e_WrapVertically) || (wrapType == EWorldBoundsType::e_WrapAll);
    
    s_WorldMin = minXY;
    s_WorldMax = maxXY;
    s_WrapX = wrapX && (maxXY.x > minXY.x);
    s_WrapY = wrapY && (maxXY.y > minXY.y);
}

float2 Physics2DSolver::MinimumImage(float2 dp)
{
    if (s_WrapX)
    {
        float w = s_WorldMax.x - s_WorldMin.x;
        float half = w * 0.5f;
        if      (dp.x >  half) dp.x -= w;
        else if (dp.x < -half) dp.x += w;
    }
    if (s_WrapY)
    {
        float h = s_WorldMax.y - s_WorldMin.y;
        float half = h * 0.5f;
        if      (dp.y >  half) dp.y -= h;
        else if (dp.y < -half) dp.y += h;
    }
    return dp;
}

// Wrap a world-space point into the bounded region on enabled axes. Used at
// the end of each substep so positions stay in range and bodies reappear on
// the opposite side after leaving.
static float2 WrapPos(float2 p)
{
    if (Physics2DSolver::IsWrapX())
    {
        float lo = Physics2DSolver::GetWorldMin().x;
        float hi = Physics2DSolver::GetWorldMax().x;
        float w  = hi - lo;
        while (p.x <  lo) p.x += w;
        while (p.x >= hi) p.x -= w;
    }
    if (Physics2DSolver::IsWrapY())
    {
        float lo = Physics2DSolver::GetWorldMin().y;
        float hi = Physics2DSolver::GetWorldMax().y;
        float h  = hi - lo;
        while (p.y <  lo) p.y += h;
        while (p.y >= hi) p.y -= h;
    }
    return p;
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------
void Physics2DSolver::Init()
{
    s_Bodies = nullptr;
    s_Forces = nullptr;
    DefaultParams();
}

void Physics2DSolver::Terminate()
{
    Clear();
}

void Physics2DSolver::Clear()
{
    // Body and force destructors unlink themselves via RemoveBody/RemoveForce,
    // so just keep deleting the head.
    while (s_Forces)
        delete s_Forces;
    while (s_Bodies)
        delete s_Bodies;
}

void Physics2DSolver::DefaultParams()
{
    s_Params.Gravity       = -9.8f;
    s_Params.Iterations    = 10;

    // See the VBD paper: beta typically in [1, 1000] and depends on units /
    // incrementing strategy; alpha in (0, 1] trades error-correction smoothness
    // against responsiveness; gamma < 1 decays penalty/lambda across steps.
    s_Params.Beta          = 100000.0f;
    s_Params.Alpha         = 0.99f;
    s_Params.Gamma         = 0.99f;

    // Post-stabilization adds one extra positional iteration; removes need for alpha.
    s_Params.PostStabilize = true;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------
void Physics2DSolver::AddBody(PhysicsBody2D* body)
{
    body->m_pNextLinkedBody = s_Bodies;
    s_Bodies = body;
}

void Physics2DSolver::RemoveBody(PhysicsBody2D* body)
{
    // Destroy any forces still referencing this body. Each Force2D dtor
    // unlinks itself from both bodies' per-body chains, so repeatedly
    // deleting the head of m_pForceList drains the list safely.
    while (body->m_pForceList)
        delete body->m_pForceList;

    PhysicsBody2D** slot = &s_Bodies;
    while (*slot && *slot != body)
        slot = &(*slot)->m_pNextLinkedBody;
    if (*slot == body)
        *slot = body->m_pNextLinkedBody;
    body->m_pNextLinkedBody = nullptr;
}

void Physics2DSolver::AddForce(Force2D* force)
{
    force->m_pNext = s_Forces;
    s_Forces = force;
}

void Physics2DSolver::RemoveForce(Force2D* force)
{
    Force2D** slot = &s_Forces;
    while (*slot && *slot != force)
        slot = &(*slot)->m_pNext;
    if (*slot == force)
        *slot = force->m_pNext;
    force->m_pNext = nullptr;
}

// ---------------------------------------------------------------------------
// Picking
// ---------------------------------------------------------------------------
PhysicsBody2D* Physics2DSolver::Pick(float2 at, float2& local)
{
    for (PhysicsBody2D* body = s_Bodies; body != nullptr; body = body->m_pNextLinkedBody)
    {
        float2x2 Rt = rotation(-body->m_Position.z);
        local = Rt * (at - XY(body->m_Position));

        auto view = body->getShapeView();
        if (view.Kind == PhysicsBody2D::ShapeKind::Box)
        {
            float2 h = view.Box->GetSize() * 0.5f;
            if (local.x >= -h.x && local.x <= h.x &&
                local.y >= -h.y && local.y <= h.y)
                return body;
        }
        else
        {
            float r = view.Sphere->GetRadius();
            if (Dot(local, local) <= r * r)
                return body;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Bounding-circle radius used by the naive O(n^2) broadphase
// ---------------------------------------------------------------------------
static float BoundingRadius(PhysicsBody2D* body)
{
    auto view = body->getShapeView();
    if (view.Kind == PhysicsBody2D::ShapeKind::Box)
    {
        float2 s = view.Box->GetSize();
        return s.length() * 0.5f;
    }
    if (view.Kind == PhysicsBody2D::ShapeKind::Sphere)
        return view.Sphere->GetRadius();
    // Static bodies have world-spanning extent; always pass broadphase.
    return 1.0e18f;
}

// ---------------------------------------------------------------------------
// Step
// ---------------------------------------------------------------------------
// Run one substep of the AVBD solver with the given dt. The public Step()
// wraps this with a substep count chosen from the per-body motion analysis.
void Physics2DSolver::StepOnce(float dt)
{
    s_Dt = dt;

#if DEBUG_TELEPORT
    // Snapshot pre-step positions for the teleport guard below.
    struct PreSnap { PhysicsBody2D* body; float2 pos; float3 vel; };
    PreSnap preSnap[64];
    int     preSnapCount = 0;
    for (PhysicsBody2D* b = s_Bodies; b != nullptr && preSnapCount < 64; b = b->m_pNextLinkedBody)
    {
        if (b->m_Mass <= 0.f) continue;
        preSnap[preSnapCount++] = { b, float2(b->m_Position.x, b->m_Position.y), b->m_Velocity };
    }
#endif
    const float gravity       = s_Params.Gravity;
    const int   iterations    = s_Params.Iterations;
    const float alpha         = s_Params.Alpha;
    const float beta          = s_Params.Beta;
    const float gamma         = s_Params.Gamma;
    const bool  postStabilize = s_Params.PostStabilize;

    // -- Broadphase (naive O(n^2)) ------------------------------------------
    for (PhysicsBody2D* bodyA = s_Bodies; bodyA != nullptr; bodyA = bodyA->m_pNextLinkedBody)
    {
        for (PhysicsBody2D* bodyB = bodyA->m_pNextLinkedBody; bodyB != nullptr; bodyB = bodyB->m_pNextLinkedBody)
        {
            // Skip pairs where both sides are sleeping: neither has moved since
            // last step, so any existing manifold state still holds and no new
            // pair can start overlapping without first involving an awake body.
            if (bodyA->IsSleeping() && bodyB->IsSleeping())
                continue;

            // World-wrap aware inter-body offset: minimum-image convention so
            // pairs near opposite seams see each other across the wrap.
            float2 dp = Physics2DSolver::MinimumImage(XY(bodyA->m_Position) - XY(bodyB->m_Position));
            // CCD-lite: inflate overlap threshold by the distance each body could
            // travel this step, so fast-approach pairs still generate a manifold
            // before they tunnel through each other.
            float2 vA = XY(bodyA->m_Velocity);
            float2 vB = XY(bodyB->m_Velocity);
            float  sweep = (vA.length() + vB.length()) * dt;
            float  r  = BoundingRadius(bodyA) + BoundingRadius(bodyB) + sweep;
            if (Dot(dp, dp) <= r * r)
            {
                // An awake body is within interaction range of a sleeping one:
                // wake the sleeper so it can react on this step. Static bodies
                // stay asleep — they don't participate in the solver's primal
                // update, so waking them buys nothing.
                if (bodyA->IsSleeping() && bodyA->getShapeView().Kind != PhysicsBody2D::ShapeKind::Static)
                    bodyA->Wake();
                if (bodyB->IsSleeping() && bodyB->getShapeView().Kind != PhysicsBody2D::ShapeKind::Static)
                    bodyB->Wake();

                if (!bodyA->IsConstrainedTo(*bodyB))
                {
                    // ContactManifold2D registers itself into s_Forces via Force2D ctor.
                    new ContactManifold2D(*bodyA, *bodyB);
                }
            }
        }
    }

    // -- Initialize and warmstart forces ------------------------------------
    for (Force2D* force = s_Forces; force != nullptr; )
    {
        if (!force->Initialize())
        {
            // Inactive force: remove and continue.
            Force2D* next = force->m_pNext;
            delete force;
            force = next;
            continue;
        }

        // Scale the penalty floor by the heaviest constrained body's M/dt².
        // PENALTY_MIN alone is mass-agnostic: at 10× world/sphere scale, mass
        // grows 100× (area × density) so the BDF1 system matrix M/dt² dwarfs
        // the floor and contacts feel soft until the penalty ramps up over
        // several iterations — producing visible ground penetration. Anchoring
        // the floor to M/dt² keeps constraints stiff at any scale.
        float heaviest = force->m_pBodyA->m_Mass;
        if (force->m_pBodyB != force->m_pBodyA && force->m_pBodyB->m_Mass > heaviest)
            heaviest = force->m_pBodyB->m_Mass;
        float massFloor  = heaviest / (dt * dt);
        float penaltyMin = PENALTY_MIN > massFloor ? PENALTY_MIN : massFloor;

        for (int i = 0; i < force->Rows(); i++)
        {
            if (postStabilize)
            {
                // Reuse the full lambda from the previous step; just decay penalty.
                force->m_Penalty[i] = clamp(force->m_Penalty[i] * gamma, penaltyMin, PENALTY_MAX);
            }
            else
            {
                // Warmstart both lambda and penalty (Eq. 19)
                force->m_Lambda[i]  = force->m_Lambda[i] * alpha * gamma;
                force->m_Penalty[i] = clamp(force->m_Penalty[i] * gamma, penaltyMin, PENALTY_MAX);
            }

            // Don't let the penalty exceed the soft material stiffness.
            force->m_Penalty[i] = min(force->m_Penalty[i], force->m_Stiffness[i]);
        }

        force = force->m_pNext;
    }

    // -- Initialize and warmstart bodies (primal variables) -----------------
    for (PhysicsBody2D* body = s_Bodies; body != nullptr; body = body->m_pNextLinkedBody)
    {
        // Clamp angular velocity
        body->m_Velocity.z = clamp(body->m_Velocity.z, -50.0f, 50.0f);

        // Inertial position (Eq. 2)
        body->m_Inertial = body->m_Position + body->m_Velocity * dt;
        if (body->m_bReceivesGravity)
            body->m_Inertial += float3(0, gravity, 0) * (dt * dt);

        // Adaptive warmstart (original VBD paper)
        float3 accel       = (body->m_Velocity - body->m_PreviousVelocity) / dt;
        float  accelExt    = accel.y * (float)sign(gravity);
        float  accelWeight = clamp(accelExt / fabsf(gravity), 0.0f, 1.0f);
        if (!isfinite(accelWeight))
            accelWeight = 0.0f;

        // Save x- and compute warmstarted position. The velocity-based warmstart
        // can overshoot into a nearby surface for fast bodies: if the body is
        // already in contact, skip it entirely; otherwise clamp the linear
        // displacement to the bounding radius so we can't tunnel in one step.
        // Save x- and compute warmstarted position
        body->m_InitialPosition = body->m_Position;
        body->m_Position        = body->m_Position + body->m_Velocity * dt
                                + float3(0, gravity, 0) * (accelWeight * dt * dt);
    }

    // -- Main solver loop ---------------------------------------------------
    const int totalIterations = iterations + (postStabilize ? 1 : 0);

    for (int it = 0; it < totalIterations; it++)
    {
        // With post-stabilization: fully remove (1.0) existing error on the
        // solver iterations, then keep it (0.0) for the stabilization pass.
        float currentAlpha = alpha;
        if (postStabilize)
            currentAlpha = it < iterations ? 1.0f : 0.0f;

        // Primal update
        for (PhysicsBody2D* body = s_Bodies; body != nullptr; body = body->m_pNextLinkedBody)
        {
            if (body->m_Mass <= 0 || body->m_bSleeping)
                continue;

            // Left/right-hand sides of the linear system (Eqs. 5, 6)
            float3x3 M   = Diag3(body->m_Mass, body->m_Mass, body->m_Moment);
            float    inv = 1.0f / (dt * dt);
            float3x3 lhs = inv * M;
            float3   rhs = (inv * M) * (body->m_Position - body->m_Inertial);

            // Iterate the body's per-body force chain (matches reference exactly).
            for (Force2D* force = body->m_pForceList; force != nullptr;
                 force = (force->m_pBodyA == body) ? force->m_pNextA : force->m_pNextB)
            {
                force->ComputeConstraint(currentAlpha);
                force->ComputeDerivatives(*body);

                for (int i = 0; i < force->Rows(); i++)
                {
                    // Lambda is zero unless it's a hard constraint.
                    float lambda = isinf(force->m_Stiffness[i]) ? force->m_Lambda[i] : 0.0f;

                    // Clamped force magnitude (Sec 3.2)
                    float f = clamp(force->m_Penalty[i] * force->m_C[i] + lambda,
                                    force->m_Fmin[i], force->m_Fmax[i]);

                    // Diagonally-lumped geometric stiffness (Sec 3.5)
                    float3x3 G = fabsf(f) * Diag3(Length(Col3(force->m_H[i], 0)),
                                                  Length(Col3(force->m_H[i], 1)),
                                                  Length(Col3(force->m_H[i], 2)));

                    // Accumulate force (Eq. 13) and hessian (Eq. 17)
                    rhs += force->m_J[i] * f;
                    lhs = lhs + Outer3(force->m_J[i], force->m_J[i] * force->m_Penalty[i]) + G;
                }
            }

            // Solve SPD system and apply update (Eq. 4). Apply the step, then
            // clamp the body's cumulative linear displacement from its
            // substep-start position to the bounding radius. This enforces
            // the same "max one body-radius of motion per substep" invariant
            // that drives our adaptive substepping, blocking the rare case
            // where a warmed-up penalty + large residual in post-stabilization
            // solves for a position jump that teleports past other colliders.
            float3 step = Solve3(lhs, rhs);
            body->m_Position -= step;

            float rad = BoundingRadius(body);
            if (isfinite(rad) && rad > 0.0f)
            {
                float dx = body->m_Position.x - body->m_InitialPosition.x;
                float dy = body->m_Position.y - body->m_InitialPosition.y;
                float dispLen = sqrtf(dx * dx + dy * dy);
                if (dispLen > rad)
                {
                    float s = rad / dispLen;
                    body->m_Position.x = body->m_InitialPosition.x + dx * s;
                    body->m_Position.y = body->m_InitialPosition.y + dy * s;
                }
            }
        }

        // Dual update — skipped during post-stabilization passes so we don't
        // bake stabilization into the persistent penalty/lambda state.
        if (it < iterations)
        {
            for (Force2D* force = s_Forces; force != nullptr; force = force->m_pNext)
            {
                force->ComputeConstraint(currentAlpha);

                for (int i = 0; i < force->Rows(); i++)
                {
                    float lambda = isinf(force->m_Stiffness[i]) ? force->m_Lambda[i] : 0.0f;

                    // Update lambda (Eq. 11)
                    force->m_Lambda[i] = clamp(force->m_Penalty[i] * force->m_C[i] + lambda,
                                               force->m_Fmin[i], force->m_Fmax[i]);

                    // Fracture test
                    if (fabsf(force->m_Lambda[i]) >= force->m_Fracture[i])
                        force->Disable();

                    // Update penalty if within the force bounds (Eq. 16)
                    if (force->m_Lambda[i] > force->m_Fmin[i] &&
                        force->m_Lambda[i] < force->m_Fmax[i])
                    {
                        float bumped = force->m_Penalty[i] + beta * fabsf(force->m_C[i]);
                        force->m_Penalty[i] = min(bumped, min(PENALTY_MAX, force->m_Stiffness[i]));
                    }
                }
            }
        }

        // After the final solver iteration (before any post-stab passes),
        // compute new velocities via BDF1.
        if (it == iterations - 1)
        {
            for (PhysicsBody2D* body = s_Bodies; body != nullptr; body = body->m_pNextLinkedBody)
            {
                body->m_PreviousVelocity = body->m_Velocity;
                if (body->m_Mass > 0 && !body->m_bSleeping)
                {
                    body->m_Velocity = (body->m_Position - body->m_InitialPosition) / dt;

                    // Per-body linear + angular damping: v *= 1 / (1 + dt*damping).
                    // Drains residual energy geometrically so the body can actually
                    // reach zero, instead of asymptoting under penalty-based drag.
                    float linScale = 1.0f / (1.0f + dt * body->m_LinearDamping);
                    float angScale = 1.0f / (1.0f + dt * body->m_AngularDamping);
                    body->m_Velocity.x *= linScale;
                    body->m_Velocity.y *= linScale;
                    body->m_Velocity.z *= angScale;

                    // Rest snap: once velocity is small enough it will never matter
                    // visually, zero it so the body is truly at rest and can stop
                    // re-triggering contact work every frame.
                    bool slow = fabsf(body->m_Velocity.x) < SLEEP_LIN_THRESH &&
                                fabsf(body->m_Velocity.y) < SLEEP_LIN_THRESH;
                    if (slow)
                    {
                        body->m_Velocity.x = 0.0f;
                        body->m_Velocity.y = 0.0f;
                    }
                    bool slowAng = fabsf(body->m_Velocity.z) < SLEEP_ANG_THRESH;
                    if (slowAng)
                        body->m_Velocity.z = 0.0f;

                    // Sleep promotion: after enough consecutive substeps where
                    // both linear and angular velocity have been snapped to
                    // zero, put the body fully to sleep so the solver skips
                    // it entirely until something wakes it.
                    if (slow && slowAng)
                    {
                        if (++body->m_SleepCounter >= SLEEP_FRAMES)
                            body->PutToSleep();
                    }
                    else
                    {
                        body->m_SleepCounter = 0;
                    }
                }
            }
        }
    }

#if DEBUG_TELEPORT
    // Detect any body that displaced more than its bounding radius in this
    // substep, BEFORE world-wrap is applied (wrap is a legal teleport). We
    // use minimum-image distance so a body that moved normally but happens to
    // be near a seam isn't flagged.
    for (int k = 0; k < preSnapCount; ++k)
    {
        PhysicsBody2D* b = preSnap[k].body;
        float2 after = float2(b->m_Position.x, b->m_Position.y);
        float2 delta = MinimumImage(after - preSnap[k].pos);
        float  move  = delta.length();
        float  rad   = BoundingRadius(b);
        if (isfinite(rad) && move > rad)
        {
            printf("[TELEPORT] body=%p dt=%.4f move=%.4f rad=%.4f pre=(%.3f,%.3f) post=(%.3f,%.3f) vel_pre=(%.3f,%.3f) vel_post=(%.3f,%.3f)\n",
                   (void*)b, dt, move, rad,
                   preSnap[k].pos.x, preSnap[k].pos.y,
                   after.x, after.y,
                   preSnap[k].vel.x, preSnap[k].vel.y,
                   b->m_Velocity.x, b->m_Velocity.y);

            // Dump contact partners so we can see which collision triggered it.
            for (Force2D* f = b->m_pForceList; f != nullptr;
                 f = (f->m_pBodyA == b) ? f->m_pNextA : f->m_pNextB)
            {
                if (f->ActiveContactCount() > 0)
                {
                    const PhysicsBody2D* other = (f->GetBodyA() == b) ? f->GetBodyB() : f->GetBodyA();
                    printf("  contact with body=%p contacts=%d\n", (void*)other, f->ActiveContactCount());
                }
            }
        }
    }
#endif

    // -- World wrap ---------------------------------------------------------
    // Wrap body positions into the world bounds after the step. Velocity has
    // already been computed from (m_Position - m_InitialPosition)/dt above, so
    // wrapping now doesn't corrupt it. We still wrap m_InitialPosition by the
    // same offset to keep any downstream code that diffs them (e.g. subsequent
    // post-stabilization) self-consistent.
    if (s_WrapX || s_WrapY)
    {
        for (PhysicsBody2D* body = s_Bodies; body != nullptr; body = body->m_pNextLinkedBody)
        {
            if (body->m_Mass <= 0.f)
                continue;

            float2 before = float2(body->m_Position.x, body->m_Position.y);
            float2 after  = WrapPos(before);
            float2 shift  = after - before;

            if (shift.x != 0.f || shift.y != 0.f)
            {
                body->m_Position.x        += shift.x;
                body->m_Position.y        += shift.y;
                body->m_InitialPosition.x += shift.x;
                body->m_InitialPosition.y += shift.y;
            }
        }
    }
}

void Physics2DSolver::Step(float dt)
{
    auto timer = CTimerManager::GetCPUTimer("Physics2DSolver::Step");
    timer->Start();
    
    // Adaptive substepping: instead of committing to a substep count up front,
    // reassess the budget each substep using the *current* velocity. A mid-
    // frame collision (e.g. a wall bounce) can spike velocity well above what
    // the incoming motion suggested; re-evaluating per substep ensures we
    // still respect the "half-bounding-radius per substep" tunneling rule
    // after such an impulse, instead of coasting on stale counts and
    // punching through the next body we hit.
    float remaining = dt;
    int   stepsLeft = MAX_SUBSTEPS;

    while (remaining > 0.0f && stepsLeft > 0)
    {
        float maxRatio = 0.0f;
        for (PhysicsBody2D* b = s_Bodies; b != nullptr; b = b->m_pNextLinkedBody)
        {
            if (b->m_Mass <= 0.f)
                continue;

            float rad = BoundingRadius(b);
            if (!isfinite(rad) || rad <= 0.f)
                continue;

            float speed = float2(b->m_Velocity.x, b->m_Velocity.y).length();
            float ratio = speed * remaining / rad;
            if (ratio > maxRatio)
                maxRatio = ratio;
        }

        // Split the remainder into N pieces where each covers <= rad/2.
        int subs = (int)ceilf(maxRatio * 2.0f);
        if (subs < 1)               subs = 1;
        if (subs > stepsLeft)       subs = stepsLeft;

        float subDt = remaining / (float)subs;
        StepOnce(subDt);

        remaining -= subDt;
        --stepsLeft;
    }

    timer->Stop();
}
