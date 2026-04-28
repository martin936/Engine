#include "2DPhysics.h"
#include "Engine/Engine.h"

#define INERTIA_FORM_FACTOR_BOX_2D(boxSize) (float2::dotproduct(boxSize, boxSize) * (1.f / 12.f))
#define INERTIA_FORM_FACTOR_SPHERE_2D(radius) (0.25f * M_PI * sphereRadius * sphereRadius)


PhysicsBody2D::PhysicsBody2D(const PhysicsBodyDesc& desc, float volume, float inertiaFormFactor)
{
    m_Mass = volume * desc.Density;
    m_Moment = m_Mass * inertiaFormFactor;
    m_Friction = desc.Friction;
    m_Restitution = desc.Restitution;
    m_LinearDamping = 0.0f;
    m_AngularDamping = 0.0f;

    m_Position = float3(desc.InitialPosition.x, desc.InitialPosition.y, desc.InitialAngle);
    m_Velocity = float3(desc.InitialVelocity.x, desc.InitialVelocity.y, desc.InitialAngularVelocity);
    m_InitialPosition = m_Position;
    m_PreviousVelocity = m_Velocity;
    
    m_pNextLinkedBody   = nullptr;
    m_pForceList        = nullptr;
    m_bReceivesGravity  = true;
    m_bSleeping         = false;
    m_SleepCounter      = 0;

    Physics2DSolver::AddBody(this);
}

bool PhysicsBody2D::IsConstrainedTo(const PhysicsBody2D& other) const
{
    // Walk this body's force chain, picking nextA or nextB depending on which
    // side of the force references us.
    for (Force2D* f = m_pForceList; f != nullptr;
         f = (f->m_pBodyA == this) ? f->m_pNextA : f->m_pNextB)
    {
        if ((f->m_pBodyA == this && f->m_pBodyB == &other) ||
            (f->m_pBodyA == &other && f->m_pBodyB == this))
            return true;
    }

    return false;
}

RigidBox2D::RigidBox2D(const PhysicsBodyDesc& desc, float2& boxSize) : PhysicsBody2D(desc, boxSize.x * boxSize.y, INERTIA_FORM_FACTOR_BOX_2D(boxSize))
{
    m_Size = boxSize;
}

RigidBox2D::~RigidBox2D()
{
    Physics2DSolver::RemoveBody(this);
}

RigidSphere2D::RigidSphere2D(const PhysicsBodyDesc& desc, float sphereRadius) : PhysicsBody2D(desc, M_PI * sphereRadius * sphereRadius, INERTIA_FORM_FACTOR_SPHERE_2D(sphereRadius))
{
    m_Radius = sphereRadius;
}

RigidSphere2D::~RigidSphere2D()
{
    Physics2DSolver::RemoveBody(this);
}

static PhysicsBody2D::PhysicsBodyDesc MakeStaticDesc(float friction)
{
    PhysicsBody2D::PhysicsBodyDesc desc = {};
    desc.InitialPosition        = float2(0.f, 0.f);
    desc.InitialVelocity        = float2(0.f, 0.f);
    desc.InitialAngle           = 0.f;
    desc.InitialAngularVelocity = 0.f;
    desc.Density                = 0.f;
    desc.Friction               = friction;
    desc.Restitution            = 0.f;
    return desc;
}

RigidStatic2D::RigidStatic2D(float friction) : PhysicsBody2D(MakeStaticDesc(friction), 1.f, 1.f)
{
    // Large mass + an infinite-stiffness pin constraint at the initial pose.
    // High inertia makes contacts from dynamic bodies see the static as immovable
    // on the primal step; the pin's augmented Lagrangian then nails residual drift.
    // Gravity is disabled so the pin doesn't have to re-balance weight each step.
    m_Mass              = 1.0e3f;
    m_Moment            = 1.0e3f;
    m_bReceivesGravity  = false;
    // Statics never move, so they start asleep and stay that way. The sleep
    // check in broadphase treats "both sleeping" as a skip — this keeps
    // sphere-static pairs from doing broadphase work once the dynamic side
    // has also settled, while still waking on demand if the dynamic moves.
    m_bSleeping         = true;

    new Pin2D(*this);
}

RigidStatic2D::~RigidStatic2D()
{
    Physics2DSolver::RemoveBody(this);
}

