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
#include <spinecpp/WeightedMeshAttachment.h>
#include <spinecpp/Slot.h>
#include <spinecpp/Bone.h>
#include <spinecpp/Skeleton.h>

namespace spine
{

WeightedMeshAttachment::WeightedMeshAttachment(const std::string& name, const std::string& path)
    : Attachment(name, Attachment::Type::WeightedMesh)
    , path(path)
{
}

void WeightedMeshAttachment::updateUVs()
{
    auto size = regionUV2 - regionUV;

    uvs.clear();
    uvs.resize(regionUVs.size());

    if (regionRotate)
    {
        for (size_t i = 0; i < uvs.size(); ++i)
        {
            auto& uv = uvs[i];
            const auto& ruv = regionUVs[i];

            uv.x = regionUV.x + ruv.y * size.x;
            uv.y = regionUV.y + size.y - ruv.x * size.y;
        }
    }
    else
    {
        for (size_t i = 0; i < uvs.size(); ++i)
        {
            auto& uv = uvs[i];
            const auto& ruv = regionUVs[i];

            uv.x = regionUV.x + ruv.x * size.x;
            uv.y = regionUV.y + ruv.y * size.y;
        }
    }
}

void WeightedMeshAttachment::computeWorldVertices(const Slot& slot, float* outWorldVertices) const
{
    auto& bone = slot.bone;
    float x = bone.skeleton.translation.x;
    float y = bone.skeleton.translation.y;

    auto& skeleton = bone.skeleton;
    auto& skeletonBones = skeleton.bones;

    int w = 0, v = 0, b = 0, f = 0;
    if (slot.attachmentVertices.empty())
    {
        for (; v < int(bones.size()); w += 2)
        {
            float wx = 0, wy = 0;
            const int nn = bones[v] + v;
            v++;
            for (; v <= nn; v++, b += 3)
            {
                auto& bone = skeletonBones[bones[v]];

                const float vx = weights[b], vy = weights[b + 1], weight = weights[b + 2];
                wx += (vx * bone.a + vy * bone.b + bone.worldPos.x) * weight;
                wy += (vx * bone.c + vy * bone.d + bone.worldPos.y) * weight;
            }

            outWorldVertices[w] = wx + x;
            outWorldVertices[w + 1] = wy + y;
        }
    }
    else {
        auto& vertices = slot.attachmentVertices;

        for (; v < int(bones.size()); w += 2) {
            float wx = 0, wy = 0;
            const int nn = bones[v] + v;
            v++;
            for (; v <= nn; v++, b += 3, ++f) {
                auto& bone = skeletonBones[bones[v]];

                const float vx = weights[b] + vertices[f].x;
                const float vy = weights[b + 1] + vertices[f].y;
                const float weight = weights[b + 2];

                wx += (vx * bone.a + vy * bone.b + bone.worldPos.x) * weight;
                wy += (vx * bone.c + vy * bone.d + bone.worldPos.y) * weight;
            }

            outWorldVertices[w] = wx + x;
            outWorldVertices[w + 1] = wy + y;
        }
    }
}

}
