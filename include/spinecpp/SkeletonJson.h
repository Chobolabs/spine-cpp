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
#pragma once

#include <string>
#include <vector>

namespace spine
{

class AttachmentLoader;
class Atlas;
struct SkeletonData;
class Animation;
struct CurveFrame;
class MeshAttachment;
class VertexAttachment;

namespace sajson
{
class value;
}

class SkeletonJson
{
public:
    SkeletonJson(AttachmentLoader& loader);
    SkeletonJson(const Atlas& atlas);
    ~SkeletonJson();

    const std::string& getError() const { return m_error; }

    SkeletonData* readSkeletonData(const std::string& json);
    SkeletonData* readSkeletonDataFile(const std::string& path);

    void setScale(float s) { m_scale = s; }

private:
    void setError(const std::string& e1, const std::string& e2);

    void readAnimation(Animation& outAnim, const SkeletonData& skeletonData, const sajson::value& json);
    void readCurve(CurveFrame& frame, const sajson::value& json);
    void readVertices(const sajson::value& json, VertexAttachment& attachment, int verticesLength);

    struct LinkedMesh
    {
        LinkedMesh(MeshAttachment* mesh, const char* skin, int slotIndex, const char* parent)
            : mesh(mesh)
            , skin(skin)
            , slotIndex(slotIndex)
            , parent(parent)
        {}

        MeshAttachment* mesh;
        const char* skin;
        int slotIndex;
        const char* parent;
    };

    float m_scale = 1.f;
    bool m_ownsLoader;
    AttachmentLoader* m_loader;
    std::string m_error;

    std::vector<LinkedMesh> m_linkedMeshes;
};

}