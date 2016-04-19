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
#include <spinecpp/Bone.h>
#include <spinecpp/Skeleton.h>
#include <spinecpp/extension.h>

#include <cmath>

using namespace std;

namespace spine
{

bool Bone::m_isYDown = false;

void Bone::setYDown(bool yDown)
{
    m_isYDown = yDown;
}

bool Bone::isYDown()
{
    return m_isYDown;
}

Bone::Bone(const BoneData& data, const Skeleton& skeleton, const Bone* parent)
    : data(data)
    , skeleton(skeleton)
    , parent(parent)
{
    setToSetupPose();
}

void Bone::setToSetupPose()
{
    translation = data.translation;
    rotation = data.rotation;
    scale = data.scale;
}

void Bone::updateWorldTransform()
{
    updateWorldTransformWith(translation, rotation, scale);
}

void Bone::updateWorldTransformWith(Vector translation, float rotation, Vector scale)
{
    appliedRotation = rotation;
    appliedScale = scale;

    float radians = rotation * DEG_RAD;
    float cosine = cos(radians);
    float sine = sin(radians);
    float la = cosine * scale.x, lb = -sine * scale.y, lc = sine * scale.x, ld = cosine * scale.y;

    if (!parent) // Root bone.
    {
        if (skeleton.flipX)
        {
            translation.x = -translation.x;
            la = -la;
            lb = -lb;
        }

        if (skeleton.flipY != m_isYDown)
        {
            translation.y = -translation.y;
            lc = -lc;
            ld = -ld;
        }

        a = la;
        b = lb;
        c = lc;
        d = ld;
        worldPos = translation;
        worldSign.x = scale.x > 0 ? 1.f : -1.f;
        worldSign.y = scale.y > 0 ? 1.f : -1.f;

        return;
    }

    float pa = parent->a;
    float pb = parent->b;
    float pc = parent->c;
    float pd = parent->d;

    worldPos.x = pa * translation.x + pb * translation.y + parent->worldPos.x;
    worldPos.y = pc * translation.x + pd * translation.y + parent->worldPos.y;

    worldSign.x = parent->worldSign.x * (scale.x > 0 ? 1.f : -1.f);
    worldSign.y = parent->worldSign.y * (scale.y > 0 ? 1.f : -1.f);

    if (data.inheritRotation && data.inheritScale)
    {
        a = pa * la + pb * lc;
        b = pa * lb + pb * ld;
        c = pc * la + pd * lc;
        d = pc * lb + pd * ld;
    }
    else 
    {
        if (data.inheritRotation) /* No scale inheritance. */
        {
            pa = 1;
            pb = 0;
            pc = 0;
            pd = 1;

            auto p = parent;

            do
            {
                cosine = cos(p->appliedRotation * DEG_RAD);
                sine = sin(p->appliedRotation * DEG_RAD);
                float temp = pa * cosine + pb * sine;
                pb = pa * -sine + pb * cosine;
                pa = temp;
                temp = pc * cosine + pd * sine;
                pd = pc * -sine + pd * cosine;
                pc = temp;

                if (!p->data.inheritRotation) break;
                p = p->parent;
            } while (p);

            a = pa * la + pb * lc;
            b = pa * lb + pb * ld;
            c = pc * la + pd * lc;
            d = pc * lb + pd * ld;
        }
        else if (data.inheritScale) /* No rotation inheritance. */
        {
            pa = 1;
            pb = 0;
            pc = 0;
            pd = 1;

            auto p = parent;

            do {
                float za, zb, zc, zd;
                float r = p->rotation;
                float psx = p->appliedScale.x, psy = appliedScale.y;
                cosine = cos(r * DEG_RAD);
                sine = sin(r * DEG_RAD);
                za = cosine * psx;
                zb = -sine * psy;
                zc = sine * psx;
                zd = cosine * psy;
                float temp = pa * za + pb * zc;
                pb = pa * zb + pb * zd;
                pa = temp;
                temp = pc * za + pd * zc;
                pd = pc * zb + pd * zd;
                pc = temp;

                if (psx < 0) r = -r;
                cosine = cos(-r * DEG_RAD);
                sine = sin(-r * DEG_RAD);
                temp = pa * cosine + pb * sine;
                pb = pa * -sine + pb * cosine;
                pa = temp;
                temp = pc * cosine + pd * sine;
                pd = pc * -sine + pd * cosine;
                pc = temp;

                if (!p->data.inheritScale) break;
                p = p->parent;
            } while (p);


            a = pa * la + pb * lc;
            b = pa * lb + pb * ld;
            c = pc * la + pd * lc;
            d = pc * lb + pd * ld;
        }
        else
        {
            a = la;
            b = lb;
            c = lc;
            d = ld;
        }

        if (skeleton.flipX)
        {
            a = -a;
            b = -b;
        }

        if (skeleton.flipY != m_isYDown)
        {
            c = -c;
            d = -d;
        }
    }
}

float Bone::getWorldRotationX() const
{
    return atan2(c, a) * RAD_DEG;
}

float Bone::getWorldRotationY() const
{
    return atan2(d, b) * RAD_DEG;
}

float Bone::getWorldScaleX() const
{
    return sqrt(a * a + b * b) * worldSign.x;
}

float Bone::getWorldScaleY() const
{
    return sqrt(c * c + d * d) * worldSign.y;
}

void Bone::worldToLocal(Vector world, Vector& outLocal)
{
    float x = world.x - worldPos.x, y = world.y - worldPos.y;
    float invDet = 1 / (a * d - b * c);
    outLocal.x = (x * d * invDet - y * b * invDet);
    outLocal.y = (y * a * invDet - x * c * invDet);
}

void Bone::localToWorld(Vector local, Vector& outWorld)
{
    outWorld.x = local.x * a + local.y * b + worldPos.x;
    outWorld.y = local.x * c + local.y * d + worldPos.x;
}

}
