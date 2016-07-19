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
#include <spinecpp/SkeletonData.h>

namespace
{
    template <typename T>
    const T* findByName(const std::vector<T>& v, const char* name)
    {
        for (auto& t : v)
        {
            if (t.name == name)
            {
                return &t;
            }
        }

        return nullptr;
    }

    template <typename T>
    int findIndexByName(const std::vector<T>& v, const char* name)
    {
        for (size_t i = 0; i < v.size(); ++i)
        {
            if (v[i].name == name)
            {
                return int(i);
            }
        }

        return -1;
    }
}

namespace spine
{

const BoneData* SkeletonData::findBone(const char* boneName) const
{
    return findByName(bones, boneName);
}

int SkeletonData::findBoneIndex(const char* boneName) const
{
    return findIndexByName(bones, boneName);
}

const SlotData* SkeletonData::findSlot(const char* slotName) const
{
    return findByName(slots, slotName);
}

int SkeletonData::findSlotIndex(const char* slotName) const
{
    return findIndexByName(slots, slotName);
}

const Skin* SkeletonData::findSkin(const char* skinName) const
{
    return findByName(skins, skinName);
}

const EventData* SkeletonData::findEvent(const char* eventName) const
{
    return findByName(events, eventName);
}

const Animation* SkeletonData::findAnimation(const char* animationName) const
{
    return findByName(animations, animationName);
}

const IkConstraintData* SkeletonData::findIkConstraint(const char* constraintName) const
{
    return findByName(ikConstraints, constraintName);
}

int SkeletonData::findIkConstraintIndex(const char* constraintName) const
{
    return findIndexByName(ikConstraints, constraintName);
}

const TransformConstraintData* SkeletonData::findTransformConstraint(const char* constraintName) const
{
    return findByName(transformConstraints, constraintName);
}

int SkeletonData::findTransformConstraintIndex(const char* constraintName) const
{
    return findIndexByName(transformConstraints, constraintName);
}

const PathConstraintData* SkeletonData::findPathConstraint(const char* constraintName) const
{
    return findByName(pathConstraints, constraintName);
}


int SkeletonData::findPathConstraintIndex(const char* constraintName) const
{
    return findIndexByName(pathConstraints, constraintName);
}

}
