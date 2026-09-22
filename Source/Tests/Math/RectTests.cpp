#include "TestUtils.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Math/Rect.h>

using namespace Urho3D;

TEST_CASE("Rect default construction is undefined", "[Math]")
{
    const Rect r;
    REQUIRE_FALSE(r.Defined());
    REQUIRE(r.min_ == Vector2(M_INFINITY, M_INFINITY));
    REQUIRE(r.max_ == Vector2(-M_INFINITY, -M_INFINITY));

    REQUIRE(Rect(Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f)).Defined());
    REQUIRE(Rect::ZERO.Defined());
}

TEST_CASE("Rect construction variants agree", "[Math]")
{
    const Rect fromCorners(Vector2(1.0f, 2.0f), Vector2(3.0f, 4.0f));
    REQUIRE(Rect(1.0f, 2.0f, 3.0f, 4.0f) == fromCorners);
    REQUIRE(Rect(Vector4(1.0f, 2.0f, 3.0f, 4.0f)) == fromCorners);

    const float data[] = {1.0f, 2.0f, 3.0f, 4.0f};
    REQUIRE(Rect(data) == fromCorners);

    Rect copy(fromCorners);
    REQUIRE(copy == fromCorners);

    Rect assigned;
    assigned = fromCorners;
    REQUIRE(assigned == fromCorners);
    REQUIRE(assigned != Rect::ZERO);

    REQUIRE(Rect::ZERO == Rect(0.0f, 0.0f, 0.0f, 0.0f));
    REQUIRE(Rect::POSITIVE == Rect(0.0f, 0.0f, 1.0f, 1.0f));
    REQUIRE(Rect::FULL == Rect(-1.0f, -1.0f, 1.0f, 1.0f));
}

TEST_CASE("Rect accessors", "[Math]")
{
    const Rect r(1.0f, 2.0f, 5.0f, 8.0f);

    REQUIRE(r.Min() == Vector2(1.0f, 2.0f));
    REQUIRE(r.Max() == Vector2(5.0f, 8.0f));
    REQUIRE_EQ_F(r.Left(), 1.0f);
    REQUIRE_EQ_F(r.Top(), 2.0f);
    REQUIRE_EQ_F(r.Right(), 5.0f);
    REQUIRE_EQ_F(r.Bottom(), 8.0f);

    REQUIRE(r.Size() == Vector2(4.0f, 6.0f));
    REQUIRE(r.HalfSize() == Vector2(2.0f, 3.0f));
    REQUIRE(r.Center() == Vector2(3.0f, 5.0f));

    REQUIRE(r.ToVector4() == Vector4(1.0f, 2.0f, 5.0f, 8.0f));
    REQUIRE(r.Data()[0] == 1.0f);
    REQUIRE(r.Equals(Rect(1.0f, 2.0f, 5.0f, 8.0f)));
    REQUIRE_FALSE(r.Equals(Rect::ZERO));
}

TEST_CASE("Rect arithmetic", "[Math]")
{
    const Rect a(1.0f, 1.0f, 2.0f, 2.0f);
    const Rect b(1.0f, 1.0f, 1.0f, 1.0f);

    REQUIRE(a + b == Rect(2.0f, 2.0f, 3.0f, 3.0f));
    REQUIRE(a - b == Rect(0.0f, 0.0f, 1.0f, 1.0f));
    REQUIRE(a * 2.0f == Rect(2.0f, 2.0f, 4.0f, 4.0f));
    REQUIRE(a / 2.0f == Rect(0.5f, 0.5f, 1.0f, 1.0f));

    Rect acc(1.0f, 1.0f, 2.0f, 2.0f);
    acc += b;
    REQUIRE(acc == Rect(2.0f, 2.0f, 3.0f, 3.0f));
    acc -= b;
    REQUIRE(acc == a);
    acc *= 2.0f;
    REQUIRE(acc == Rect(2.0f, 2.0f, 4.0f, 4.0f));
    acc /= 2.0f;
    REQUIRE(acc.Equals(a));
}

TEST_CASE("Rect Define, Merge and Clear", "[Math]")
{
    Rect r;
    r.Define(Vector2(1.0f, 1.0f));
    REQUIRE(r.Defined());
    REQUIRE(r.min_ == Vector2(1.0f, 1.0f));
    REQUIRE(r.max_ == Vector2(1.0f, 1.0f));

    r.Merge(Vector2(3.0f, 5.0f));
    REQUIRE(r == Rect(1.0f, 1.0f, 3.0f, 5.0f));

    r.Merge(Vector2(-2.0f, -4.0f));
    REQUIRE(r == Rect(-2.0f, -4.0f, 3.0f, 5.0f));

    SECTION("merging an interior point changes nothing")
    {
        const Rect before = r;
        r.Merge(Vector2(0.0f, 0.0f));
        REQUIRE(r == before);
    }

    SECTION("merging a rect grows to the union")
    {
        r.Merge(Rect(-10.0f, 0.0f, 0.0f, 10.0f));
        REQUIRE(r == Rect(-10.0f, -4.0f, 3.0f, 10.0f));
    }

    SECTION("Clear returns to the undefined state")
    {
        r.Clear();
        REQUIRE_FALSE(r.Defined());
    }

    SECTION("Define from another rect copies it")
    {
        Rect target;
        target.Define(Rect(1.0f, 2.0f, 3.0f, 4.0f));
        REQUIRE(target == Rect(1.0f, 2.0f, 3.0f, 4.0f));
    }

    SECTION("Define from corners")
    {
        Rect target;
        target.Define(Vector2(1.0f, 2.0f), Vector2(3.0f, 4.0f));
        REQUIRE(target == Rect(1.0f, 2.0f, 3.0f, 4.0f));
    }

    SECTION("merging into an undefined rect adopts the point")
    {
        Rect empty;
        empty.Merge(Vector2(7.0f, 8.0f));
        REQUIRE(empty == Rect(7.0f, 8.0f, 7.0f, 8.0f));
    }
}

TEST_CASE("Rect Clip", "[Math]")
{
    Rect r(0.0f, 0.0f, 10.0f, 10.0f);
    r.Clip(Rect(5.0f, 5.0f, 20.0f, 20.0f));
    REQUIRE(r == Rect(5.0f, 5.0f, 10.0f, 10.0f));

    SECTION("clipping against a fully containing rect is a no-op")
    {
        Rect inner(2.0f, 2.0f, 3.0f, 3.0f);
        inner.Clip(Rect(0.0f, 0.0f, 10.0f, 10.0f));
        REQUIRE(inner == Rect(2.0f, 2.0f, 3.0f, 3.0f));
    }

    SECTION("clipping against a disjoint rect degenerates the rect")
    {
        Rect disjoint(0.0f, 0.0f, 1.0f, 1.0f);
        disjoint.Clip(Rect(5.0f, 5.0f, 6.0f, 6.0f));
        REQUIRE(disjoint.Size().x_ <= 0.0f);
    }
}

TEST_CASE("Rect containment", "[Math]")
{
    const Rect r(0.0f, 0.0f, 10.0f, 10.0f);

    REQUIRE(r.IsInside(Vector2(5.0f, 5.0f)) == INSIDE);
    REQUIRE(r.IsInside(Vector2(0.0f, 0.0f)) == INSIDE);
    REQUIRE(r.IsInside(Vector2(10.0f, 10.0f)) == INSIDE);
    REQUIRE(r.IsInside(Vector2(-1.0f, 5.0f)) == OUTSIDE);
    REQUIRE(r.IsInside(Vector2(11.0f, 5.0f)) == OUTSIDE);
    REQUIRE(r.IsInside(Vector2(5.0f, -1.0f)) == OUTSIDE);
    REQUIRE(r.IsInside(Vector2(5.0f, 11.0f)) == OUTSIDE);

    REQUIRE(r.IsInside(Rect(2.0f, 2.0f, 8.0f, 8.0f)) == INSIDE);
    REQUIRE(r.IsInside(Rect(-5.0f, -5.0f, 5.0f, 5.0f)) == INTERSECTS);
    REQUIRE(r.IsInside(Rect(20.0f, 20.0f, 30.0f, 30.0f)) == OUTSIDE);
    REQUIRE(r.IsInside(r) == INSIDE);
}

TEST_CASE("Rect string round trip", "[Math]")
{
    const Rect r(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(r.ToString() == "1 2 3 4");
    REQUIRE(ToRect(r.ToString()) == r);
}

TEST_CASE("IntRect construction and accessors", "[Math]")
{
    REQUIRE(IntRect() == IntRect(0, 0, 0, 0));
    REQUIRE(IntRect::ZERO == IntRect(0, 0, 0, 0));

    const IntRect r(1, 2, 5, 8);
    REQUIRE(IntRect(IntVector2(1, 2), IntVector2(5, 8)) == r);

    const int data[] = {1, 2, 5, 8};
    REQUIRE(IntRect(data) == r);

    REQUIRE(r.Left() == 1);
    REQUIRE(r.Top() == 2);
    REQUIRE(r.Right() == 5);
    REQUIRE(r.Bottom() == 8);
    REQUIRE(r.Min() == IntVector2(1, 2));
    REQUIRE(r.Max() == IntVector2(5, 8));
    REQUIRE(r.Width() == 4);
    REQUIRE(r.Height() == 6);
    REQUIRE(r.Size() == IntVector2(4, 6));
    REQUIRE(r.Data()[0] == 1);
    REQUIRE(r != IntRect::ZERO);
}

TEST_CASE("IntRect arithmetic", "[Math]")
{
    const IntRect a(1, 1, 3, 3);
    const IntRect b(1, 1, 1, 1);

    REQUIRE(a + b == IntRect(2, 2, 4, 4));
    REQUIRE(a - b == IntRect(0, 0, 2, 2));
    REQUIRE(a * 2.0f == IntRect(2, 2, 6, 6));
    REQUIRE(a / 2.0f == IntRect(0, 0, 1, 1));

    IntRect acc(1, 1, 3, 3);
    acc += b;
    REQUIRE(acc == IntRect(2, 2, 4, 4));
    acc -= b;
    REQUIRE(acc == a);
    acc *= 2.0f;
    REQUIRE(acc == IntRect(2, 2, 6, 6));
    acc /= 2.0f;
    REQUIRE(acc == a);
}

TEST_CASE("IntRect containment uses a half open range", "[Math]")
{
    const IntRect r(0, 0, 10, 10);

    REQUIRE(r.IsInside(IntVector2(5, 5)) == INSIDE);
    REQUIRE(r.IsInside(IntVector2(0, 0)) == INSIDE);
    REQUIRE(r.IsInside(IntVector2(10, 10)) == OUTSIDE);
    REQUIRE(r.IsInside(IntVector2(9, 9)) == INSIDE);
    REQUIRE(r.IsInside(IntVector2(-1, 0)) == OUTSIDE);

    REQUIRE(r.IsInside(IntRect(2, 2, 8, 8)) == INSIDE);
    REQUIRE(r.IsInside(IntRect(-5, -5, 5, 5)) == INTERSECTS);
    REQUIRE(r.IsInside(IntRect(20, 20, 30, 30)) == OUTSIDE);
}

TEST_CASE("IntRect Clip and Merge", "[Math]")
{
    IntRect r(0, 0, 10, 10);
    r.Clip(IntRect(5, 5, 20, 20));
    REQUIRE(r == IntRect(5, 5, 10, 10));

    IntRect m(0, 0, 5, 5);
    m.Merge(IntRect(3, 3, 10, 10));
    REQUIRE(m == IntRect(0, 0, 10, 10));

    SECTION("a degenerate rect is replaced rather than unioned")
    {
        IntRect zero = IntRect::ZERO;
        zero.Merge(IntRect(1, 1, 2, 2));
        REQUIRE(zero == IntRect(1, 1, 2, 2));
    }

    SECTION("merging a degenerate rect into a real one is ignored")
    {
        IntRect real(0, 0, 5, 5);
        real.Merge(IntRect(10, 10, 10, 10));
        REQUIRE(real == IntRect(0, 0, 5, 5));
    }
}

TEST_CASE("IntRect string round trip", "[Math]")
{
    const IntRect r(1, 2, 3, 4);
    REQUIRE(r.ToString() == "1 2 3 4");
    REQUIRE(ToIntRect(r.ToString()) == r);
}

TEST_CASE("Rect Clip trims each side independently", "[Math]")
{
    Rect left(0.0f, 0.0f, 10.0f, 10.0f);
    left.Clip(Rect(2.0f, -5.0f, 20.0f, 20.0f));
    REQUIRE_EQ_F(left.min_.x_, 2.0f);

    Rect top(0.0f, 0.0f, 10.0f, 10.0f);
    top.Clip(Rect(-5.0f, 2.0f, 20.0f, 20.0f));
    REQUIRE_EQ_F(top.min_.y_, 2.0f);

    Rect right(0.0f, 0.0f, 10.0f, 10.0f);
    right.Clip(Rect(-5.0f, -5.0f, 8.0f, 20.0f));
    REQUIRE_EQ_F(right.max_.x_, 8.0f);

    Rect bottom(0.0f, 0.0f, 10.0f, 10.0f);
    bottom.Clip(Rect(-5.0f, -5.0f, 20.0f, 8.0f));
    REQUIRE_EQ_F(bottom.max_.y_, 8.0f);
}

TEST_CASE("IntRect Clip trims each side independently", "[Math]")
{
    IntRect left(0, 0, 10, 10);
    left.Clip(IntRect(2, -5, 20, 20));
    REQUIRE(left.left_ == 2);

    IntRect top(0, 0, 10, 10);
    top.Clip(IntRect(-5, 2, 20, 20));
    REQUIRE(top.top_ == 2);

    IntRect right(0, 0, 10, 10);
    right.Clip(IntRect(-5, -5, 8, 20));
    REQUIRE(right.right_ == 8);

    IntRect bottom(0, 0, 10, 10);
    bottom.Clip(IntRect(-5, -5, 20, 8));
    REQUIRE(bottom.bottom_ == 8);

    SECTION("clipping to a disjoint rect collapses to an empty rect")
    {
        IntRect disjoint(0, 0, 5, 5);
        disjoint.Clip(IntRect(10, 10, 20, 20));
        REQUIRE(disjoint == IntRect());
    }
}

TEST_CASE("IntRect Merge grows each side independently", "[Math]")
{
    IntRect box(5, 5, 10, 10);

    box.Merge(IntRect(0, 6, 9, 9));
    REQUIRE(box.left_ == 0);

    box.Merge(IntRect(6, 0, 9, 9));
    REQUIRE(box.top_ == 0);

    box.Merge(IntRect(6, 6, 20, 9));
    REQUIRE(box.right_ == 20);

    box.Merge(IntRect(6, 6, 9, 20));
    REQUIRE(box.bottom_ == 20);
}

TEST_CASE("Rect Merge grows each side independently", "[Math]")
{
    Rect box(5.0f, 5.0f, 10.0f, 10.0f);

    box.Merge(Rect(0.0f, 6.0f, 9.0f, 9.0f));
    REQUIRE_EQ_F(box.min_.x_, 0.0f);

    box.Merge(Rect(6.0f, 0.0f, 9.0f, 9.0f));
    REQUIRE_EQ_F(box.min_.y_, 0.0f);

    box.Merge(Rect(6.0f, 6.0f, 20.0f, 9.0f));
    REQUIRE_EQ_F(box.max_.x_, 20.0f);

    box.Merge(Rect(6.0f, 6.0f, 9.0f, 20.0f));
    REQUIRE_EQ_F(box.max_.y_, 20.0f);
}
