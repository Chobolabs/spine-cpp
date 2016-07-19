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
#include <spinecpp/TransformConstraint.h>
#include <spinecpp/Skeleton.h>
#include <spinecpp/extension.h>

#include <cmath>
using namespace std;

namespace spine
{

TransformConstraint::TransformConstraint(const TransformConstraintData& data, Skeleton& skeleton)
    : data(data)
{
    rotateMix = data.rotateMix;
    translateMix = data.translateMix;
    scaleMix = data.scaleMix;
    shearMix = data.shearMix;

    bones.reserve(data.bones.size());
    for (auto boneData : data.bones)
    {
        bones.emplace_back(skeleton.findBone(boneData->name));
    }

    target = skeleton.findBone(data.target->name);
}

void TransformConstraint::apply()
{
    float ta = target->a, tb = target->b, tc = target->c, td = target->d;

    for (auto bone : bones)
    {
        if (rotateMix > 0)
        {
            float a = bone->a, b = bone->b, c = bone->c, d = bone->d;
            float r = atan2(tc, ta) - atan2(c, a) + data.offsetRotation * DEG_RAD;

            if (r > PI) r -= PI_DBL;
            else if (r < -PI) r += PI_DBL;
            r *= rotateMix;
            float cosine = cos(r);
            float sine = sin(r);
            bone->a = cosine * a - sine * c;
            bone->b = cosine * b - sine * d;
            bone->c = sine * a + cosine * c;
            bone->d = sine * b + cosine * d;
        }

        if (translateMix > 0)
        {
            Vector pos;
            target->localToWorld(data.offsetTranslation, pos);

            bone->worldPos += (pos - bone->worldPos) * translateMix;
        }

        if (scaleMix > 0)
        {
            float bs = sqrt(bone->a * bone->a + bone->c * bone->c);
            float ts = sqrt(ta * ta + tc * tc);
            float s = bs > 0.00001f ? (bs + (ts - bs + data.offsetScale.x) * scaleMix) / bs : 0;
            bone->a *= s;
            bone->c *= s;

            bs = sqrt(bone->b * bone->b + bone->d * bone->d);
            ts = sqrt(tb * tb + td * td);
            s = bs > 0.00001f ? (bs + (ts - bs + data.offsetScale.y) * scaleMix) / bs : 0;
            bone->b *= s;
            bone->d *= s;
        }

        if (shearMix > 0)
        {
            float b = bone->b, d = bone->d;
            float by = atan2(d, b);
            float r = atan2(td, tb) - atan2(tc, ta) - (by - atan2(bone->c, bone->a));
            float s = sqrt(b * b + d * d);

            if (r > PI) r -= PI_DBL;
            else if (r < -PI) r += PI_DBL;
            r = by + (r + data.offsetShearY * DEG_RAD) * shearMix;
            bone->b = cos(r) * s;
            bone->d = sin(r) * s;
        }
    }
}

}
