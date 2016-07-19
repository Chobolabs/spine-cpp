////////////////////////////////////////////////////////////////////////////////
// Spine Runtimes Software License
// Version 2.4
//
// Copyright (c) 2013-2016, Esoteric Software
// Copyright (c) 2016, Chobolabs
// All rights reserved.
//
// You are granted a perpetual, non-exclusive, non-sublicensable and
// non-transferable license to use, install, execute and perform the Spine
// Runtimes Software (the "Software") and derivative works solely for personal
// or internal use. Without the written permission of Esoteric Software (see
// Section 2 of the Spine Software License Agreement), you may not (a) modify,
// translate, adapt or otherwise create derivative works, improvements of
// the Software or develop new applications using the Software or (b) remove,
// delete, alter or obscure any trademarks or any copyright, trademark, patent
// or other intellectual property or proprietary rights notices on or in the
// Software, including any copy thereof. Redistributions in binary or source
// form must include this license and terms.
//
// THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE AND CHOBOLABS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE OR CHOBOLABS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Timeline.h"
#include "Vector.h"
#include "Color.h"

#include <string>

namespace spine
{

struct Event;
class Attachment;

struct CurveFrame
{
    void setLinear();
    void setStepped();

    // Sets the control handle positions for an interpolation bezier curve used to transition from this keyframe to the next.
    // cx1 and cx2 are from 0 to 1, representing the percent of time between the two keyframes. cy1 and cy2 are the percent of
    // the difference between the keyframe's values.
    void setCurve(Vector c1, Vector c2);

    float getCurvePercent(float percent) const;

    bool isSameCurveAs(const CurveFrame& other) const;

    enum class Type
    {
        Linear,
        Stepped,
        Bezier
    }
    type = Type::Linear;

    static const int BEZIER_SEGMENTS = 10;
    static const int BEZIER_DATA_SIZE = BEZIER_SEGMENTS - 1;

    // only used for bezier curves
    // points to buffer in CurveTimeline
    Vector* bezierData = nullptr;
};

class CurveTimeline : public Timeline
{
protected:
    CurveTimeline(int framesCount, Timeline::Type type);
    ~CurveTimeline();
    Vector* m_bezierDataBuffer = nullptr;
};

class RotateTimeline : public CurveTimeline
{
public:
    RotateTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        float angle;
    };

    std::vector<Frame> frames;
    int boneIndex = 0;
};

class TranslateTimeline : public CurveTimeline
{
public:
    TranslateTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        Vector translation;
    };

    std::vector<Frame> frames;
    int boneIndex = 0;
};

class ScaleTimeline : public CurveTimeline
{
public:
    ScaleTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        Vector scale;
    };

    std::vector<Frame> frames;
    int boneIndex = 0;
};

class ShearTimeline : public CurveTimeline
{
public:
    ShearTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        Vector shear;
    };

    std::vector<Frame> frames;
    int boneIndex = 0;
};

class ColorTimeline : public CurveTimeline
{
public:
    ColorTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        Color color;
    };

    std::vector<Frame> frames;
    int slotIndex = 0;
};

class AttachmentTimeline : public Timeline
{
public:
    AttachmentTimeline();

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame
    {
        // @param attachmentName May be empty.
        Frame(float time, const std::string& attachmentName)
            : time(time)
            , attachmentName(attachmentName)
        {}
        float time;
        std::string attachmentName;
    };

    std::vector<Frame> frames;
    int slotIndex = 0;
};

class EventTimeline : public Timeline
{
public:
    EventTimeline();

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    typedef Event Frame;
    std::vector<Frame> frames;
};

class DrawOrderTimeline : public Timeline
{
public:
    DrawOrderTimeline(int framesCount, int slotsCount);
    ~DrawOrderTimeline();

    void setFrame(int frameIndex, float time, const std::vector<int>& drawOrder);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame
    {
        float time;
        int* drawOrder;
    };

    std::vector<Frame> frames;

private:
    const int m_slotsCount;
    int* m_drawOrderBuffer;
};

class DeformTimeline : public CurveTimeline
{
public:
    DeformTimeline(int framesCount, size_t frameVerticesCount);
    ~DeformTimeline();

    void setFrame(int frameIndex, float time, const std::vector<Vector>& vertices);
    void setFrame(int frameIndex, float time, const std::vector<float>& vertices);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        Vector* vertices;
    };

    std::vector<Frame> frames;
    int slotIndex = 0;
    const Attachment* attachment = nullptr;

private:
    void setFrame(int frameIndex, float time, const float* vertexData);

    const size_t m_frameVerticesCount;
    Vector* m_verticesBuffer;
};

class IkConstraintTimeline : public CurveTimeline
{
public:
    IkConstraintTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        float mix;
        int bendDirection;
    };

    std::vector<Frame> frames;
    int ikConstraintIndex = 0;
};

class TransformConstraintTimeline : public CurveTimeline
{
public:
    TransformConstraintTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        float rotateMix;
        float translateMix;
        float scaleMix;
        float shearMix;
    };

    std::vector<Frame> frames;
    int transformConstraintIndex = 0;
};

class PathConstraintTimeline : public CurveTimeline
{
public:
    PathConstraintTimeline(int framesCount, Timeline::Type type);

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        float value;
    };

    std::vector<Frame> frames;
    int pathConstraintIndex = 0;

protected:
    void applyToValue(float time, float alpha, float& inOutValue) const;
};

class PathConstraintPositionTimeline : public PathConstraintTimeline
{
public:
    PathConstraintPositionTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;
};

class PathConstraintSpacingTimeline : public PathConstraintTimeline
{
public:
    PathConstraintSpacingTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;
};

class PathConstraintMixTimeline : public CurveTimeline
{
public:
    PathConstraintMixTimeline(int framesCount);

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const override;

    virtual void clearIdentityFrames() override;

    struct Frame : public CurveFrame
    {
        float time;
        float rotateMix;
        float translateMix;
    };

    std::vector<Frame> frames;
    int pathConstraintIndex = 0;
};

}