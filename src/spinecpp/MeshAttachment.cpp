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
#include <spinecpp/MeshAttachment.h>
#include <spinecpp/Slot.h>
#include <spinecpp/Bone.h>
#include <spinecpp/Skeleton.h>

namespace spine
{

MeshAttachment::MeshAttachment(const std::string& name, const std::string& path)
    : Attachment(name, Attachment::Type::Mesh)
    , path(path)
{
}

void MeshAttachment::updateUVs()
{
    auto size = regionUV2 - regionUV;

    uvs.clear();
    uvs.resize(vertices.size());

    if (regionRotate)
    {
        for (size_t i = 0; i < uvs.size(); ++i)
        {
            auto& uv = uvs[i];
            const auto& regionUV = regionUVs[i];

            uv.x = regionUV.x + regionUV.y * size.x;
            uv.y = regionUV.y + size.y - regionUV.x * size.y;
        }
    }
    else
    {
        for (size_t i = 0; i < uvs.size(); ++i)
        {
            auto& uv = uvs[i];
            const auto& regionUV = regionUVs[i];

            uv.x = regionUV.x + regionUV.x * size.x;
            uv.y = regionUV.y + regionUV.y * size.y;
        }
    }
}

void MeshAttachment::computeWorldVertices(const Slot& slot, float* worldVertices) const
{
    auto& bone = slot.bone;
    float x = bone.skeleton.translation.x + bone.worldPos.x;
    float y = bone.skeleton.translation.y + bone.worldPos.y;

    const auto& verts =
        slot.attachmentVertices.size() == vertices.size()
        ?
        slot.attachmentVertices
        :
        vertices;

    int i = 0;
    for (auto& vertex : verts)
    {
        worldVertices[i] = vertex.x * bone.a + vertex.y * bone.b + x;
        worldVertices[i + 1] = vertex.x * bone.c + vertex.y * bone.d + y;
        i += 2;
    }
}

}
