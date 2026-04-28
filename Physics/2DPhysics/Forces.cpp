#include "Bodies.h"
#include "Forces.h"

#include "2DPhysics.h"

Force2D::Force2D(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB)
{
    m_pBodyA  = &bodyA;
    m_pBodyB  = &bodyB;
    m_pNext   = nullptr;
    m_pNextA  = nullptr;
    m_pNextB  = nullptr;

    // Set some reasonable defaults
    for (int i = 0; i < MAX_ROWS; i++)
    {
        m_J[i] = float3(0.f);
        m_H[i] = float3x3(0.f);
        m_C[i] = 0.0f;
        m_Stiffness[i] = INFINITY;
        m_Fmax[i] = INFINITY;
        m_Fmin[i] = -INFINITY;
        m_Fracture[i] = INFINITY;

        m_Penalty[i] = 0.0f;
        m_Lambda[i] = 0.0f;
    }

    // Insert into the solver's global list and the body chains. Single-body
    // constraints (bodyA == bodyB) link into that body only once; the per-body
    // iterator will walk via m_pNextA in that case.
    Physics2DSolver::AddForce(this);

    m_pNextA = m_pBodyA->m_pForceList;
    m_pBodyA->m_pForceList = this;

    if (m_pBodyB != m_pBodyA)
    {
        m_pNextB = m_pBodyB->m_pForceList;
        m_pBodyB->m_pForceList = this;
    }

    // Attaching a new force (spring, joint, drag, contact manifold) implies
    // something is about to act on these bodies — wake any that were asleep
    // so the solver picks them up starting this step. Statics stay asleep
    // because they never participate in the primal update regardless.
    using K = PhysicsBody2D::ShapeKind;
    if (m_pBodyA->IsSleeping() && m_pBodyA->getShapeView().Kind != K::Static)
        m_pBodyA->Wake();
    if (m_pBodyB != m_pBodyA &&
        m_pBodyB->IsSleeping() && m_pBodyB->getShapeView().Kind != K::Static)
        m_pBodyB->Wake();
}

Force2D::~Force2D()
{
    Physics2DSolver::RemoveForce(this);

    // Unlink from bodyA's per-body list. At each node, pick nextA or nextB
    // depending on which side of that force references bodyA.
    Force2D** p = &m_pBodyA->m_pForceList;
    while (*p != this)
        p = ((*p)->m_pBodyA == m_pBodyA) ? &(*p)->m_pNextA : &(*p)->m_pNextB;
    *p = m_pNextA;

    // Single-body constraints only linked once.
    if (m_pBodyB != m_pBodyA)
    {
        p = &m_pBodyB->m_pForceList;
        while (*p != this)
            p = ((*p)->m_pBodyA == m_pBodyB) ? &(*p)->m_pNextA : &(*p)->m_pNextB;
        *p = m_pNextB;
    }
}

bool Force2D::BodyHasContact(const PhysicsBody2D& body)
{
    for (Force2D* f = body.m_pForceList; f != nullptr;
         f = (f->m_pBodyA == &body) ? f->m_pNextA : f->m_pNextB)
    {
        if (f->ActiveContactCount() > 0)
            return true;
    }
    return false;
}

void Force2D::Disable()
{
    // Disable this force by clearing the relevant fields
    for (int i = 0; i < MAX_ROWS; i++)
    {
        m_Stiffness[i] = 0;
        m_Penalty[i] = 0;
        m_Lambda[i] = 0;
    }
}

Pin2D::Pin2D(PhysicsBody2D& body, float torqueArm) : Force2D(body, body)
{
    m_Target    = body.GetPosition();
    m_TorqueArm = torqueArm;

    m_Stiffness[0] = INFINITY;
    m_Stiffness[1] = INFINITY;
    m_Stiffness[2] = INFINITY;
}

bool Pin2D::Initialize()
{
    float3 p = m_pBodyA->GetPosition();
    m_C0 = float3(p.x - m_Target.x,
                  p.y - m_Target.y,
                  (p.z - m_Target.z) * m_TorqueArm);
    return true;
}

void Pin2D::ComputeConstraint(float alpha)
{
    float3 p = m_pBodyA->GetPosition();
    float3 Cn(p.x - m_Target.x,
              p.y - m_Target.y,
              (p.z - m_Target.z) * m_TorqueArm);

    for (int i = 0; i < Rows(); i++)
    {
        if (isinf(m_Stiffness[i]))
            m_C[i] = Cn[i] - m_C0[i] * alpha;
        else
            m_C[i] = Cn[i];
    }
}

void Pin2D::ComputeDerivatives(PhysicsBody2D& /*body*/)
{
    m_J[0] = float3(1.0f, 0.0f, 0.0f);
    m_J[1] = float3(0.0f, 1.0f, 0.0f);
    m_J[2] = float3(0.0f, 0.0f, m_TorqueArm);

    m_H[0] = float3x3(0.f);
    m_H[1] = float3x3(0.f);
    m_H[2] = float3x3(0.f);
}

Drag2D::Drag2D(PhysicsBody2D& body, float linearDrag, float angularDrag) : Force2D(body, body)
{
    m_LinearDrag      = linearDrag;
    m_AngularDrag     = angularDrag;
    m_Enabled         = true;
    m_RequiresContact = false;
    m_Start           = body.GetPosition();

    // Finite stiffness -> lambda stays zero (soft row); penalty carries the force.
    // Actual per-step stiffness is refreshed in Initialize() from the solver dt.
    m_Stiffness[0] = linearDrag;
    m_Stiffness[1] = linearDrag;
    m_Stiffness[2] = angularDrag;
}

bool Drag2D::Initialize()
{
    m_Start = m_pBodyA->GetPosition();

    bool active = m_Enabled && (m_LinearDrag > 0.0f || m_AngularDrag > 0.0f);

    // Contact-gated mode: only damp this step if the body is currently touching
    // another body. Contact manifolds are prepended to s_Forces when created,
    // so they Initialize before this force and expose up-to-date contact counts.
    if (active && m_RequiresContact)
        active = BodyHasContact(*m_pBodyA);

    if (!active)
    {
        m_Stiffness[0] = m_Stiffness[1] = m_Stiffness[2] = 0.0f;
        m_Penalty[0]   = m_Penalty[1]   = m_Penalty[2]   = 0.0f;
        return true;
    }

    // Penalty we want the solver to apply: c/dt, so that penalty * C ≈
    // (c/dt) * (v*dt) = c*v, i.e. F = -c*v viscous drag once converged.
    float dt = Physics2DSolver::GetStepDt();
    float inv_dt = dt > 0.0f ? 1.0f / dt : 60.0f;

    m_Stiffness[0] = m_LinearDrag  * inv_dt;
    m_Stiffness[1] = m_LinearDrag  * inv_dt;
    m_Stiffness[2] = m_AngularDrag * inv_dt;

    // Pin the penalty at the target stiffness so drag acts on the very first
    // iteration instead of ramping up from PENALTY_MIN via the dual update.
    m_Penalty[0]   = m_Stiffness[0];
    m_Penalty[1]   = m_Stiffness[1];
    m_Penalty[2]   = m_Stiffness[2];

    return true;
}

void Drag2D::ComputeConstraint(float /*alpha*/)
{
    // Soft rows -> alpha-stabilization (C0) is not used.
    float3 p = m_pBodyA->GetPosition();
    m_C[0] = p.x - m_Start.x;
    m_C[1] = p.y - m_Start.y;
    m_C[2] = p.z - m_Start.z;
}

void Drag2D::ComputeDerivatives(PhysicsBody2D& /*body*/)
{
    m_J[0] = float3(1.0f, 0.0f, 0.0f);
    m_J[1] = float3(0.0f, 1.0f, 0.0f);
    m_J[2] = float3(0.0f, 0.0f, 1.0f);

    m_H[0] = float3x3(0.f);
    m_H[1] = float3x3(0.f);
    m_H[2] = float3x3(0.f);
}

Joint2D::Joint2D(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB, float2 radiusA, float2 radiusB, float3 stiffness, float fracture) : Force2D(bodyA, bodyB)
{
    m_Stiffness[0] = stiffness.x;
    m_Stiffness[1] = stiffness.y;
    m_Stiffness[2] = stiffness.z;

    m_Fmax[2] = fracture;
    m_Fmin[2] = -fracture;
    m_Fracture[2] = fracture;

    m_RestAngle = bodyA.GetPosition().z - bodyB.GetPosition().z;
    m_TorqueArm = 0.f;
}

bool Joint2D::Initialize()
{
    // Store constraint function at beginnning of timestep C(x-)
    float2 C_pos = transform(m_pBodyA->GetPosition(), m_RadiusA) - transform(m_pBodyB->GetPosition(), m_RadiusB);
    float C_angle = (m_pBodyA->GetPosition().z - m_pBodyB->GetPosition().z - m_RestAngle) * m_TorqueArm;
    m_C0 = float3(C_pos.x, C_pos.y, C_angle);
    
    return m_Stiffness[0] != 0 || m_Stiffness[1] != 0 || m_Stiffness[2] != 0;
}

void Joint2D::ComputeConstraint(float alpha)
{
    // Compute constraint function at current state C(x)
    float3 Cn;
    float2 C_pos = transform(m_pBodyA->GetPosition(), m_RadiusA) - transform(m_pBodyB->GetPosition(), m_RadiusB);
    float C_angle = (m_pBodyA->GetPosition().z - m_pBodyB->GetPosition().z - m_RestAngle) * m_TorqueArm;
    m_C0 = float3(C_pos.x, C_pos.y, C_angle);

    for (int i = 0; i < Rows(); i++)
    {
        // Store stabilized constraint function, if a hard constraint (Eq. 18)
        if (isinf(m_Stiffness[i]))
            m_C[i] = Cn[i] - m_C0[i] * alpha;
        else
            m_C[i] = Cn[i];
    }
}

void Joint2D::ComputeDerivatives(PhysicsBody2D& body)
{
    // Compute the first and second derivatives for the desired body
    if (&body == m_pBodyA)
    {
        float2 r = rotate(m_pBodyA->GetPosition().z, m_RadiusA);
        m_J[0] = { 1.0f, 0.0f, -r.y };
        m_J[1] = { 0.0f, 1.0f, r.x };
        m_J[2] = { 0.0f, 0.0f, m_TorqueArm };
        m_H[0] = { float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, -r.x) };
        m_H[1] = { float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, -r.y) };
        m_H[2] = { float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0) };
    }
    else
    {
        float2 r = rotate(m_pBodyB->GetPosition().z, m_RadiusB);
        m_J[0] = { 1.0f, 0.0f, r.y };
        m_J[1] = { 0.0f, 1.0f, -r.x };
        m_J[2] = { 0.0f, 0.0f, -m_TorqueArm };
        m_H[0] = { float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, r.x) };
        m_H[1] = { float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, r.y) };
        m_H[2] = { float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0) };
    }
}

Spring2D::Spring2D(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB, float2 radiusA, float2 radiusB, float stiffness, float restLength) : Force2D(bodyA, bodyB)
{
    m_RadiusA = radiusA;
    m_RadiusB = radiusB;
    m_RestLength = restLength;
    
    m_Stiffness[0] = stiffness;
    if (m_RestLength < 0)
        m_RestLength = (transform(m_pBodyA->GetPosition(), m_RadiusA) - transform(m_pBodyB->GetPosition(), m_RadiusB)).length();
}

bool Spring2D::Initialize()
{
    return m_Stiffness[0] != 0;
}

void Spring2D::ComputeConstraint(float alpha)
{
    // Compute constraint function at current state C(x)
    m_C[0] = (transform(m_pBodyA->GetPosition(), m_RadiusA) - transform(m_pBodyB->GetPosition(), m_RadiusB)).length() - m_RestLength;
}

void Spring2D::ComputeDerivatives(PhysicsBody2D& body)
{
    // Compute the first and second derivatives for the desired body
    float2x2 S = { float2(0, -1), float2(1, 0) };
    float2x2 I = { float2(1, 0), float2(0, 1) };

    float2 d = transform(m_pBodyA->GetPosition(), m_RadiusA) - transform(m_pBodyB->GetPosition(), m_RadiusB);
    float dlen2 = float2::dotproduct(d, d);
    if (dlen2 == 0)
        return;

    float dlen = sqrtf(dlen2);
    float2 n = d / dlen;
    float2x2 dxx = (1.f / dlen) * (I - outer(n, n));

    if (&body == m_pBodyA)
    {
        float2 Sr = rotate(m_pBodyA->GetPosition().z, S * m_RadiusA);
        float2 r = rotate(m_pBodyA->GetPosition().z, m_RadiusA);

        float2 dxr = dxx * Sr;
        float drr = -float2::dotproduct(n, r) - float2::dotproduct(n, r);

        m_J[0] = float3(n.x, n.y, float2::dotproduct(n, Sr));
        m_H[0] = {
            float3(dxx.m00, dxx.m01, dxr.x),
            float3(dxx.m10, dxx.m11, dxr.y),
            float3(dxr.x,   dxr.y,   drr)
        };
    }
    else
    {
        float2 Sr = rotate(m_pBodyB->GetPosition().z, S * m_RadiusB);
        float2 r = rotate(m_pBodyB->GetPosition().z, m_RadiusB);

        float2 dxr = dxx * Sr;
        float drr = float2::dotproduct(n, r) + float2::dotproduct(n, r);

        m_J[0] = float3(-n.x, -n.y, float2::dotproduct(n, -1 * Sr));
        m_H[0] = {
            float3(dxx.m00, dxx.m01, dxr.x),
            float3(dxx.m10, dxx.m11, dxr.y),
            float3(dxr.x,   dxr.y,   drr)
        };
    }
}

ContactManifold2D::ContactManifold2D(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB) : Force2D(bodyA, bodyB)
{
    m_NumContacts = 0;
    m_WrapOffset  = float2(0.f, 0.f);
    m_Fmax[0] = m_Fmax[2] = 0.0f;
    m_Fmin[0] = m_Fmin[2] = -INFINITY;
}

bool ContactManifold2D::Initialize()
{
    // Compute friction
    m_Friction = sqrtf(m_pBodyA->GetFriction() * m_pBodyB->GetFriction());

    // Combined restitution: take the max so one bouncy body makes the pair bouncy.
    float restitution = max(m_pBodyA->GetRestitution(), m_pBodyB->GetRestitution());

    // Store previous contact state
    Contact2D oldContacts[2] = { m_Contacts[0], m_Contacts[1] };
    float oldm_Penalty[4] = { m_Penalty[0], m_Penalty[1], m_Penalty[2], m_Penalty[3] };
    float oldLambda[4] = { m_Lambda[0], m_Lambda[1], m_Lambda[2], m_Lambda[3] };
    bool oldStick[2] = { m_Contacts[0].stick, m_Contacts[1].stick };
    int oldNumContacts = m_NumContacts;

    // Compute the wrap offset for this pair: we want to measure bodyB relative
    // to the image of itself closest to bodyA under minimum-image wrapping.
    // Skip it entirely for pairs involving a static body — statics span the
    // whole world via their SDF, so their nominal origin position is not a
    // meaningful anchor for the minimum-image calculation.
    bool staticPair = m_pBodyA->getShapeView().Kind == PhysicsBody2D::ShapeKind::Static
                   || m_pBodyB->getShapeView().Kind == PhysicsBody2D::ShapeKind::Static;

    if (staticPair)
    {
        m_WrapOffset = float2(0.f, 0.f);
    }
    else
    {
        float2 posA2d = float2(m_pBodyA->GetPosition().x, m_pBodyA->GetPosition().y);
        float2 posB2d = float2(m_pBodyB->GetPosition().x, m_pBodyB->GetPosition().y);
        float2 dpRaw  = posA2d - posB2d;
        float2 dpImg  = Physics2DSolver::MinimumImage(dpRaw);
        m_WrapOffset  = dpRaw - dpImg;  // add to posB to reach its image nearest posA
    }

    // Compute new contacts
    m_NumContacts = Collide(*m_pBodyA, *m_pBodyB, m_Contacts, m_WrapOffset);

    // Merge old contact data with new contacts
    for (int i = 0; i < m_NumContacts; i++)
    {
        m_Penalty[i * 2 + 0] = m_Penalty[i * 2 + 1] = 0.0f;
        m_Lambda[i * 2 + 0] = m_Lambda[i * 2 + 1] = 0.0f;

        for (int j = 0; j < oldNumContacts; j++)
        {
            if (m_Contacts[i].feature.value == oldContacts[j].feature.value)
            {
                m_Penalty[i * 2 + 0] = oldm_Penalty[j * 2 + 0];
                m_Penalty[i * 2 + 1] = oldm_Penalty[j * 2 + 1];
                m_Lambda[i * 2 + 0] = oldLambda[j * 2 + 0];
                m_Lambda[i * 2 + 1] = oldLambda[j * 2 + 1];
                m_Contacts[i].stick = oldStick[j];

                // If static friction in last frame, use the old contact points
                if (oldStick[j])
                {
                    m_Contacts[i].rA = oldContacts[j].rA;
                    m_Contacts[i].rB = oldContacts[j].rB;
                }
            }
        }
    }

    for (int i = 0; i < m_NumContacts; i++)
    {
        // Compute the contact basis (Eq. 15)
        float2 normal = m_Contacts[i].normal;
        float2 tangent = { normal.y, -normal.x };
        float2x2 basis = {
            float2(normal.x, normal.y),
            float2(tangent.x, tangent.y)
        };

        float2 rAW = rotate(m_pBodyA->GetPosition().z, m_Contacts[i].rA);
        float2 rBW = rotate(m_pBodyB->GetPosition().z, m_Contacts[i].rB);

        // Precompute the constraint and derivatives at C(x-), since we use a truncated Taylor series for contacts (Sec 4).
        // Note that we discard the second order term, since it is insignificant for contacts
        m_Contacts[i].JAn = { basis.m00, basis.m01, cross(rAW, normal) };
        m_Contacts[i].JBn = { -basis.m00, -basis.m01, -cross(rBW, normal) };
        m_Contacts[i].JAt = { basis.m10, basis.m11, cross(rAW, tangent) };
        m_Contacts[i].JBt = { -basis.m10, -basis.m11, -cross(rBW, tangent) };

        float2 posA = float2(m_pBodyA->GetPosition().x, m_pBodyA->GetPosition().y);
        // Use bodyB's image closest to bodyA so the initial gap is measured
        // across any wrap seam rather than through the world.
        float2 posB = float2(m_pBodyB->GetPosition().x, m_pBodyB->GetPosition().y) + m_WrapOffset;

        m_Contacts[i].C0 = basis * (posA + rAW - posB - rBW) + float2{ COLLISION_MARGIN, 0 };

        // Restitution bias: want post-step relative normal velocity = -e * v_pre,
        // i.e. dp_rel_n = -e * v_rel_n_pre * dt. Adding this signed target to C as
        // +e*v_rel_n*dt (v_rel_n is negative on approach) makes the solver aim for
        // a bounce instead of zero closing velocity. Gated by a rest threshold to
        // avoid jitter on resting contacts.
        float v_rel_n = float3::dotproduct(m_Contacts[i].JAn, m_pBodyA->m_Velocity)
                      + float3::dotproduct(m_Contacts[i].JBn, m_pBodyB->m_Velocity);

        if (restitution > 0.0f && v_rel_n < -RESTITUTION_REST_THRESH)
            m_Contacts[i].restitutionBias = restitution * v_rel_n * Physics2DSolver::GetStepDt();
        else
            m_Contacts[i].restitutionBias = 0.0f;
    }

    return m_NumContacts > 0;
}

void ContactManifold2D::ComputeConstraint(float alpha)
{
    for (int i = 0; i < m_NumContacts; i++)
    {
        // Compute the Taylor series approximation of the constraint function C(x) (Sec 4)
        float3 dpA = m_pBodyA->m_Position - m_pBodyA->m_InitialPosition;
        float3 dpB = m_pBodyB->m_Position - m_pBodyB->m_InitialPosition;
        
        m_C[i * 2 + 0] = m_Contacts[i].C0.x * (1 - alpha) + m_Contacts[i].restitutionBias * alpha + float3::dotproduct(m_Contacts[i].JAn, dpA) + float3::dotproduct(m_Contacts[i].JBn, dpB);
        m_C[i * 2 + 1] = m_Contacts[i].C0.y * (1 - alpha) + float3::dotproduct(m_Contacts[i].JAt, dpA) + float3::dotproduct(m_Contacts[i].JBt, dpB);

        // Update the friction bounds using the latest lambda values
        float frictionBound = abs(m_Lambda[i * 2 + 0]) * m_Friction;
        m_Fmax[i * 2 + 1] = frictionBound;
        m_Fmin[i * 2 + 1] = -frictionBound;

        // Check if the contact is sticking, so that on the next frame we can use the old contact points for better static friction handling
        m_Contacts[i].stick = abs(m_Lambda[i * 2 + 1]) < frictionBound && abs(m_Contacts[i].C0.y) < STICK_THRESH;
    }
}

void ContactManifold2D::ComputeDerivatives(PhysicsBody2D& body)
{
    // Just store precomputed derivatives in J for the desired body
    for (int i = 0; i < m_NumContacts; i++)
    {
        if (&body == m_pBodyA)
        {
            m_J[i * 2 + 0] = m_Contacts[i].JAn;
            m_J[i * 2 + 1] = m_Contacts[i].JAt;
        }
        else
        {
            m_J[i * 2 + 0] = m_Contacts[i].JBn;
            m_J[i * 2 + 1] = m_Contacts[i].JBt;
        }
    }
}
