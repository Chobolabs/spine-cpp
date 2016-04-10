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

#include <spinecpp/Vector.h>

#include <vector>

namespace spine
{

class Skeleton;
class BoundingBoxAttachment;

struct Polygon
{
    std::vector<Vector> vertices;

    bool containsPoint(Vector v) const;
    bool intersectsSegment(Vector a, Vector b) const;
};

class SkeletonBounds
{
public:
    SkeletonBounds();
    ~SkeletonBounds();

    void update(const Skeleton& skeleton, bool updateAabb);

    /** Returns true if the axis aligned bounding box contains the point. */
    bool aabbContainsPoint(Vector pt) const;

    /** Returns true if the axis aligned bounding box intersects the line segment. */
    bool aabbIntersectsSegment(Vector a, Vector b) const;

    /** Returns true if the axis aligned bounding box intersects the axis aligned bounding box of the specified bounds. */
    bool aabbIntersectsSkeleton(const SkeletonBounds& bounds) const;

    /** Returns the first bounding box attachment that contains the point, or null. When doing many checks, it is usually more
    * efficient to only call this method if spSkeletonBounds_aabbContainsPoint returns true. */
    const BoundingBoxAttachment* containsPoint(Vector pt) const;

    /** Returns the first bounding box attachment that contains the line segment, or null. When doing many checks, it is usually
    * more efficient to only call this method if spSkeletonBounds_aabbIntersectsSegment returns true. */
    const BoundingBoxAttachment* intersectsSegment(Vector a, Vector b) const;

    /** Returns the polygon for the specified bounding box, or null. */
    const Polygon* getPolygon(const BoundingBoxAttachment& boundingBox) const;


    struct Bounds
    {
        const BoundingBoxAttachment* boundingBox;
        Polygon polygon;
    };

    const std::vector<Bounds>& getBounds() { return m_bounds; }
    const Vector& getMin() const { return m_min; }
    const Vector& getMax() const { return m_max; }

private:
    std::vector<Bounds> m_bounds;

    Vector m_min, m_max;
};

}