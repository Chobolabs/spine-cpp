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

namespace
{

inline float sin_deg(float f)
{
    return sin(f * DEG_RAD);
}

inline float cos_deg(float f)
{
    return cos(f * DEG_RAD);
}

}

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

Bone::Bone(const BoneData& data, const Skeleton& skeleton, Bone* parent)
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
    shear = data.shear;
}

void Bone::updateWorldTransform()
{
    updateWorldTransformWith(translation, rotation, scale, shear);
}

void Bone::updateWorldTransformWith(Vector translation, float rotation, Vector scale, Vector shear)
{
    appliedRotation = rotation;

    float rotationY = rotation + 90 + shear.y;
    float la = cos_deg(rotation + shear.x) * scale.x, lb = cos_deg(rotationY) * scale.y;
    float lc = sin_deg(rotation + shear.x) * scale.x, ld = sin_deg(rotationY) * scale.y;

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
                float cosine = cos_deg(p->appliedRotation); 
                float sine = sin_deg(p->appliedRotation);
                float temp = pa * cosine + pb * sine;
                pb = pb * cosine - pa * sine;
                pa = temp;
                temp = pc * cosine + pd * sine;
                pd = pd * cosine - pc * sine;
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

                float psx = p->scale.x, psy = p->scale.y;
                float cosine = cos_deg(p->appliedRotation);
                float sine = sin_deg(p->appliedRotation);
                za = cosine * psx; zb = sine * psy; zc = sine * psx; zd = cosine * psy;
                float temp = pa * za + pb * zc;
                pb = pb * zd - pa * zb;
                pa = temp;
                temp = pc * za + pd * zc;
                pd = pd * zd - pc * zb;
                pc = temp;

                if (psx >= 0) sine = -sine;
                temp = pa * cosine + pb * sine;
                pb = pb * cosine - pa * sine;
                pa = temp;
                temp = pc * cosine + pd * sine;
                pd = pd * cosine - pc * sine;
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
    return sqrt(a * a + c * c) * worldSign.x;
}

float Bone::getWorldScaleY() const
{
    return sqrt(b * b + d * d) * worldSign.y;
}

float Bone::worldToLocalRotationX() const
{
    if (!parent) return rotation;
    return atan2(parent->a * c - parent->c * a, parent->d * a - parent->b * c) * RAD_DEG;
}

float Bone::worldToLocalRotationY() const
{
    if (!parent) return rotation;
    return atan2(parent->a * d - parent->c * b, parent->d * b - parent->b * d) * RAD_DEG;
}

void Bone::rotateWorld(float degrees)
{
    float cosine = cos_deg(degrees), sine = sin_deg(degrees);

    float a = this->a, b = this->b, c = this->c, d = this->d;

    this->a = cosine * a - sine * c;
    this->b = cosine * b - sine * d;
    this->c = sine * a + cosine * c;
    this->d = sine * b + cosine * d;
}

/** Computes the local transform from the world transform. This can be useful to perform processing on the local transform
* after the world transform has been modified directly (eg, by a constraint).
* <p>
* Some redundant information is lost by the world transform, such as -1,-1 scale versus 180 rotation. The computed local
* transform values may differ from the original values but are functionally the same. */
void Bone::updateLocalTransform()
{
    if (!parent)
    {
        float det = a * d - b * c;
        translation = worldPos;
        rotation = atan2(c, a) * RAD_DEG;
        scale.x = sqrt(a * a + c * c);
        scale.y = sqrt(b * b + d * d);
        shear.x = 0;
        shear.y = atan2(a * b + c * d, det) * RAD_DEG;
    }
    else {
        float pa = parent->a, pb = parent->b, pc = parent->c, pd = parent->d;
        float pid = 1 / (pa * pd - pb * pc);
        float dx = worldPos.x - parent->worldPos.x, dy = worldPos.y - parent->worldPos.y;
        float ia = pid * pd;
        float id = pid * pa;
        float ib = pid * pb;
        float ic = pid * pc;
        float ra = ia * a - ib * c;
        float rb = ia * b - ib * d;
        float rc = id * c - ic * a;
        float rd = id * d - ic * b;
        translation.x = (dx * pd * pid - dy * pb * pid);
        translation.y = (dy * pa * pid - dx * pc * pid);
        shear.x = 0;
        scale.x = sqrt(ra * ra + rc * rc);
        if (scale.x > 0.0001f)
        {
            float det = ra * rd - rb * rc;
            scale.y = det / scale.x;
            shear.y = atan2(ra * rb + rc * rd, det) * RAD_DEG;
            rotation = atan2(rc, ra) * RAD_DEG;
        }
        else
        {
            scale.x = 0;
            scale.y = sqrt(rb * rb + rd * rd);
            shear.y = 0;
            rotation = 90 - atan2(rd, rb) * RAD_DEG;
        }
        appliedRotation = rotation;
    }
}

void Bone::worldToLocal(Vector world, Vector& outLocal)
{
    float invDet = 1 / (a * d - b * c);
    float x = world.x - worldPos.x, y = world.y - worldPos.y;
    outLocal.x = (x * d * invDet - y * b * invDet);
    outLocal.y = (y * a * invDet - x * c * invDet);
}

void Bone::localToWorld(Vector local, Vector& outWorld)
{
    outWorld.x = local.x * a + local.y * b + worldPos.x;
    outWorld.y = local.x * c + local.y * d + worldPos.y;
}

}
