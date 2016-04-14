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

#include <vector>
#include <functional>

namespace spine
{

struct AnimationStateData;
struct Event;

class AnimationState;
class Animation;
class Skeleton;

class TrackEntryFactory;

enum class EventType
{
    Anim_Start,
    Anim_End,
    Anim_Complete,
    Anim_Event,
};

typedef std::function<void(AnimationState& state, int trackIndex, EventType type, const Event* event, int loopCount)> AnimationStateListener;

struct TrackEntry
{
    TrackEntry(const AnimationState& state, const Animation& anim);
    ~TrackEntry();

    const AnimationState& state;
    TrackEntry* previous = nullptr;
    TrackEntry* next = nullptr;
    const Animation& animation;
    int loop = false; // kept as int for alignment purposes
    float delay = 0;
    float time = 0;
    float lastTime = -1;
    float endTime = 0;
    float timeScale = 1;
    
    float mixTime = 0;
    float mixDuration = 0;
    float mix = 1;

    AnimationStateListener listener;
    void* rendererObject = nullptr;
};

class AnimationState
{
public:
    AnimationState(const AnimationStateData& data, TrackEntryFactory* trackEntryFactory = nullptr);
    ~AnimationState();

    void update(float delta);
    void apply(Skeleton& skeleton);

    void clearTracks();
    void clearTrack(int trackIndex);

    // Set the current animation. Any queued animations are cleared.
    TrackEntry* setAnimationByName(int trackIndex, const std::string& animationName, bool loop);
    TrackEntry* setAnimation(int trackIndex, const Animation& animation, bool loop);

    // Adds an animation to be played delay seconds after the current or last queued animation, taking into account any mix
    // duration.
    TrackEntry* addAnimationByName(int trackIndex, const std::string& animationName, bool loop, float delay);
    TrackEntry* addAnimation(int trackIndex, const Animation& animation, bool loop, float delay);

    TrackEntry* getCurrent(int trackIndex);

    const AnimationStateData& data;
    float timeScale = 1;
    AnimationStateListener listener;

    std::vector<TrackEntry*> tracks;

    void* rendererObject = nullptr;

    TrackEntryFactory& trackEntryFactory;

private:
    void setCurrent(int index, TrackEntry* entry);
    void disposeAllEntries(TrackEntry* entry);
    TrackEntry* expandToIndex(int index);

    std::vector<const Event*> m_events;
};

}
