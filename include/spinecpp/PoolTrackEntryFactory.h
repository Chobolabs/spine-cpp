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

#include "TrackEntryFactory.h"

#include <vector>
#include <cstdint>
#include <cstddef>

namespace spine
{

// Provides a pool for animation track entries.
// Helps with keeping them cache-local.
// Using this makes sense if you mix many animations into a state. 
// If your typical case is to have 1 or 2 tracks, you probably won't see a big benefit from this.
class PoolTrackEntryFactory : public TrackEntryFactory
{
public:
    PoolTrackEntryFactory(size_t pageSize);
    ~PoolTrackEntryFactory();

    virtual TrackEntry* createTrackEntry(const AnimationState& state, const Animation& anim) override;
    virtual void destroyTrackEntry(TrackEntry* entry) override;

private:
    struct Page
    {
        Page(size_t size);
        ~Page();

        TrackEntry* newEntry(const AnimationState& state, const Animation& anim);
        void destroyEntry(TrackEntry* e);

        bool OwnsEntry(TrackEntry* e) const;

        char* const buffer;
        const size_t numElements;
        size_t numFreeElems;
    };

    std::vector<Page> m_pages;
};

}