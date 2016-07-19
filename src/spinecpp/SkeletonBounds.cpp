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
#include <spinecpp/SkeletonBounds.h>
#include <spinecpp/Skeleton.h>
#include <spinecpp/Slot.h>
#include <spinecpp/Attachment.h>
#include <spinecpp/BoundingBoxAttachment.h>

#include <limits>

namespace spine
{

bool Polygon::containsPoint(Vector pt) const
{
    bool inside = 0;
    size_t prevIndex = vertices.size() - 1;

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        auto& v = vertices[i];
        auto prev = vertices[prevIndex];

        if ((v.y < pt.y && prev.y >= pt.y) || (prev.y < pt.y && v.y >= pt.y))
        {
            if (v.x + (pt.y - v.y) / (prev.y - v.y) * (prev.x - v.x) < pt.x)
            {
                inside = !inside;
            }
        }
        prevIndex = i;
    }
    return inside;
}

bool Polygon::intersectsSegment(Vector a, Vector b) const
{
    auto size = b - a;
    float det1 = a.x * b.y - a.y * b.x;

    Vector bv = vertices.back();

    for (auto& v : vertices)
    {
        float det2 = bv.x * v.y - bv.y * v.x;
        auto vsize = bv - v;
        float det3 = size.x * vsize.y - size.y * vsize.x;
        float x = (det1 * vsize.x - size.x * det2) / det3;

        if (((x >= bv.x && x <= v.x) || (x >= v.x && x <= bv.x)) && ((x >= a.x && x <= b.x) || (x >= b.x && x <= a.x)))
        {
            float y = (det1 * vsize.y - size.y * det2) / det3;

            if (((y >= bv.y && y <= v.y) || (y >= v.y && y <= bv.y)) && ((y >= a.y && y <= b.y) || (y >= b.y && y <= a.y)))
            {
                return true;
            }
        }
        bv = v;
    }
    return false;
}


SkeletonBounds::SkeletonBounds()
{

}

SkeletonBounds::~SkeletonBounds()
{
    // only defined so we call the appoprite destructors here
}


void SkeletonBounds::update(const Skeleton& skeleton, bool updateAabb)
{
    auto& slots = skeleton.slots;
    m_bounds.resize(slots.size());

    m_min.x = m_min.y = std::numeric_limits<float>::lowest();
    m_max.x = m_max.y = std::numeric_limits<float>::max();

    for (size_t i = 0; i < slots.size(); ++i)
    {
        auto& slot = slots[i];
        auto& bounds = m_bounds[i];

        auto attachment = slot.getAttachment();

        if (!attachment || attachment->type != Attachment::Type::BoundingBox)
        {
            continue;
        }

        auto bb = static_cast<const BoundingBoxAttachment*>(attachment);
        bounds.boundingBox = bb;

        bounds.polygon.vertices.resize(bb->worldVerticesCount);
        bb->computeWorldVertices(slot, reinterpret_cast<float*>(bounds.polygon.vertices.data()));

        if (updateAabb)
        {
            for (const auto& pv : bounds.polygon.vertices)
            {
                if (pv.x < m_min.x) m_min.x = pv.x;
                if (pv.y < m_min.y) m_min.y = pv.y;
                if (pv.x > m_max.x) m_max.x = pv.x;
                if (pv.y > m_max.y) m_max.y = pv.y;
            }
        }
    }
}

bool SkeletonBounds::aabbContainsPoint(Vector pt) const
{
    return pt.x >= m_min.x && pt.x <= m_max.x && pt.y >= m_min.y && pt.y <= m_max.y;
}

bool SkeletonBounds::aabbIntersectsSegment(Vector a, Vector b) const
{
    if ((a.x <= m_min.x && b.x <= m_min.x) || (a.y <= m_min.y && b.y <= m_min.y) || (a.x >= m_max.x && b.x >= m_max.x)
        || (a.y >= m_max.y && b.y >= m_max.y))
    {
        return false;
    }

    float m = (b.y - a.y) / (b.x - a.x);
    float y = m * (m_min.x - a.x) + a.y;
    if (y > m_min.y && y < m_max.y) return true;
    y = m * (m_max.x - a.x) + a.y;
    if (y > m_min.y && y < m_max.y) return true;
    float x = (m_min.y - a.y) / m + a.x;
    if (x > m_min.x && x < m_max.x) return true;
    x = (m_max.y - a.y) / m + a.x;
    if (x > m_min.x && x < m_max.x) return true;

    return false;
}

bool SkeletonBounds::aabbIntersectsSkeleton(const SkeletonBounds& bounds) const
{
    return m_min.x < bounds.m_max.x && m_max.x > bounds.m_min.x && m_min.y < bounds.m_max.y && m_max.y > bounds.m_min.y;
}

const BoundingBoxAttachment* SkeletonBounds::containsPoint(Vector pt) const
{
    for (auto& bounds : m_bounds)
    {
        if (bounds.polygon.containsPoint(pt))
        {
            return bounds.boundingBox;
        }
    }

    return nullptr;
}

const BoundingBoxAttachment* SkeletonBounds::intersectsSegment(Vector a, Vector b) const
{
    for (auto& bounds : m_bounds)
    {
        if (bounds.polygon.intersectsSegment(a, b))
        {
            return bounds.boundingBox;
        }
    }

    return nullptr;
}

const Polygon* SkeletonBounds::getPolygon(const BoundingBoxAttachment& boundingBox) const
{
    for (auto& bounds : m_bounds)
    {
        if (bounds.boundingBox == &boundingBox)
        {
            return &bounds.polygon;
        }
    }

    return nullptr;
}

}
