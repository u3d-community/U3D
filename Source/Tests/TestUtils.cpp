#include "TestUtils.h"

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>

using namespace Urho3D;

namespace U3DTest
{

TestContext::TestContext() :
    context_(new Context())
{
    context_->RegisterSubsystem(new FileSystem(context_));
    auto* log = new Log(context_);
    log->SetQuiet(true);
    log->SetLevel(LOG_ERROR);
    context_->RegisterSubsystem(log);
}

TestContext::~TestContext() = default;

Context* HeadlessContext()
{
    static SharedPtr<Context> context;
    if (!context)
    {
        context = new Context();
        SharedPtr<Engine> engine(new Engine(context));

        VariantMap parameters;
        parameters[EP_HEADLESS] = true;
        parameters[EP_LOG_QUIET] = true;
        parameters[EP_LOG_NAME] = ScratchPath("Engine.log");
        parameters[EP_FRAME_LIMITER] = false;
        parameters[EP_WORKER_THREADS] = false;
        parameters[EP_SOUND] = false;
        parameters[EP_RESOURCE_PREFIX_PATHS] = U3D_TEST_RESOURCE_PREFIX;
        parameters[EP_RESOURCE_PATHS] = "Data;CoreData";

        if (!engine->Initialize(parameters))
            FAIL("headless engine initialisation failed");
    }
    return context;
}

String ScratchPath(const String& name)
{
    static bool created = false;
    const String dir(U3D_TEST_SCRATCH_DIR);
    if (!created)
    {
        TestContext context;
        context->GetSubsystem<FileSystem>()->CreateDir(dir);
        created = true;
    }
    return dir + "/" + name;
}

void RemoveScratch(const String& name)
{
    const String path = ScratchPath(name);
    TestContext context;
    auto* fileSystem = context->GetSubsystem<FileSystem>();
    if (fileSystem->FileExists(path))
        fileSystem->Delete(path);
}

String ResourcePath(const String& name)
{
    return String(U3D_TEST_RESOURCE_PREFIX) + "/" + name;
}

}
