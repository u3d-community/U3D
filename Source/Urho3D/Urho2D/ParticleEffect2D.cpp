//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Urho2D/ParticleEffect2D.h"
#include "../Urho2D/Sprite2D.h"

#include "../DebugNew.h"

namespace Urho3D
{

static const int srcBlendFuncs[] =
{
    1,      // GL_ONE
    1,      // GL_ONE
    0x0306, // GL_DST_COLOR
    0x0302, // GL_SRC_ALPHA
    0x0302, // GL_SRC_ALPHA
    1,      // GL_ONE
    0x0305, // GL_ONE_MINUS_DST_ALPHA
    1,      // GL_ONE
    0x0302  // GL_SRC_ALPHA
};

static const int destBlendFuncs[] =
{
    0,      // GL_ZERO
    1,      // GL_ONE
    0,      // GL_ZERO
    0x0303, // GL_ONE_MINUS_SRC_ALPHA
    1,      // GL_ONE
    0x0303, // GL_ONE_MINUS_SRC_ALPHA
    0x0304, // GL_DST_ALPHA
    1,      // GL_ONE
    1       // GL_ONE
};

// Make sure that there are are as many blend functions as we have blend modes.
static_assert(sizeof(srcBlendFuncs) / sizeof(srcBlendFuncs[0]) == MAX_BLENDMODES, "");
static_assert(sizeof(destBlendFuncs) / sizeof(destBlendFuncs[0]) == MAX_BLENDMODES, "");

ParticleEffect2D::ParticleEffect2D(Context* context) :
    Resource(context),
    sourcePositionVariance_(7.0f, 7.0f),
    speed_(260.0f),
    speedVariance_(10.0f),
    particleLifeSpan_(1.000f),
    particleLifespanVariance_(0.700f),
    angle_(0.0f),
    angleVariance_(360.0f),
    gravity_(0.0f, 0.0f),
    radialAcceleration_(-380.0f),
    tangentialAcceleration_(-140.0f),
    radialAccelVariance_(0.0f),
    tangentialAccelVariance_(0.0f),
    startColor_(1.0f, 0.0f, 0.0f, 1.0f),
    startColorVariance_(0.0f, 0.0f, 0.0f, 0.0f),
    finishColor_(1.0f, 1.0f, 0.0f, 1.0f),
    finishColorVariance_(0.0f, 0.0f, 0.0f, 0.0f),
    maxParticles_(600),
    startParticleSize_(60.0f),
    startParticleSizeVariance_(40.0f),
    finishParticleSize_(5.0f),
    finishParticleSizeVariance_(5.0f),
    duration_(M_INFINITY),
    emitterType_(EMITTER_TYPE_GRAVITY),
    maxRadius_(100.0f),
    maxRadiusVariance_(0.0f),
    minRadius_(0.0f),
    minRadiusVariance_(0.0f),
    rotatePerSecond_(0.0f),
    rotatePerSecondVariance_(0.0f),
    blendMode_(BLEND_ALPHA),
    rotationStart_(0.0f),
    rotationStartVariance_(0.0f),
    rotationEnd_(0.0f),
    rotationEndVariance_(0.0f)
{
}

ParticleEffect2D::~ParticleEffect2D() = default;

void ParticleEffect2D::RegisterObject(Context* context)
{
    context->RegisterFactory<ParticleEffect2D>();
}

bool ParticleEffect2D::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    loadSpriteName_.Clear();

    XMLFile xmlFile(context_);
    if (!xmlFile.Load(source))
        return false;

    XMLElement rootElem = xmlFile.GetRoot("particleEmitterConfig");
    if (!rootElem)
        return false;

    // Note: not accurate
    bool success = Load(rootElem);
    if (success)
        SetMemoryUse(source.GetSize());
    return success;
}

bool ParticleEffect2D::EndLoad()
{
    // Apply the sprite now
    if (!loadSpriteName_.Empty())
    {
        auto* cache = GetSubsystem<ResourceCache>();
        sprite_ = cache->GetResource<Sprite2D>(loadSpriteName_);
        if (!sprite_)
            URHO3D_LOGERROR("Could not load sprite " + loadSpriteName_ + " for particle effect");

        loadSpriteName_.Clear();
    }

    return true;
}

bool ParticleEffect2D::Save(Serializer& dest) const
{
    if (!sprite_)
        return false;

    XMLFile xmlFile(context_);
    XMLElement rootElem = xmlFile.CreateRoot("particleEmitterConfig");

    Save(rootElem);

    return xmlFile.Save(dest);
}

bool ParticleEffect2D::Save(XMLElement& dest) const
{
    if (!sprite_)
        return false;

    if (dest.IsNull())
    {
        URHO3D_LOGERROR("Can not save particle effect to null XML element");
        return false;
    }

    String fileName = GetFileNameAndExtension(sprite_->GetName());
    dest.CreateChild("texture").SetAttribute("name", fileName);

    WriteVector2(dest, "sourcePosition", Vector2::ZERO);
    WriteVector2(dest, "sourcePositionVariance", sourcePositionVariance_);

    WriteFloat(dest, "speed", speed_);
    WriteFloat(dest, "speedVariance", speedVariance_);

    WriteFloat(dest, "particleLifeSpan", particleLifeSpan_);
    WriteFloat(dest, "particleLifespanVariance", particleLifespanVariance_);

    WriteFloat(dest, "angle", angle_);
    WriteFloat(dest, "angleVariance", angleVariance_);

    WriteVector2(dest, "gravity", gravity_);

    WriteFloat(dest, "radialAcceleration", radialAcceleration_);
    WriteFloat(dest, "tangentialAcceleration", tangentialAcceleration_);

    WriteFloat(dest, "radialAccelVariance", radialAccelVariance_);
    WriteFloat(dest, "tangentialAccelVariance", tangentialAccelVariance_);

    WriteColor(dest, "startColor", startColor_);
    WriteColor(dest, "startColorVariance", startColorVariance_);

    WriteColor(dest, "finishColor", finishColor_);
    WriteColor(dest, "finishColorVariance", finishColorVariance_);

    WriteInt(dest, "maxParticles", maxParticles_);

    WriteFloat(dest, "startParticleSize", startParticleSize_);
    WriteFloat(dest, "startParticleSizeVariance", startParticleSizeVariance_);

    WriteFloat(dest, "finishParticleSize", finishParticleSize_);
    // Typo in pex file
    WriteFloat(dest, "FinishParticleSizeVariance", finishParticleSizeVariance_);

    float duration = duration_;
    if (duration == M_INFINITY)
        duration = -1.0f;
    WriteFloat(dest, "duration", duration);
    WriteInt(dest, "emitterType", (int)emitterType_);

    WriteFloat(dest, "maxRadius", maxRadius_);
    WriteFloat(dest, "maxRadiusVariance", maxRadiusVariance_);
    WriteFloat(dest, "minRadius", minRadius_);
    WriteFloat(dest, "minRadiusVariance", minRadiusVariance_);

    WriteFloat(dest, "rotatePerSecond", rotatePerSecond_);
    WriteFloat(dest, "rotatePerSecondVariance", rotatePerSecondVariance_);

    WriteInt(dest, "blendFuncSource", srcBlendFuncs[blendMode_]);
    WriteInt(dest, "blendFuncDestination", destBlendFuncs[blendMode_]);

    WriteFloat(dest, "rotationStart", rotationStart_);
    WriteFloat(dest, "rotationStartVariance", rotationStartVariance_);

    WriteFloat(dest, "rotationEnd", rotationEnd_);
    WriteFloat(dest, "rotationEndVariance", rotationEndVariance_);

    return true;
}

bool ParticleEffect2D::Load(const XMLElement& source)
{
    // Reset to defaults first so that missing parameters in case of a live reload behave as expected
    sourcePositionVariance_ = Vector2(7.0f, 7.0f);
    speed_ = 260.0f;
    speedVariance_ = 10.0f;
    particleLifeSpan_ = 1.000f;
    particleLifespanVariance_ = 0.700f;
    angle_ = 0.0f;
    angleVariance_ = 360.0f;
    gravity_ = Vector2(0.0f, 0.0f);
    radialAcceleration_ = -380.0f;
    tangentialAcceleration_ = -140.0f;
    radialAccelVariance_ = 0.0f;
    tangentialAccelVariance_ = 0.0f;
    startColor_ = Color(1.0f, 0.0f, 0.0f, 1.0f);
    startColorVariance_ = Color(0.0f, 0.0f, 0.0f, 0.0f);
    finishColor_ = Color(1.0f, 1.0f, 0.0f, 1.0f);
    finishColorVariance_ = Color(0.0f, 0.0f, 0.0f, 0.0f);
    maxParticles_ = 600;
    startParticleSize_ = 60.0f;
    startParticleSizeVariance_ = 40.0f;
    finishParticleSize_ = 5.0f;
    finishParticleSizeVariance_ = 5.0f;
    duration_ = M_INFINITY;
    emitterType_ = EMITTER_TYPE_GRAVITY;
    maxRadius_ = 100.0f;
    maxRadiusVariance_ = 0.0f;
    minRadius_ = 0.0f;
    minRadiusVariance_ = 0.0f;
    rotatePerSecond_ = 0.0f;
    rotatePerSecondVariance_ = 0.0f;
    blendMode_ = BLEND_ALPHA;
    rotationStart_ = 0.0f;
    rotationStartVariance_ = 0.0f;
    rotationEnd_ = 0.0f;
    rotationEndVariance_ = 0.0f;

    if (source.IsNull())
    {
        URHO3D_LOGERROR("Can not load particle effect 2D from null XML element");
        return false;
    }


    if(source.HasChild("texture"))
    {
        String texture = source.GetChild("texture").GetAttribute("name");
        loadSpriteName_ = GetParentPath(GetName()) + texture;
        // If async loading, request the sprite beforehand
        if (GetAsyncLoadState() == ASYNC_LOADING)
            GetSubsystem<ResourceCache>()->BackgroundLoadResource<Sprite2D>(loadSpriteName_, true, this);
    }


    if(source.HasChild("sourcePositionVariance"))
        sourcePositionVariance_ = ReadVector2(source, "sourcePositionVariance");


    if(source.HasChild("speed"))
        speed_ = ReadFloat(source, "speed");

    if(source.HasChild("speedVariance"))
        speedVariance_ = ReadFloat(source, "speedVariance");


    if(source.HasChild("particleLifeSpan"))
        particleLifeSpan_ = Max(0.01f, ReadFloat(source, "particleLifeSpan"));

    if(source.HasChild("particleLifespanVariance"))
        particleLifespanVariance_ = ReadFloat(source, "particleLifespanVariance");


    if(source.HasChild("angle"))
        angle_ = ReadFloat(source, "angle");
    if(source.HasChild("angleVariance"))
        angleVariance_ = ReadFloat(source, "angleVariance");


    if(source.HasChild("gravity"))
        gravity_ = ReadVector2(source, "gravity");


    if(source.HasChild("radialAcceleration"))
        radialAcceleration_ = ReadFloat(source, "radialAcceleration");

    if(source.HasChild("tangentialAcceleration"))
        tangentialAcceleration_ = ReadFloat(source, "tangentialAcceleration");


    if(source.HasChild("radialAccelVariance"))
        radialAccelVariance_ = ReadFloat(source, "radialAccelVariance");

    if(source.HasChild("tangentialAccelVariance"))
        tangentialAccelVariance_ = ReadFloat(source, "tangentialAccelVariance");


    if(source.HasChild("startColor"))
        startColor_ = ReadColor(source, "startColor");

    if(source.HasChild("startColorVariance"))
        startColorVariance_ = ReadColor(source, "startColorVariance");


    if(source.HasChild("finishColor"))
        finishColor_ = ReadColor(source, "finishColor");

    if(source.HasChild("finishColorVariance"))
        finishColorVariance_ = ReadColor(source, "finishColorVariance");


    if(source.HasChild("maxParticles"))
        maxParticles_ = ReadInt(source, "maxParticles");


    if(source.HasChild("startParticleSize"))
        startParticleSize_ = ReadFloat(source, "startParticleSize");

    if(source.HasChild("startParticleSizeVariance"))
        startParticleSizeVariance_ = ReadFloat(source, "startParticleSizeVariance");


    if(source.HasChild("finishParticleSize"))
        finishParticleSize_ = ReadFloat(source, "finishParticleSize");

    // Typo in pex file
    if(source.HasChild("FinishParticleSizeVariance"))
        finishParticleSizeVariance_ = ReadFloat(source, "FinishParticleSizeVariance");

    if (source.HasChild("duration"))
    {
        float duration = ReadFloat(source, "duration");
        if (duration > 0.0f)
            duration_ = duration;
    }


    if(source.HasChild("emitterType"))
        emitterType_ = (EmitterType2D)ReadInt(source, "emitterType");


    if(source.HasChild("maxRadius"))
        maxRadius_ = ReadFloat(source, "maxRadius");

    if(source.HasChild("maxRadiusVariance"))
        maxRadiusVariance_ = ReadFloat(source, "maxRadiusVariance");

    if(source.HasChild("minRadius"))
        minRadius_ = ReadFloat(source, "minRadius");

    if(source.HasChild("minRadiusVariance"))
        minRadiusVariance_ = ReadFloat(source, "minRadiusVariance");


    if(source.HasChild("rotatePerSecond"))
        rotatePerSecond_ = ReadFloat(source, "rotatePerSecond");

    if(source.HasChild("rotatePerSecondVariance"))
        rotatePerSecondVariance_ = ReadFloat(source, "rotatePerSecondVariance");

    if(source.HasChild("blendFuncSource") && source.HasChild("blendFuncDestination"))
    {
        int blendFuncSource = ReadInt(source, "blendFuncSource");
        int blendFuncDestination = ReadInt(source, "blendFuncDestination");
        for (int i = 0; i < MAX_BLENDMODES; ++i)
        {
            if (blendFuncSource == srcBlendFuncs[i] && blendFuncDestination == destBlendFuncs[i])
            {
                blendMode_ = (BlendMode)i;
                break;
            }
        }
    }

    if(source.HasChild("rotationStart"))
        rotationStart_ = ReadFloat(source, "rotationStart");

    if(source.HasChild("rotationStartVariance"))
        rotationStartVariance_ = ReadFloat(source, "rotationStartVariance");

    if(source.HasChild("rotationEnd"))
        rotationEnd_ = ReadFloat(source, "rotationEnd");

    if(source.HasChild("rotationEndVariance"))
        rotationEndVariance_ = ReadFloat(source, "rotationEndVariance");

    return true;
}

void ParticleEffect2D::SetSprite(Sprite2D* sprite)
{
    sprite_ = sprite;
}

void ParticleEffect2D::SetSourcePositionVariance(const Vector2& sourcePositionVariance)
{
    sourcePositionVariance_ = sourcePositionVariance;
}

void ParticleEffect2D::SetSpeed(float speed)
{
    speed_ = speed;
}

void ParticleEffect2D::SetSpeedVariance(float speedVariance)
{
    speedVariance_ = speedVariance;
}

void ParticleEffect2D::SetParticleLifeSpan(float particleLifeSpan)
{
    particleLifeSpan_ = particleLifeSpan;
}

void ParticleEffect2D::SetParticleLifespanVariance(float particleLifespanVariance)
{
    particleLifespanVariance_ = particleLifespanVariance;
}

void ParticleEffect2D::SetAngle(float angle)
{
    angle_ = angle;
}

void ParticleEffect2D::SetAngleVariance(float angleVariance)
{
    angleVariance_ = angleVariance;
}

void ParticleEffect2D::SetGravity(const Vector2& gravity)
{
    gravity_ = gravity;
}

void ParticleEffect2D::SetRadialAcceleration(float radialAcceleration)
{
    radialAcceleration_ = radialAcceleration;
}

void ParticleEffect2D::SetTangentialAcceleration(float tangentialAcceleration)
{
    tangentialAcceleration_ = tangentialAcceleration;
}

void ParticleEffect2D::SetRadialAccelVariance(float radialAccelVariance)
{
    radialAccelVariance_ = radialAccelVariance;
}

void ParticleEffect2D::SetTangentialAccelVariance(float tangentialAccelVariance)
{
    tangentialAccelVariance_ = tangentialAccelVariance;
}

void ParticleEffect2D::SetStartColor(const Color& startColor)
{
    startColor_ = startColor;
}

void ParticleEffect2D::SetStartColorVariance(const Color& startColorVariance)
{
    startColorVariance_ = startColorVariance;
}

void ParticleEffect2D::SetFinishColor(const Color& finishColor)
{
    finishColor_ = finishColor;
}

void ParticleEffect2D::SetFinishColorVariance(const Color& finishColorVariance)
{
    finishColorVariance_ = finishColorVariance;
}

void ParticleEffect2D::SetMaxParticles(int maxParticles)
{
    maxParticles_ = maxParticles;
}

void ParticleEffect2D::SetStartParticleSize(float startParticleSize)
{
    startParticleSize_ = startParticleSize;
}

void ParticleEffect2D::SetStartParticleSizeVariance(float startParticleSizeVariance)
{
    startParticleSizeVariance_ = startParticleSizeVariance;
}

void ParticleEffect2D::SetFinishParticleSize(float finishParticleSize)
{
    finishParticleSize_ = finishParticleSize;
}

void ParticleEffect2D::SetFinishParticleSizeVariance(float finishParticleSizeVariance)
{
    finishParticleSizeVariance_ = finishParticleSizeVariance;
}

void ParticleEffect2D::SetDuration(float duration)
{
    duration_ = duration;
}

void ParticleEffect2D::SetEmitterType(EmitterType2D emitterType)
{
    emitterType_ = emitterType;
}

void ParticleEffect2D::SetMaxRadius(float maxRadius)
{
    maxRadius_ = maxRadius;
}

void ParticleEffect2D::SetMaxRadiusVariance(float maxRadiusVariance)
{
    maxRadiusVariance_ = maxRadiusVariance;
}

void ParticleEffect2D::SetMinRadius(float minRadius)
{
    minRadius_ = minRadius;
}

void ParticleEffect2D::SetMinRadiusVariance(float minRadiusVariance)
{
    minRadiusVariance_ = minRadiusVariance;
}

void ParticleEffect2D::SetRotatePerSecond(float rotatePerSecond)
{
    rotatePerSecond_ = rotatePerSecond;
}

void ParticleEffect2D::SetRotatePerSecondVariance(float rotatePerSecondVariance)
{
    rotatePerSecondVariance_ = rotatePerSecondVariance;
}

void ParticleEffect2D::SetBlendMode(BlendMode blendMode)
{
    blendMode_ = blendMode;
}

void ParticleEffect2D::SetRotationStart(float rotationStart)
{
    rotationStart_ = rotationStart;
}

void ParticleEffect2D::SetRotationStartVariance(float rotationStartVariance)
{
    rotationStartVariance_ = rotationStartVariance;
}

void ParticleEffect2D::SetRotationEnd(float rotationEnd)
{
    rotationEnd_ = rotationEnd;
}

void ParticleEffect2D::SetRotationEndVariance(float rotationEndVariance)
{
    rotationEndVariance_ = rotationEndVariance;
}

SharedPtr<ParticleEffect2D> ParticleEffect2D::Clone(const String& cloneName) const
{
    SharedPtr<ParticleEffect2D> ret(new ParticleEffect2D(context_));

    ret->SetName(cloneName);
    ret->sprite_ = sprite_;
    ret->sourcePositionVariance_ = sourcePositionVariance_;
    ret->speed_ = speed_;
    ret->speedVariance_ = speedVariance_;
    ret->particleLifeSpan_ = particleLifeSpan_;
    ret->particleLifespanVariance_ = particleLifespanVariance_;
    ret->angle_ = angle_;
    ret->angleVariance_ = angleVariance_;
    ret->gravity_ = gravity_;
    ret->radialAcceleration_ = radialAcceleration_;
    ret->tangentialAcceleration_ = tangentialAcceleration_;
    ret->radialAccelVariance_ = radialAccelVariance_;
    ret->tangentialAccelVariance_ = tangentialAccelVariance_;
    ret->startColor_ = startColor_;
    ret->startColorVariance_ = startColorVariance_;
    ret->finishColor_ = finishColor_;
    ret->finishColorVariance_ = finishColorVariance_;
    ret->maxParticles_ = maxParticles_;
    ret->startParticleSize_ = startParticleSize_;
    ret->startParticleSizeVariance_ = startParticleSizeVariance_;
    ret->finishParticleSize_ = finishParticleSize_;
    ret->finishParticleSizeVariance_ = finishParticleSizeVariance_;
    ret->duration_ = duration_;
    ret->emitterType_ = emitterType_;
    ret->maxRadius_ = maxRadius_;
    ret->maxRadiusVariance_ = maxRadiusVariance_;
    ret->minRadius_ = minRadius_;
    ret->minRadiusVariance_ = minRadiusVariance_;
    ret->rotatePerSecond_ = rotatePerSecond_;
    ret->rotatePerSecondVariance_ = rotatePerSecondVariance_;
    ret->blendMode_ = blendMode_;
    ret->rotationStart_ = rotationStart_;
    ret->rotationStartVariance_ = rotationStartVariance_;
    ret->rotationEnd_ = rotationEnd_;
    ret->rotationEndVariance_ = rotationEndVariance_;
    /// \todo Zero if source was created programmatically
    ret->SetMemoryUse(GetMemoryUse());

    return ret;
}

int ParticleEffect2D::ReadInt(const XMLElement& element, const String& name) const
{
    return element.GetChild(name).GetInt("value");
}

float ParticleEffect2D::ReadFloat(const XMLElement& element, const String& name) const
{
    return element.GetChild(name).GetFloat("value");
}

Color ParticleEffect2D::ReadColor(const XMLElement& element, const String& name) const
{
    XMLElement child = element.GetChild(name);
    return Color(child.GetFloat("red"), child.GetFloat("green"), child.GetFloat("blue"), child.GetFloat("alpha"));
}

Vector2 ParticleEffect2D::ReadVector2(const XMLElement& element, const String& name) const
{
    XMLElement child = element.GetChild(name);
    return Vector2(child.GetFloat("x"), child.GetFloat("y"));
}

void ParticleEffect2D::WriteInt(XMLElement& element, const String& name, int value) const
{
    XMLElement child = element.CreateChild(name);
    child.SetInt("value", value);
}

void ParticleEffect2D::WriteFloat(XMLElement& element, const String& name, float value) const
{
    XMLElement child = element.CreateChild(name);
    child.SetFloat("value", value);
}

void ParticleEffect2D::WriteColor(XMLElement& element, const String& name, const Color& color) const
{
    XMLElement child = element.CreateChild(name);
    child.SetFloat("red", color.r_);
    child.SetFloat("green", color.g_);
    child.SetFloat("blue", color.b_);
    child.SetFloat("alpha", color.a_);
}

void ParticleEffect2D::WriteVector2(XMLElement& element, const String& name, const Vector2& value) const
{
    XMLElement child = element.CreateChild(name);
    child.SetFloat("x", value.x_);
    child.SetFloat("y", value.y_);
}

}
