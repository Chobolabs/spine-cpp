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
    auto pp = bone.parent;
    float id = 1 / (pp->a * pp->d - pp->b * pp->c);
    auto pos = target - pp->worldPos;
    float tx = (pos.x * pp->d - pos.y * pp->b) * id - bone.translation.x,
        ty = (pos.y * pp->a - pos.x * pp->c) * id - bone.translation.y;

    float rotationIK = atan2(ty, tx) * RAD_DEG - bone.shear.x;
    if (bone.scale.x < 0) 
        rotationIK += 180;
    
    if (rotationIK > 180)
        rotationIK -= 360;
    else if (rotationIK < -180) 
        rotationIK += 360;

    bone.updateWorldTransformWith(bone.translation, bone.rotation + (rotationIK - bone.rotation) * alpha, bone.appliedScale, bone.shear);
}

void IkConstraint::apply2(Bone& parent, Bone& child, Vector target, int bendDir, float alpha)
{
    float px = parent.translation.x, py = parent.translation.y, psx = parent.appliedScale.x, psy = parent.appliedScale.y;
    int os1, os2, s2;
    float cx, cy, csx;
    int u;
    const Bone *pp;
    float ppa, ppb, ppc, ppd, id;
    float x, y;
    float tx, ty;
    float dx, dy;
    float l1, l2, a1, a2;
    float os;
    float rotation;

    if (alpha == 0) return;
    if (psx < 0) {
        psx = -psx;
        os1 = 180;
        s2 = -1;
    }
    else {
        os1 = 0;
        s2 = 1;
    }
    if (psy < 0) {
        psy = -psy;
        s2 = -s2;
    }
    cx = child.translation.x; cy = child.translation.y; csx = child.appliedScale.x;
    u = abs(psx - psy) <= 0.0001f;
    if (!u && cy != 0) {
        child.worldPos.x = parent.a * cx + parent.worldPos.x;
        child.worldPos.y = parent.c * cx + parent.worldPos.y;
        cy = 0;
    }
    if (csx < 0) {
        csx = -csx;
        os2 = 180;
    }
    else
        os2 = 0;
    pp = parent.parent;
    ppa = pp->a; ppb = pp->b; ppc = pp->c; ppd = pp->d; id = 1 / (ppa * ppd - ppb * ppc);
    x = target.x - pp->worldPos.x; y = target.y - pp->worldPos.y;
    tx = (x * ppd - y * ppb) * id - px; ty = (y * ppa - x * ppc) * id - py;
    x = child.worldPos.x - pp->worldPos.x;
    y = child.worldPos.y - pp->worldPos.y;
    dx = (x * ppd - y * ppb) * id - px; dy = (y * ppa - x * ppc) * id - py;
    l1 = sqrt(dx * dx + dy * dy); l2 = child.data.length * csx;
outer:
    if (u) {
        float cosine, a, o;
        l2 *= psx;
        cosine = (tx * tx + ty * ty - l1 * l1 - l2 * l2) / (2 * l1 * l2);
        if (cosine < -1)
            cosine = -1;
        else if (cosine > 1) cosine = 1;
        a2 = acos(cosine) * bendDir;
        a = l1 + l2 * cosine, o = l2 * sin(a2);
        a1 = atan2(ty * a - tx * o, tx * a + ty * o);
    }
    else {
        float minAngle, minDist, minX, minY, maxAngle, maxDist, maxX, maxY, angle;
        float a = psx * l2, b = psy * l2, ta = atan2(ty, tx);
        float aa = a * a, bb = b * b, ll = l1 * l1, dd = tx * tx + ty * ty;
        float c0 = bb * ll + aa * dd - aa * bb, c1 = -2 * bb * l1, c2 = bb - aa;
        float d = c1 * c1 - 4 * c2 * c0;
        if (d >= 0) {
            float q = sqrt(d), r0, r, r1;
            if (c1 < 0) q = -q;
            q = -(c1 + q) / 2;
            r0 = q / c2; r1 = c0 / q;
            r = abs(r0) < abs(r1) ? r0 : r1;
            if (r * r <= dd) {
                y = sqrt(dd - r * r) * bendDir;
                a1 = ta - atan2(y, r);
                a2 = atan2(y / psy, (r - l1) / psx);
                goto outer;
            }
        }
        minAngle = 0; minDist = std::numeric_limits<float>::max(); minX = 0; minY = 0;
        maxAngle = 0; maxDist = 0; maxX = 0; maxY = 0;
        x = l1 + a;
        d = x * x;
        if (d > maxDist) {
            maxAngle = 0;
            maxDist = d;
            maxX = x;
        }
        x = l1 - a;
        d = x * x;
        if (d < minDist) {
            minAngle = PI;
            minDist = d;
            minX = x;
        }
        angle = acos(-a * l1 / (aa - bb));
        x = a * cos(angle) + l1;
        y = b * sin(angle);
        d = x * x + y * y;
        if (d < minDist) {
            minAngle = angle;
            minDist = d;
            minX = x;
            minY = y;
        }
        if (d > maxDist) {
            maxAngle = angle;
            maxDist = d;
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
    os = atan2(cy, cx) * s2;
    a1 = (a1 - os) * RAD_DEG + os1;
    a2 = ((a2 + os) * RAD_DEG - child.shear.x) * s2 + os2;
    if (a1 > 180)
        a1 -= 360;
    else if (a1 < -180) a1 += 360;
    if (a2 > 180)
        a2 -= 360;
    else if (a2 < -180) a2 += 360;

    rotation = parent.rotation;
    parent.updateWorldTransformWith(Vector(px, py), rotation + (a1 - rotation) * alpha, parent.appliedScale, Vector(0, 0));
    rotation = child.rotation;
    child.updateWorldTransformWith(Vector(cx, cy), rotation + (a2 - rotation) * alpha, child.appliedScale, child.shear);
}

}
