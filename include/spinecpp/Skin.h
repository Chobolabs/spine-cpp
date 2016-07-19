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

class Skeleton;
class SkinEntry;
class Attachment;

class Skin
{
public:
    Skin(const std::string& name);
    ~Skin();

    const std::string name;

    // The skin will take ownership of the attachment.
    void addAttachment(int slotIndex, const std::string& name, Attachment* attachment);

    // Returns nullptr if the attachment was not found.
    const Attachment* getAttachment(int slotIndex, const char* attachmentName) const;

    // Returns nullptr if the slot or attachment was not found.
    const char* getAttachmentName(int slotIndex, int attachmentIndex) const;

    // Attach each attachment in this skin if the corresponding attachment in oldSkin is currently attached.
    void attachAll(Skeleton& skeleton, const Skin& oldSkin) const;

private:
    friend class SkeletonJson;
    friend class Skeleton;

    struct SkinEntry
    {
        SkinEntry(int i, const std::string& n, Attachment* a)
            : slotIndex(i)
            , name(n)
            , attachment(a)
        {}

        int slotIndex;
        std::string name;
        // The Skin owns the attachment.
        Attachment* attachment;
    };

    std::vector<SkinEntry> m_entries;
};

}