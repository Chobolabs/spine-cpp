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
#include <spinecpp/Skeleton.h>

#include <cassert>

namespace spine
{

Skeleton::Skeleton(const SkeletonData& data)
    : data(data)
    , color(1, 1, 1, 1)
{
    bones.reserve(data.bones.size());
    for (auto& bd : data.bones)
    {
        Bone* parent = nullptr;
        // find parent
        for (auto& b : bones)
        {
            if (&b.data == bd.parent)
            {
                parent = &b;
                break;
            }
        }

        bones.emplace_back(bd, *this, parent);
    }

    slots.reserve(data.slots.size());
    for (auto& sd : data.slots)
    {
        Bone* bone = nullptr;

        for (auto& b : bones)
        {
            if (&b.data == sd.boneData)
            {
                bone = &b;
                break;
            }
        }

        assert(bone);
        if (bone)
        {
            slots.emplace_back(sd, *bone);
        }
    }

    drawOrder.reserve(slots.size());
    for (auto& s : slots)
    {
        drawOrder.emplace_back(&s);
    }

    ikConstraints.reserve(data.ikConstraints.size());
    for (auto& ikd : data.ikConstraints)
    {
        ikConstraints.emplace_back(ikd, *this);
    }

    transformConstraints.reserve(data.transformConstraints.size());
    for (auto& tcd : data.transformConstraints)
    {
        transformConstraints.emplace_back(tcd, *this);
    }

    updateCache();
}

void Skeleton::updateCache()
{
    m_updateCache.clear();
    m_updateCache.reserve(bones.size() + ikConstraints.size() + transformConstraints.size());

    for (auto& bone : bones)
    {
        m_updateCache.emplace_back(bone);
        for (auto& ik : ikConstraints)
        {
            if (&bone == ik.bones.back())
            {
                m_updateCache.emplace_back(ik);
                break;
            }
        }
    }

    for (auto& tc : transformConstraints)
    {
        for (auto i = m_updateCache.rbegin(); i != m_updateCache.rend(); ++i)
        {
            auto updatable = i->data;
            if (updatable == tc.bone)
            {
                m_updateCache.emplace(i.base(), tc);
                break;
            }
        }
    }
}

void Skeleton::updateWorldTransform()
{
    for (auto& c : m_updateCache)
    {
        switch (c.type)
        {
        case UpdateCacheType::Bone:
            reinterpret_cast<Bone*>(c.data)->updateWorldTransform();
            break;
        case UpdateCacheType::IkConstraint:
            reinterpret_cast<IkConstraint*>(c.data)->apply();
            break;
        case UpdateCacheType::TransformConstraint:
            reinterpret_cast<TransformConstraint*>(c.data)->apply();
            break;
        }
    }
}

void Skeleton::setToSetupPose()
{
    setBonesToSetupPose();
    setSlotsToSetupPose();
}

void Skeleton::setBonesToSetupPose()
{
    for (auto& b : bones)
    {
        b.setToSetupPose();
    }

    for (auto& ik : ikConstraints)
    {
        ik.bendDirection = ik.data.bendDirection;
        ik.mix = ik.data.mix;
    }

    for (auto& tc : transformConstraints)
    {
        tc.rotateMix = tc.data.rotateMix;
        tc.translateMix = tc.data.translateMix;
        tc.scaleMix = tc.data.scaleMix;
        tc.shearMix = tc.data.shearMix;
    }
}

void Skeleton::setSlotsToSetupPose()
{
    drawOrder.clear();
    for (auto& slot : slots)
    {
        slot.setToSetupPose();
        drawOrder.emplace_back(&slot);
    }
}

void Skeleton::resetDrawOrder()
{
    drawOrder.clear();
    for (auto& slot : slots)
    {
        drawOrder.emplace_back(&slot);
    }
}

void Skeleton::setDrawOrder(const int* drawOrder)
{
    for (size_t i = 0; i < slots.size(); ++i)
    {
        this->drawOrder[i] = &slots[drawOrder[i]];
    }
}

namespace
{
template <typename T>
const T* findByName(const std::vector<T>& v, const std::string& name)
{
    for (auto& t : v)
    {
        if (t.getName() == name)
        {
            return &t;
        }
    }

    return nullptr;
}

template <typename T>
T* findByName(std::vector<T>& v, const std::string& name)
{
    for (auto& t : v)
    {
        if (t.getName() == name)
        {
            return &t;
        }
    }

    return nullptr;
}


template <typename T>
int findIndexByName(const std::vector<T>& v, const std::string& name)
{
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (v[i].getName() == name)
        {
            return int(i);
        }
    }

    return -1;
}
}

Bone* Skeleton::findBone(const std::string& name)
{
    return findByName(bones, name);
}

const Bone* Skeleton::findBone(const std::string& name) const
{
    return findByName(bones, name);
}

int Skeleton::findBoneIndex(const std::string& name) const
{
    return findIndexByName(bones, name);
}

const Slot* Skeleton::findSlot(const std::string& name) const
{
    return findByName(slots, name);
}

int Skeleton::findSlotIndex(const std::string& name) const
{
    return findIndexByName(slots, name);
}

bool Skeleton::setSkinByName(const std::string& name)
{
    if (name.empty())
    {
        setSkin(nullptr);
        return true;
    }

    auto skin = data.findSkin(name.c_str());
    if (!skin)
    {
        return false;
    }

    setSkin(skin);
    return true;
}

void Skeleton::setSkin(const Skin* skin)
{
    if (skin)
    {
        if (m_skin)
        {
            skin->attachAll(*this, *m_skin);
        }
        else
        {
            /* No previous skin, attach setup pose attachments. */
            for (size_t i = 0; i < slots.size(); ++i)
            {
                auto& slot = slots[i];
                if (!slot.data.attachmentName.empty())
                {
                    auto attachment = skin->getAttachment(int(i), slot.data.attachmentName.c_str());
                    if (attachment)
                    {
                        slot.setAttachment(attachment);
                    }
                }
            }
        }
    }

    m_skin = skin;
}

const Attachment* Skeleton::getAttachmentForSlotName(const std::string& slotName, const std::string& attachmentName) const
{
    int slotIndex = data.findSlotIndex(slotName.c_str());
    return getAttachmentForSlotIndex(slotIndex, attachmentName);
}

const Attachment* Skeleton::getAttachmentForSlotIndex(int slotIndex, const std::string& attachmentName) const
{
    if (slotIndex == -1) return nullptr;

    if (m_skin)
    {
        auto attachment = m_skin->getAttachment(slotIndex, attachmentName.c_str());
        if (attachment) return attachment;
    }

    if (data.defaultSkin)
    {
        auto attachment = data.defaultSkin->getAttachment(slotIndex, attachmentName.c_str());
        if (attachment) return attachment;
    }

    return nullptr;
}

bool Skeleton::setAttachment(const std::string& slotName, const std::string& attachmentName)
{
    for (size_t i = 0; i < slots.size(); ++i)
    {
        auto& slot = slots[i];
        if (slot.getName() == slotName)
        {
            if (attachmentName.empty())
            {
                slot.setAttachment(nullptr);
            }
            else
            {
                auto attachment = getAttachmentForSlotIndex(int(i), attachmentName);
                if (!attachment) return false;
                slot.setAttachment(attachment);
            }

            return true;
        }
    }

    return false;
}

const IkConstraint* Skeleton::findIkConstraint(const std::string& name) const
{
    return findByName(ikConstraints, name);
}

const TransformConstraint* Skeleton::findTransformConstraint(const std::string& name) const
{
    return findByName(transformConstraints, name);
}

void Skeleton::update(float deltaTime)
{
    time += deltaTime;
}

}