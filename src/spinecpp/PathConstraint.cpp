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
#include <spinecpp/PathConstraint.h>
#include <spinecpp/PathAttachment.h>
#include <spinecpp/Skeleton.h>
#include <spinecpp/extension.h>

#include <cstdlib>
#include <cmath>

#define PATHCONSTRAINT_NONE -1
#define PATHCONSTRAINT_BEFORE -2
#define PATHCONSTRAINT_AFTER -3

namespace spine
{

PathConstraint::PathConstraint(const PathConstraintData& data, Skeleton& skeleton)
    : data(data)
{
    bones.reserve(data.bones.size());

    for (auto boneData : data.bones)
    {
        bones.emplace_back(skeleton.findBone(boneData->name));
    }

    target = skeleton.findSlot(data.target->name);

    position = data.position;
    spacing = data.spacing;
    rotateMix = data.rotateMix;
    translateMix = data.translateMix;
}

void PathConstraint::apply()
{
    bool translate = translateMix > 0;
    bool rotate = rotateMix > 0;

    if (!translate && !rotate) return;

    auto attachment = target->getAttachment();

    if (!attachment || attachment->type != Attachment::Type::Path) return;

    const bool lengthSpacing = data.spacingMode == SpacingMode::Length;
    const auto rotateMode = data.rotateMode;
    const bool tangents = rotateMode == RotateMode::Tangent;
    const bool scale = rotateMode == RotateMode::ChainScale;
    const auto spacesCount = tangents ? bones.size() : bones.size() + 1;
    
    if (spaces.size() != spacesCount)
    {
        spaces.resize(spacesCount);
    }

    if (scale || lengthSpacing)
    {
        if (scale)
        {
            if (lengths.size() != bones.size())
            {
                lengths.resize(bones.size());
            }
        }

        for (size_t i = 0; i < spacesCount - 1; ++i)
        {
            auto bone = bones[i];
            auto length = bone->data.length;
            auto x = length * bone->a, y = length * bone->c;
            length = std::sqrt(x * x + y * y);
            if (scale)
            {
                lengths[i] = length;
            }

            spaces[i + 1] = lengthSpacing ? std::max(0.f, length + spacing) : spacing;
        }
    }
    else
    {
        for (size_t i = 1; i < spacesCount; ++i)
        {
            spaces[i] = spacing;
        }
    }

    computeWorldPositions();
    
    auto& skeleton = target->bone.skeleton;

    auto bonePos = positions[0].position;
    const auto offsetRotation = data.offsetRotation;

    const bool tip = scale && offsetRotation == 0;

    for (size_t i = 0; i < bones.size(); ++i)
    {
        auto bone = bones[i];

        bone->worldPos += (bonePos - skeleton.translation - bone->worldPos) * translateMix;

        const Vector delta = positions[i + 1].position - bonePos;

        if (scale)
        {
            auto length = lengths[i];
            if (length != 0)
            {
                float s = (delta.length() / length - 1) * rotateMix + 1;
                bone->a *= s;
                bone->c *= s;
            }
        }

        bonePos = positions[i + 1].position;

        if (rotate)
        {
            float a = bone->a, b = bone->b, c = bone->c, d = bone->d;

            float r;
            
            if (tangents)
                r = positions[i].data;
            else if (spaces[i + 1] == 0)
                r = positions[i + 1].data;
            else
                r = delta.angle();

            r -= atan2(c, a) - offsetRotation * DEG_RAD;

            if (tip)
            {
                float cosine = std::cos(r);
                float sine = std::sin(r);
                float length = bone->data.length;
                bonePos.x += (length * (cosine * a - sine * c) - delta.x) * rotateMix;
                bonePos.y += (length * (sine * a + cosine * c) - delta.y) * rotateMix;
            }

            if (r > PI)
                r -= PI_DBL;
            else if (r < -PI)
                r += PI_DBL;

            r *= rotateMix;

            float cosine = std::cos(r);
            float sine = std::sin(r);

            bone->a = cosine * a - sine * c;
            bone->b = cosine * b - sine * d;
            bone->c = sine * a + cosine * c;
            bone->d = sine * b + cosine * d;
        }
    }
}

void PathConstraint::addBeforePosition(float pos, int o)
{
    auto delta = world[1] - world[0];
    float r = delta.angle();
    positions[o].position.x = world[0].x + pos * std::cos(r);
    positions[o].position.y = world[0].y + pos * std::sin(r);
    positions[o].data = r;
}

void PathConstraint::addAfterPosition(float pos, int i, int o)
{
    auto delta = world[i + 1] - world[i];
    float r = delta.angle();
    positions[o].position.x = world[i + 1].x + pos * std::cos(r);
    positions[o].position.y = world[i + 1].y + pos * std::sin(r);
    positions[o].data = r;
}

void PathConstraint::addCurvePosition(float p, float x1, float y1, float cx1, float cy1, float cx2, float cy2, float x2, float y2, bool tangents, int o)
{
    float tt, ttt, u, uu, uuu;
    float ut, ut3, uut3, utt3;
    float x, y;
    if (p == 0) p = 0.0001f;
    tt = p * p, ttt = tt * p, u = 1 - p, uu = u * u, uuu = uu * u;
    ut = u * p, ut3 = ut * 3, uut3 = u * ut3, utt3 = ut3 * p;
    x = x1 * uuu + cx1 * uut3 + cx2 * utt3 + x2 * ttt, y = y1 * uuu + cy1 * uut3 + cy2 * utt3 + y2 * ttt;
    positions[o].position.x = x;
    positions[o].position.y = y;
    if (tangents) positions[o].data = std::atan2(y - (y1 * uu + cy1 * ut * 2 + cy2 * tt), x - (x1 * uu + cx1 * ut * 2 + cx2 * tt));
}

void PathConstraint::computeWorldPositions()
{
    const bool tangents = data.rotateMode == RotateMode::Tangent;
    const bool percentPosition = data.positionMode == PositionMode::Percent;        
    const bool percentSpacing = data.spacingMode == SpacingMode::Percent;

    auto path = static_cast<const PathAttachment*>(target->getAttachment());

    if (positions.size() != spaces.size() + 1)
    {
        positions.resize(spaces.size() + 1);
    }

    bool closed = !!path->closed;
    auto verticesLength = int(path->worldVerticesCount * 2);
    auto curveCount = verticesLength / 6;
    auto prevCurve = PATHCONSTRAINT_NONE;
    float position = this->position;

    if (!path->constantSpeed)
    {
        auto& lengths = path->lengths;
        curveCount -= closed ? 1 : 2;
        float pathLength = lengths[curveCount];
        if (percentPosition) position *= pathLength;
        
        if (percentSpacing)
        {
            for (auto& space : spaces)
            {
                space *= pathLength;
            }
        }

        if (world.size() != 4) world.resize(4);
        auto pworld = reinterpret_cast<float*>(world.data());

        for (int i = 0, curve = 0; i < int(spaces.size()); ++i)
        {
            float space = spaces[i];
            position += space;
            float pos = position;

            if (closed)
            {
                pos = std::fmod(pos, pathLength);
                if (pos < 0) pos += pathLength;
                curve = 0;
            }
            else if (pos < 0)
            {
                if (prevCurve != PATHCONSTRAINT_BEFORE)
                {
                    prevCurve = PATHCONSTRAINT_BEFORE;
                    path->computeWorldVertices(2, 4, *target, pworld, 0);
                }
                addBeforePosition(pos, i);
                continue;
            }
            else if (pos > pathLength)
            {
                if (prevCurve != PATHCONSTRAINT_AFTER)
                {
                    prevCurve = PATHCONSTRAINT_AFTER;
                    path->computeWorldVertices(verticesLength - 6, 4, *target, pworld, 0);
                }
                addAfterPosition(pos - pathLength, 0, i);
                continue;
            }

            /* Determine curve containing position. */
            for (;; curve++)
            {
                float length = lengths[curve];
                if (pos > length) continue;
                
                if (curve == 0)
                {
                    pos /= length;
                }
                else
                {
                    float prev = lengths[curve - 1];
                    pos = (pos - prev) / (length - prev);
                }

                break;
            }

            if (curve != prevCurve)
            {
                prevCurve = curve;
                if (closed && curve == curveCount)
                {
                    path->computeWorldVertices(verticesLength - 4, 4, *target, pworld, 0);
                    path->computeWorldVertices(0, 4, *target, pworld, 4);
                }
                else
                {
                    path->computeWorldVertices(curve * 6 + 2, 8, *target, pworld, 0);
                }
            }

            addCurvePosition(pos, pworld[0], pworld[1], pworld[2], pworld[3], pworld[4], pworld[5], pworld[6], pworld[7], tangents || (i > 0 && space == 0), i);
        }
        return;
    }

    /* World vertices. */
    float* pworld;

    if (closed)
    {
        verticesLength += 2;
        if (world.size() != verticesLength/2) world.resize(verticesLength/2);
        pworld = reinterpret_cast<float*>(world.data());

        path->computeWorldVertices(2, verticesLength - 4, *target, pworld, 0);
        path->computeWorldVertices(0, 2, *target, pworld, verticesLength - 4);
        world.back() = world.front();
    }
    else
    {
        --curveCount;
        verticesLength -= 4;
        if (world.size() != verticesLength / 2) world.resize(verticesLength / 2);
        pworld = reinterpret_cast<float*>(world.data());

        path->computeWorldVertices(2, verticesLength, *target, pworld, 0);
    }

    /* Curve lengths. */
    if (curves.size() != curveCount) curves.resize(curveCount);
    float pathLength = 0;

    float x1 = pworld[0], y1 = pworld[1], cx1 = 0, cy1 = 0, cx2 = 0, cy2 = 0, x2 = 0, y2 = 0;
    for (int i = 0, w = 2; i < curveCount; ++i, w += 6) {
        cx1 = pworld[w];
        cy1 = pworld[w + 1];
        cx2 = pworld[w + 2];
        cy2 = pworld[w + 3];
        x2 = pworld[w + 4];
        y2 = pworld[w + 5];
        float tmpx = (x1 - cx1 * 2 + cx2) * 0.1875f;
        float tmpy = (y1 - cy1 * 2 + cy2) * 0.1875f;
        float dddfx = ((cx1 - cx2) * 3 - x1 + x2) * 0.09375f;
        float dddfy = ((cy1 - cy2) * 3 - y1 + y2) * 0.09375f;
        float ddfx = tmpx * 2 + dddfx;
        float ddfy = tmpy * 2 + dddfy;
        float dfx = (cx1 - x1) * 0.75f + tmpx + dddfx * 0.16666667f;
        float dfy = (cy1 - y1) * 0.75f + tmpy + dddfy * 0.16666667f;
        pathLength += sqrt(dfx * dfx + dfy * dfy);
        dfx += ddfx;
        dfy += ddfy;
        ddfx += dddfx;
        ddfy += dddfy;
        pathLength += sqrt(dfx * dfx + dfy * dfy);
        dfx += ddfx;
        dfy += ddfy;
        pathLength += sqrt(dfx * dfx + dfy * dfy);
        dfx += ddfx + dddfx;
        dfy += ddfy + dddfy;
        pathLength += sqrt(dfx * dfx + dfy * dfy);
        curves[i] = pathLength;
        x1 = x2;
        y1 = y2;
    }
    if (percentPosition) position *= pathLength;
    
    if (percentSpacing)
    {
        for (auto& space : spaces)
            space *= pathLength;
    }

    float curveLength = 0;
    for (int i = 0, curve = 0, segment = 0; i < int(spaces.size()); ++i) {
        float space = spaces[i];
        position += space;
        float p = position;

        if (closed)
        {
            p = std::fmod(p, pathLength);
            if (p < 0) p += pathLength;
            curve = 0;
        }
        else if (p < 0)
        {
            addBeforePosition(p, i);
            continue;
        }
        else if (p > pathLength)
        {
            addAfterPosition(p - pathLength, int(world.size()) - 2, i);
            continue;
        }

        /* Determine curve containing position. */
        for (;; curve++)
        {
            float length = curves[curve];
            if (p > length) continue;
            if (curve == 0)
            {
                p /= length;
            }
            else
            {
                float prev = curves[curve - 1];
                p = (p - prev) / (length - prev);
            }
            break;
        }

        /* Curve segment lengths. */
        if (curve != prevCurve)
        {
            prevCurve = curve;
            int ii = curve * 6;
            x1 = pworld[ii];
            y1 = pworld[ii + 1];
            cx1 = pworld[ii + 2];
            cy1 = pworld[ii + 3];
            cx2 = pworld[ii + 4];
            cy2 = pworld[ii + 5];
            x2 = pworld[ii + 6];
            y2 = pworld[ii + 7];
            float tmpx = (x1 - cx1 * 2 + cx2) * 0.03f;
            float tmpy = (y1 - cy1 * 2 + cy2) * 0.03f;
            float dddfx = ((cx1 - cx2) * 3 - x1 + x2) * 0.006f;
            float dddfy = ((cy1 - cy2) * 3 - y1 + y2) * 0.006f;
            float ddfx = tmpx * 2 + dddfx;
            float ddfy = tmpy * 2 + dddfy;
            float dfx = (cx1 - x1) * 0.3f + tmpx + dddfx * 0.16666667f;
            float dfy = (cy1 - y1) * 0.3f + tmpy + dddfy * 0.16666667f;
            curveLength = std::sqrt(dfx * dfx + dfy * dfy);
            segments[0] = curveLength;
            for (ii = 1; ii < 8; ++ii) {
                dfx += ddfx;
                dfy += ddfy;
                ddfx += dddfx;
                ddfy += dddfy;
                curveLength += sqrt(dfx * dfx + dfy * dfy);
                segments[ii] = curveLength;
            }
            dfx += ddfx;
            dfy += ddfy;
            curveLength += sqrt(dfx * dfx + dfy * dfy);
            segments[8] = curveLength;
            dfx += ddfx + dddfx;
            dfy += ddfy + dddfy;
            curveLength += sqrt(dfx * dfx + dfy * dfy);
            segments[9] = curveLength;
            segment = 0;
        }

        /* Weight by segment length. */
        p *= curveLength;
        for (;; segment++)
        {
            float length = segments[segment];
            if (p > length) continue;
            if (segment == 0)
            {
                p /= length;
            }
            else
            {
                float prev = segments[segment - 1];
                p = segment + (p - prev) / (length - prev);
            }
            break;
        }

        addCurvePosition(p * 0.1f, x1, y1, cx1, cy1, cx2, cy2, x2, y2, tangents || (i > 0 && space == 0), i);
    }
}


}