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
#include <spinecpp/RegionAttachment.h>
#include <spinecpp/Slot.h>
#include <spinecpp/Bone.h>
#include <spinecpp/Skeleton.h>
#include <spinecpp/extension.h>
#include <cmath>

namespace spine
{

RegionAttachment::RegionAttachment(const std::string& name, const std::string& path)
    : Attachment(name, Attachment::Type::Region)
    , path(path)
{
}

void RegionAttachment::setUVs(float u, float v, float u2, float v2, bool rotate)
{
    if (rotate) {
        uvs[1].x = u;
        uvs[1].y = v2;
        uvs[2].x = u;
        uvs[2].y = v;
        uvs[3].x = u2;
        uvs[3].y = v;
        uvs[0].x = u2;
        uvs[0].y = v2;
    }
    else {
        uvs[0].x = u;
        uvs[0].y = v2;
        uvs[1].x = u;
        uvs[1].y = v;
        uvs[2].x = u2;
        uvs[2].y = v;
        uvs[3].x = u2;
        uvs[3].y = v2;
    }
}


void RegionAttachment::updateOffset()
{
    float regionScaleX = size.x / regionOriginalWidth * scale.x;
    float regionScaleY = size.y / regionOriginalHeight * scale.y;
    float localX = -size.x / 2 * scale.x + regionOffsetX * regionScaleX;
    float localY = -size.y / 2 * scale.y + regionOffsetY * regionScaleY;
    float localX2 = localX + regionWidth * regionScaleX;
    float localY2 = localY + regionHeight * regionScaleY;
    float radians = rotation * DEG_RAD;
    float cosine = std::cos(radians), sine = std::sin(radians);
    float localXCos = localX * cosine + translation.x;
    float localXSin = localX * sine;
    float localYCos = localY * cosine + translation.y;
    float localYSin = localY * sine;
    float localX2Cos = localX2 * cosine + translation.x;
    float localX2Sin = localX2 * sine;
    float localY2Cos = localY2 * cosine + translation.y;
    float localY2Sin = localY2 * sine;
    offset[0].x = localXCos - localYSin;
    offset[0].y = localYCos + localXSin;
    offset[1].x = localXCos - localY2Sin;
    offset[1].y = localY2Cos + localXSin;
    offset[2].x = localX2Cos - localY2Sin;
    offset[2].y = localY2Cos + localX2Sin;
    offset[3].x = localX2Cos - localYSin;
    offset[3].y = localYCos + localX2Sin;
}

void RegionAttachment::computeWorldVertices(const Bone& bone, float* vertices) const
{
    float x = bone.skeleton.translation.x + bone.worldPos.x;
    float y = bone.skeleton.translation.y + bone.worldPos.y;

    vertices[0] = offset[0].x * bone.a + offset[0].y * bone.b + x;
    vertices[1] = offset[0].x * bone.c + offset[0].y * bone.d + y;
    vertices[2] = offset[1].x * bone.a + offset[1].y * bone.b + x;
    vertices[3] = offset[1].x * bone.c + offset[1].y * bone.d + y;
    vertices[4] = offset[2].x * bone.a + offset[2].y * bone.b + x;
    vertices[5] = offset[2].x * bone.c + offset[2].y * bone.d + y;
    vertices[6] = offset[3].x * bone.a + offset[3].y * bone.b + x;
    vertices[7] = offset[3].x * bone.c + offset[3].y * bone.d + y;
}

}
