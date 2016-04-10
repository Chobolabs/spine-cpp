This is a guide for migrating from spine-c to spine-cpp.

## API

Spine-cpp uses C++ objects with methods in the namespace `spine`.

Spine-cpp strings are of type `std::string`

**spine-c**

    spSkeleton* skeleton;
    ...
    spSkeleton_setSkinByName(skeleton, "myskin");

**spine-cpp**

    spine::Skeleton* skeleton;
    ...
    std::string name = "myskin";
    skeleton->setSkinByName(name);

## Integration

Spine-c requires you to implement the following functions:

    extern "C"
    {
    void _spAtlasPage_createTexture(spAtlasPage* page, const char* path);
    void _spAtlasPage_disposeTexture(spAtlasPage* page);
    char* _spUtil_readFile(const char* path, int* length);
    }

Conversely spine-cpp requires you to implement the following:

    namespace spine
    {
    void AtlasPage_createTexture(spine::Atlas::Page& page, const char* path);
    void AtlasPage_disposeTexture(spine::Atlas::Page& page);
    std::string Util_readFile(const std::string& path);
    }

The main difference is in `Util_readFile` which must return a std::string with the file's contents.

## Creating a destroying objects

Spine-cpp doesn't use the create/dispose functions, from spine-c and instead it relies on constructors and destructors. You can choose to create your spine-cpp object with `new`/`delete` or just declare them as value types.

**spine-c**

    spAtlas* atlas = spAtlas_createFromFile("spineboy.atlas", 0);    
    spSkeletonJson* sjson = spSkeletonJson_create(atlas);
    spSkeletonData* data = spSkeletonJson_readSkeletonDataFile(sjson, "spineboy.json");
    spSkeletonJson_dispose(sjson);
    spSkeleton* skeleton = spSkeleton_create(data);
    spAnimationStateData* animStateData = spAnimationStateData_create(data);
    spAnimationState* animState = spAnimationState_create(animStateData);
    ...
    spAnimationState_dispose(animState);
    spSkeleton_dispose(skeleton);
    spAnimationStateData_dispose(animStateData);
    spSkeletonData_dispose(data);
    spAtlas_dispose(atlas);

**spine-cpp**

    auto atlas = Atlas::createFromFile("spineboy.atlas", nullptr);
    SkeletonJson sjson(*atlas);
    auto data = sjson.readSkeletonDataFile("spineboy.json");
    auto skeleton = new Skeleton(*data);
    auto animStateData = new AnimationStateData(*data);
    auto animState = new AnimationState(*animStateData);
    ...
    delete m_animState;
    delete m_skeleton;
    delete m_animStateData;
    delete m_skeletonData;
    delete m_atlas;

## Vectors

Spine-cpp introduces a simple two-dimensional vector type: `spine::Vector`. It's used internally and externally, for example for texture coordinates.

**spine-c**

    MyVec2 regionUVS[] = { 
        { region->uvs[SP_VERTEX_X1], region->uvs[SP_VERTEX_Y1] },
        { region->uvs[SP_VERTEX_X2], region->uvs[SP_VERTEX_Y2] },
        { region->uvs[SP_VERTEX_X3], region->uvs[SP_VERTEX_Y3] },
        { region->uvs[SP_VERTEX_X4], region->uvs[SP_VERTEX_Y4] },
    };

**spine-cpp**

    MyVec2 regionUVS[] = { 
        { region->uvs[0].x, region->uvs[0].y },
        { region->uvs[1].x, region->uvs[1].y },
        { region->uvs[2].x, region->uvs[2].y },
        { region->uvs[3].x, region->uvs[3].y },
    };

However for better compatibility `computeWorldVertices` functions still have an output parameter of type `float*`

Some spine-c types, like `spBone`, have fields `float x, y;` or `float width, height;` or `float scaleX, scaleY;`.

In spine-cpp the same same fields are spine vectors called `Vector translation;`, `Vector size;`, `Vector scale;`

## Colors

Spine-cpp introduces a new type `spine::Color` which is used for color fields. Spine-c's `float r,g,b,a;` is `Color color` in spine-cpp
