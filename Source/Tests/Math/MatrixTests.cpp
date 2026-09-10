#include "TestUtils.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Math/Matrix2.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Matrix4.h>

using namespace Urho3D;

namespace
{

const float kTolerance = 0.001f;

bool NearlyEqual(const Vector3& lhs, const Vector3& rhs, float eps = kTolerance)
{
    return Abs(lhs.x_ - rhs.x_) <= eps && Abs(lhs.y_ - rhs.y_) <= eps && Abs(lhs.z_ - rhs.z_) <= eps;
}

template <class T> bool NearlyEqual(const T& lhs, const T& rhs, unsigned rows, unsigned columns, float eps = kTolerance)
{
    for (unsigned i = 0; i < rows; ++i)
    {
        for (unsigned j = 0; j < columns; ++j)
        {
            if (Abs(lhs.Element(i, j) - rhs.Element(i, j)) > eps)
                return false;
        }
    }
    return true;
}

}

TEST_CASE("Matrix2 basics", "[Math]")
{
    REQUIRE(Matrix2() == Matrix2::IDENTITY);
    REQUIRE(Matrix2::IDENTITY == Matrix2(1.0f, 0.0f, 0.0f, 1.0f));
    REQUIRE(Matrix2::ZERO == Matrix2(0.0f, 0.0f, 0.0f, 0.0f));

    const Matrix2 m(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(m.m00_ == 1.0f);
    REQUIRE(m.m01_ == 2.0f);
    REQUIRE(m.m10_ == 3.0f);
    REQUIRE(m.m11_ == 4.0f);

    const Matrix2 copy(m);
    REQUIRE(copy == m);

    Matrix2 assigned;
    assigned = m;
    REQUIRE(assigned == m);
    REQUIRE(assigned != Matrix2::IDENTITY);

    REQUIRE(m.Data()[0] == 1.0f);
    REQUIRE(m.Equals(Matrix2(1.0f, 2.0f, 3.0f, 4.0f)));
    REQUIRE_FALSE(m.IsNaN());
    REQUIRE_FALSE(m.IsInf());
    REQUIRE(m.ToString() == "1 2 3 4");
}

TEST_CASE("Matrix2 algebra", "[Math]")
{
    const Matrix2 m(1.0f, 2.0f, 3.0f, 4.0f);

    REQUIRE(Matrix2::IDENTITY * Vector2(3.0f, 4.0f) == Vector2(3.0f, 4.0f));
    REQUIRE(m * Vector2(1.0f, 0.0f) == Vector2(1.0f, 3.0f));

    REQUIRE(m + Matrix2::ZERO == m);
    REQUIRE((m - m) == Matrix2::ZERO);
    REQUIRE(m * 2.0f == Matrix2(2.0f, 4.0f, 6.0f, 8.0f));
    REQUIRE(Matrix2::IDENTITY * m == m);

    REQUIRE(m.Transpose() == Matrix2(1.0f, 3.0f, 2.0f, 4.0f));
    REQUIRE(m.Transpose().Transpose() == m);

    SECTION("the inverse undoes the matrix")
    {
        REQUIRE((m * m.Inverse()).Equals(Matrix2::IDENTITY));
        REQUIRE(Matrix2::IDENTITY.Inverse() == Matrix2::IDENTITY);
    }

    SECTION("scale round trip")
    {
        Matrix2 scaled;
        scaled.SetScale(Vector2(2.0f, 3.0f));
        REQUIRE(scaled.Scale().Equals(Vector2(2.0f, 3.0f)));

        Matrix2 uniform;
        uniform.SetScale(4.0f);
        REQUIRE(uniform.Scale().Equals(Vector2(4.0f, 4.0f)));

        REQUIRE(Matrix2::IDENTITY.Scaled(Vector2(2.0f, 3.0f)).Scale().Equals(Vector2(2.0f, 3.0f)));
    }
}

TEST_CASE("Matrix3 basics", "[Math]")
{
    REQUIRE(Matrix3() == Matrix3::IDENTITY);
    REQUIRE(Matrix3::ZERO.Element(0, 0) == 0.0f);

    const Matrix3 m(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);

    REQUIRE(m.Element(0, 0) == 1.0f);
    REQUIRE(m.Element(1, 2) == 6.0f);
    REQUIRE(m.Element(2, 1) == 8.0f);

    REQUIRE(m.Row(0) == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(m.Row(2) == Vector3(7.0f, 8.0f, 9.0f));
    REQUIRE(m.Column(0) == Vector3(1.0f, 4.0f, 7.0f));
    REQUIRE(m.Column(2) == Vector3(3.0f, 6.0f, 9.0f));

    REQUIRE(m.Transpose().Row(0) == m.Column(0));
    REQUIRE(m.Transpose().Transpose() == m);

    REQUIRE(m.Equals(Matrix3(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f)));
    REQUIRE_FALSE(m.IsNaN());
    REQUIRE_FALSE(m.IsInf());
    REQUIRE(m.ToString() == "1 2 3 4 5 6 7 8 9");
    REQUIRE(ToMatrix3(m.ToString()).Equals(m));

    Matrix3 assigned;
    assigned = m;
    REQUIRE(assigned == m);
    REQUIRE(assigned != Matrix3::IDENTITY);
}

TEST_CASE("Matrix3 algebra", "[Math]")
{
    const Matrix3 m(
        2.0f, 0.0f, 0.0f,
        0.0f, 3.0f, 0.0f,
        0.0f, 0.0f, 4.0f);

    REQUIRE(m * Vector3(1.0f, 1.0f, 1.0f) == Vector3(2.0f, 3.0f, 4.0f));
    REQUIRE(Matrix3::IDENTITY * Vector3(1.0f, 2.0f, 3.0f) == Vector3(1.0f, 2.0f, 3.0f));

    REQUIRE(m + Matrix3::ZERO == m);
    REQUIRE((m - m) == Matrix3::ZERO);
    REQUIRE((m * 2.0f).Element(0, 0) == 4.0f);
    REQUIRE((Matrix3::IDENTITY * m).Equals(m));

    SECTION("the inverse undoes the matrix")
    {
        REQUIRE((m * m.Inverse()).Equals(Matrix3::IDENTITY));
        REQUIRE(Matrix3::IDENTITY.Inverse().Equals(Matrix3::IDENTITY));
    }

    SECTION("scale round trip")
    {
        REQUIRE(m.Scale().Equals(Vector3(2.0f, 3.0f, 4.0f)));

        Matrix3 scaled;
        scaled.SetScale(Vector3(5.0f, 6.0f, 7.0f));
        REQUIRE(scaled.Scale().Equals(Vector3(5.0f, 6.0f, 7.0f)));

        Matrix3 uniform;
        uniform.SetScale(3.0f);
        REQUIRE(uniform.Scale().Equals(Vector3(3.0f, 3.0f, 3.0f)));

        REQUIRE(Matrix3::IDENTITY.Scaled(Vector3(1.0f, 2.0f, 3.0f)).Scale().Equals(Vector3(1.0f, 2.0f, 3.0f)));
    }

    SECTION("a rotation matrix preserves length")
    {
        const Matrix3 rotation = Quaternion(37.0f, Vector3(1.0f, 2.0f, 3.0f)).RotationMatrix();
        const Vector3 v(1.0f, 2.0f, 3.0f);
        REQUIRE_NEAR((rotation * v).Length(), v.Length(), kTolerance);
        REQUIRE(rotation.Scale().Equals(Vector3::ONE));
    }

    SECTION("SignedScale recovers a negative scale that Scale cannot")
    {
        Matrix3 mirrored;
        mirrored.SetScale(Vector3(1.0f, -2.0f, 3.0f));
        REQUIRE(mirrored.Scale().Equals(Vector3(1.0f, 2.0f, 3.0f)));
        REQUIRE(mirrored.SignedScale(Matrix3::IDENTITY).Equals(Vector3(1.0f, -2.0f, 3.0f)));
    }
}

TEST_CASE("Matrix3x4 construction from translation, rotation and scale", "[Math]")
{
    REQUIRE(Matrix3x4() == Matrix3x4::IDENTITY);

    const Vector3 translation(1.0f, 2.0f, 3.0f);
    const Quaternion rotation(45.0f, Vector3::UP);
    const Vector3 scale(2.0f, 2.0f, 2.0f);

    const Matrix3x4 m(translation, rotation, scale);

    REQUIRE(m.Translation().Equals(translation));
    REQUIRE(m.Scale().Equals(scale));

    SECTION("Decompose returns what was composed")
    {
        Vector3 outTranslation;
        Quaternion outRotation;
        Vector3 outScale;
        m.Decompose(outTranslation, outRotation, outScale);

        REQUIRE(outTranslation.Equals(translation));
        REQUIRE(outScale.Equals(scale));
        REQUIRE(outRotation.Equals(rotation));
    }

    SECTION("uniform scale overload matches the vector overload")
    {
        REQUIRE(Matrix3x4(translation, rotation, 2.0f).Equals(m));
    }

    SECTION("the rotation can be read back")
    {
        REQUIRE(m.Rotation().Equals(rotation));
        REQUIRE(m.RotationMatrix().Equals(rotation.RotationMatrix()));
    }
}

TEST_CASE("Matrix3x4 transforms points", "[Math]")
{
    SECTION("translation only")
    {
        const Matrix3x4 m(Vector3(1.0f, 2.0f, 3.0f), Quaternion::IDENTITY, 1.0f);
        REQUIRE((m * Vector3::ZERO).Equals(Vector3(1.0f, 2.0f, 3.0f)));
    }

    SECTION("a direction ignores translation when w is zero")
    {
        const Matrix3x4 m(Vector3(10.0f, 0.0f, 0.0f), Quaternion::IDENTITY, 1.0f);
        REQUIRE((m * Vector4(1.0f, 0.0f, 0.0f, 0.0f)).Equals(Vector3(1.0f, 0.0f, 0.0f)));
        REQUIRE((m * Vector4(1.0f, 0.0f, 0.0f, 1.0f)).Equals(Vector3(11.0f, 0.0f, 0.0f)));
    }

    SECTION("rotation then translation applies in the expected order")
    {
        const Matrix3x4 m(Vector3(0.0f, 0.0f, 10.0f), Quaternion(90.0f, Vector3::UP), 1.0f);
        REQUIRE((m * Vector3::FORWARD).Equals(Vector3(1.0f, 0.0f, 10.0f)));
    }

    SECTION("the inverse undoes the transform")
    {
        const Matrix3x4 m(Vector3(1.0f, 2.0f, 3.0f), Quaternion(30.0f, Vector3::RIGHT), 2.0f);
        const Vector3 point(4.0f, 5.0f, 6.0f);
        REQUIRE(NearlyEqual(m.Inverse() * (m * point), point));
        REQUIRE(NearlyEqual(m * m.Inverse(), Matrix3x4::IDENTITY, 3, 4));
    }

    SECTION("composition matches sequential application")
    {
        const Matrix3x4 a(Vector3(1.0f, 0.0f, 0.0f), Quaternion(30.0f, Vector3::UP), 1.0f);
        const Matrix3x4 b(Vector3(0.0f, 2.0f, 0.0f), Quaternion(45.0f, Vector3::RIGHT), 1.0f);
        const Vector3 point(1.0f, 1.0f, 1.0f);
        REQUIRE(((a * b) * point).Equals(a * (b * point)));
    }
}

TEST_CASE("Matrix3x4 setters and accessors", "[Math]")
{
    Matrix3x4 m = Matrix3x4::IDENTITY;

    m.SetTranslation(Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(m.Translation().Equals(Vector3(1.0f, 2.0f, 3.0f)));

    m.SetScale(Vector3(2.0f, 3.0f, 4.0f));
    REQUIRE(m.Scale().Equals(Vector3(2.0f, 3.0f, 4.0f)));
    REQUIRE(m.Translation().Equals(Vector3(1.0f, 2.0f, 3.0f)));

    m.SetScale(5.0f);
    REQUIRE(m.Scale().Equals(Vector3(5.0f, 5.0f, 5.0f)));

    const Matrix3 rotation = Quaternion(60.0f, Vector3::UP).RotationMatrix();
    m.SetRotation(rotation);
    REQUIRE(m.RotationMatrix().Equals(rotation));
    REQUIRE(m.Translation().Equals(Vector3(1.0f, 2.0f, 3.0f)));

    REQUIRE(m.Element(0, 3) == 1.0f);
    REQUIRE(m.Row(0).w_ == 1.0f);
    REQUIRE(m.Column(3).Equals(Vector3(1.0f, 2.0f, 3.0f)));

    REQUIRE_FALSE(m.IsNaN());
    REQUIRE_FALSE(m.IsInf());
    REQUIRE(m.Data()[0] == m.Element(0, 0));

    REQUIRE(Matrix3x4::IDENTITY.ToString() == "1 0 0 0 0 1 0 0 0 0 1 0");
    REQUIRE(ToMatrix3x4(Matrix3x4::IDENTITY.ToString()).Equals(Matrix3x4::IDENTITY));
}

TEST_CASE("Matrix3x4 converts to Matrix4", "[Math]")
{
    const Matrix3x4 affine(Vector3(1.0f, 2.0f, 3.0f), Quaternion(30.0f, Vector3::UP), 2.0f);
    const Matrix4 full = affine.ToMatrix4();

    const Vector3 point(4.0f, 5.0f, 6.0f);
    REQUIRE((full * point).Equals(affine * point));

    REQUIRE(full.Element(3, 0) == 0.0f);
    REQUIRE(full.Element(3, 3) == 1.0f);

    SECTION("Matrix3x4 can be built back from a Matrix4")
    {
        REQUIRE(Matrix3x4(full).Equals(affine));
    }
}

TEST_CASE("Matrix4 basics", "[Math]")
{
    REQUIRE(Matrix4() == Matrix4::IDENTITY);
    REQUIRE(Matrix4::ZERO.Element(0, 0) == 0.0f);

    Matrix4 m = Matrix4::IDENTITY;
    m.SetTranslation(Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(m.Translation().Equals(Vector3(1.0f, 2.0f, 3.0f)));

    m.SetScale(Vector3(2.0f, 3.0f, 4.0f));
    REQUIRE(m.Scale().Equals(Vector3(2.0f, 3.0f, 4.0f)));

    m.SetScale(2.0f);
    REQUIRE(m.Scale().Equals(Vector3(2.0f, 2.0f, 2.0f)));

    const Matrix3 rotation = Quaternion(45.0f, Vector3::RIGHT).RotationMatrix();
    m.SetRotation(rotation);
    REQUIRE(m.RotationMatrix().Equals(rotation));
    REQUIRE(m.Rotation().Equals(Quaternion(45.0f, Vector3::RIGHT)));

    REQUIRE(m.Row(3) == Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    REQUIRE(m.Column(3).Equals(Vector4(1.0f, 2.0f, 3.0f, 1.0f)));

    REQUIRE_FALSE(m.IsNaN());
    REQUIRE_FALSE(m.IsInf());
    REQUIRE(m.Data()[0] == m.Element(0, 0));

    Matrix4 assigned;
    assigned = m;
    REQUIRE(assigned == m);
    REQUIRE(assigned != Matrix4::ZERO);

    REQUIRE(Matrix4::IDENTITY.ToString() == "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
    REQUIRE(ToMatrix4(Matrix4::IDENTITY.ToString()).Equals(Matrix4::IDENTITY));
}

TEST_CASE("Matrix4 algebra", "[Math]")
{
    const Matrix4 m = Matrix3x4(Vector3(1.0f, 2.0f, 3.0f), Quaternion(30.0f, Vector3::UP), 2.0f).ToMatrix4();

    REQUIRE((m + Matrix4::ZERO).Equals(m));
    REQUIRE((m - m).Equals(Matrix4::ZERO));
    REQUIRE((Matrix4::IDENTITY * m).Equals(m));
    REQUIRE((m * Matrix4::IDENTITY).Equals(m));
    REQUIRE((m * 2.0f).Element(0, 0) == m.Element(0, 0) * 2.0f);

    REQUIRE(m.Transpose().Transpose().Equals(m));
    REQUIRE(m.Transpose().Row(0) == m.Column(0));

    SECTION("the inverse undoes the matrix")
    {
        REQUIRE(NearlyEqual(m * m.Inverse(), Matrix4::IDENTITY, 4, 4));
        const Vector3 point(4.0f, 5.0f, 6.0f);
        REQUIRE(NearlyEqual(m.Inverse() * (m * point), point));
    }

    SECTION("Decompose returns what was composed")
    {
        Vector3 translation;
        Quaternion rotation;
        Vector3 scale;
        m.Decompose(translation, rotation, scale);
        REQUIRE(translation.Equals(Vector3(1.0f, 2.0f, 3.0f)));
        REQUIRE(scale.Equals(Vector3(2.0f, 2.0f, 2.0f)));
        REQUIRE(rotation.Equals(Quaternion(30.0f, Vector3::UP)));
    }

    SECTION("a Vector4 transform keeps the w component")
    {
        REQUIRE((Matrix4::IDENTITY * Vector4(1.0f, 2.0f, 3.0f, 4.0f)) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    }

    SECTION("multiplying a Matrix4 by a Matrix3x4 matches the promoted product")
    {
        const Matrix3x4 affine(Vector3(0.0f, 1.0f, 0.0f), Quaternion(15.0f, Vector3::RIGHT), 1.0f);
        REQUIRE((m * affine).Equals(m * affine.ToMatrix4()));
    }
}

TEST_CASE("Matrix4 SignedScale", "[Math]")
{
    Matrix4 mirrored = Matrix4::IDENTITY;
    mirrored.SetScale(Vector3(1.0f, -2.0f, 3.0f));

    REQUIRE(mirrored.Scale().Equals(Vector3(1.0f, 2.0f, 3.0f)));
    REQUIRE(mirrored.SignedScale(Matrix3::IDENTITY).Equals(Vector3(1.0f, -2.0f, 3.0f)));
}

TEST_CASE("Matrix Equals rejects differing elements", "[Math]")
{
    REQUIRE_FALSE(Matrix2::IDENTITY.Equals(Matrix2::ZERO));
    REQUIRE_FALSE(Matrix3::IDENTITY.Equals(Matrix3::ZERO));
    REQUIRE_FALSE(Matrix3x4::IDENTITY.Equals(Matrix3x4::ZERO));
    REQUIRE_FALSE(Matrix4::IDENTITY.Equals(Matrix4::ZERO));

    Matrix4 almost = Matrix4::IDENTITY;
    almost.m23_ = 1.0f;
    REQUIRE_FALSE(almost.Equals(Matrix4::IDENTITY));
}

TEST_CASE("Matrix IsNaN and IsInf detect a poisoned element", "[Math]")
{
    const float nan = M_INFINITY - M_INFINITY;

    Matrix2 nanMatrix2 = Matrix2::IDENTITY;
    nanMatrix2.m11_ = nan;
    REQUIRE(nanMatrix2.IsNaN());
    REQUIRE_FALSE(nanMatrix2.IsInf());

    Matrix2 infMatrix2 = Matrix2::IDENTITY;
    infMatrix2.m11_ = M_INFINITY;
    REQUIRE(infMatrix2.IsInf());

    Matrix3 nanMatrix3 = Matrix3::IDENTITY;
    nanMatrix3.m22_ = nan;
    REQUIRE(nanMatrix3.IsNaN());
    Matrix3 infMatrix3 = Matrix3::IDENTITY;
    infMatrix3.m22_ = M_INFINITY;
    REQUIRE(infMatrix3.IsInf());

    Matrix3x4 nanMatrix3x4 = Matrix3x4::IDENTITY;
    nanMatrix3x4.m23_ = nan;
    REQUIRE(nanMatrix3x4.IsNaN());
    Matrix3x4 infMatrix3x4 = Matrix3x4::IDENTITY;
    infMatrix3x4.m23_ = M_INFINITY;
    REQUIRE(infMatrix3x4.IsInf());

    Matrix4 nanMatrix4 = Matrix4::IDENTITY;
    nanMatrix4.m33_ = nan;
    REQUIRE(nanMatrix4.IsNaN());
    Matrix4 infMatrix4 = Matrix4::IDENTITY;
    infMatrix4.m33_ = M_INFINITY;
    REQUIRE(infMatrix4.IsInf());
}

TEST_CASE("Matrix3x4 element wise arithmetic", "[Math]")
{
    const Matrix3x4 a(
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f);

    const Matrix3x4 sum = a + a;
    REQUIRE_NEAR(sum.Element(0, 0), 2.0f, kTolerance);
    REQUIRE_NEAR(sum.Element(2, 3), 24.0f, kTolerance);

    const Matrix3x4 difference = sum - a;
    REQUIRE(NearlyEqual(difference, a, 3, 4));

    const Matrix3x4 scaled = a * 2.0f;
    REQUIRE(NearlyEqual(scaled, sum, 3, 4));

    REQUIRE(NearlyEqual(a + Matrix3x4::ZERO, a, 3, 4));
}

TEST_CASE("Matrix3x4 from a Matrix3 zeroes the translation", "[Math]")
{
    const Matrix3 rotation = Quaternion(45.0f, Vector3::UP).RotationMatrix();
    const Matrix3x4 affine(rotation);

    REQUIRE(affine.Translation().Equals(Vector3::ZERO));
    REQUIRE(affine.RotationMatrix().Equals(rotation));
    REQUIRE((affine * Vector3::FORWARD).Equals(rotation * Vector3::FORWARD));
}

TEST_CASE("Matrix3x4 multiplied by a Matrix4", "[Math]")
{
    const Matrix3x4 affine(Vector3(1.0f, 2.0f, 3.0f), Quaternion(30.0f, Vector3::UP), 2.0f);
    const Matrix4 full = Matrix3x4(Vector3(0.0f, 1.0f, 0.0f), Quaternion(15.0f, Vector3::RIGHT), 1.0f).ToMatrix4();

    const Matrix4 product = affine * full;
    REQUIRE(NearlyEqual(product, affine.ToMatrix4() * full, 4, 4));

    REQUIRE_NEAR(product.Element(3, 3), full.Element(3, 3), kTolerance);
}

TEST_CASE("Matrix3x4 SignedScale recovers a mirrored scale", "[Math]")
{
    Matrix3x4 mirrored = Matrix3x4::IDENTITY;
    mirrored.SetScale(Vector3(1.0f, -2.0f, 3.0f));

    REQUIRE(mirrored.Scale().Equals(Vector3(1.0f, 2.0f, 3.0f)));
    REQUIRE(mirrored.SignedScale(Matrix3::IDENTITY).Equals(Vector3(1.0f, -2.0f, 3.0f)));
}
