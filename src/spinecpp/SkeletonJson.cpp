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
#include <spinecpp/SkeletonJson.h>
#include <spinecpp/extension.h>
#include <spinecpp/SkeletonData.h>
#include <spinecpp/Atlas.h>
#include <spinecpp/AtlasAttachmentLoader.h>
#include <spinecpp/RegionAttachment.h>
#include <spinecpp/MeshAttachment.h>
#include <spinecpp/PathAttachment.h>
#include <spinecpp/BoundingBoxAttachment.h>
#include <spinecpp/Timelines.h>
#include <spinecpp/Event.h>
#include "sajson/sajson.h"

#include <memory>

using namespace std;

namespace spine
{

SkeletonJson::SkeletonJson(AttachmentLoader& loader)
    : m_ownsLoader(false)
    , m_loader(&loader)
{
}

SkeletonJson::SkeletonJson(const Atlas& atlas)
    : SkeletonJson(*new AtlasAttachmentLoader(atlas))
{
    m_ownsLoader = true;
}

SkeletonJson::~SkeletonJson()
{
    if (m_ownsLoader)
    {
        delete m_loader;
    }
}

void SkeletonJson::setError(const std::string& e1, const std::string& e2)
{
    m_error.clear();
    m_error.reserve(e1.length() + e2.length() + 10);

    m_error = e1;
    m_error += e2;
}

SkeletonData* SkeletonJson::readSkeletonDataFile(const std::string& path)
{
    string json = Util_readFile(path);

    if (json.empty())
    {
        setError("Unable to read file: ", path);
        return nullptr;
    }

    return readSkeletonData(json);
}

static Color Color_fromHexString(const string& c)
{
    if (c.length() != 8) return Color(1, 1, 1, 1);

    Color ret;

    char digits[3];
    digits[2] = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        digits[0] = c[2 * i];
        digits[1] = c[2 * i + 1];
        ret.at(i) = float(strtoul(digits, nullptr, 16)) / 255;
    }

    return ret;
}

SkeletonData* SkeletonJson::readSkeletonData(const std::string& json)
{
    using sajson::literal;
    const sajson::document& doc = sajson::parse(sajson::string(json.c_str(), json.length()));

    if (!doc.is_valid())
    {
        setError("Invalid skeleton json: ", doc.get_error_message());
        return nullptr;
    }

    m_error.clear();

    std::unique_ptr<SkeletonData> skeletonData(new SkeletonData);

    const auto& root = doc.get_root();
    const auto len = root.get_length();

    /* Skeleton. */
    auto iskeleton = root.find_object_key(literal("skeleton"));
    if (iskeleton < len)
    {
        const auto& jskeleton = root.get_object_value(iskeleton);
        skeletonData->hash = jskeleton.get_value_of_key(literal("hash")).get_string_value();
        skeletonData->version = jskeleton.get_value_of_key(literal("spine")).get_string_value();
        skeletonData->size.x = jskeleton.get_value_of_key(literal("width")).get_safe_float_value();
        skeletonData->size.y = jskeleton.get_value_of_key(literal("height")).get_safe_float_value();
    }

    {
        /* Bones. */
        const auto ibones = root.find_object_key(literal("bones"));
        assert(ibones < len);
        const auto& jbones = root.get_object_value(ibones);
        const auto numBones = jbones.get_length();
        skeletonData->bones.reserve(numBones);

        for (size_t i = 0; i < numBones; ++i)
        {
            const auto& jbone = jbones.get_array_element(i);
            const char* parentName = jbone.get_safe_string_value_of_key(literal("parent"));

            const BoneData* parent = nullptr;
            if (parentName)
            {
                parent = skeletonData->findBone(parentName);
                if (!parent)
                {
                    setError("Parent bone not found: ", parentName);
                    return nullptr;
                }
            }

            string boneName = jbone.get_safe_value_of_key_as_string(literal("name"));
            skeletonData->bones.emplace_back(int(i), boneName, parent);
            auto& bone = skeletonData->bones.back();

            bone.length = jbone.get_safe_float_value_of_key(literal("length")) * m_scale;
            bone.translation.x = jbone.get_safe_float_value_of_key(literal("x")) * m_scale;
            bone.translation.y = jbone.get_safe_float_value_of_key(literal("y")) * m_scale;
            bone.rotation = jbone.get_safe_float_value_of_key(literal("rotation"));
            bone.scale.x = jbone.get_safe_float_value_of_key(literal("scaleX"), 1);
            bone.scale.y = jbone.get_safe_float_value_of_key(literal("scaleY"), 1);
            bone.shear.x = jbone.get_safe_float_value_of_key(literal("shearY"));
            bone.shear.y = jbone.get_safe_float_value_of_key(literal("shearY"));
            bone.inheritRotation = !!jbone.get_safe_integer_value_of_key(literal("inheritRotation"), 1);
            bone.inheritScale = !!jbone.get_safe_integer_value_of_key(literal("inheritScale"), 1);
            // no color...
        }
    }
    
    /* Slots. */
    const auto islots = root.find_object_key(literal("slots"));
    if (islots < len)
    {
        const auto& jslots = root.get_object_value(islots);
        const auto numSlots = jslots.get_length();

        skeletonData->slots.reserve(numSlots);

        for (size_t i = 0; i < numSlots; ++i)
        {
            const auto& jslot = jslots.get_array_element(i);

            // bone
            const char* boneName = jslot.get_safe_string_value_of_key(literal("bone"));
            auto bone = skeletonData->findBone(boneName);
            if (!bone)
            {
                setError("Slot bone not found: ", boneName);
                return nullptr;
            }

            string slotName = jslot.get_safe_value_of_key_as_string(literal("name"));
            skeletonData->slots.emplace_back(int(i), slotName, bone);
            auto& slot = skeletonData->slots.back();

            string color = jslot.get_safe_value_of_key_as_string(literal("color"));
            slot.color = Color_fromHexString(color);

            slot.attachmentName = jslot.get_safe_value_of_key_as_string(literal("attachment"));

            string blendStr = jslot.get_safe_value_of_key_as_string(literal("blend"));
            if (!blendStr.empty())
            {
                if (blendStr == "additive")
                {
                    slot.blendMode = BlendMode::Additive;
                }
                else if (blendStr == "multiply")
                {
                    slot.blendMode = BlendMode::Multiply;
                }
                else if (blendStr == "screen")
                {
                    slot.blendMode = BlendMode::Screen;
                }
            }
        }
    }

    /* IK constraints. */
    const auto iiks = root.find_object_key(literal("ik"));
    if (iiks < len)
    {
        const auto& jiks = root.get_object_value(iiks);
        const auto numIks = jiks.get_length();

        skeletonData->ikConstraints.reserve(numIks);

        for (size_t i = 0; i < numIks; ++i)
        {
            const auto& jik = jiks.get_array_element(i);

            string ikName = jik.get_safe_value_of_key_as_string(literal("name"));
            skeletonData->ikConstraints.emplace_back(ikName);
            auto& ik = skeletonData->ikConstraints.back();

            // ik bones
            const auto ibones = jik.find_object_key(literal("bones"));
            assert(ibones < jik.get_length());
            const auto& jbones = jik.get_object_value(ibones);
            const auto numBones = jbones.get_length();
            ik.bones.reserve(numBones);
            for (size_t j = 0; j < numBones; ++j)
            {
                const auto& jbone = jbones.get_array_element(j);
                const char* boneName = jbone.get_string_value();
                auto bone = skeletonData->findBone(boneName);
                if (!bone)
                {
                    setError("IK bone not found: ", boneName);
                    return nullptr;
                }

                ik.bones.emplace_back(bone);
            }

            // ik target
            const char* targetBoneName = jik.get_safe_string_value_of_key(literal("target"));
            auto targetBone = skeletonData->findBone(targetBoneName);
            if (!targetBone)
            {
                setError("IK target bone not found: ", targetBoneName);
                return nullptr;
            }
            ik.target = targetBone;

            // other
            ik.bendDirection = jik.get_safe_integer_value_of_key(literal("bendPositive"), 1);
            if (!ik.bendDirection) ik.bendDirection = -1;

            ik.mix = jik.get_safe_float_value_of_key(literal("mix"), 1);
        }
    }


    /* Transform constraints. */
    const auto itransforms = root.find_object_key(literal("transform"));
    if (itransforms < len)
    {
        const auto& jtransforms = root.get_object_value(itransforms);
        const auto numTransforms = jtransforms.get_length();

        skeletonData->transformConstraints.reserve(numTransforms);

        for (size_t i = 0; i < numTransforms; ++i)
        {
            const auto& jtransform = jtransforms.get_array_element(i);

            string transformName = jtransform.get_safe_value_of_key_as_string(literal("name"));
            skeletonData->transformConstraints.emplace_back(transformName);
            auto& transform = skeletonData->transformConstraints.back();

            // bones
            const auto& jbones = jtransform.get_value_of_key(literal("bones"));
            transform.bones.resize(jbones.get_length());
            for (size_t b = 0; b < jbones.get_length(); ++b)
            {
                const char* boneName = jbones.get_array_element(b).get_string_value();
                auto bone = skeletonData->findBone(boneName);
                if (!bone)
                {
                    setError("Transform bone not found: ", boneName);
                    return nullptr;
                }
                transform.bones[b] = bone;
            }

            // target
            const char* targetBoneName = jtransform.get_safe_string_value_of_key(literal("target"));
            auto targetBone = skeletonData->findBone(targetBoneName);
            if (!targetBone)
            {
                setError("Transform constraint target bone not found: ", targetBoneName);
                return nullptr;
            }
            transform.target = targetBone;

            // other
            transform.offsetRotation = jtransform.get_safe_float_value_of_key(literal("rotation"));
            transform.offsetTranslation.x = jtransform.get_safe_float_value_of_key(literal("x")) * m_scale;
            transform.offsetTranslation.y = jtransform.get_safe_float_value_of_key(literal("y")) * m_scale;
            transform.offsetScale.x = jtransform.get_safe_float_value_of_key(literal("scaleX")) * m_scale;
            transform.offsetScale.y = jtransform.get_safe_float_value_of_key(literal("scaleY")) * m_scale;
            transform.offsetShearY = jtransform.get_safe_float_value_of_key(literal("shearY")) * m_scale;
            transform.rotateMix = jtransform.get_safe_float_value_of_key(literal("rotateMix"), 1);
            transform.translateMix = jtransform.get_safe_float_value_of_key(literal("translateMix"), 1);
            transform.scaleMix = jtransform.get_safe_float_value_of_key(literal("scaleMix"), 1);
            transform.shearMix = jtransform.get_safe_float_value_of_key(literal("shearMix"), 1);
        }
    }

    /* Path constraints */
    const auto ipaths = root.find_object_key(literal("path"));
    if (ipaths < len)
    {
        const auto& jpaths = root.get_object_value(ipaths);
        const auto numPaths = jpaths.get_length();

        skeletonData->pathConstraints.reserve(numPaths);

        for (size_t i = 0; i < numPaths; ++i)
        {
            const auto& jpath = jpaths.get_array_element(i);
            string pathName = jpath.get_safe_value_of_key_as_string(literal("name"));
            skeletonData->pathConstraints.emplace_back(pathName);
            auto& path = skeletonData->pathConstraints.back();

            auto ibones = jpath.find_object_key(literal("bones"));
            if (ibones < jpath.get_length())
            {
                const auto& jbones = jpath.get_object_value(ibones);
                path.bones.resize(jbones.get_length());
                for (size_t b = 0; b < jbones.get_length(); ++b)
                {
                    const char* boneName = jbones.get_array_element(b).get_string_value();
                    auto bone = skeletonData->findBone(boneName);
                    if (!bone)
                    {
                        setError("Path bone not found: ", boneName);
                        return nullptr;
                    }
                    path.bones[b] = bone;
                }
            }

            const char* targetName = jpath.get_safe_string_value_of_key(literal("target"));
            path.target = skeletonData->findSlot(targetName);
            if (!path.target)
            {
                setError("Path target slot not found: ", targetName);
                return nullptr;
            }

            const char* mode = jpath.get_safe_string_value_of_key(literal("positionMode"), "percent");
            if (strcmp(mode, "fixed") == 0) path.positionMode = PositionMode::Fixed;
            else if (strcmp(mode, "percent") == 0) path.positionMode = PositionMode::Percent;

            mode = jpath.get_safe_string_value_of_key(literal("spacingMode"), "length");
            if (strcmp(mode, "length") == 0) path.spacingMode = SpacingMode::Length;
            else if (strcmp(mode, "fixed") == 0) path.spacingMode = SpacingMode::Fixed;
            else if (strcmp(mode, "percent") == 0) path.spacingMode = SpacingMode::Percent;

            mode = jpath.get_safe_string_value_of_key(literal("rotateMode"), "tangent");
            if (strcmp(mode, "tangent") == 0) path.rotateMode = RotateMode::Tangent;
            else if (strcmp(mode, "chain") == 0) path.rotateMode = RotateMode::Chain;
            else if (strcmp(mode, "chainScale") == 0) path.rotateMode = RotateMode::ChainScale;

            path.offsetRotation = jpath.get_safe_float_value_of_key(literal("rotation"), 0);
            path.position = jpath.get_safe_float_value_of_key(literal("position"), 0);
            if (path.positionMode == PositionMode::Fixed) path.position *= m_scale;
            path.spacing = jpath.get_safe_float_value_of_key(literal("spacing"), 0);
            if (path.spacingMode == SpacingMode::Length || path.spacingMode == SpacingMode::Fixed) path.spacing *= m_scale;
            path.rotateMix = jpath.get_safe_float_value_of_key(literal("rotateMix"), 1);
            path.translateMix = jpath.get_safe_float_value_of_key(literal("translateMix"), 1);
        }
    }

    /* Skins. */
    const auto iskins = root.find_object_key(literal("skins"));
    if (iskins < len)
    {
        const auto& jskins = root.get_object_value(iskins);
        const auto numSkins = jskins.get_length();

        skeletonData->skins.reserve(numSkins);

        for (size_t i = 0; i < numSkins; ++i)
        {
            const auto& jskin = jskins.get_object_value(i);
            string skinName = jskins.get_object_key(i);

            skeletonData->skins.emplace_back(skinName);
            Skin& skin = skeletonData->skins.back();

            if (skin.name == "default")
            {
                skeletonData->defaultSkin = &skin;
            }

            const auto numSlots = jskin.get_length();
            skin.m_entries.reserve(numSlots);
            for (size_t j = 0; j < numSlots; ++j)
            {
                const auto& jslot = jskin.get_object_value(j);
                const char* slotName = jskin.get_object_key(j);

                auto slotIndex = skeletonData->findSlotIndex(slotName);
                const auto numAttachments = jslot.get_length();
                for (size_t k = 0; k < numAttachments; ++k)
                {
                    const auto& jattachment = jslot.get_object_value(k);
                    string skinAttachmentName = jslot.get_object_key(k);

                    string name = jattachment.get_safe_value_of_key_as_string(literal("name"), skinAttachmentName);
                    string path = jattachment.get_safe_value_of_key_as_string(literal("path"), name);

                    const char* stype = jattachment.get_safe_string_value_of_key(literal("type"), "region");
                    Attachment::Type type;
                    if (strcmp(stype, "region") == 0)
                    {
                        type = Attachment::Type::Region;
                    }
                    else if (strcmp(stype, "mesh") == 0)
                    {
                        type = Attachment::Type::Mesh;
                    }
                    else if (strcmp(stype, "linkedmesh") == 0)
                    {
                        type = Attachment::Type::LinkedMesh;
                    }
                    else if (strcmp(stype, "boundingbox") == 0)
                    {
                        type = Attachment::Type::BoundingBox;
                    }
                    else if (strcmp(stype, "path") == 0)
                    {
                        type = Attachment::Type::Path;
                    }
                    else
                    {
                        setError("Unknown skin attachment type: ", stype);
                        return nullptr;
                    }

                    auto attachment = m_loader->createAttachment(skin, type, name, path);

                    if (!attachment)
                    {
                        if (!m_loader->error1.empty())
                        {
                            setError(m_loader->error1, m_loader->error2);
                            return nullptr;
                        }
                        continue;
                    }

                    switch (type)
                    {
                    case Attachment::Type::Region:
                    {
                        auto region = static_cast<RegionAttachment*>(attachment);
                        region->translation.x = jattachment.get_safe_float_value_of_key(literal("x")) * m_scale;
                        region->translation.y = jattachment.get_safe_float_value_of_key(literal("y")) * m_scale;
                        region->scale.x = jattachment.get_safe_float_value_of_key(literal("scaleX"), 1);
                        region->scale.y = jattachment.get_safe_float_value_of_key(literal("scaleY"), 1);
                        region->rotation = jattachment.get_safe_float_value_of_key(literal("rotation"));
                        region->size.x = jattachment.get_safe_float_value_of_key(literal("width"), 32) * m_scale;
                        region->size.y = jattachment.get_safe_float_value_of_key(literal("height"), 32) * m_scale;

                        string color = jattachment.get_safe_value_of_key_as_string(literal("color"));
                        region->color = Color_fromHexString(color);

                        region->updateOffset();

                        m_loader->configureAttachment(region);
                    }
                    break;
                    case Attachment::Type::Mesh:
                    case Attachment::Type::LinkedMesh:
                    {
                        auto mesh = static_cast<MeshAttachment*>(attachment);

                        string color = jattachment.get_safe_value_of_key_as_string(literal("color"));
                        mesh->color = Color_fromHexString(color);

                        mesh->size.x = jattachment.get_safe_float_value_of_key(literal("width"), 32) * m_scale;
                        mesh->size.y = jattachment.get_safe_float_value_of_key(literal("height"), 32) * m_scale;

                        auto iparent = jattachment.find_object_key(literal("parent"));
                        if (iparent == jattachment.get_length())
                        {
                            const auto& jtriangles = jattachment.get_value_of_key(literal("triangles"));
                            auto numTriangles = jtriangles.get_length();
                            mesh->triangles.resize(numTriangles);
                            for (size_t t = 0; t < numTriangles; ++t)
                            {
                                mesh->triangles[t] = jtriangles.get_array_element(t).get_integer_value();
                            }

                            const auto& juvs = jattachment.get_value_of_key(literal("uvs"));
                            auto numUVs = juvs.get_length();
                            mesh->regionUVs.reserve(numUVs / 2);
                            for (size_t uv = 0; uv < numUVs; uv += 2)
                            {
                                float x = juvs.get_array_element(uv).get_safe_float_value();
                                float y = juvs.get_array_element(uv + 1).get_safe_float_value();

                                mesh->regionUVs.emplace_back(x, y);
                            }

                            readVertices(jattachment, *mesh, int(numUVs));

                            mesh->updateUVs();

                            mesh->hullLength = jattachment.get_safe_integer_value_of_key(literal("hull"));

                            auto iedges = jattachment.find_object_key(literal("edges"));
                            if (iedges < jattachment.get_length())
                            {
                                const auto& jedges = jattachment.get_object_value(iedges);
                                mesh->edges.resize(jedges.get_length());
                                for (size_t e = 0; e < mesh->edges.size(); ++e)
                                {
                                    mesh->edges[e] = jedges.get_array_element(e).get_integer_value();
                                }
                            }

                            m_loader->configureAttachment(mesh);
                        }
                        else
                        {
                            mesh->inheritDeform = !!jattachment.get_safe_integer_value_of_key(literal("deform"), 1);
                            const char* skin = jattachment.get_safe_string_value_of_key(literal("skin"));
                            const auto& jparent = jattachment.get_object_value(iparent);
                            const char* parent = jparent.get_string_value();
                            m_linkedMeshes.emplace_back(mesh, skin, slotIndex, parent);
                        }
                    }
                    break;
                    case Attachment::Type::BoundingBox:
                    {
                        auto bbox = static_cast<BoundingBoxAttachment*>(attachment);

                        int vertexCount = jattachment.get_safe_integer_value_of_key(literal("vertexCount"));
                        readVertices(jattachment, *bbox, 2 * vertexCount);

                        m_loader->configureAttachment(bbox);
                    }
                    break;
                    case Attachment::Type::Path:
                    {
                        auto path = static_cast<PathAttachment*>(attachment);

                        path->closed = jattachment.get_safe_integer_value_of_key(literal("closed"));
                        path->constantSpeed = jattachment.get_safe_integer_value_of_key(literal("constantSpeed"), 1);
                        int vertexCount = jattachment.get_safe_integer_value_of_key(literal("vertexCount"));
                        readVertices(jattachment, *path, 2 * vertexCount);

                        path->lengths.resize(vertexCount / 3);

                        auto ilengths = jattachment.find_object_key(literal("lengths"));
                        const auto& jlengths = jattachment.get_object_value(ilengths);
                        for (size_t l = 0; l < path->lengths.size(); ++l)
                        {
                            path->lengths[l] = jlengths.get_array_element(l).get_safe_float_value() * m_scale;
                        }
                    }
                    break;
                    }
                    
                    skin.m_entries.emplace_back(slotIndex, skinAttachmentName, attachment);
                }
            }
        }
    }

    /* Linked meshes. */
    for (const auto& linkedMesh : m_linkedMeshes)
    {
        auto skin = linkedMesh.skin ? skeletonData->findSkin(linkedMesh.skin) : skeletonData->defaultSkin;
        if (!skin)
        {
            setError("Linked mesh skin not found:", linkedMesh.skin);
            return nullptr;
        }

        auto parent = skin->getAttachment(linkedMesh.slotIndex, linkedMesh.parent);
        if (!parent)
        {
            setError("Parent mesh not found:", linkedMesh.parent);
            return nullptr;
        }

        linkedMesh.mesh->setParentMesh(static_cast<const MeshAttachment*>(parent));
        linkedMesh.mesh->updateUVs();

        m_loader->configureAttachment(linkedMesh.mesh);
    }

    m_linkedMeshes.clear();

    /* Events. */
    const auto ievents = root.find_object_key(literal("events"));
    if (ievents < len)
    {
        const auto& jevents = root.get_object_value(ievents);
        const auto numEvents = jevents.get_length();

        skeletonData->events.reserve(numEvents);

        for (size_t i = 0; i < numEvents; ++i)
        {
            const auto& jevent = jevents.get_object_value(i);
            string eventName = jevents.get_object_key(i);

            skeletonData->events.emplace_back(eventName);
            EventData& event = skeletonData->events.back();

            event.intValue = jevent.get_safe_integer_value_of_key(literal("int"));
            event.floatValue = jevent.get_safe_float_value_of_key(literal("float"));
            event.stringValue = jevent.get_safe_value_of_key_as_string(literal("string"));
        }

    }

    /* Animations. */
    const auto ianims = root.find_object_key(literal("animations"));
    if (ianims < len)
    {
        const auto& janims = root.get_object_value(ianims);
        const auto numAnims = janims.get_length();

        skeletonData->animations.reserve(numAnims);

        for (size_t i = 0; i < numAnims; ++i)
        {
            const auto& janim = janims.get_object_value(i);
            string animName = janims.get_object_key(i);

            skeletonData->animations.emplace_back(animName);
            Animation& anim = skeletonData->animations.back();

            readAnimation(anim, *skeletonData, janim);
        }
    }

    return skeletonData.release();
}

void SkeletonJson::readAnimation(Animation& anim, const SkeletonData& skeletonData, const sajson::value& json)
{
    using sajson::literal;

    auto len = json.get_length();

    auto ibones = json.find_object_key(literal("bones"));
    auto islots = json.find_object_key(literal("slots"));
    auto iik = json.find_object_key(literal("ik"));
    auto itransform = json.find_object_key(literal("transform"));
    auto ipaths = json.find_object_key(literal("paths"));
    auto ideform = json.find_object_key(literal("deform"));

    auto idrawOrder = json.find_object_key(literal("drawOrder"));
    if (idrawOrder >= len)
    {
        // backwards compatibility
        idrawOrder = json.find_object_key(literal("draworder"));
    }

    auto ievents = json.find_object_key(literal("events"));

    // count timelines
    size_t numTimelines = 0;
    if (ibones < len)
    {
        const auto& jbones = json.get_object_value(ibones);
        for (size_t i = 0; i < jbones.get_length(); ++i)
        {
            numTimelines += jbones.get_object_value(i).get_length();
        }
    }

    if (islots < len)
    {
        const auto& jslots = json.get_object_value(islots);
        for (size_t i = 0; i < jslots.get_length(); ++i)
        {
            numTimelines += jslots.get_object_value(i).get_length();
        }
    }

    if (iik < len)
    {
        numTimelines += json.get_object_value(iik).get_length();
    }

    if (itransform < len)
    {
        numTimelines += json.get_object_value(itransform).get_length();
    }

    if (ipaths < len)
    {
        const auto& jpaths = json.get_object_value(ipaths);
        for (size_t i = 0; i < jpaths.get_length(); ++i)
        {
            numTimelines += jpaths.get_object_value(i).get_length();
        }
    }

    if (ideform < len)
    {
        const auto& jdeform = json.get_object_value(ideform);
        for (size_t s = 0; s < jdeform.get_length(); ++s)
        {
            const auto& jslots = jdeform.get_object_value(s);
            for (size_t i = 0; i < jslots.get_length(); ++i)
            {
                numTimelines += jslots.get_object_value(i).get_length();
            }
        }
    }

    if (idrawOrder < len) ++numTimelines;

    if (ievents < len) ++numTimelines;

    anim.timelines.reserve(numTimelines);

    /* Slot timelines. */
    if (islots < len)
    {
        const auto& jslots = json.get_object_value(islots);
        for (size_t i = 0; i < jslots.get_length(); ++i)
        {
            const auto& jslot = jslots.get_object_value(i);
            const char* slotName = jslots.get_object_key(i);

            int slotIndex = skeletonData.findSlotIndex(slotName);
            if (slotIndex == -1)
            {
                setError("Animation slot not found: ", slotName);
                return;
            }

            for (size_t j = 0; j < jslot.get_length(); ++j)
            {
                const auto& jtimeline = jslot.get_object_value(j);
                const char* timelineTypeName = jslot.get_object_key(j);

                if (strcmp(timelineTypeName, "color") == 0)
                {
                    auto timeline = new ColorTimeline(int(jtimeline.get_length()));
                    timeline->slotIndex = slotIndex;

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);
                        string color = jframe.get_safe_value_of_key_as_string(literal("color"));
                        float time = jframe.get_safe_float_value_of_key(literal("time"));
                        timeline->frames[f].time = time;
                        timeline->frames[f].color = Color_fromHexString(color);
                        readCurve(timeline->frames[f], jframe);
                    }

                    anim.timelines.emplace_back(timeline);
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
                else if (strcmp(timelineTypeName, "attachment") == 0)
                {
                    auto timeline = new AttachmentTimeline;
                    timeline->slotIndex = slotIndex;
                    timeline->frames.reserve(jtimeline.get_length());

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);

                        const auto& jname = jframe.get_value_of_key(literal("name"));
                        string attachmentName;
                        if (jname.get_type() == sajson::TYPE_STRING)
                        {
                            attachmentName = jname.as_std_string();
                        }

                        float time = jframe.get_safe_float_value_of_key(literal("time"));

                        timeline->frames.emplace_back(time, attachmentName);
                    }

                    anim.timelines.emplace_back(timeline);
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
                else
                {
                    setError("Invalid timeline type for a slot", timelineTypeName);
                    return;
                }
            }
        }
    }

    /* Bone timelines. */
    if (ibones < len)
    {
        const auto& jbones = json.get_object_value(ibones);
        for (size_t i = 0; i < jbones.get_length(); ++i)
        {
            const auto& jbone = jbones.get_object_value(i);
            const char* boneName = jbones.get_object_key(i);

            int boneIndex = skeletonData.findBoneIndex(boneName);
            if (boneIndex == -1)
            {
                setError("Animation bone not found: ", boneName);
                return;
            }

            for (size_t j = 0; j < jbone.get_length(); ++j)
            {
                const auto& jtimeline = jbone.get_object_value(j);
                const char* timelineTypeName = jbone.get_object_key(j);

                if (strcmp(timelineTypeName, "rotate") == 0)
                {
                    auto timeline = new RotateTimeline(int(jtimeline.get_length()));
                    timeline->boneIndex = boneIndex;

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);
                        float time = jframe.get_safe_float_value_of_key(literal("time"));
                        float angle = jframe.get_safe_float_value_of_key(literal("angle"));
                        timeline->frames[f].time = time;
                        timeline->frames[f].angle = angle;
                        readCurve(timeline->frames[f], jframe);
                    }

                    anim.timelines.emplace_back(timeline);
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
                else if (strcmp(timelineTypeName, "scale") == 0)
                {
                    auto timeline = new ScaleTimeline(int(jtimeline.get_length()));
                    timeline->boneIndex = boneIndex;

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);
                        float time = jframe.get_safe_float_value_of_key(literal("time"));
                        Vector scale;
                        scale.x = jframe.get_safe_float_value_of_key(literal("x"));
                        scale.y = jframe.get_safe_float_value_of_key(literal("y"));
                        timeline->frames[f].time = time;
                        timeline->frames[f].scale = scale;
                        readCurve(timeline->frames[f], jframe);
                    }

                    anim.timelines.emplace_back(timeline);
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
                else if (strcmp(timelineTypeName, "translate") == 0)
                {
                    auto timeline = new TranslateTimeline(int(jtimeline.get_length()));
                    timeline->boneIndex = boneIndex;

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);
                        float time = jframe.get_safe_float_value_of_key(literal("time"));
                        Vector t;
                        t.x = jframe.get_safe_float_value_of_key(literal("x")) * m_scale;
                        t.y = jframe.get_safe_float_value_of_key(literal("y")) * m_scale;
                        timeline->frames[f].time = time;
                        timeline->frames[f].translation = t;
                        readCurve(timeline->frames[f], jframe);
                    }

                    anim.timelines.emplace_back(timeline);
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
                else if (strcmp(timelineTypeName, "shear") == 0)
                {
                    auto timeline = new ShearTimeline(int(jtimeline.get_length()));
                    timeline->boneIndex = boneIndex;

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);
                        float time = jframe.get_safe_float_value_of_key(literal("time"));
                        Vector sh;
                        sh.x = jframe.get_safe_float_value_of_key(literal("x")) * m_scale;
                        sh.y = jframe.get_safe_float_value_of_key(literal("y")) * m_scale;
                        timeline->frames[f].time = time;
                        timeline->frames[f].shear = sh;
                        readCurve(timeline->frames[f], jframe);
                    }

                    anim.timelines.emplace_back(timeline);
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
                else
                {
                    setError("Invalid timeline type for a bone", timelineTypeName);
                    return;
                }
            }
        }
    }

    /* IK constraint timelines. */
    if (iik < len)
    {
        const auto& jiks = json.get_object_value(iik);
        for (size_t i = 0; i < jiks.get_length(); ++i)
        {
            const auto& jtimeline = jiks.get_object_value(i);
            const char* ikConstraintName = jiks.get_object_key(i);

            auto ikConstraintIndex = skeletonData.findIkConstraintIndex(ikConstraintName);
            auto timeline = new IkConstraintTimeline(int(jtimeline.get_length()));

            timeline->ikConstraintIndex = ikConstraintIndex;

            for (size_t f = 0; f < jtimeline.get_length(); ++f)
            {
                const auto& jframe = jtimeline.get_array_element(f);
                float time = jframe.get_safe_float_value_of_key(literal("time"));
                float mix = jframe.get_safe_float_value_of_key(literal("mix"), 1.f);
                int bendPositive = jframe.get_safe_integer_value_of_key(literal("bendPositive"), 1);
                timeline->frames[f].time = time;
                timeline->frames[f].mix = mix;
                timeline->frames[f].bendDirection = bendPositive ? 1 : -1;
                readCurve(timeline->frames[f], jframe);
            }

            anim.timelines.emplace_back(timeline);
            anim.duration = std::max(anim.duration, timeline->frames.back().time);
        }
    }

    /* Transform constraint timelines. */
    if (itransform < len)
    {
        const auto& jtransforms = json.get_object_value(itransform);
        for (size_t i = 0; i < jtransforms.get_length(); ++i)
        {
            const auto& jtimeline = jtransforms.get_object_value(i);
            const char* transformConstraintName = jtransforms.get_object_key(i);

            auto transformConstraintIndex = skeletonData.findTransformConstraintIndex(transformConstraintName);
            auto timeline = new TransformConstraintTimeline(int(jtimeline.get_length()));
            timeline->transformConstraintIndex = transformConstraintIndex;

            for (size_t f = 0; f < jtimeline.get_length(); ++f)
            {
                const auto& jframe = jtimeline.get_array_element(f);
                timeline->frames[f].time = jframe.get_safe_float_value_of_key(literal("time"));
                timeline->frames[f].rotateMix = jframe.get_safe_float_value_of_key(literal("rotateMix"), 1);
                timeline->frames[f].translateMix = jframe.get_safe_float_value_of_key(literal("translateMix"), 1);
                timeline->frames[f].scaleMix = jframe.get_safe_float_value_of_key(literal("scaleMix"), 1);
                timeline->frames[f].shearMix = jframe.get_safe_float_value_of_key(literal("shearMix"), 1);
                readCurve(timeline->frames[f], jframe);
            }

            anim.timelines.emplace_back(timeline);
            anim.duration = std::max(anim.duration, timeline->frames.back().time);
        }
    }

    /** Path constraint timelines. */
    if (ipaths < len)
    {
        const auto& jpaths = json.get_object_value(ipaths);

        for (size_t i = 0; i < jpaths.get_length(); ++i)
        {
            const auto& jpath = jpaths.get_object_value(i);
            const char* pathConstraintName = jpaths.get_object_key(i);

            int constraintIndex = skeletonData.findPathConstraintIndex(pathConstraintName);

            if (constraintIndex == -1)
            {
                setError("Path constraint not found: ", pathConstraintName);
                return;
            }

            auto& data = skeletonData.pathConstraints[constraintIndex];

            for (size_t j = 0; j < jpath.get_length(); ++j)
            {
                const auto& jtimeline = jpath.get_object_value(j);
                const char* timelineTypeName = jpath.get_object_key(j);

                if (strcmp(timelineTypeName, "position") == 0 || strcmp(timelineTypeName, "spacing") == 0)
                {
                    PathConstraintTimeline* timeline;
                    float timelineScale = 1;

                    if(strcmp(timelineTypeName, "spacing") == 0)
                    {
                        timeline = new PathConstraintSpacingTimeline(int(jtimeline.get_length()));
                        if (data.spacingMode == SpacingMode::Length || data.spacingMode == SpacingMode::Fixed)
                        {
                            timelineScale = m_scale;
                        }
                    }
                    else
                    {
                        timeline = new PathConstraintPositionTimeline(int(jtimeline.get_length()));
                        if (data.positionMode == PositionMode::Fixed)
                        {
                            timelineScale = m_scale;
                        }
                    }

                    timeline->pathConstraintIndex = constraintIndex;

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);
                        timeline->frames[f].time = jframe.get_safe_float_value_of_key(literal("time"));
                        timeline->frames[f].value = jframe.get_safe_float_value_of_key(literal(timelineTypeName)) * timelineScale;
                        readCurve(timeline->frames[f], jframe);
                    }

                    anim.timelines.emplace_back(timeline);
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
                else if (strcmp(timelineTypeName, "mix") == 0)
                {
                    auto timeline = new PathConstraintMixTimeline(int(jtimeline.get_length()));
                    timeline->pathConstraintIndex = constraintIndex;

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);
                        timeline->frames[f].time = jframe.get_safe_float_value_of_key(literal("time"));
                        timeline->frames[f].rotateMix = jframe.get_safe_float_value_of_key(literal("rotateMix"), 1);
                        timeline->frames[f].translateMix = jframe.get_safe_float_value_of_key(literal("translateMix"), 1);
                        readCurve(timeline->frames[f], jframe);
                    }

                    anim.timelines.emplace_back(timeline);
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
            }
        }
    }

    /* Deform timelines. */
    if (ideform < len)
    {
        const auto& jdeforms = json.get_object_value(ideform);

        for (size_t i = 0; i < jdeforms.get_length(); ++i)
        {
            const auto& jdeform = jdeforms.get_object_value(i);
            const char* skinName = jdeforms.get_object_key(i);

            auto skin = skeletonData.findSkin(skinName);

            for (size_t j = 0; j < jdeform.get_length(); ++j)
            {
                const auto& jslot = jdeform.get_object_value(j);
                const char* slotName = jdeform.get_object_key(j);

                auto slotIndex = skeletonData.findSlotIndex(slotName);

                for (size_t t = 0; t < jslot.get_length(); ++t)
                {
                    const auto& jtimeline = jslot.get_object_value(t);
                    const char* attachmentName = jslot.get_object_key(t);

                    auto attachment = static_cast<const VertexAttachment*>(skin->getAttachment(slotIndex, attachmentName));
                    if (!attachment)
                    {
                        setError("Animation ffd skin attachment not found: ", attachmentName);
                        return;
                    }

                    bool weighted = !attachment->bones.empty();
                    size_t numVertices = 0;                    
                    if (weighted)
                    {
                        numVertices = attachment->vertices.size() / 3;
                    }
                    else
                    {
                        numVertices = attachment->vertices.size() / 2;
                    }

                    auto timeline = new DeformTimeline(int(jtimeline.get_length()), numVertices);
                    timeline->slotIndex = slotIndex;
                    timeline->attachment = attachment;

                    vector<Vector> verts;

                    for (size_t f = 0; f < jtimeline.get_length(); ++f)
                    {
                        const auto& jframe = jtimeline.get_array_element(f);

                        float time = jframe.get_safe_float_value_of_key(literal("time"));

                        const auto& ivertices = jframe.find_object_key(literal("vertices"));
                        if (ivertices >= jframe.get_length())
                        {
                            if (weighted)
                            {
                                verts.assign(numVertices, Vector(0, 0));
                                timeline->setFrame(int(f), time, verts);
                            }
                            else
                            {
                                timeline->setFrame(int(f), time, attachment->vertices.vec());
                            }
                        }
                        else
                        {
                            int offset = jframe.get_safe_integer_value_of_key(literal("offset"));

                            const auto& jvertices = jframe.get_object_value(ivertices);
                            verts.assign(numVertices, Vector(0, 0));
                            float* fverts = reinterpret_cast<float*>(verts.data()) + offset;
                            for (size_t v = 0; v < jvertices.get_length(); ++v)
                            {
                                *fverts++ = jvertices.get_array_element(v).get_safe_float_value() * m_scale;
                            }

                            if (!weighted)
                            {
                                float* fverts = reinterpret_cast<float*>(verts.data());
                                for (size_t v = 0; v < 2*numVertices; ++v)
                                {
                                    fverts[v] += attachment->vertices[v];
                                }
                            }

                            timeline->setFrame(int(f), time, verts);
                        }

                        readCurve(timeline->frames[f], jframe);
                    }
                    anim.timelines.emplace_back(timeline);                    
                    anim.duration = std::max(anim.duration, timeline->frames.back().time);
                }
            }
        }
    }

    /* Draw order timeline. */
    if (idrawOrder < len)
    {
        const auto& jdrawOrder = json.get_object_value(idrawOrder);

        auto timeline = new DrawOrderTimeline(int(jdrawOrder.get_length()), int(skeletonData.slots.size()));

        vector<int> drawOrder;
        vector<int> drawOrderUnchanged; // keeps a list of the unchanged indices

        for (size_t f = 0; f < jdrawOrder.get_length(); ++f)
        {
            const auto& jframe = jdrawOrder.get_array_element(f);

            float time = jframe.get_safe_float_value_of_key(literal("time"));

            const auto ioffsets = jframe.find_object_key(literal("offsets"));
            if (ioffsets < jframe.get_length())
            {
                const auto& joffsets = jframe.get_object_value(ioffsets);

                // mark all indices as unchanged
                drawOrder.assign(skeletonData.slots.size(), -1);
                // prepare unchaged list
                drawOrderUnchanged.reserve(skeletonData.slots.size());
                drawOrderUnchanged.resize(drawOrder.size() - joffsets.get_length());
                int unchangedIndex = 0;
                int drawOrderIndex = 0;

                for (size_t i = 0; i < joffsets.get_length(); ++i)
                {
                    const auto& joffset = joffsets.get_array_element(i);
                    auto slotName = joffset.get_safe_string_value_of_key(literal("slot"));

                    int slotIndex = skeletonData.findSlotIndex(slotName);
                    if (slotIndex == -1)
                    {
                        setError("Animation drawOrder timeline slot not found: ", slotName);
                        return;
                    }

                    // collect unchaged indices in helper
                    while (drawOrderIndex != slotIndex)
                    {
                        drawOrderUnchanged[unchangedIndex++] = drawOrderIndex++;
                    }

                    int offset = joffset.get_safe_integer_value_of_key(literal("offset"));

                    // set changed item
                    drawOrder[drawOrderIndex + offset] = drawOrderIndex;
                    ++drawOrderIndex;
                }

                // collect remaining unchanged
                while (drawOrderIndex < int(drawOrder.size()))
                {
                    drawOrderUnchanged[unchangedIndex++] = drawOrderIndex++;
                }

                // fill unchanged in draw order
                unchangedIndex = 0;
                for (auto& d : drawOrder)
                {
                    if (d == -1)
                    {
                        d = drawOrderUnchanged[unchangedIndex++];
                    }
                }
            }

            timeline->setFrame(int(f), time, drawOrder);
            drawOrder.clear();
            drawOrderUnchanged.clear();
        }

        anim.timelines.emplace_back(timeline);
        anim.duration = std::max(anim.duration, timeline->frames.back().time);
    }

    /* Event timeline. */
    if (ievents < len)
    {
        const auto& jevents = json.get_object_value(ievents);

        auto timeline = new EventTimeline;
        timeline->frames.reserve(jevents.get_length());

        for (size_t f = 0; f < jevents.get_length(); ++f)
        {
            const auto& jframe = jevents.get_array_element(f);

            float time = jframe.get_safe_float_value_of_key(literal("time"));

            auto eventName = jframe.get_safe_string_value_of_key(literal("name"));

            auto eventData = skeletonData.findEvent(eventName);
            if (!eventData)
            {
                setError("Event in animation timeline not found: ", eventName);
                return;
            }

            timeline->frames.emplace_back(time, *eventData);
            auto& event = timeline->frames.back();

            event.intValue = jframe.get_safe_integer_value_of_key(literal("int"));
            event.floatValue = jframe.get_safe_float_value_of_key(literal("float"));
            event.stringValue = jframe.get_safe_value_of_key_as_string(literal("string"));
        }

        anim.timelines.emplace_back(timeline);
        anim.duration = std::max(anim.duration, timeline->frames.back().time);
    }
}

void SkeletonJson::readCurve(CurveFrame& frame, const sajson::value& json)
{
    using sajson::literal;
    auto icurve = json.find_object_key(literal("curve"));

    if (icurve >= json.get_length()) return;

    const auto& jcurve = json.get_object_value(icurve);

    if (jcurve.get_type() == sajson::TYPE_STRING && strcmp(jcurve.get_string_value(), "stepped") == 0)
    {
        frame.setStepped();
    }
    else if (jcurve.get_type() == sajson::TYPE_ARRAY)
    {
        Vector c1, c2;

        c1.x = jcurve.get_array_element(0).get_safe_float_value();
        c1.y = jcurve.get_array_element(1).get_safe_float_value();

        c2.x = jcurve.get_array_element(2).get_safe_float_value();
        c2.y = jcurve.get_array_element(3).get_safe_float_value();

        frame.setCurve(c1, c2);
    }
}

void SkeletonJson::readVertices(const sajson::value& json, VertexAttachment& attachment, int verticesLength)
{
    attachment.worldVerticesCount = verticesLength / 2;

    auto ivertices = json.find_object_key(sajson::literal("vertices"));

    if (ivertices >= json.get_length()) return;

    const auto& jvertices = json.get_object_value(ivertices);
    const auto numVertices = jvertices.get_length();

    if (int(numVertices) == verticesLength)
    {
        // no bones and weights just plain vertices
        auto& v = attachment.vertices;
        v.resize(verticesLength);
        for (size_t i = 0; i < verticesLength; ++i)
        {
            v[i] = jvertices.get_array_element(i).get_safe_float_value() * m_scale;
        }

        attachment.bones.clear();
    }
    else
    {
        // format of jvertices is bones, bone, x, y, weight

        size_t numBones = 0;
        size_t numWeights = 0;
        for (size_t v = 0; v < numVertices;)
        {
            int bc = jvertices.get_array_element(v).get_integer_value();
            numBones += bc + 1;
            numWeights += 3 * bc;
            v += 1 + bc * 4;
        }

        attachment.bones.resize(numBones);
        attachment.vertices.resize(numWeights);

        int b = 0;
        int w = 0;
        for (size_t v = 0; v < numVertices;)
        {
            int bc = jvertices.get_array_element(v++).get_integer_value();
            attachment.bones[b++] = bc;
            for (int ib = 0; ib < bc; ++ib)
            {
                attachment.bones[b++] = jvertices.get_array_element(v).get_integer_value();
                attachment.vertices[w++] = jvertices.get_array_element(v + 1).get_safe_float_value() * m_scale;
                attachment.vertices[w++] = jvertices.get_array_element(v + 2).get_safe_float_value() * m_scale;
                attachment.vertices[w++] = jvertices.get_array_element(v + 3).get_safe_float_value();
                v += 4;
            }
        }
    }
}

}
