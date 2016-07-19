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
#include <spinecpp/Atlas.h>
#include <spinecpp/extension.h>
#include <cassert>
#include <cstring>
#include <cstdlib>

using namespace std;

namespace
{

spine::Atlas* abortAtlas(spine::Atlas* a)
{
    delete a;
    return nullptr;
}

struct Str
{
    Str(const char* begin, const char* end)
        : begin(begin)
        , end(end)
    {}

    Str()
        : Str(nullptr, nullptr)
    {}

    Str(const std::string& s)
        : Str(s.c_str(), s.c_str() + s.length())
    {}

    const char* begin;
    const char* end;
};

static void trim(Str* str) {
    while (isspace(*str->begin) && str->begin < str->end)
        (str->begin)++;
    if (str->begin == str->end) return;
    str->end--;
    while (isspace(*str->end) && str->end >= str->begin)
        str->end--;
    str->end++;
}

/* Tokenize string without modification. Returns 0 on failure. */
static int readLine(const char** begin, const char* end, Str* str) {
    if (*begin == end) return 0;
    str->begin = *begin;

    /* Find next delimiter. */
    while (*begin != end && **begin != '\n')
        (*begin)++;

    str->end = *begin;
    trim(str);

    if (*begin != end) (*begin)++;
    return 1;
}

/* Moves str->begin past the first occurence of c. Returns 0 on failure. */
static int beginPast(Str* str, char c) {
    const char* begin = str->begin;
    while (1) {
        char lastSkippedChar = *begin;
        if (begin == str->end) return 0;
        begin++;
        if (lastSkippedChar == c) break;
    }
    str->begin = begin;
    return 1;
}

/* Returns 0 on failure. */
static int readValue(const char** begin, const char* end, Str* str) {
    readLine(begin, end, str);
    if (!beginPast(str, ':')) return 0;
    trim(str);
    return 1;
}

/* Returns the number of tuple values read (1, 2, 4, or 0 for failure). */
static int readTuple(const char** begin, const char* end, Str tuple[]) {
    int i;
    Str str = { NULL, NULL };
    readLine(begin, end, &str);
    if (!beginPast(&str, ':')) return 0;

    for (i = 0; i < 3; ++i) {
        tuple[i].begin = str.begin;
        if (!beginPast(&str, ',')) break;
        tuple[i].end = str.begin - 2;
        trim(&tuple[i]);
    }
    tuple[i].begin = str.begin;
    tuple[i].end = str.end;
    trim(&tuple[i]);
    return i + 1;
}

static std::string toStdString(Str* str) {
    int length = (int)(str->end - str->begin);
    return std::string(str->begin, length);
}

static int indexOf(const char** array, int count, Str* str) {
    int length = (int)(str->end - str->begin);
    int i;
    for (i = count - 1; i >= 0; i--)
        if (strncmp(array[i], str->begin, length) == 0) return i;
    return 0;
}

static int equals(Str* str, const char* other) {
    return strncmp(other, str->begin, str->end - str->begin) == 0;
}

static int toInt(Str* str) {
    return (int)strtol(str->begin, (char**)&str->end, 10);
}

static char* mallocString(Str* str) {
    int length = (int)(str->end - str->begin);
    char* string = (char*)malloc(length + 1);
    memcpy(string, str->begin, length);
    string[length] = '\0';
    return string;
}

static const char* formatNames[] = { "", "Alpha", "Intensity", "LuminanceAlpha", "RGB565", "RGBA4444", "RGB888", "RGBA8888" };
static const char* textureFilterNames[] = { "", "Nearest", "Linear", "MipMap", "MipMapNearestNearest", "MipMapLinearNearest",
"MipMapNearestLinear", "MipMapLinearLinear" };

}

namespace spine
{

Atlas::Page::~Page()
{
    AtlasPage_disposeTexture(*this);
}

Atlas::~Atlas()
{
    for (auto page : pages)
    {
        delete page;
    }

    for (auto region : regions)
    {
        delete region;
    }
}

Atlas* Atlas::create(const char* begin, int length, const std::string& prefixDir, void* rendererObject)
{
    int count;
    const char* end = begin + length;
    int needsSlash = !prefixDir.empty() && prefixDir.back() != '/' && prefixDir.back() != '\\';

    auto atlas = new Atlas;
    atlas->rendererObject = rendererObject;

    Str str;
    Str tuple[4];
    Page* page = nullptr;
    while (readLine(&begin, end, &str)) {
        if (str.end - str.begin == 0)
        {
            page = nullptr;
        }
        else if (!page)
        {
            char* name = mallocString(&str);
            char* path = (char*)malloc(prefixDir.length() + needsSlash + strlen(name) + 1);
            memcpy(path, prefixDir.c_str(), prefixDir.length());
            if (needsSlash) path[prefixDir.length()] = '/';
            strcpy(path + prefixDir.length() + needsSlash, name);

            page = new Page(*atlas, name);
            free(name);

            atlas->pages.push_back(page);

            switch (readTuple(&begin, end, tuple))
            {
            case 0:
                return abortAtlas(atlas);
            case 2:  /* size is only optional for an atlas packed with an old TexturePacker. */
                page->width = toInt(tuple);
                page->height = toInt(tuple + 1);
                if (!readTuple(&begin, end, tuple)) return abortAtlas(atlas);
            }
            page->format = Format(indexOf(formatNames, 7, tuple));

            if (!readTuple(&begin, end, tuple)) return abortAtlas(atlas);
            page->minFilter = Filter(indexOf(textureFilterNames, 7, tuple));
            page->magFilter = Filter(indexOf(textureFilterNames, 7, tuple + 1));

            if (!readValue(&begin, end, &str)) return abortAtlas(atlas);

            page->uWrap = Wrap::ClampToEdge;
            page->vWrap = Wrap::ClampToEdge;
            if (!equals(&str, "none")) {
				if (str.end - str.begin == 1) {
					if (*str.begin == 'x')
						page->uWrap = Wrap::Repeat;
					else if (*str.begin == 'y')
                        page->vWrap = Wrap::Repeat;
				} else if (equals(&str, "xy")) {
                    page->uWrap = Wrap::Repeat;
                    page->vWrap = Wrap::Repeat;
				}
            }

            AtlasPage_createTexture(*page, path);
            free(path);
        }
        else
        {
            auto region = new Region(*page, std::string(str.begin, str.end - str.begin));
            atlas->regions.push_back(region);

            if (!readValue(&begin, end, &str)) return abortAtlas(atlas);
            region->rotate = !!equals(&str, "true");

            if (readTuple(&begin, end, tuple) != 2) return abortAtlas(atlas);
            region->x = toInt(tuple);
            region->y = toInt(tuple + 1);

            if (readTuple(&begin, end, tuple) != 2) return abortAtlas(atlas);
            region->width = toInt(tuple);
            region->height = toInt(tuple + 1);

            region->u = region->x / (float)page->width;
            region->v = region->y / (float)page->height;
            if (region->rotate) {
                region->u2 = (region->x + region->height) / (float)page->width;
                region->v2 = (region->y + region->width) / (float)page->height;
            }
            else {
                region->u2 = (region->x + region->width) / (float)page->width;
                region->v2 = (region->y + region->height) / (float)page->height;
            }

            if (!(count = readTuple(&begin, end, tuple))) return abortAtlas(atlas);
            if (count == 4) { /* split is optional */
                region->splits.resize(4);
                region->splits[0] = toInt(tuple);
                region->splits[1] = toInt(tuple + 1);
                region->splits[2] = toInt(tuple + 2);
                region->splits[3] = toInt(tuple + 3);

                if (!(count = readTuple(&begin, end, tuple))) return abortAtlas(atlas);
                if (count == 4) { /* pad is optional, but only present with splits */
                    region->pads.resize(4);
                    region->pads[0] = toInt(tuple);
                    region->pads[1] = toInt(tuple + 1);
                    region->pads[2] = toInt(tuple + 2);
                    region->pads[3] = toInt(tuple + 3);

                    if (!readTuple(&begin, end, tuple)) return abortAtlas(atlas);
                }
            }

            region->originalWidth = toInt(tuple);
            region->originalHeight = toInt(tuple + 1);

            readTuple(&begin, end, tuple);
            region->offsetX = toInt(tuple);
            region->offsetY = toInt(tuple + 1);

            if (!readValue(&begin, end, &str)) return abortAtlas(atlas);
            region->index = toInt(&str);
        }
    }

    return atlas;
}

Atlas* Atlas::createFromFile(const char* path, void* rendererObject) {
    /* Get directory from atlas path. */
    const char* lastForwardSlash = strrchr(path, '/');
    const char* lastBackwardSlash = strrchr(path, '\\');
    const char* lastSlash = lastForwardSlash > lastBackwardSlash ? lastForwardSlash : lastBackwardSlash;
    if (lastSlash == path) lastSlash++; /* Never drop starting slash. */
    int dirLength = (int)(lastSlash ? lastSlash - path : 0);
    char* dir = (char*)malloc(dirLength + 1);
    memcpy(dir, path, dirLength);
    dir[dirLength] = '\0';

    std::string data = Util_readFile(path);

    Atlas* atlas = nullptr;
    if (!data.empty())
    {
        atlas = Atlas::create(data.c_str(), int(data.length()), dir, rendererObject);
    }

    free(dir);
    return atlas;
}

const Atlas::Region* Atlas::findRegion(const std::string& name) const
{
    for (auto& region : regions)
    {
        if (region->name == name)
            return region;
    }

    return nullptr;
}

}
