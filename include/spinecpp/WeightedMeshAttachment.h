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

#include <spinecpp/Attachment.h>
#include <spinecpp/Vector.h>
#include <spinecpp/Color.h>

#include <vector>

namespace spine
{

class Slot;

class WeightedMeshAttachment : public Attachment
{
public:
    WeightedMeshAttachment(const std::string& name, const std::string& path);

    void updateUVs();
    void computeWorldVertices(const Slot& slot, float* outWorldVertices) const;

    const std::string path;

    std::vector<int> bones;
    std::vector<float> weights;

    // Chobo: actually indices
    std::vector<int> triangles;

    std::vector<Vector> regionUVs;
    std::vector<Vector> uvs;
    int hullLength = 0;

    Color color = Color(1, 1, 1, 1);

    const void* rendererObject = nullptr;

    int regionOffsetX = 0, regionOffsetY = 0; // Pixels stripped from the bottom left, unrotated.
    int regionWidth = 0, regionHeight = 0; // Unrotated, stripped pixel size.
    int regionOriginalWidth = 0, regionOriginalHeight = 0; // Unrotated, unstripped pixel size.
    Vector regionUV = Vector(0, 0);
    Vector regionUV2 = Vector(0, 0);
    bool regionRotate = false;

    // Nonessential.
    std::vector<int> edges;
    Vector size = Vector(0, 0);
};

}