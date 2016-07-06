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
#include <spinecpp/PoolTrackEntryFactory.h>
#include <spinecpp/AnimationState.h>

#include <cassert>
#include <cstring>

namespace spine
{

PoolTrackEntryFactory::PoolTrackEntryFactory(size_t pageSize)
{
    m_pages.emplace_back(pageSize);
}

PoolTrackEntryFactory::~PoolTrackEntryFactory()
{
}

TrackEntry* PoolTrackEntryFactory::createTrackEntry(const AnimationState& state, const Animation& anim)
{
    for (auto& page : m_pages)
    {
        if (page.numFreeElems)
        {
            return page.newEntry(state, anim);
        }
    }

    m_pages.emplace_back(m_pages.front().numElements);
    return m_pages.back().newEntry(state, anim);
}

void PoolTrackEntryFactory::destroyTrackEntry(TrackEntry* entry)
{
    for (auto& page : m_pages)
    {
        if (page.OwnsEntry(entry))
        {
            return page.destroyEntry(entry);
            return;
        }
    }

    assert(false); // entry not owned by any page
}

PoolTrackEntryFactory::Page::Page(size_t size)
    : buffer(new char[size * sizeof(TrackEntry)])
    , numElements(size)
    , numFreeElems(size)
{
    memset(buffer, 0, size * sizeof(TrackEntry));
}

PoolTrackEntryFactory::Page::~Page()
{
    auto lim = numElements * sizeof(TrackEntry);
    for (size_t i = 0; i < lim; i += sizeof(TrackEntry))
    {        
        assert(!*(buffer + i));
    }

    delete[] buffer;
}

TrackEntry* PoolTrackEntryFactory::Page::newEntry(const AnimationState& state, const Animation& anim)
{
    assert(numFreeElems != 0);
    auto lim = numElements * sizeof(TrackEntry);
    for (size_t i = 0; i < lim; i += sizeof(TrackEntry))
    {
        auto b = buffer + i;
        if (!*b)
        {
            --numFreeElems;
            return new (b) TrackEntry(state, anim);
        }
    }

    assert(false); // no free found?
    return nullptr;
}

bool PoolTrackEntryFactory::Page::OwnsEntry(TrackEntry* e) const
{
    auto c = reinterpret_cast<char*>(e);
    return (c >= buffer) && (c < buffer + numElements * sizeof(TrackEntry));
}

void PoolTrackEntryFactory::Page::destroyEntry(TrackEntry* e)
{
    assert(OwnsEntry(e));

    e->~TrackEntry();
    *reinterpret_cast<char*>(e) = 0;
    
    // entry destroyed multiple times
    assert(numFreeElems < numElements);
    ++numFreeElems;
}

}