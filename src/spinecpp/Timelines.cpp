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
#include <spinecpp/Timelines.h>
#include <spinecpp/IkConstraint.h>
#include <spinecpp/Skeleton.h>
#include <spinecpp/Bone.h>
#include <spinecpp/Slot.h>
#include <spinecpp/Event.h>
#include <spinecpp/Attachment.h>
#include <spinecpp/MeshAttachment.h>

#include <algorithm>
#include <cstring>

namespace spine
{

namespace
{
template <typename Frame>
typename std::vector<Frame>::const_iterator findFrame(const std::vector<Frame>& frames, float time)
{
    return std::upper_bound(frames.begin(), frames.end(), time, [](float t, const Frame& f) -> bool
    {
        return t < f.time;
    });
}


inline void normalizeAngle(float& angle)
{
    while (angle > 180)
        angle -= 360;
    while (angle < -180)
        angle += 360;
}

inline float clamp(float f, float min, float max)
{
    if (f < min) return min;
    if (f > max) return max;
    return f;
}

inline float saturate(float f)
{
    return clamp(f, 0, 1);
}

template <typename Frame>
void initFramesBezeierData(std::vector<Frame>& frames, Vector* bezierBuffer)
{
    Vector* ptr = bezierBuffer;
    for (auto& f : frames)
    {
        f.bezierData = ptr;
        ptr += CurveFrame::BEZIER_DATA_SIZE;
    }
}

}

void CurveFrame::setLinear()
{
    type = CurveFrame::Type::Linear;
}

void CurveFrame::setStepped()
{
    type = CurveFrame::Type::Stepped;
}

void CurveFrame::setCurve(Vector c1, Vector c2)
{
    float tmpx = (-c1.x * 2 + c2.x) * 0.03f, tmpy = (-c1.y * 2 + c2.y) * 0.03f;
	float dddfx = ((c1.x - c2.x) * 3 + 1) * 0.006f, dddfy = ((c1.y - c2.y) * 3 + 1) * 0.006f;
	float ddfx = tmpx * 2 + dddfx, ddfy = tmpy * 2 + dddfy;
	float dfx = c1.x * 0.3f + tmpx + dddfx * 0.16666667f, dfy = c1.y * 0.3f + tmpy + dddfy * 0.16666667f;
    float x = dfx, y = dfy;

    type = CurveFrame::Type::Bezier;

    for (int i = 0; i < BEZIER_DATA_SIZE; ++i)
    {
        auto& v = bezierData[i];
        v.x = x;
        v.y = y;
        dfx += ddfx;
        dfy += ddfy;
        ddfx += dddfx;
        ddfy += dddfy;
        x += dfx;
        y += dfy;
    }
}

float CurveFrame::getCurvePercent(float percent) const
{
    percent = saturate(percent);
    if (type == CurveFrame::Type::Linear) return percent;
    if (type == CurveFrame::Type::Stepped) return 0;

    Vector prev(0, 0);
    for (int i = 0; i < BEZIER_DATA_SIZE; ++i)
    {
        auto& v = bezierData[i];

        if (v.x > percent)
        {
            return prev.y + (v.y - prev.y) * (percent - prev.x) / (v.x - prev.x);
        }

        prev = v;
    }

    return prev.y + (1 - prev.y) * (percent - prev.x) / (1 - prev.x); /* Last point is 1,1. */
}

bool CurveFrame::isSameCurveAs(const CurveFrame& other) const
{
    if (type != other.type) return false;

    if (type != CurveFrame::Type::Bezier) return true;

    return memcmp(bezierData, other.bezierData, BEZIER_DATA_SIZE * sizeof(Vector)) == 0;
}

CurveTimeline::CurveTimeline(int framesCount, Timeline::Type type)
    : Timeline(type)
{
    m_bezierDataBuffer = new Vector[framesCount * CurveFrame::BEZIER_DATA_SIZE];
}

CurveTimeline::~CurveTimeline()
{
    delete[] m_bezierDataBuffer;
}

///////////////////////////////////////////////////////////////////////////////

RotateTimeline::RotateTimeline(int framesCount)
    : CurveTimeline(framesCount, Timeline::Type::Rotate)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void RotateTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    auto& bone = skeleton.bones[boneIndex];

    if (time >= frames.back().time) // time is after last frame
    {
        float amount = bone.data.rotation + frames.back().angle - bone.rotation;
        normalizeAngle(amount);
        bone.rotation += amount * alpha;
        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    float amount = curFrame->angle - prevFrame->angle;
    normalizeAngle(amount);
    amount = bone.data.rotation + (prevFrame->angle + amount * percent) - bone.rotation;
    normalizeAngle(amount);
    bone.rotation += amount * alpha;
}

void RotateTimeline::clearIdentityFrames()
{
    float angle = frames.front().angle;    
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].angle != angle)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}

///////////////////////////////////////////////////////////////////////////////

TranslateTimeline::TranslateTimeline(int framesCount)
    : CurveTimeline(framesCount, Timeline::Type::Translate)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void TranslateTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    auto& bone = skeleton.bones[boneIndex];

    if (time >= frames.back().time) // time is after last frame
    {
        bone.translation += (bone.data.translation + frames.back().translation - bone.translation) * alpha;
        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    bone.translation +=
        (
        bone.data.translation
        + prevFrame->translation + (curFrame->translation - prevFrame->translation) * percent
        - bone.translation
        ) * alpha;
}

void TranslateTimeline::clearIdentityFrames()
{
    auto translation = frames.front().translation;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].translation != translation)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}


///////////////////////////////////////////////////////////////////////////////

ScaleTimeline::ScaleTimeline(int framesCount)
    : CurveTimeline(framesCount, Timeline::Type::Scale)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void ScaleTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    auto& bone = skeleton.bones[boneIndex];

    if (time >= frames.back().time) // time is after last frame
    {
        bone.scale += (bone.data.scale * frames.back().scale - bone.scale) * alpha;
        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    bone.scale +=
        (
        bone.data.scale
        * (prevFrame->scale + (curFrame->scale - prevFrame->scale) * percent)
        - bone.scale
        ) * alpha;
}

void ScaleTimeline::clearIdentityFrames()
{
    auto scale = frames.front().scale;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].scale != scale)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}

///////////////////////////////////////////////////////////////////////////////

ShearTimeline::ShearTimeline(int framesCount)
    : CurveTimeline(framesCount, Timeline::Type::Shear)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void ShearTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    auto& bone = skeleton.bones[boneIndex];

    if (time >= frames.back().time) // time is after last frame
    {
        bone.shear += (bone.data.shear + frames.back().shear - bone.shear) * alpha;
        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    bone.shear += 
        (bone.data.shear 
        + (prevFrame->shear + (curFrame->shear - prevFrame->shear) * percent)
        - bone.shear
        ) * alpha;
}

void ShearTimeline::clearIdentityFrames()
{
    auto shear = frames.front().shear;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].shear != shear)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}

///////////////////////////////////////////////////////////////////////////////

ColorTimeline::ColorTimeline(int framesCount)
    : CurveTimeline(framesCount, Timeline::Type::Color)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void ColorTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    Color color;

    if (time >= frames.back().time)
    {
        // time is after last frame
        color = frames.back().color;
    }
    else
    {
        // Interpolate between the previous frame and the current frame.
        auto curFrame = findFrame(frames, time);
        auto prevFrame = curFrame - 1;

        float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
        percent = prevFrame->getCurvePercent(percent);

        color.r = prevFrame->color.r + (curFrame->color.r - prevFrame->color.r) * percent;
        color.g = prevFrame->color.g + (curFrame->color.g - prevFrame->color.g) * percent;
        color.b = prevFrame->color.b + (curFrame->color.b - prevFrame->color.b) * percent;
        color.a = prevFrame->color.a + (curFrame->color.a - prevFrame->color.a) * percent;
    }

    auto& slot = skeleton.slots[slotIndex];

    if (alpha < 1)
    {
        slot.color.r += (color.r - slot.color.r) * alpha;
        slot.color.g += (color.g - slot.color.g) * alpha;
        slot.color.b += (color.b - slot.color.b) * alpha;
        slot.color.a += (color.a - slot.color.a) * alpha;
    }
    else
    {
        slot.color = color;
    }
}

void ColorTimeline::clearIdentityFrames()
{
    auto color = frames.front().color;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].color != color)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}


///////////////////////////////////////////////////////////////////////////////

AttachmentTimeline::AttachmentTimeline()
    : Timeline(Timeline::Type::Attachment)
{}

void AttachmentTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    std::vector<Frame>::const_iterator prevFrame;
    if (time >= frames.back().time) // time is after last frame
    {
        prevFrame = frames.end() - 1;
    }
    else
    {
        prevFrame = findFrame(frames, time) - 1;
    }

    const Attachment* attachment = nullptr;

    if (!prevFrame->attachmentName.empty())
    {
        attachment = skeleton.getAttachmentForSlotIndex(slotIndex, prevFrame->attachmentName);
    }

    auto& slot = skeleton.slots[slotIndex];
    slot.setAttachment(attachment);
}

void AttachmentTimeline::clearIdentityFrames()
{
    auto att = frames.front().attachmentName.c_str();;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].attachmentName != att)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}

///////////////////////////////////////////////////////////////////////////////

EventTimeline::EventTimeline()
    : Timeline(Timeline::Type::Event)
{}

/** Fires events for frames > lastTime and <= time. */
void EventTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (!firedEvents) return; // no output for events

    if (lastTime > time)
    {
        // Fire events after last time for looped animations.
        apply(skeleton, lastTime, std::numeric_limits<float>::max(), firedEvents, alpha);
        lastTime = -1; // start from beginning
    }
    else if (lastTime >= frames.back().time)
    {
        // Last time is after last frame.
        return;
    }

    if (time < frames.front().time) return; // time is before first frame

    std::vector<Frame>::const_iterator frame;
    if (lastTime < frames.front().time)
    {
        frame = frames.begin();
    }
    else
    {
        // Find potentially many frames with the same time
        frame = findFrame(frames, lastTime);
        lastTime = frame->time;

        while (frame != frames.begin())
        {
            if ((frame - 1)->time != lastTime) break;
            --frame;
        }

        // frame here points to the first of a list of frames with the same time
    }

    for (; frame != frames.end() && time >= frame->time; ++frame)
    {
        auto pFrame = frames.data() + (frame - frames.begin()); // pointer
        firedEvents->push_back(pFrame);
    }
}

void EventTimeline::clearIdentityFrames()
{
    // this is never identity
    return;
}


///////////////////////////////////////////////////////////////////////////////

DrawOrderTimeline::DrawOrderTimeline(int framesCount, int slotsCount)
    : Timeline(Timeline::Type::Draworder)
    , m_slotsCount(slotsCount)
{
    frames.resize(framesCount);
    m_drawOrderBuffer = new int[framesCount * slotsCount];
}

DrawOrderTimeline::~DrawOrderTimeline()
{
    delete[] m_drawOrderBuffer;
}

void DrawOrderTimeline::setFrame(int frameIndex, float time, const std::vector<int>& drawOrder)
{
    auto& frame = frames[frameIndex];
    frame.time = time;
    if (drawOrder.empty())
    {
        frame.drawOrder = nullptr;
    }
    else
    {
        frame.drawOrder = m_drawOrderBuffer + frameIndex * m_slotsCount;
        memcpy(frame.drawOrder, drawOrder.data(), m_slotsCount * sizeof(int));
    }
}

void DrawOrderTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    std::vector<Frame>::const_iterator frame;
    if (time >= frames.back().time) // time is after last frame
    {
        frame = frames.end() - 1;
    }
    else
    {
        frame = findFrame(frames, time) - 1;
    }

    if (frame->drawOrder)
    {
        skeleton.setDrawOrder(frame->drawOrder);
    }
    else
    {
        skeleton.resetDrawOrder();
    }
}

void DrawOrderTimeline::clearIdentityFrames()
{
    auto order = frames.front().drawOrder;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        auto curOrder = frames[i].drawOrder;

        if (order == curOrder) continue;

        if (order == nullptr || curOrder == nullptr) return;

        if (memcmp(curOrder, order, m_slotsCount * sizeof(int)))
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}


///////////////////////////////////////////////////////////////////////////////

DeformTimeline::DeformTimeline(int framesCount, size_t frameVerticesCount)
    : CurveTimeline(framesCount, Timeline::Type::Deform)
    , m_frameVerticesCount(frameVerticesCount)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);

    m_verticesBuffer = new Vector[framesCount * m_frameVerticesCount];

    auto ptr = m_verticesBuffer;
    for (auto& frame : frames)
    {
        frame.vertices = ptr;
        ptr += frameVerticesCount;
    }
}

DeformTimeline::~DeformTimeline()
{
    delete[] m_verticesBuffer;
}

void DeformTimeline::setFrame(int frameIndex, float time, const std::vector<Vector>& vertices)
{
    setFrame(frameIndex, time, reinterpret_cast<const float*>(vertices.data()));
}

void DeformTimeline::setFrame(int frameIndex, float time, const std::vector<float>& vertices)
{
    setFrame(frameIndex, time, vertices.data());
}

void DeformTimeline::setFrame(int frameIndex, float time, const float* vertexData)
{
    auto& frame = frames[frameIndex];
    frame.time = time;
    memcpy(frame.vertices, vertexData, sizeof(Vector)*m_frameVerticesCount);
}

void DeformTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    auto& slot = skeleton.slots[slotIndex];
    if (slot.getAttachment() != attachment)
    {
        // attachments don't match, so try parent attachments
        if (!slot.getAttachment()) return;
        switch (slot.getAttachment()->type)
        {
        case Attachment::Type::Mesh:
        {
            auto mesh = static_cast<const MeshAttachment*>(slot.getAttachment());
            if (!mesh->inheritDeform || mesh->getParentMesh() != attachment) return;
            break;
        }
        default:
            return;
        }
    }

    if (slot.attachmentVertices.size() != m_frameVerticesCount)
    {
        // slot's attachment vertices aren't initilalized
        // we cannot mix with those
        alpha = 1;

        // ... so no point in preserving them, too
        slot.attachmentVertices.clear();
    }

    slot.attachmentVertices.resize(m_frameVerticesCount);

    if (time >= frames.back().time) // time is after last frame
    {
        const Vector* verts = frames.back().vertices;

        if (alpha < 1)
        {
            for (size_t i = 0; i < m_frameVerticesCount; ++i)
            {
                slot.attachmentVertices[i] += (verts[i] - slot.attachmentVertices[i]) * alpha;
            }
        }
        else
        {
            memcpy(slot.attachmentVertices.data(), verts, sizeof(Vector) * m_frameVerticesCount);
        }

        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    auto prevVertices = prevFrame->vertices;
    auto curVertices = curFrame->vertices;
    auto& slotVertices = slot.attachmentVertices;

    if (alpha < 1)
    {
        for (size_t i = 0; i < m_frameVerticesCount; ++i)
        {
            slotVertices[i] += (prevVertices[i] + (curVertices[i] - prevVertices[i]) * percent - slotVertices[i]) * alpha;
        }
    }
    else
    {
        for (size_t i = 0; i < m_frameVerticesCount; ++i)
        {
            slotVertices[i] = prevVertices[i] + (curVertices[i] - prevVertices[i]) * percent;
        }
    }
}

void DeformTimeline::clearIdentityFrames()
{
    auto verts = frames.front().vertices;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        auto curVerts = frames[i].vertices;

        if (verts == curVerts) continue;

        if (verts == nullptr || curVerts == nullptr) return;

        if(memcmp(curVerts, verts, sizeof(Vector)*m_frameVerticesCount))
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}

///////////////////////////////////////////////////////////////////////////////

IkConstraintTimeline::IkConstraintTimeline(int framesCount)
    : CurveTimeline(framesCount, Timeline::Type::IkConstraint)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void IkConstraintTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    auto& constraint = skeleton.ikConstraints[ikConstraintIndex];

    if (time >= frames.back().time) // time is after last frame
    {
        constraint.mix += (frames.back().mix - constraint.mix) * alpha;
        constraint.bendDirection = frames.back().bendDirection;
        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    float mix = prevFrame->mix + (curFrame->mix - prevFrame->mix) * percent;
    constraint.mix += (mix - constraint.mix) * alpha;
    constraint.bendDirection = prevFrame->bendDirection;
}

void IkConstraintTimeline::clearIdentityFrames()
{
    auto mix = frames.front().mix;
    // auto bd = frames.front().bendDirection;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].mix != mix)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}

///////////////////////////////////////////////////////////////////////////////

TransformConstraintTimeline::TransformConstraintTimeline(int framesCount)
    : CurveTimeline(framesCount, Timeline::Type::TransformConstraint)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void TransformConstraintTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    auto& constraint = skeleton.transformConstraints[transformConstraintIndex];

    if (time >= frames.back().time) // time is after last frame
    {
        constraint.rotateMix += (frames.back().rotateMix - constraint.rotateMix) * alpha;
        constraint.translateMix += (frames.back().translateMix - constraint.translateMix) * alpha;
        constraint.scaleMix += (frames.back().scaleMix - constraint.scaleMix) * alpha;
        constraint.shearMix += (frames.back().shearMix - constraint.shearMix) * alpha;
        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    auto rotate = prevFrame->rotateMix;
    auto translate = prevFrame->translateMix;
    auto scale = prevFrame->scaleMix;
    auto shear = prevFrame->shearMix;

    constraint.rotateMix += (rotate + (curFrame->rotateMix - rotate) * percent - constraint.rotateMix) * alpha;
    constraint.translateMix += (translate + (curFrame->translateMix - translate) * percent - constraint.translateMix) * alpha;
    constraint.scaleMix += (scale + (curFrame->scaleMix - scale) * percent - constraint.scaleMix) * alpha;
    constraint.shearMix += (shear + (curFrame->shearMix - shear) * percent - constraint.shearMix) * alpha;
}

void TransformConstraintTimeline::clearIdentityFrames()
{
    float rotateMix = frames.front().rotateMix;
    float translateMix = frames.front().translateMix;
    float scaleMix = frames.front().scaleMix;
    float shearMix = frames.front().shearMix;

    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].rotateMix != rotateMix)
        {
            return;
        }

        if (frames[i].translateMix != translateMix)
        {
            return;
        }

        if (frames[i].scaleMix != scaleMix)
        {
            return;
        }

        if (frames[i].shearMix != shearMix)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}

///////////////////////////////////////////////////////////////////////////////

PathConstraintTimeline::PathConstraintTimeline(int framesCount, Timeline::Type type)
    : CurveTimeline(framesCount, type)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void PathConstraintTimeline::clearIdentityFrames()
{
    auto val = frames.front().value;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].value != val)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}

void PathConstraintTimeline::applyToValue(float time, float alpha, float& inOutValue) const
{
    if (time < frames.front().time) return; // time is before first frame

    if (time >= frames.back().time) // time is after last frame
    {
        inOutValue += (frames.back().value - inOutValue) * alpha;
        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    auto value = prevFrame->value;
    inOutValue += (value + (curFrame->value - value) * percent - inOutValue) * alpha;
}

///////////////////////////////////////////////////////////////////////////////

PathConstraintPositionTimeline::PathConstraintPositionTimeline(int framesCount)
    : PathConstraintTimeline(framesCount, Timeline::Type::PathConstraintPosition)
{
}

void PathConstraintPositionTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    auto& constraint = skeleton.pathConstraints[pathConstraintIndex];
    applyToValue(time, alpha, constraint.position);
}

PathConstraintSpacingTimeline::PathConstraintSpacingTimeline(int framesCount)
    : PathConstraintTimeline(framesCount, Timeline::Type::PathConstraintSpacing)
{
}

void PathConstraintSpacingTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    auto& constraint = skeleton.pathConstraints[pathConstraintIndex];
    applyToValue(time, alpha, constraint.spacing);
}

///////////////////////////////////////////////////////////////////////////////

PathConstraintMixTimeline::PathConstraintMixTimeline(int framesCount)
    : CurveTimeline(framesCount, Timeline::Type::PathConstraintMix)
{
    frames.resize(framesCount);
    initFramesBezeierData(frames, m_bezierDataBuffer);
}

void PathConstraintMixTimeline::apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const
{
    if (time < frames.front().time) return; // time is before first frame

    auto& constraint = skeleton.pathConstraints[pathConstraintIndex];

    if (time >= frames.back().time) // time is after last frame
    {
        constraint.rotateMix += (frames.back().rotateMix - constraint.rotateMix) * alpha;
        constraint.translateMix += (frames.back().translateMix - constraint.translateMix) * alpha;
        return;
    }

    // Interpolate between the previous frame and the current frame.
    auto curFrame = findFrame(frames, time);
    auto prevFrame = curFrame - 1;

    float percent = 1 - (time - curFrame->time) / (prevFrame->time - curFrame->time);
    percent = prevFrame->getCurvePercent(percent);

    auto rotateMix = prevFrame->rotateMix;
    auto translateMix = prevFrame->translateMix;
    constraint.rotateMix += (rotateMix + (curFrame->rotateMix - rotateMix) * percent - constraint.rotateMix) * alpha;
    constraint.translateMix += (translateMix + (curFrame->translateMix - translateMix) * percent - constraint.translateMix) * alpha;
}

void PathConstraintMixTimeline::clearIdentityFrames()
{
    auto rotateMix = frames.front().rotateMix;
    auto translateMix = frames.front().translateMix;
    for (size_t i = 1; i < frames.size(); ++i)
    {
        if (frames[i].rotateMix != rotateMix)
        {
            return;
        }

        if (frames[i].translateMix != translateMix)
        {
            return;
        }
    }

    frames.erase(frames.begin() + 1, frames.end());
}


}
