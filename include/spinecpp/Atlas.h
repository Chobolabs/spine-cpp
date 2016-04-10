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

#include <string>
#include <vector>

namespace spine
{

class Atlas
{
public:
    enum class Format
    {
        Unknown,
        Alpha,
        Intensity,
        Luminance,
        RGB565,
        RGBA4444,
        RGB888,
        RGBA8888
    };

    enum class Filter
    {
        Unknown,
        Nearest,
        Linear,
        Mipmap,
        Mipmap_Nearest_Nearest,
        Mipmap_Linear_Nearest,
        Mipmap_Nearest_Linear,
        Mipmap_Linear_Linear
    };

    enum class Wrap
    {
        MirroredRepeat,
        ClampToEdge,
        Repeat
    };

    struct Page
    {
        Page(const Atlas& atlas, const std::string& name)
            : atlas(atlas)
            , name(name)
        {}

        ~Page(); // dispose of texture

        const Atlas& atlas;
        const std::string name;
        Format format;
        Filter minFilter, magFilter;
        Wrap uWrap = Wrap::MirroredRepeat, vWrap = Wrap::MirroredRepeat;

        void* rendererObject = nullptr;
        int width = 0, height = 0;
    };

    struct Region
    {
        Region(const Page& page, const std::string& name)
            : page(page)
            , name(name)
        {}

        const Page& page;
        const std::string name;

        int x, y, width, height;
        float u, v, u2, v2;
        int offsetX, offsetY;
        int originalWidth, originalHeight;
        int index;
        bool rotate;
        bool flip;
        std::vector<int> splits;
        std::vector<int> pads;
    };

    ~Atlas();

    // Image files referenced in the atlas file will be prefixed with dir.
    static Atlas* create(const char* begin, int length, const std::string& prefixDir, void* rendererObject);

    // Image files referenced in the atlas file will be prefixed with the directory containing the atlas file.
    static Atlas* createFromFile(const char* path, void* rendererObject);

    // Returns nullptr if the region was not found.
    const Region* findRegion(const std::string& name) const;

    std::vector<Page*> pages;
    std::vector<Region*> regions;

    void* rendererObject;
};

}
