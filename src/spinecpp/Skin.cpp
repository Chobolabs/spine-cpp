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
#include <spinecpp/Skin.h>
#include <spinecpp/Attachment.h>
#include <spinecpp/Skeleton.h>
#include <spinecpp/AttachmentLoader.h>

namespace spine
{

Skin::Skin(const std::string& name)
    : name(name)
{

}

Skin::~Skin()
{
    for (auto& e : m_entries)
    {
        if (e.attachment->loader)
        {
            e.attachment->loader->onDestroyingAttachment(e.attachment);
        }
        delete e.attachment;
    }
}

void Skin::addAttachment(int slotIndex, const std::string& name, Attachment* attachment)
{
    m_entries.emplace_back(slotIndex, name, attachment);
}

const Attachment* Skin::getAttachment(int slotIndex, const char* attachmentName) const
{
    for (auto& e : m_entries)
    {
        if (e.slotIndex == slotIndex && e.name == attachmentName)
        {
            return e.attachment;
        }
    }

    return nullptr;
}

/* Returns nullptr if the slot or attachment was not found. */
const char* Skin::getAttachmentName(int slotIndex, int attachmentIndex) const
{
    int i = 0;
    for (auto& e : m_entries)
    {
        if (e.slotIndex == slotIndex)
        {
            if (i == attachmentIndex)
            {
                return e.name.c_str();
            }
            ++i;
        }
    }

    return nullptr;
}

void Skin::attachAll(Skeleton& skeleton, const Skin& oldSkin) const
{
    for (auto& e : oldSkin.m_entries)
    {
        auto& slot = skeleton.slots[e.slotIndex];
        if (slot.getAttachment() == e.attachment)
        {
            auto newAttachment = getAttachment(e.slotIndex, e.name.c_str());
            if (newAttachment)
            {
                slot.setAttachment(newAttachment);
            }
        }
    }
}

}
