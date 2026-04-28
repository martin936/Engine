#ifndef __2D_PHYSICS_FORCES_H__
#define __2D_PHYSICS_FORCES_H__

#include "Engine/Maths/Maths.h"

#ifndef MAX_ROWS
#define MAX_ROWS 4
#endif

class PhysicsBody2D;
class RigidStatic2D;

class Force2D
{
    friend class Physics2DSolver;
    friend class PhysicsBody2D;
public:
    Force2D(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB);
    virtual ~Force2D();

    virtual int Rows() const = 0;
    virtual bool Initialize() = 0;
    virtual void ComputeConstraint(float alpha) = 0;
    virtual void ComputeDerivatives(PhysicsBody2D& body) = 0;

    // Number of active contact points this force represents. Non-contact forces
    // return 0; ContactManifold2D returns its current m_NumContacts. Used by
    // other forces (e.g. Drag2D) to query a body's contact state.
    virtual int ActiveContactCount() const { return 0; }

    // Returns true if `body` has at least one contact manifold with active
    // contact points. Helper for derived forces (friendship with PhysicsBody2D
    // isn't inherited, so we expose the per-body chain walk here).
    static bool BodyHasContact(const PhysicsBody2D& body);

    void Disable();

    PhysicsBody2D const* GetBodyA() const
    {
        return m_pBodyA;
    }

    PhysicsBody2D const* GetBodyB() const
    {
        return m_pBodyB;
    }

    Force2D* GetNext()
    {
        return m_pNext;
    }
    
protected:

    PhysicsBody2D* m_pBodyA;
    PhysicsBody2D* m_pBodyB;

    Force2D* m_pNext;        // Global list, owned by Physics2DSolver
    Force2D* m_pNextA;       // Next force in bodyA's per-body chain
    Force2D* m_pNextB;       // Next force in bodyB's per-body chain

    float3 m_J[MAX_ROWS];
    float3x3 m_H[MAX_ROWS];
    float m_C[MAX_ROWS];
    float m_Fmin[MAX_ROWS];
    float m_Fmax[MAX_ROWS];
    float m_Stiffness[MAX_ROWS];
    float m_Fracture[MAX_ROWS];
    float m_Penalty[MAX_ROWS];
    float m_Lambda[MAX_ROWS];
};

class Spring2D : Force2D
{
public:

    Spring2D(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB, float2 radiusA, float2 radiusB, float stiffness, float restLength = -1);

    int Rows() const override { return 1; }

    bool Initialize() override;
    void ComputeConstraint(float alpha) override;
    void ComputeDerivatives(PhysicsBody2D& body) override;

private:
    
    float2 m_RadiusA;
    float2 m_RadiusB;
    float m_RestLength;
};

// Single-body hard anchor: pins position and angle to the world-space values
// captured at construction, via three infinite-stiffness rows handled by the
// AVBD augmented Lagrangian.
class Pin2D : public Force2D
{
public:
    Pin2D(PhysicsBody2D& body, float torqueArm = 1.0f);

    int Rows() const override { return 3; }

    bool Initialize() override;
    void ComputeConstraint(float alpha) override;
    void ComputeDerivatives(PhysicsBody2D& body) override;

private:
    float3 m_Target;
    float3 m_C0;
    float  m_TorqueArm;
};

// Single-body viscous drag: opposes linear and angular motion with a force
// proportional to velocity. Implemented as a soft anchor at the step-start pose
// (captured each Initialize); stiffness = drag / dt reproduces F = -c*v once the
// solver converges. SetEnabled toggles it at runtime without re-registering.
class Drag2D : public Force2D
{
public:
    Drag2D(PhysicsBody2D& body, float linearDrag, float angularDrag);

    int Rows() const override { return 3; }

    bool Initialize() override;
    void ComputeConstraint(float alpha) override;
    void ComputeDerivatives(PhysicsBody2D& body) override;

    void SetEnabled(bool enabled)        { m_Enabled = enabled; }
    void SetLinearDrag(float c)          { m_LinearDrag = c; }
    void SetAngularDrag(float c)         { m_AngularDrag = c; }
    // If true, drag only applies on steps where the body has an active contact
    // (contact manifold with at least one contact point). Default: false.
    void SetRequiresContact(bool v)      { m_RequiresContact = v; }
    bool IsEnabled() const               { return m_Enabled; }

private:
    float3 m_Start;
    float  m_LinearDrag;
    float  m_AngularDrag;
    bool   m_Enabled;
    bool   m_RequiresContact;
};

class Joint2D : Force2D
{
public:

    Joint2D(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB, float2 radiusA, float2 radiusB, float3 stiffness = float3{ INFINITY, INFINITY, INFINITY }, float fracture = INFINITY);

    int Rows() const override { return 3; }

    bool Initialize() override;
    void ComputeConstraint(float alpha) override;
    void ComputeDerivatives(PhysicsBody2D& body) override;

private:
    
    float2 m_RadiusA;
    float2 m_RadiusB;
    float3 m_C0;
    float m_TorqueArm;
    float m_RestAngle;
};

class ContactManifold2D : Force2D
{
public:
    
    // Used to track contact features between frames
    union FeaturePair
    {
        struct Edges
        {
            char inEdge1;
            char outEdge1;
            char inEdge2;
            char outEdge2;
        } e;
        int value;
    };

    // Contact point information for a single contact
    struct Contact2D
    {
        FeaturePair feature;
        float2 rA;
        float2 rB;
        float2 normal;

        float3 JAn, JBn, JAt, JBt;
        float2 C0;
        float  restitutionBias;   // -e * v_rel_n_pre * dt, applied to normal row during main iters
        bool stick;
    };

    ContactManifold2D(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB);

    int Rows() const override { return m_NumContacts * 2; }
    int ActiveContactCount() const override { return m_NumContacts; }

    bool Initialize() override;
    void ComputeConstraint(float alpha) override;
    void ComputeDerivatives(PhysicsBody2D& body) override;

    // wrapB is added to bodyB's effective position during geometry calculation,
    // so bodies on opposite sides of a wrap seam can still collide via the
    // minimum-image convention. Pass float2(0,0) for no wrap.
    static int Collide(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB, Contact2D* contacts, float2 wrapB = float2(0.f, 0.f));

private:

    static int CollideBoxBox(RigidBox2D& bodyA, RigidBox2D& bodyB, Contact2D* contacts, float2 wrapB);
    static int CollideBoxSphere(RigidBox2D& bodyA, RigidSphere2D& bodyB, Contact2D* contacts, float2 wrapB);
    static int CollideSphereSphere(RigidSphere2D& bodyA, RigidSphere2D& bodyB, Contact2D* contacts, float2 wrapB);
    static int CollideBoxStatic(RigidBox2D& bodyA, RigidStatic2D& bodyB, Contact2D* contacts);
    static int CollideSphereStatic(RigidSphere2D& bodyA, RigidStatic2D& bodyB, Contact2D* contacts);

    Contact2D m_Contacts[2];
    int m_NumContacts;
    float m_Friction;
    float2 m_WrapOffset;    // Offset to add to bodyB's position to reach its image closest to bodyA.
};

#endif
