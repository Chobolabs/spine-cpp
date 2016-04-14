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
#include <spinecpp/AnimationState.h>
#include <spinecpp/Animation.h>
#include <spinecpp/SkeletonData.h>
#include <spinecpp/AnimationStateData.h>
#include <spinecpp/TrackEntryFactory.h>
#include <cassert>
#include <cmath>

namespace spine
{

TrackEntry::TrackEntry(const AnimationState& state, const Animation& anim)
    : state(state)
    , animation(anim)
{
}

TrackEntry::~TrackEntry()
{
    if (previous)
    {
        state.trackEntryFactory.destroyTrackEntry(previous);
    }
}

namespace
{

class DefaultTrackEntryFactory : public TrackEntryFactory
{
    // default new/delete track entry management
    virtual TrackEntry* createTrackEntry(const AnimationState& state, const Animation& anim) override
    {
        return new TrackEntry(state, anim);
    }

    virtual void destroyTrackEntry(TrackEntry* entry) override
    {
        delete entry;
    }

} defaultTrackEntryFactory;

}

AnimationState::AnimationState(const AnimationStateData& data, TrackEntryFactory* factory)
    : data(data)
    , trackEntryFactory(factory ? *factory : defaultTrackEntryFactory)
{
    m_events.reserve(64);
}

AnimationState::~AnimationState()
{
    clearTracks();
}

void AnimationState::disposeAllEntries(TrackEntry* entry)
{
    while (entry)
    {
        auto n = entry->next;
        trackEntryFactory.destroyTrackEntry(entry);
        entry = n;
    }
}

void AnimationState::update(float delta)
{
    delta *= timeScale;
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto current = tracks[i];
        if (!current) continue;

        current->time += delta * current->timeScale;
        if (current->previous) {
            float previousDelta = delta * current->previous->timeScale;
            current->previous->time += previousDelta;
            current->mixTime += previousDelta;
        }

        if (current->next) {
            current->next->time = current->lastTime - current->next->delay;
            if (current->next->time >= 0)
            {
                setCurrent(int(i), current->next);
            }
        }
        else {
            /* End non-looping animation when it reaches its end time and there is no next entry. */
            if (!current->loop && current->lastTime >= current->endTime)
            {
                clearTrack(int(i));
            }
        }
    }
}

void AnimationState::apply(Skeleton& skeleton)
{
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto current = tracks[i];
        if (!current) continue;

        m_events.clear(); // prepare events for current frame

        float time = current->time;
        if (!current->loop && time > current->endTime)
        {
            time = current->endTime;
        }

        auto previous = current->previous;
        if (!previous)
        {
            current->animation.mix(skeleton, current->lastTime, time, current->loop, &m_events, current->mix);
        }
        else
        {
            float alpha = current->mixTime / current->mixDuration * current->mix;

            float previousTime = previous->time;
            if (!previous->loop && previousTime > previous->endTime)
            {
                previousTime = previous->endTime;
            }

            previous->animation.apply(skeleton, previousTime, previousTime, previous->loop, nullptr);

            if (alpha >= 1)
            {
                alpha = 1;
                trackEntryFactory.destroyTrackEntry(current->previous);
                current->previous = nullptr;
            }

            current->animation.mix(skeleton, current->lastTime, time, current->loop, &m_events, alpha);
        }

        bool entryChanged = false;
        for (auto& event : m_events)
        {
            if (current->listener)
            {
                current->listener(*this, int(i), EventType::Anim_Event, event, 0);

                if (tracks[i] != current)
                {
                    // event changed current animation (tracks[i]), so stop processing it
                    entryChanged = true;
                    break;
                }
            }

            if (listener)
            {
                listener(*this, int(i), EventType::Anim_Event, event, 0);

                if (tracks[i] != current)
                {
                    // event changed current animation (tracks[i]), so stop processing it
                    entryChanged = true;
                    break;
                }
            }
        }

        if (entryChanged)
        {
            // event changed current animation (tracks[i]), so stop processing it
            continue;
        }

        /* Check if completed the animation or a loop iteration. */
        if (current->loop ?
            (std::fmod(current->lastTime, current->endTime) > std::fmod(time, current->endTime))
            : (current->lastTime < current->endTime && time >= current->endTime))
        {
            int count = (int)(time / current->endTime);

            if (current->listener)
            {
                current->listener(*this, int(i), EventType::Anim_Complete, nullptr, count);
                if (tracks[i] != current)
                {
                    // event changed current animation (tracks[i]), so stop processing it
                    continue;
                }
            }

            if (listener)
            {
                listener(*this, int(i), EventType::Anim_Complete, nullptr, count);

                if (tracks[i] != current)
                {
                    // event changed current animation (tracks[i]), so stop processing it
                    continue;
                }
            }
        }

        current->lastTime = current->time;
    }
}

void AnimationState::clearTracks()
{
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        clearTrack(int(i));
    }

    tracks.clear();
}

void AnimationState::clearTrack(int trackIndex)
{
    if (trackIndex >= int(tracks.size())) return;

    auto current = tracks[trackIndex];

    if (!current) return;

    if (current->listener)
    {
        current->listener(*this, trackIndex, EventType::Anim_End, 0, 0);
    }

    if (listener)
    {
        listener(*this, trackIndex, EventType::Anim_End, 0, 0);
    }

    tracks[trackIndex] = 0;

    disposeAllEntries(current);
}

TrackEntry* AnimationState::expandToIndex(int index)
{
    if (index < int(tracks.size()))
    {
        return tracks[index];
    }

    tracks.resize(index + 1);
    return nullptr;
}

void AnimationState::setCurrent(int index, TrackEntry* entry)
{
    auto current = expandToIndex(index);

    if (current)
    {
        auto previous = current->previous;
        current->previous = nullptr;

        if (current->listener)
        {
            current->listener(*this, index, EventType::Anim_End, 0, 0);
        }

        if (listener)
        {
            listener(*this, index, EventType::Anim_End, 0, 0);
        }

        entry->mixDuration = data.getMix(&current->animation, &entry->animation);
        if (entry->mixDuration > 0)
        {
            entry->mixTime = 0;
            /* If a mix is in progress, mix from the closest animation. */
            if (previous && current->mixTime / current->mixDuration < 0.5f)
            {
                entry->previous = previous;
                previous = current;
            }
            else
            {
                entry->previous = current;
            }
        }
        else
        {
            trackEntryFactory.destroyTrackEntry(current);
        }

        if (previous)
        {
            trackEntryFactory.destroyTrackEntry(previous);
        }
    }

    tracks[index] = entry;

    if (entry->listener)
    {
        entry->listener(*this, index, EventType::Anim_Start, 0, 0);
        if (tracks[index] != entry)
        {
            // event changed current animation (tracks[i]), so stop processing start
            return;
        }
    }

    if (listener)
    {
        listener(*this, index, EventType::Anim_Start, 0, 0);
    }
}

TrackEntry* AnimationState::setAnimationByName(int trackIndex, const std::string& animationName, bool loop)
{
    auto animation = data.skeletonData.findAnimation(animationName.c_str());
    assert(animation); // Chobo: this may return nullptr and this class doesn't deal with it
    return setAnimation(trackIndex, *animation, loop);
}

TrackEntry* AnimationState::setAnimation(int trackIndex, const Animation& animation, bool loop)
{
    auto current = expandToIndex(trackIndex);

    if (current)
    {
        disposeAllEntries(current->next);
    }

    auto entry = trackEntryFactory.createTrackEntry(*this, animation);
    entry->loop = loop;
    entry->endTime = animation.duration;

    setCurrent(trackIndex, entry);
    return entry;
}


TrackEntry* AnimationState::addAnimationByName(int trackIndex, const std::string& animationName, bool loop, float delay)
{
    auto animation = data.skeletonData.findAnimation(animationName.c_str());
    assert(animation); // Chobo: this may return nullptr and this class doesn't deal with it
    return addAnimation(trackIndex, *animation, loop, delay);
}

TrackEntry* AnimationState::addAnimation(int trackIndex, const Animation& animation, bool loop, float delay)
{
    auto entry = trackEntryFactory.createTrackEntry(*this, animation);
    entry->loop = loop;
    entry->endTime = animation.duration;

    auto last = expandToIndex(trackIndex);
    if (last)
    {
        while (last->next)
        {
            last = last->next;
        }
        last->next = entry;
    }
    else
    {
        tracks[trackIndex] = entry;
    }

    if (delay <= 0)
    {
        if (last)
        {
            delay += last->endTime - data.getMix(&last->animation, &animation);
        }
        else
        {
            delay = 0;
        }
    }
    entry->delay = delay;

    return entry;
}

TrackEntry* AnimationState::getCurrent(int trackIndex)
{
    if (trackIndex >= int(tracks.size())) return nullptr;
    return tracks[trackIndex];
}

}

