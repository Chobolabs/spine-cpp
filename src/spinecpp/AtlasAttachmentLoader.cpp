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
#include <spinecpp/AtlasAttachmentLoader.h>
#include <spinecpp/Atlas.h>
#include <spinecpp/RegionAttachment.h>
#include <spinecpp/MeshAttachment.h>
#include <spinecpp/PathAttachment.h>
#include <spinecpp/BoundingBoxAttachment.h>

namespace spine
{

AtlasAttachmentLoader::AtlasAttachmentLoader(const Atlas& atlas)
    : m_atlas(atlas)
{
}

Attachment* AtlasAttachmentLoader::createAttachmentImpl(const Skin& skin, Attachment::Type type, const std::string& name, const std::string& path)
{
    switch (type) {
    case Attachment::Type::Region:
    {
        auto region = m_atlas.findRegion(path);

        if (!region)
        {
            setError("Region not found: ", path);
            return nullptr;
        }

        auto attachment = new RegionAttachment(name, path);

        attachment->rendererObject = region;
        attachment->setUVs(region->u, region->v, region->u2, region->v2, region->rotate);
        attachment->regionOffsetX = region->offsetX;
        attachment->regionOffsetY = region->offsetY;
        attachment->regionWidth = region->width;
        attachment->regionHeight = region->height;
        attachment->regionOriginalWidth = region->originalWidth;
        attachment->regionOriginalHeight = region->originalHeight;

        return attachment;
    }
    case Attachment::Type::Mesh:
    case Attachment::Type::LinkedMesh:
    {
        auto region = m_atlas.findRegion(path);

        if (!region)
        {
            setError("Region for mesh not found: ", path);
            return nullptr;
        }

        auto attachment = new MeshAttachment(name, path);
        attachment->rendererObject = region;
        attachment->regionUV.x = region->u;
        attachment->regionUV.y = region->v;
        attachment->regionUV2.x = region->u2;
        attachment->regionUV2.y = region->v2;
        attachment->regionRotate = region->rotate;
        attachment->regionOffsetX = region->offsetX;
        attachment->regionOffsetY = region->offsetY;
        attachment->regionWidth = region->width;
        attachment->regionHeight = region->height;
        attachment->regionOriginalWidth = region->originalWidth;
        attachment->regionOriginalHeight = region->originalHeight;
        return attachment;
    }
    case Attachment::Type::BoundingBox:
        return new BoundingBoxAttachment(name);
    case Attachment::Type::Path:
        return new PathAttachment(name);
    default:
        setUnknownTypeError(type);
        return nullptr;
    }
}

}