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

namespace spine
{

struct Event;
class Skeleton;

class Timeline
{
public:
    enum class Type
    {
        Scale,
        Rotate,
        Translate,
        Shear,
        Color,
        Attachment,
        Event,
        Draworder,
        Deform,
        IkConstraint,
        TransformConstraint,
        PathConstraintPosition,
        PathConstraintSpacing,
        PathConstraintMix,
    };

    Timeline(Type type)
        : type(type)
    {}

    virtual ~Timeline() {}

    virtual void apply(Skeleton& skeleton, float lastTime, float time, std::vector<const Event*>* firedEvents, float alpha) const = 0;

    // Will clear all frames except the first if the transformations inside are identical.
    // This may be unsafe if your code relies on changing individual frames of individual timelines.
    // This however is probably rarely the case. If it is not, you can safely call this function to 
    // get rid of frames left by mistake by the animators.
    virtual void clearIdentityFrames() = 0;

private:
    const Type type;
};

}
