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

#include <spinecpp/BoneData.h>
#include <spinecpp/SlotData.h>
#include <spinecpp/Skin.h>
#include <spinecpp/EventData.h>
#include <spinecpp/Animation.h>
#include <spinecpp/IkConstraintData.h>
#include <spinecpp/TransformConstraintData.h>

namespace spine
{

struct SkeletonData
{
    std::string version;
    std::string hash;

    Vector size = Vector(0, 0);

    std::vector<BoneData> bones;
    std::vector<SlotData> slots;
    std::vector<Skin> skins;
    Skin* defaultSkin = nullptr;
    std::vector<EventData> events;
    std::vector<Animation> animations;
    std::vector<IkConstraintData> ikConstraints;
    std::vector<TransformConstraintData> transformConstraints;

    const BoneData* findBone(const char* boneName) const;
    int findBoneIndex(const char* boneName) const;

    const SlotData* findSlot(const char* slotName) const;
    int findSlotIndex(const char* slotName) const;

    const Skin* findSkin(const char* skinName) const;

    const EventData* findEvent(const char* eventName) const;

    const Animation* findAnimation(const char* animationName) const;

    const IkConstraintData* findIkConstraint(const char* constraintName) const;
    int findIkConstraintIndex(const char* constraintName) const;

    const TransformConstraintData* findTransformConstraint(const char* constraintName) const;
    int findTransformConstraintIndex(const char* constraintName) const;
};

}
