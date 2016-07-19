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

#include "SkeletonData.h"
#include "Slot.h"
#include "Skin.h"
#include "IkConstraint.h"
#include "TransformConstraint.h"
#include "PathConstraint.h"

namespace spine
{

class Skeleton
{
public:
    Skeleton(const SkeletonData& data);

    /* Caches information about bones and constraints. Must be called if bones or constraints, or weighted path attachments
    * are added or removed. */
    void updateCache();
    void updateWorldTransform();

    /* Sets the bones, constraints, and slots to their setup pose values. */
    void setToSetupPose();
    /* Sets the bones and constraints to their setup pose values. */
    void setBonesToSetupPose();
    void setSlotsToSetupPose();

    // sets draw order to identity
    void resetDrawOrder();

    // assumes input points to a buffer of indices of size at least m_slots.size();
    void setDrawOrder(const int* drawOrder);

    /* Returns 0 if the bone was not found. */
    Bone* findBone(const std::string& name);
    const Bone* findBone(const std::string& name) const;
    /* Returns -1 if the bone was not found. */
    int findBoneIndex(const std::string& name) const;

    const Bone& getRoot() const { return bones.front(); }
    Bone& getRoot() { return bones.front(); }

    /* Returns 0 if the slot was not found. */
    const Slot* findSlot(const std::string& name) const;
    /* Returns -1 if the slot was not found. */
    int findSlotIndex(const std::string& name) const;

    /* Sets the skin used to look up attachments before looking in the SkeletonData defaultSkin. Attachments from the new skin are
    * attached if the corresponding attachment from the old skin was attached. If there was no old skin, each slot's setup mode
    * attachment is attached from the new skin.
    * @param skin May be 0.*/
    void setSkin(const Skin* skin);
    /* Returns false if the skin was not found. See setSkin.
    * @param skinName May be empty. */
    bool setSkinByName(const std::string& name);

    const Skin* getSkin() { return m_skin; }

    /* Returns 0 if the slot or attachment was not found. */
    const Attachment* getAttachmentForSlotName(const std::string& slotName, const std::string& attachmentName) const;
    /* Returns 0 if the slot or attachment was not found. */
    const Attachment* getAttachmentForSlotIndex(int slotIndex, const std::string& attachmentName) const;
    /* Returns 0 if the slot or attachment was not found.
    * @param attachmentName May be 0. */
    bool setAttachment(const std::string& slotName, const std::string& attachmentName);

    /* Returns 0 if the IK constraint was not found. */
    const IkConstraint* findIkConstraint(const std::string& name) const;

    /* Returns 0 if the transform constraint was not found. */
    const TransformConstraint* findTransformConstraint(const std::string& name) const;

    const PathConstraint* findPathConstraint(const std::string& name) const;

    void update(float deltaTime);

    const SkeletonData& data;

    std::vector<Bone> bones;

    std::vector<Slot> slots;
    std::vector<Slot*> drawOrder; // pointers from to m_slots

    std::vector<IkConstraint> ikConstraints;
    std::vector<IkConstraint*> ikConstraintsSorted;

    std::vector<TransformConstraint> transformConstraints;

    std::vector<PathConstraint> pathConstraints;

    Color color;

    float time = 0;

    bool flipX = false, flipY = false;
    Vector translation = Vector(0, 0);

private:
    const Skin* m_skin = nullptr;

    enum class UpdateCacheType
    {
        Bone,
        IkConstraint,
        PathConstraint,
        TransformConstraint,
    };

    void sortBone(Bone& b);
    void sortReset(std::vector<Bone*>& bones);
    void sortPathConstraintAttachment(const Skin& skin, int slotIndex, Bone& slotBone);
    void sortPathConstraintAttachmentBones(const Attachment* attachment, Bone& slotBone);

    struct UpdateCacheElem
    {
        UpdateCacheElem(Bone& b)
            : data(&b)
            , type(UpdateCacheType::Bone)
        {}

        UpdateCacheElem(IkConstraint& ik)
            : data(&ik)
            , type(UpdateCacheType::IkConstraint)
        {}

        UpdateCacheElem(PathConstraint& p)
            : data(&p)
            , type(UpdateCacheType::PathConstraint)
        {}

        UpdateCacheElem(TransformConstraint& t)
            : data(&t)
            , type(UpdateCacheType::TransformConstraint)
        {}

        void* data;
        UpdateCacheType type;
    };
    std::vector<UpdateCacheElem> m_updateCache;
};

}
