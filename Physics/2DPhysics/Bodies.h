#ifndef __2D_PHYSICS_BODIES_H__
#define __2D_PHYSICS_BODIES_H__

#include "Engine/Maths/Maths.h"

class RigidBox2D;
class RigidSphere2D;
class RigidStatic2D;
class Force2D;

class PhysicsBody2D
{
    friend class ContactManifold2D;
    friend class Physics2DSolver;
    friend class Force2D;
public:

    struct PhysicsBodyDesc
    {
        float2 InitialPosition;
        float2 InitialVelocity;
        
        float InitialAngle;
        float InitialAngularVelocity;
        float Density;
        float Friction;
        float Restitution;           // [0, 1] — 0 fully inelastic, 1 fully elastic
    };
    
    PhysicsBody2D(const PhysicsBodyDesc& desc, float volume, float inertiaFormFactor);
    virtual ~PhysicsBody2D() = default;

    // A pure virtual function that forces children to identify themselves
    enum class ShapeKind { Box, Sphere, Static };
    struct ShapeView
    {
        ShapeKind Kind;
        union
        {
            RigidBox2D*    Box;
            RigidSphere2D* Sphere;
            RigidStatic2D* Static;
        };

        ShapeView(RigidBox2D* box)      : Kind(ShapeKind::Box),    Box(box)       {}
        ShapeView(RigidSphere2D* sphere): Kind(ShapeKind::Sphere), Sphere(sphere) {}
        ShapeView(RigidStatic2D* stat)  : Kind(ShapeKind::Static), Static(stat)   {}
    };
    virtual ShapeView getShapeView() = 0;

    bool IsConstrainedTo(const PhysicsBody2D& other) const;

    float3 GetPosition() const
    {
        return m_Position;
    }

    float GetFriction() const
    {
        return m_Friction;
    }

    float GetRestitution() const       { return m_Restitution; }
    void  SetRestitution(float e)      { m_Restitution = e; }

    float GetLinearDamping() const     { return m_LinearDamping; }
    void  SetLinearDamping(float d)    { m_LinearDamping = d; }

    float GetAngularDamping() const    { return m_AngularDamping; }
    void  SetAngularDamping(float d)   { m_AngularDamping = d; }

    // Sleep state: sleeping bodies are skipped by the primal solver update
    // and by broadphase when both sides sleep. Near-rest motion over several
    // consecutive substeps trips the sleep flag; any significant movement,
    // broadphase overlap with an awake body, or a freshly attached force
    // wakes it again.
    bool IsSleeping() const            { return m_bSleeping; }
    void Wake()                        { m_bSleeping = false; m_SleepCounter = 0; }
    void PutToSleep()
    {
        m_bSleeping        = true;
        m_SleepCounter     = 0;
        m_Velocity         = float3(0.f, 0.f, 0.f);
        m_PreviousVelocity = float3(0.f, 0.f, 0.f);
    }

protected:

    float3 m_Position;
    float3 m_Velocity;
    float3 m_Inertial;
    float3 m_PreviousVelocity;
    float3 m_InitialPosition;

    float m_Mass;
    float m_Moment;
    float m_Friction;
    float m_Restitution;
    float m_LinearDamping;
    float m_AngularDamping;
    bool  m_bReceivesGravity;
    bool  m_bSleeping;
    int   m_SleepCounter;

    PhysicsBody2D* m_pNextLinkedBody;
    Force2D*       m_pForceList;      // Head of the per-body force chain
};

class RigidBox2D : public PhysicsBody2D
{
public:
    RigidBox2D(const PhysicsBodyDesc& desc, float2& boxSize);
    ~RigidBox2D() override;

    ShapeView getShapeView() override
    { 
        return ShapeView(this);
    }

    float2 GetSize() const
    {
        return m_Size;
    }

private:    
    float2 m_Size;
};

class RigidSphere2D : public PhysicsBody2D
{
public:
    RigidSphere2D(const PhysicsBodyDesc& desc, float sphereRadius);
    ~RigidSphere2D() override;

    ShapeView getShapeView() override { return ShapeView(this); }

    float GetRadius() const
    {
        return m_Radius;
    }

private:
    float m_Radius;
};

class RigidStatic2D : public PhysicsBody2D
{
public:
    RigidStatic2D(float friction = 0.5f);
    ~RigidStatic2D() override;

    ShapeView getShapeView() override { return ShapeView(this); }

    // World-space signed distance. Negative inside the static body.
    virtual float  SampleSDF(float2 worldPos) const = 0;
    // Outward gradient of the SDF at worldPos.
    virtual float2 SampleSDFNormal(float2 worldPos) const = 0;
};

#endif
