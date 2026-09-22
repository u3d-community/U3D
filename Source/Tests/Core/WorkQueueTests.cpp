#include "TestUtils.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/WorkQueue.h>

#include <atomic>

using namespace Urho3D;

namespace
{

std::atomic<int> g_counter{0};

void Increment(const WorkItem* item, unsigned threadIndex)
{
    ++g_counter;
}

PODVector<unsigned> g_order;

void RecordPriority(const WorkItem* item, unsigned threadIndex)
{
    g_order.Push(item->priority_);
}

void AddRange(const WorkItem* item, unsigned threadIndex)
{
    auto* start = static_cast<int*>(item->start_);
    auto* end = static_cast<int*>(item->end_);
    for (int* i = start; i != end; ++i)
        *i += 1;
}

class EventCounter : public Object
{
    URHO3D_OBJECT(EventCounter, Object);

public:
    explicit EventCounter(Context* context) : Object(context), completions_(0) {}

    void Listen() { SubscribeToEvent(E_WORKITEMCOMPLETED, URHO3D_HANDLER(EventCounter, HandleCompleted)); }

    void HandleCompleted(StringHash eventType, VariantMap& eventData) { ++completions_; }

    int completions_;
};

}

TEST_CASE("WorkQueue runs items on the main thread when it has no workers", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<WorkQueue> queue(new WorkQueue(context));

    REQUIRE(queue->GetNumThreads() == 0);

    g_counter = 0;
    SharedPtr<WorkItem> item = queue->GetFreeItem();
    REQUIRE(item.NotNull());
    item->workFunction_ = Increment;
    item->priority_ = 0;
    item->sendEvent_ = false;

    queue->AddWorkItem(item);
    queue->Complete(0);

    REQUIRE(g_counter == 1);
    REQUIRE(queue->IsCompleted(0));
    REQUIRE_FALSE(queue->IsCompleting());
}

TEST_CASE("WorkQueue processes a batch of items", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<WorkQueue> queue(new WorkQueue(context));

    g_counter = 0;
    for (int i = 0; i < 16; ++i)
    {
        SharedPtr<WorkItem> item = queue->GetFreeItem();
        item->workFunction_ = Increment;
        item->priority_ = 0;
        queue->AddWorkItem(item);
    }

    queue->Complete(0);
    REQUIRE(g_counter == 16);

    SECTION("items operating on a data range each see their own slice")
    {
        PODVector<int> values;
        values.Resize(64);
        for (unsigned i = 0; i < values.Size(); ++i)
            values[i] = 0;

        for (unsigned chunk = 0; chunk < 4; ++chunk)
        {
            SharedPtr<WorkItem> item = queue->GetFreeItem();
            item->workFunction_ = AddRange;
            item->start_ = values.Buffer() + chunk * 16;
            item->end_ = values.Buffer() + (chunk + 1) * 16;
            queue->AddWorkItem(item);
        }

        queue->Complete(0);
        for (unsigned i = 0; i < values.Size(); ++i)
            REQUIRE(values[i] == 1);
    }
}

TEST_CASE("WorkQueue pools and reuses items", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<WorkQueue> queue(new WorkQueue(context));

    g_counter = 0;
    SharedPtr<WorkItem> first = queue->GetFreeItem();
    first->workFunction_ = Increment;
    queue->AddWorkItem(first);
    queue->Complete(0);

    REQUIRE(g_counter == 1);

    REQUIRE_FALSE(first->completed_);
    REQUIRE(first->workFunction_ == nullptr);
    REQUIRE(first->priority_ == M_MAX_UNSIGNED);
    first.Reset();

    SharedPtr<WorkItem> second = queue->GetFreeItem();
    REQUIRE(second.NotNull());
    REQUIRE_FALSE(second->completed_);

    SECTION("a reused item runs again cleanly")
    {
        g_counter = 0;
        second->workFunction_ = Increment;
        queue->AddWorkItem(second);
        queue->Complete(0);
        REQUIRE(g_counter == 1);
    }
}

TEST_CASE("WorkQueue removes queued items", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<WorkQueue> queue(new WorkQueue(context));

    g_counter = 0;

    SharedPtr<WorkItem> item = queue->GetFreeItem();
    item->workFunction_ = Increment;
    queue->AddWorkItem(item);

    REQUIRE(queue->RemoveWorkItem(item));
    queue->Complete(0);
    REQUIRE(g_counter == 0);

    SECTION("removing an item that was never queued reports failure")
    {
        SharedPtr<WorkItem> stray(new WorkItem());
        REQUIRE_FALSE(queue->RemoveWorkItem(stray));
    }

    SECTION("a batch of items can be removed at once")
    {
        Vector<SharedPtr<WorkItem> > batch;
        for (int i = 0; i < 4; ++i)
        {
            SharedPtr<WorkItem> queued = queue->GetFreeItem();
            queued->workFunction_ = Increment;
            queue->AddWorkItem(queued);
            batch.Push(queued);
        }

        REQUIRE(queue->RemoveWorkItems(batch) == 4);
        queue->Complete(0);
        REQUIRE(g_counter == 0);
    }
}

TEST_CASE("WorkQueue priorities and settings", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<WorkQueue> queue(new WorkQueue(context));

    queue->SetTolerance(50);
    REQUIRE(queue->GetTolerance() == 50);

    queue->SetNonThreadedWorkMs(10);
    REQUIRE(queue->GetNonThreadedWorkMs() == 10);

    SECTION("the non threaded budget has a floor of one millisecond")
    {
        queue->SetNonThreadedWorkMs(0);
        REQUIRE(queue->GetNonThreadedWorkMs() == 1);
        queue->SetNonThreadedWorkMs(-5);
        REQUIRE(queue->GetNonThreadedWorkMs() == 1);
    }

    SECTION("higher priority items run first")
    {
        g_order.Clear();

        SharedPtr<WorkItem> low = queue->GetFreeItem();
        low->workFunction_ = RecordPriority;
        low->priority_ = 0;

        SharedPtr<WorkItem> high = queue->GetFreeItem();
        high->workFunction_ = RecordPriority;
        high->priority_ = 10;

        queue->AddWorkItem(low);
        queue->AddWorkItem(high);
        queue->Complete(0);

        REQUIRE(g_order.Size() == 2);
        REQUIRE(g_order[0] == 10);
        REQUIRE(g_order[1] == 0);
        REQUIRE(queue->IsCompleted(0));
    }

    SECTION("an empty queue reports itself complete")
    {
        REQUIRE(queue->IsCompleted(0));
        REQUIRE(queue->IsCompleted(100));
    }
}

TEST_CASE("WorkQueue with worker threads", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<WorkQueue> queue(new WorkQueue(context));

    queue->CreateThreads(2);
    REQUIRE(queue->GetNumThreads() == 2);

    g_counter = 0;
    for (int i = 0; i < 64; ++i)
    {
        SharedPtr<WorkItem> item = queue->GetFreeItem();
        item->workFunction_ = Increment;
        queue->AddWorkItem(item);
    }

    queue->Complete(0);
    REQUIRE(g_counter == 64);

    SECTION("pausing and resuming keeps the queue usable")
    {
        queue->Pause();
        queue->Resume();

        g_counter = 0;
        SharedPtr<WorkItem> item = queue->GetFreeItem();
        item->workFunction_ = Increment;
        queue->AddWorkItem(item);
        queue->Complete(0);
        REQUIRE(g_counter == 1);
    }

    SECTION("worker threads split a data range correctly")
    {
        PODVector<int> values;
        values.Resize(256);
        for (unsigned i = 0; i < values.Size(); ++i)
            values[i] = 0;

        for (unsigned chunk = 0; chunk < 8; ++chunk)
        {
            SharedPtr<WorkItem> item = queue->GetFreeItem();
            item->workFunction_ = AddRange;
            item->start_ = values.Buffer() + chunk * 32;
            item->end_ = values.Buffer() + (chunk + 1) * 32;
            queue->AddWorkItem(item);
        }

        queue->Complete(0);
        for (unsigned i = 0; i < values.Size(); ++i)
            REQUIRE(values[i] == 1);
    }
}

TEST_CASE("WorkQueue sends a completion event when asked", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<WorkQueue> queue(new WorkQueue(context));
    SharedPtr<EventCounter> listener(new EventCounter(context));
    listener->Listen();

    g_counter = 0;
    SharedPtr<WorkItem> item = queue->GetFreeItem();
    item->workFunction_ = Increment;
    item->sendEvent_ = true;
    queue->AddWorkItem(item);
    queue->Complete(0);

    REQUIRE(g_counter == 1);
    REQUIRE(listener->completions_ == 1);

    SECTION("no event is raised when the flag is clear")
    {
        listener->completions_ = 0;
        SharedPtr<WorkItem> quiet = queue->GetFreeItem();
        quiet->workFunction_ = Increment;
        quiet->sendEvent_ = false;
        queue->AddWorkItem(quiet);
        queue->Complete(0);
        REQUIRE(listener->completions_ == 0);
    }
}

TEST_CASE("ProcessUtils reports the host environment", "[Core]")
{
    REQUIRE_FALSE(GetPlatform().Empty());
    REQUIRE(GetNumLogicalCPUs() > 0);
    REQUIRE(GetNumPhysicalCPUs() > 0);
    REQUIRE(GetNumLogicalCPUs() >= GetNumPhysicalCPUs());
    REQUIRE(GetTotalMemory() > 0);

    REQUIRE_FALSE(GetOSVersion().Empty());
}

TEST_CASE("ProcessUtils parses command lines", "[Core]")
{
    SECTION("a plain command line drops the executable by default")
    {
        const Vector<String>& parsed = ParseArguments(String("program --first --second"));
        REQUIRE(parsed.Size() == 2);
        REQUIRE(parsed[0] == "--first");
        REQUIRE(parsed[1] == "--second");
        REQUIRE(GetArguments() == parsed);
    }

    SECTION("the executable can be kept")
    {
        const Vector<String>& parsed = ParseArguments(String("--only"), false);
        REQUIRE(parsed.Size() == 1);
        REQUIRE(parsed[0] == "--only");
    }

    SECTION("quoted arguments stay together")
    {
        const Vector<String>& parsed = ParseArguments(String("program \"one two\" three"), true);
        REQUIRE(parsed.Size() == 2);
        REQUIRE(parsed[0] == "one two");
        REQUIRE(parsed[1] == "three");
    }

    SECTION("repeated whitespace is collapsed")
    {
        const Vector<String>& parsed = ParseArguments(String("program   a    b"), true);
        REQUIRE(parsed.Size() == 2);
        REQUIRE(parsed[0] == "a");
        REQUIRE(parsed[1] == "b");
    }

    SECTION("an empty command line yields no arguments")
    {
        REQUIRE(ParseArguments(String(""), false).Empty());
    }

    SECTION("the argc and argv form skips the program name")
    {
        char program[] = "program";
        char first[] = "-a";
        char second[] = "-b";
        char* argv[] = {program, first, second};

        const Vector<String>& parsed = ParseArguments(3, argv);
        REQUIRE(parsed.Size() == 2);
        REQUIRE(parsed[0] == "-a");
        REQUIRE(parsed[1] == "-b");
    }

    SECTION("the raw pointer form matches the String form")
    {
        const Vector<String> fromLiteral = ParseArguments("program -x");
        REQUIRE(fromLiteral.Size() == 1);
        REQUIRE(fromLiteral[0] == "-x");
    }

    SECTION("the wide character forms parse the same way")
    {
        const Vector<String> fromWide = ParseArguments(WString(String("program -w")));
        REQUIRE(fromWide.Size() == 1);
        REQUIRE(fromWide[0] == "-w");

        const Vector<String> fromWidePointer = ParseArguments(L"program -z");
        REQUIRE(fromWidePointer.Size() == 1);
        REQUIRE(fromWidePointer[0] == "-z");
    }
}

TEST_CASE("ProcessUtils minidump directory round trips", "[Core]")
{
    const String original = GetMiniDumpDir();

    SetMiniDumpDir("/tmp/u3d-minidumps");
    REQUIRE(GetMiniDumpDir() == "/tmp/u3d-minidumps/");

    SetMiniDumpDir(original);
}
