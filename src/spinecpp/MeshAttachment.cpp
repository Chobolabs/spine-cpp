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
#include <cassert>

namespace spine
{

MeshAttachment::MeshAttachment(const std::string& name, const std::string& path)
    : VertexAttachment(name, Attachment::Type::Mesh)
    , path(path)
{
}

void MeshAttachment::updateUVs()
{
    auto size = regionUV2 - regionUV;

    uvs.clear();
    uvs.resize(worldVerticesCount);

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

void MeshAttachment::setParentMesh(const MeshAttachment* parentMesh)
{
    m_parentMesh = parentMesh;
    if (parentMesh)
    {
        worldVerticesCount = parentMesh->worldVerticesCount;
        
        bones = parentMesh->bones;
        vertices = parentMesh->vertices;

        regionUVs = parentMesh->regionUVs;
        triangles = parentMesh->triangles;

        hullLength = parentMesh->hullLength;

        edges = parentMesh->edges;
        
        size = parentMesh->size;
    }
}

}
