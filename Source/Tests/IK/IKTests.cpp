#include "TestUtils.h"

#include <Urho3D/IK/IKConstraint.h>
#include <Urho3D/IK/IKEffector.h>
#include <Urho3D/IK/IKSolver.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

struct ChainFixture
{
    ChainFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        root_ = scene_->CreateChild("Root");
        middle_ = root_->CreateChild("Middle");
        middle_->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
        tip_ = middle_->CreateChild("Tip");
        tip_->SetPosition(Vector3(0.0f, 1.0f, 0.0f));

        solver_ = root_->CreateComponent<IKSolver>();
        effector_ = tip_->CreateComponent<IKEffector>();
        effector_->SetChainLength(2);
    }

    SharedPtr<Scene> scene_;
    Node* root_{};
    Node* middle_{};
    Node* tip_{};
    IKSolver* solver_{};
    IKEffector* effector_{};
};

}

TEST_CASE("IKSolver settings round trip", "[IK]")
{
    ChainFixture fixture;
    IKSolver* solver = fixture.solver_;

    REQUIRE(solver->GetAlgorithm() == IKSolver::FABRIK);

    solver->SetAlgorithm(IKSolver::ONE_BONE);
    REQUIRE(solver->GetAlgorithm() == IKSolver::ONE_BONE);
    solver->SetAlgorithm(IKSolver::TWO_BONE);
    REQUIRE(solver->GetAlgorithm() == IKSolver::TWO_BONE);
    solver->SetAlgorithm(IKSolver::FABRIK);
    REQUIRE(solver->GetAlgorithm() == IKSolver::FABRIK);

    solver->SetMaximumIterations(50);
    REQUIRE(solver->GetMaximumIterations() == 50);

    solver->SetTolerance(0.01f);
    REQUIRE_NEAR(solver->GetTolerance(), 0.01f, 0.0001f);
    solver->SetTolerance(-1.0f);
    REQUIRE(solver->GetTolerance() > 0.0f);

    SECTION("every feature flag can be toggled independently")
    {
        const IKSolver::Feature features[] = {
            IKSolver::JOINT_ROTATIONS, IKSolver::TARGET_ROTATIONS, IKSolver::UPDATE_ORIGINAL_POSE,
            IKSolver::UPDATE_ACTIVE_POSE, IKSolver::USE_ORIGINAL_POSE, IKSolver::CONSTRAINTS,
            IKSolver::AUTO_SOLVE,
        };

        for (IKSolver::Feature feature : features)
        {
            const bool original = solver->GetFeature(feature);
            solver->SetFeature(feature, !original);
            REQUIRE(solver->GetFeature(feature) == !original);
            solver->SetFeature(feature, original);
            REQUIRE(solver->GetFeature(feature) == original);
        }
    }
}

TEST_CASE("IKEffector settings round trip", "[IK]")
{
    ChainFixture fixture;
    IKEffector* effector = fixture.effector_;

    REQUIRE(effector->GetChainLength() == 2);

    effector->SetTargetPosition(Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(effector->GetTargetPosition() == Vector3(1.0f, 2.0f, 3.0f));

    effector->SetTargetRotation(Quaternion(45.0f, Vector3::UP));
    REQUIRE(effector->GetTargetRotation().Equals(Quaternion(45.0f, Vector3::UP)));
    REQUIRE_NEAR(effector->GetTargetRotationEuler().y_, 45.0f, 0.01f);

    effector->SetTargetRotationEuler(Vector3(0.0f, 90.0f, 0.0f));
    REQUIRE(effector->GetTargetRotation().Equals(Quaternion(0.0f, 90.0f, 0.0f)));

    effector->SetWeight(2.0f);
    REQUIRE_NEAR(effector->GetWeight(), 1.0f, 0.001f);
    effector->SetWeight(-1.0f);
    REQUIRE_NEAR(effector->GetWeight(), 0.0f, 0.001f);
    effector->SetWeight(0.5f);
    REQUIRE_NEAR(effector->GetWeight(), 0.5f, 0.001f);

    effector->SetRotationWeight(2.0f);
    REQUIRE_NEAR(effector->GetRotationWeight(), 1.0f, 0.001f);

    effector->SetRotationDecay(2.0f);
    REQUIRE_NEAR(effector->GetRotationDecay(), 1.0f, 0.001f);

    SECTION("a target node can be named before it exists and resolved later")
    {
        effector->SetTargetName("Goal");
        REQUIRE(effector->GetTargetName() == "Goal");

        Node* goal = fixture.scene_->CreateChild("Goal");
        goal->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
        effector->SetTargetNode(goal);
        REQUIRE(effector->GetTargetNode() == goal);
        REQUIRE(effector->GetTargetName() == "Goal");

        effector->SetTargetNode(nullptr);
        REQUIRE(effector->GetTargetNode() == nullptr);
    }

    SECTION("the effector features toggle")
    {
        effector->SetFeature(IKEffector::WEIGHT_NLERP, true);
        REQUIRE(effector->GetFeature(IKEffector::WEIGHT_NLERP));
        effector->SetFeature(IKEffector::WEIGHT_NLERP, false);
        REQUIRE_FALSE(effector->GetFeature(IKEffector::WEIGHT_NLERP));

        effector->SetINHERIT_PARENT_ROTATION(true);
        REQUIRE(effector->GetINHERIT_PARENT_ROTATION());
    }
}

TEST_CASE("IKConstraint settings round trip", "[IK]")
{
    ChainFixture fixture;

    REQUIRE(fixture.middle_->CreateComponent<IKConstraint>() == nullptr);

    SharedPtr<IKConstraint> holder(new IKConstraint(HeadlessContext()));
    IKConstraint* constraint = holder;

    constraint->SetStiffness(0.5f);
    REQUIRE_NEAR(constraint->GetStiffness(), 0.5f, 0.001f);
    constraint->SetStiffness(-1.0f);
    REQUIRE(constraint->GetStiffness() >= 0.0f);
    constraint->SetStiffness(2.0f);
    REQUIRE(constraint->GetStiffness() <= 1.0f);

    constraint->SetStretchiness(0.25f);
    REQUIRE_NEAR(constraint->GetStretchiness(), 0.25f, 0.001f);
    constraint->SetStretchiness(-1.0f);
    REQUIRE(constraint->GetStretchiness() >= 0.0f);
    constraint->SetStretchiness(2.0f);
    REQUIRE(constraint->GetStretchiness() <= 1.0f);

    constraint->SetLengthConstraints(Vector2(0.5f, 2.0f));
    REQUIRE(constraint->GetLengthConstraints() == Vector2(0.5f, 2.0f));
}

TEST_CASE("IKSolver pulls a chain towards its target", "[IK]")
{
    ChainFixture fixture;
    fixture.solver_->RebuildChainTrees();
    fixture.solver_->RecalculateSegmentLengths();
    fixture.solver_->ApplySceneToOriginalPose();

    const Vector3 restTip = fixture.tip_->GetWorldPosition();
    REQUIRE(restTip.Equals(Vector3(0.0f, 2.0f, 0.0f)));

    const Vector3 target(2.0f, 0.0f, 0.0f);
    fixture.effector_->SetTargetPosition(target);
    fixture.effector_->SetWeight(1.0f);
    fixture.solver_->Solve();

    REQUIRE_NEAR((fixture.tip_->GetWorldPosition() - target).Length(), 0.0f, 0.05f);
    REQUIRE_FALSE(fixture.tip_->GetWorldPosition().Equals(restTip));

    SECTION("the chain cannot stretch past its own length")
    {
        fixture.effector_->SetTargetPosition(Vector3(100.0f, 0.0f, 0.0f));
        fixture.solver_->Solve();

        const float reach = (fixture.tip_->GetWorldPosition() - fixture.root_->GetWorldPosition()).Length();
        REQUIRE_NEAR(reach, 2.0f, 0.05f);
    }

    SECTION("a zero weight effector leaves the chain where it is")
    {
        fixture.solver_->ApplyOriginalPoseToScene();
        const Vector3 before = fixture.tip_->GetWorldPosition();

        fixture.effector_->SetWeight(0.0f);
        fixture.effector_->SetTargetPosition(Vector3(2.0f, 0.0f, 0.0f));
        fixture.solver_->Solve();

        REQUIRE_NEAR((fixture.tip_->GetWorldPosition() - before).Length(), 0.0f, 0.01f);
    }

    SECTION("the original pose can be restored after solving")
    {
        fixture.solver_->ApplyOriginalPoseToScene();
        REQUIRE_NEAR((fixture.tip_->GetWorldPosition() - restTip).Length(), 0.0f, 0.01f);
    }

    SECTION("a partial weight lands between the rest pose and the target")
    {
        fixture.solver_->ApplyOriginalPoseToScene();
        fixture.effector_->SetWeight(0.5f);
        fixture.effector_->SetTargetPosition(target);
        fixture.solver_->Solve();

        const Vector3 reached = fixture.tip_->GetWorldPosition();
        REQUIRE((reached - restTip).Length() > 0.01f);
        REQUIRE((reached - target).Length() > 0.01f);
    }
}
