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
#include <spinecpp/IkConstraint.h>
#include <spinecpp/Skeleton.h>
#include <spinecpp/Bone.h>
#include <spinecpp/extension.h>
#include <cmath>
#include <limits>

namespace spine
{

IkConstraint::IkConstraint(const IkConstraintData& data, Skeleton& skeleton)
    : data(data)
{
    bendDirection = data.bendDirection;
    mix = data.mix;

    bones.reserve(data.bones.size());

    for (auto bd : data.bones)
    {
        bones.emplace_back(skeleton.findBone(bd->name));
    }

    target = skeleton.findBone(data.target->name);
}

void IkConstraint::apply()
{
    switch (bones.size())
    {
    case 1:
        apply1(*bones.front(), target->worldPos, mix);
        break;
    case 2:
        apply2(*bones.front(), *bones.back(), target->worldPos, bendDirection, mix);
    }
}

void IkConstraint::apply1(Bone& bone, Vector target, float alpha)
{
    float parentRotation = bone.isRoot() ? 0 : bone.parent->getWorldRotationX();
    float rotation = bone.rotation;
    float rotationIK = atan2(target.y - bone.worldPos.y, target.x - bone.worldPos.x) * RAD_DEG - parentRotation;

    if ((bone.worldSign.x != bone.worldSign.y) != (bone.skeleton.flipX != (bone.skeleton.flipY != Bone::isYDown())))
        rotationIK = 360 - rotationIK;
    if (rotationIK > 180) rotationIK -= 360;
    else if (rotationIK < -180) rotationIK += 360;
    bone.updateWorldTransformWith(bone.translation, rotation + (rotationIK - rotation) * alpha, bone.appliedScale);
}

void IkConstraint::apply2(Bone& parent, Bone& child, Vector target, int bendDir, float alpha)
{
    float px = parent.translation.x, py = parent.translation.y, psx = parent.appliedScale.x, psy = parent.appliedScale.y;
    float cx = child.translation.x, cy = child.translation.y, csx = child.appliedScale.x, cwx = child.worldPos.x, cwy = child.worldPos.y;
    int o1, o2, s2, u;
    const Bone* pp = parent.parent;
    float tx, ty, dx, dy, l1, l2, a1, a2, r;
    if (alpha == 0) return;
    if (psx < 0) {
        psx = -psx;
        o1 = 180;
        s2 = -1;
    }
    else {
        o1 = 0;
        s2 = 1;
    }
    if (psy < 0) {
        psy = -psy;
        s2 = -s2;
    }
    r = psx - psy;
    u = (r < 0 ? -r : r) <= 0.0001f;
    if (!u && cy != 0) {
        cwx = parent.a * cx + parent.worldPos.x;
        cwy = parent.c * cx + parent.worldPos.y;
        cy = 0;
    }
    if (csx < 0) {
        csx = -csx;
        o2 = 180;
    }
    else
        o2 = 0;
    if (!pp) {
        tx = target.x - px;
        ty = target.y - py;
        dx = cwx - px;
        dy = cwy - py;
    }
    else {
        float a = pp->a, b = pp->b, c = pp->c, d = pp->d, invDet = 1 / (a * d - b * c);
        float wx = pp->worldPos.x, wy = pp->worldPos.y, x = target.x - wx, y = target.y - wy;
        tx = (x * d - y * b) * invDet - px;
        ty = (y * a - x * c) * invDet - py;
        x = cwx - wx;
        y = cwy - wy;
        dx = (x * d - y * b) * invDet - px;
        dy = (y * a - x * c) * invDet - py;
    }
    l1 = sqrt(dx * dx + dy * dy);
    l2 = child.data.length * csx;
    if (u) {
        float cos, a, o;
        l2 *= psx;
        cos = (tx * tx + ty * ty - l1 * l1 - l2 * l2) / (2 * l1 * l2);
        if (cos < -1) cos = -1;
        else if (cos > 1) cos = 1;
        a2 = acos(cos) * bendDir;
        a = l1 + l2 * cos;
        o = l2 * sin(a2);
        a1 = atan2(ty * a - tx * o, tx * a + ty * o);
    }
    else {
        float a = psx * l2, b = psy * l2, ta = atan2(ty, tx);
        float aa = a * a, bb = b * b, ll = l1 * l1, dd = tx * tx + ty * ty;
        float c0 = bb * ll + aa * dd - aa * bb, c1 = -2 * bb * l1, c2 = bb - aa;
        float d = c1 * c1 - 4 * c2 * c0;
        float minAngle = 0, minDist = std::numeric_limits<float>::max(), minX = 0, minY = 0;
        float maxAngle = 0, maxDist = 0, maxX = 0, maxY = 0;
        float x = l1 + a, dist = x * x, angle, y;
        if (d >= 0) {
            float q = sqrt(d), r0, r1, ar0, ar1;;
            if (c1 < 0) q = -q;
            q = -(c1 + q) / 2;
            r0 = q / c2;
            r1 = c0 / q;
            ar0 = r0 < 0 ? -r0 : r0;
            ar1 = r1 < 0 ? -r1 : r1;
            r = ar0 < ar1 ? r0 : r1;
            if (r * r <= dd) {
                float y1 = sqrt(dd - r * r) * bendDir;
                a1 = ta - atan2(y1, r);
                a2 = atan2(y1 / psy, (r - l1) / psx);
                goto outer;
            }
        }
        if (dist > maxDist) {
            maxAngle = 0;
            maxDist = dist;
            maxX = x;
        }
        x = l1 - a;
        dist = x * x;
        if (dist < minDist) {
            minAngle = PI;
            minDist = dist;
            minX = x;
        }
        angle = acos(-a * l1 / (aa - bb));
        x = a * cos(angle) + l1;
        y = b * sin(angle);
        dist = x * x + y * y;
        if (dist < minDist) {
            minAngle = angle;
            minDist = dist;
            minX = x;
            minY = y;
        }
        if (dist > maxDist) {
            maxAngle = angle;
            maxDist = dist;
            maxX = x;
            maxY = y;
        }
        if (dd <= (minDist + maxDist) / 2) {
            a1 = ta - atan2(minY * bendDir, minX);
            a2 = minAngle * bendDir;
        }
        else {
            a1 = ta - atan2(maxY * bendDir, maxX);
            a2 = maxAngle * bendDir;
        }
    }
    outer: {
        float os = atan2(cy, cx) * s2;
        a1 = (a1 - os) * RAD_DEG + o1;
        a2 = (a2 + os) * RAD_DEG * s2 + o2;
        if (a1 > 180) a1 -= 360;
        else if (a1 < -180) a1 += 360;
        if (a2 > 180) a2 -= 360;
        else if (a2 < -180) a2 += 360;
        r = parent.rotation;
        parent.updateWorldTransformWith(Vector(px, py), r + (a1 - r) * alpha, parent.appliedScale);
        r = child.rotation;
        child.updateWorldTransformWith(Vector(cx, cy), r + (a2 - r) * alpha, child.appliedScale);
    }
}

}
