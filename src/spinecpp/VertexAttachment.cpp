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

#include <spinecpp/VertexAttachment.h>
#include <spinecpp/Slot.h>
#include <spinecpp/Bone.h>
#include <spinecpp/Skeleton.h>

namespace spine
{

VertexAttachment::VertexAttachment(const std::string& name, const Type type)
    : Attachment(name, type)
{
}

void VertexAttachment::computeWorldVertices(const Slot& slot, float* outWorldVertices) const
{
    computeWorldVertices(0, worldVerticesCount * 2, slot, outWorldVertices, 0);
}

void VertexAttachment::computeWorldVertices(int start, int count, const Slot& slot, float* outWorldVertices, int offset) const
{
    count += offset;
    auto& skeleton = slot.bone.skeleton;
    auto x = skeleton.translation.x;
    auto y = skeleton.translation.y;
    auto deformLength = slot.attachmentVertices.size() * 2;
    auto fvertices = vertices.data();
    auto deform = reinterpret_cast<const float*>(slot.attachmentVertices.data());
    
    if (bones.empty())
    {
        if (deformLength > 0) fvertices = deform;
        auto& bone = slot.bone;
        x += bone.worldPos.x;
        y += bone.worldPos.y;
        for (int v = start, w = offset; w < count; v += 2, w += 2) {
            float vx = fvertices[v], vy = fvertices[v + 1];
            outWorldVertices[w] = vx * bone.a + vy * bone.b + x;
            outWorldVertices[w + 1] = vx * bone.c + vy * bone.d + y;
        }
    }
    else
    {
        int v = 0, skip = 0;
        for (int i = 0; i < start; i += 2)
        {
            int n = bones[v];
            v += n + 1;
            skip += n;
        }

        auto& skeletonBones = skeleton.bones;
        if (deformLength == 0)
        {
            for (int w = offset, b = skip * 3; w < count; w += 2) {
                float wx = x, wy = y;
                int n = bones[v++];
                n += v;
                for (; v < n; v++, b += 3)
                {
                    auto& bone = skeletonBones[bones[v]];
                    float vx = fvertices[b], vy = fvertices[b + 1], weight = fvertices[b + 2];
                    wx += (vx * bone.a + vy * bone.b + bone.worldPos.x) * weight;
                    wy += (vx * bone.c + vy * bone.d + bone.worldPos.y) * weight;
                }
                outWorldVertices[w] = wx;
                outWorldVertices[w + 1] = wy;
            }
        }
        else
        {
            for (int w = offset, b = skip * 3, f = skip << 1; w < count; w += 2) {
                float wx = x, wy = y;
                int n = bones[v++];
                n += v;
                for (; v < n; v++, b += 3, f += 2)
                {
                    auto& bone = skeletonBones[bones[v]];
                    float vx = fvertices[b] + deform[f], vy = fvertices[b + 1] + deform[f + 1], weight = fvertices[b + 2];
                    wx += (vx * bone.a + vy * bone.b + bone.worldPos.x) * weight;
                    wy += (vx * bone.c + vy * bone.d + bone.worldPos.y) * weight;
                }
                outWorldVertices[w] = wx;
                outWorldVertices[w + 1] = wy;
            }
        }
    }
}

}
