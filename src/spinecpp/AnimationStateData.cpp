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
#include <spinecpp/AnimationStateData.h>
#include <spinecpp/Animation.h>
#include <spinecpp/SkeletonData.h>

namespace spine
{

AnimationStateData::AnimationStateData(const SkeletonData& skeletonData)
    : skeletonData(skeletonData)
{
}

void AnimationStateData::setMixByName(const std::string& fromName, const std::string& toName, float duration)
{
    auto from = skeletonData.findAnimation(fromName.c_str());
    if (!from) return;
    auto to = skeletonData.findAnimation(toName.c_str());
    if (!to) return;

    setMix(from, to, duration);
}

void AnimationStateData::setMix(const Animation* from, const Animation* to, float duration)
{
    /* Find existing FromEntry. */
    FromEntry* fromEntry = nullptr;
    for (auto& fe : m_fromEntries)
    {
        if (fe.animation == from)
        {
            for (auto& te : fe.toEntries)
            {
                if (te.animation == to)
                {
                    te.duration = duration;
                    return;
                }
            }

            fromEntry = &fe;
            break; /* Add new ToEntry to the existing FromEntry. */
        }
    }

    if (!fromEntry)
    {
        m_fromEntries.resize(m_fromEntries.size() + 1);
        fromEntry = &m_fromEntries.back();
        fromEntry->animation = from;
    }

    auto& toEntries = fromEntry->toEntries;
    toEntries.resize(toEntries.size() + 1);
    toEntries.back().animation = to;
    toEntries.back().duration = duration;
}

float AnimationStateData::getMix(const Animation* from, const Animation* to) const
{
    for (auto& fe : m_fromEntries)
    {
        if (fe.animation == from)
        {
            for (auto& te : fe.toEntries)
            {
                if (te.animation == to)
                {
                    return te.duration;
                }
            }

            break; /* we shouldn't have two entries for the same animation */
        }
    }

    return defaultMix;
}

}
