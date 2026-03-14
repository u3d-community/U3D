//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "FbxImporter.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#ifdef URHO3D_PHYSICS
#include <Urho3D/Physics/PhysicsWorld.h>
#endif
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>

#include <ufbx.h>

#include <Urho3D/DebugNew.h>

using namespace Urho3D;

// Access globals from AssetImporter.cpp
extern SharedPtr<Context> context_;
extern String inputName_;
extern String resourcePath_;
extern String outPath_;
extern String outName_;
extern bool useSubdirs_;
extern bool localIDs_;
extern bool saveBinary_;
extern bool saveJson_;
extern bool createZone_;
extern bool noAnimations_;
extern bool noHierarchy_;
extern bool noMaterials_;
extern bool noTextures_;
extern bool noMaterialDiffuseColor_;
extern bool noEmptyNodes_;
extern bool saveMaterialList_;
extern bool includeNonSkinningBones_;
extern bool verboseLog_;
extern bool emissiveAO_;
extern bool noOverwriteMaterial_;
extern bool noOverwriteTexture_;
extern bool noOverwriteNewerTexture_;
extern bool checkUniqueModel_;
extern unsigned maxBones_;
extern Vector<String> nonSkinningBoneIncludes_;
extern Vector<String> nonSkinningBoneExcludes_;
extern float importStartTime_;
extern float importEndTime_;

static const unsigned MAX_CHANNELS = 4;

// ---------------------------------------------------------------------------
// Type conversions
// ---------------------------------------------------------------------------

static Vector3 ToVector3(const ufbx_vec3& v)
{
    return Vector3((float)v.x, (float)v.y, (float)v.z);
}

static Vector2 ToVector2(const ufbx_vec2& v)
{
    return Vector2((float)v.x, (float)v.y);
}

static Quaternion ToQuaternion(const ufbx_quat& q)
{
    return Quaternion((float)q.w, (float)q.x, (float)q.y, (float)q.z);
}

static Matrix3x4 ToMatrix3x4(const ufbx_matrix& m)
{
    // ufbx_matrix: m_ij = row i, col j, stored as column vectors (cols[4])
    // Urho3D Matrix3x4: row-major m00..m23
    return Matrix3x4(
        (float)m.m00, (float)m.m01, (float)m.m02, (float)m.m03,
        (float)m.m10, (float)m.m11, (float)m.m12, (float)m.m13,
        (float)m.m20, (float)m.m21, (float)m.m22, (float)m.m23
    );
}

static String SanitateAssetName(const String& name)
{
    String fixedName = name;
    fixedName.Replace("<", "");
    fixedName.Replace(">", "");
    fixedName.Replace("?", "");
    fixedName.Replace("*", "");
    fixedName.Replace(":", "");
    fixedName.Replace("\"", "");
    fixedName.Replace("/", "");
    fixedName.Replace("\\", "");
    fixedName.Replace("|", "");
    return fixedName;
}

// ---------------------------------------------------------------------------
// Internal data structures (parallel to OutModel/OutScene in AssetImporter.cpp)
// ---------------------------------------------------------------------------

struct FbxModel
{
    String outName_;
    ufbx_node* rootNode_{};
    PODVector<ufbx_mesh*> meshes_;
    PODVector<ufbx_node*> meshNodes_;
    PODVector<ufbx_node*> bones_;
    PODVector<float> boneRadii_;
    PODVector<BoundingBox> boneHitboxes_;
    ufbx_node* rootBone_{};
    // Per mesh, per material-part: which parts to export
    // meshes_ and meshNodes_ are expanded so each entry corresponds to one (mesh, material_part) pair
};

struct FbxScene
{
    String outName_;
    ufbx_node* rootNode_{};
    Vector<FbxModel> models_;
    PODVector<ufbx_node*> nodes_;
    PODVector<unsigned> nodeModelIndices_;
};

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

static void FbxDumpNodes(ufbx_node* node, ufbx_node* rootNode, unsigned level);
static void FbxExportModel(ufbx_scene* scene, const String& outName, ufbx_node* rootNode);
static void FbxExportAnimation(ufbx_scene* scene, const String& outName, ufbx_node* rootNode);
static void FbxExportScene(ufbx_scene* scene, const String& outName, ufbx_node* rootNode, bool asPrefab);

static void FbxCollectMeshes(FbxModel& model, ufbx_node* node);
static void FbxCollectBones(FbxModel& model, ufbx_scene* scene);
static void FbxCollectBonesFinal(PODVector<ufbx_node*>& dest, const HashSet<ufbx_node*>& necessary, ufbx_node* node);
static void FbxBuildBoneCollisionInfo(FbxModel& model, ufbx_scene* scene);
static void FbxBuildAndSaveModel(FbxModel& model, ufbx_scene* scene);
static void FbxBuildAndSaveAnimations(ufbx_scene* scene, FbxModel* model);

static void FbxCollectSceneModels(FbxScene& fbxScene, ufbx_scene* scene, ufbx_node* node);
static void FbxBuildAndSaveScene(FbxScene& fbxScene, ufbx_scene* scene, bool asPrefab);
static void FbxCreateHierarchy(Scene* outScene, ufbx_node* srcNode, ufbx_node* rootNode, HashMap<ufbx_node*, Node*>& nodeMapping);
static Node* FbxCreateSceneNode(Scene* outScene, ufbx_node* srcNode, ufbx_node* rootNode, HashMap<ufbx_node*, Node*>& nodeMapping);

static void FbxExportMaterials(ufbx_scene* scene, HashSet<String>& usedTextures);
static void FbxBuildAndSaveMaterial(ufbx_scene* scene, ufbx_material* material, HashSet<String>& usedTextures);
static void FbxCopyTextures(ufbx_scene* scene, const HashSet<String>& usedTextures, const String& sourcePath);

static unsigned FbxGetBoneIndex(FbxModel& model, const String& boneName);

// Vertex structure for ufbx_generate_indices dedup
struct FbxVertex
{
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
    Color color;
    Vector4 tangent;
    float blendWeights[4];
    unsigned char blendIndices[4];
};

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

bool ImportFbx(const String& inFile, const String& outFile, const String& command, const String& rootNodeName)
{
    ufbx_load_opts opts = {};
    opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
    opts.target_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
    opts.target_axes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Z;
    opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
    opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
    opts.generate_missing_normals = true;
    opts.clean_skin_weights = true;

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(GetNativePath(inFile).CString(), &opts, &error);
    if (!scene)
    {
        PrintLine("ufbx error: " + String(error.description.data));
        ErrorExit("Could not open or parse FBX file " + inFile);
        return false;
    }

    ufbx_node* rootNode = scene->root_node;
    if (!rootNodeName.Empty())
    {
        rootNode = nullptr;
        for (size_t i = 0; i < scene->nodes.count; ++i)
        {
            if (!rootNodeName.Compare(scene->nodes.data[i]->name.data, false))
            {
                rootNode = scene->nodes.data[i];
                break;
            }
        }
        if (!rootNode)
        {
            ufbx_free_scene(scene);
            ErrorExit("Could not find scene node " + rootNodeName);
            return false;
        }
    }

    if (command == "dump")
    {
        FbxDumpNodes(rootNode, rootNode, 0);
        ufbx_free_scene(scene);
        return true;
    }

    if (command == "model")
        FbxExportModel(scene, outFile, rootNode);

    if (command == "anim")
        FbxExportAnimation(scene, outFile, rootNode);

    if (command == "scene" || command == "node")
    {
        bool asPrefab = command == "node";
        if (asPrefab)
            noHierarchy_ = false;
        FbxExportScene(scene, outFile, rootNode, asPrefab);
    }

    if (!noMaterials_)
    {
        HashSet<String> usedTextures;
        FbxExportMaterials(scene, usedTextures);
        if (!noTextures_)
            FbxCopyTextures(scene, usedTextures, GetPath(inFile));
    }

    ufbx_free_scene(scene);
    return true;
}

// ---------------------------------------------------------------------------
// Dump
// ---------------------------------------------------------------------------

static void FbxDumpNodes(ufbx_node* node, ufbx_node* rootNode, unsigned level)
{
    if (!node)
        return;

    String indent(' ', level * 2);
    Vector3 pos = ToVector3(node->local_transform.translation);

    PrintLine(indent + "Node " + String(node->name.data) + " pos " + String(pos));

    if (node->mesh)
    {
        unsigned numParts = (unsigned)node->mesh->material_parts.count;
        if (numParts <= 1)
            PrintLine(indent + "  1 geometry");
        else
            PrintLine(indent + "  " + String(numParts) + " geometries");
    }

    for (size_t i = 0; i < node->children.count; ++i)
        FbxDumpNodes(node->children.data[i], rootNode, level + 1);
}

// ---------------------------------------------------------------------------
// Mesh collection
// ---------------------------------------------------------------------------

static void FbxCollectMeshes(FbxModel& model, ufbx_node* node)
{
    if (node->mesh)
    {
        ufbx_mesh* mesh = node->mesh;
        // Check for duplicate
        bool dup = false;
        for (unsigned i = 0; i < model.meshes_.Size(); ++i)
        {
            if (mesh == model.meshes_[i] && node == model.meshNodes_[i])
            {
                PrintLine("Warning: same mesh found multiple times");
                dup = true;
                break;
            }
        }
        if (!dup)
        {
            model.meshes_.Push(mesh);
            model.meshNodes_.Push(node);
        }
    }

    for (size_t i = 0; i < node->children.count; ++i)
        FbxCollectMeshes(model, node->children.data[i]);
}

// ---------------------------------------------------------------------------
// Bone collection
// ---------------------------------------------------------------------------

static unsigned FbxGetBoneIndex(FbxModel& model, const String& boneName)
{
    for (unsigned i = 0; i < model.bones_.Size(); ++i)
    {
        if (boneName == model.bones_[i]->name.data)
            return i;
    }
    return M_MAX_UNSIGNED;
}

static void FbxCollectBonesFinal(PODVector<ufbx_node*>& dest, const HashSet<ufbx_node*>& necessary, ufbx_node* node)
{
    bool includeBone = necessary.Contains(node);
    String boneName(node->name.data);

    if (!includeBone && includeNonSkinningBones_)
    {
        if (nonSkinningBoneIncludes_.Empty())
            includeBone = true;

        for (unsigned i = 0; i < nonSkinningBoneIncludes_.Size(); ++i)
        {
            if (boneName.Contains(nonSkinningBoneIncludes_[i], false))
            {
                includeBone = true;
                break;
            }
        }
        for (unsigned i = 0; i < nonSkinningBoneExcludes_.Size(); ++i)
        {
            if (boneName.Contains(nonSkinningBoneExcludes_[i], false))
            {
                includeBone = false;
                break;
            }
        }

        if (includeBone)
            PrintLine("Including non-skinning bone " + boneName);
    }

    if (includeBone)
        dest.Push(node);

    for (size_t i = 0; i < node->children.count; ++i)
        FbxCollectBonesFinal(dest, necessary, node->children.data[i]);
}

static void FbxCollectBones(FbxModel& model, ufbx_scene* scene)
{
    HashSet<ufbx_node*> necessary;
    HashSet<ufbx_node*> rootNodes;

    bool haveSkinnedMeshes = false;
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        if (model.meshes_[i]->skin_deformers.count > 0)
        {
            haveSkinnedMeshes = true;
            break;
        }
    }

    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        ufbx_mesh* mesh = model.meshes_[i];
        ufbx_node* meshNode = model.meshNodes_[i];
        ufbx_node* meshParentNode = meshNode->parent;
        ufbx_node* boneRootNode = nullptr;

        if (mesh->skin_deformers.count > 0)
        {
            ufbx_skin_deformer* skin = mesh->skin_deformers.data[0];
            for (size_t j = 0; j < skin->clusters.count; ++j)
            {
                ufbx_skin_cluster* cluster = skin->clusters.data[j];
                if (!cluster->bone_node)
                    continue;

                ufbx_node* boneNode = cluster->bone_node;
                necessary.Insert(boneNode);
                boneRootNode = boneNode;

                for (;;)
                {
                    ufbx_node* parent = boneNode->parent;
                    if (!parent || parent == meshNode || parent == meshParentNode)
                        break;
                    boneRootNode = parent;
                    necessary.Insert(parent);
                    boneNode = parent;
                }

                rootNodes.Insert(boneRootNode);
            }
        }

        // For partially skinned models, include rigid mesh attachment nodes
        if (haveSkinnedMeshes && mesh->skin_deformers.count == 0)
        {
            ufbx_node* boneNode = meshNode;
            necessary.Insert(boneNode);
            boneRootNode = boneNode;

            for (;;)
            {
                ufbx_node* parent = boneNode->parent;
                if (!parent || parent == meshNode || parent == meshParentNode)
                    break;
                boneRootNode = parent;
                necessary.Insert(parent);
                boneNode = parent;
            }

            rootNodes.Insert(boneRootNode);
        }
    }

    // If multiple root nodes, find common parent
    if (rootNodes.Size() > 1)
    {
        for (HashSet<ufbx_node*>::Iterator i = rootNodes.Begin(); i != rootNodes.End(); ++i)
        {
            ufbx_node* commonParent = (*i);
            while (commonParent)
            {
                unsigned found = 0;
                for (HashSet<ufbx_node*>::Iterator j = rootNodes.Begin(); j != rootNodes.End(); ++j)
                {
                    if (i == j)
                        continue;
                    ufbx_node* parent = *j;
                    while (parent)
                    {
                        if (parent == commonParent)
                        {
                            ++found;
                            break;
                        }
                        parent = parent->parent;
                    }
                }

                if (found >= rootNodes.Size() - 1)
                {
                    PrintLine("Multiple roots initially found, using new root node " + String(commonParent->name.data));
                    rootNodes.Clear();
                    rootNodes.Insert(commonParent);
                    necessary.Insert(commonParent);
                    break;
                }

                commonParent = commonParent->parent;
            }

            if (rootNodes.Size() == 1)
                break;
        }
        if (rootNodes.Size() > 1)
            ErrorExit("Skeleton with multiple root nodes found, not supported");
    }

    if (rootNodes.Empty())
        return;

    model.rootBone_ = *rootNodes.Begin();
    FbxCollectBonesFinal(model.bones_, necessary, model.rootBone_);

    model.boneRadii_.Resize(model.bones_.Size());
    model.boneHitboxes_.Resize(model.bones_.Size());
    for (unsigned i = 0; i < model.bones_.Size(); ++i)
    {
        model.boneRadii_[i] = 0.0f;
        model.boneHitboxes_[i] = BoundingBox(0.0f, 0.0f);
    }
}

// ---------------------------------------------------------------------------
// Bone collision info
// ---------------------------------------------------------------------------

static void FbxBuildBoneCollisionInfo(FbxModel& model, ufbx_scene* scene)
{
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        ufbx_mesh* mesh = model.meshes_[i];
        if (mesh->skin_deformers.count == 0)
            continue;

        ufbx_skin_deformer* skin = mesh->skin_deformers.data[0];
        for (size_t ci = 0; ci < skin->clusters.count; ++ci)
        {
            ufbx_skin_cluster* cluster = skin->clusters.data[ci];
            if (!cluster->bone_node)
                continue;

            String boneName(cluster->bone_node->name.data);
            unsigned boneIndex = FbxGetBoneIndex(model, boneName);
            if (boneIndex == M_MAX_UNSIGNED)
                continue;

            for (size_t wi = 0; wi < cluster->num_weights; ++wi)
            {
                float weight = (float)cluster->weights.data[wi];
                if (weight > 0.33f)
                {
                    uint32_t vertexIndex = cluster->vertices.data[wi];
                    if (vertexIndex < mesh->num_vertices)
                    {
                        ufbx_vec3 pos = mesh->vertices.data[vertexIndex];
                        // Transform vertex to bone space using geometry_to_bone
                        ufbx_vec3 boneSpace;
                        boneSpace.x = cluster->geometry_to_bone.m00 * pos.x + cluster->geometry_to_bone.m01 * pos.y + cluster->geometry_to_bone.m02 * pos.z + cluster->geometry_to_bone.m03;
                        boneSpace.y = cluster->geometry_to_bone.m10 * pos.x + cluster->geometry_to_bone.m11 * pos.y + cluster->geometry_to_bone.m12 * pos.z + cluster->geometry_to_bone.m13;
                        boneSpace.z = cluster->geometry_to_bone.m20 * pos.x + cluster->geometry_to_bone.m21 * pos.y + cluster->geometry_to_bone.m22 * pos.z + cluster->geometry_to_bone.m23;

                        Vector3 vertex = ToVector3(boneSpace);
                        float radius = vertex.Length();
                        if (radius > model.boneRadii_[boneIndex])
                            model.boneRadii_[boneIndex] = radius;
                        model.boneHitboxes_[boneIndex].Merge(vertex);
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Blend data (skinning weights per vertex)
// ---------------------------------------------------------------------------

static void FbxGetBlendData(FbxModel& model, ufbx_mesh* mesh, ufbx_node* meshNode,
    PODVector<unsigned>& boneMappings,
    Vector<PODVector<unsigned char> >& blendIndices,
    Vector<PODVector<float> >& blendWeights,
    size_t numVertices)
{
    blendIndices.Resize(numVertices);
    blendWeights.Resize(numVertices);
    boneMappings.Clear();

    if (mesh->skin_deformers.count == 0)
    {
        // Rigid skinning: attach all vertices to the mesh node's bone
        String boneName(meshNode->name.data);
        unsigned globalIndex = FbxGetBoneIndex(model, boneName);
        if (globalIndex == M_MAX_UNSIGNED)
        {
            PrintLine("Warning: bone " + boneName + " not found, skipping rigid skinning");
            return;
        }

        if (model.bones_.Size() > maxBones_)
        {
            boneMappings.Push(globalIndex);
            for (unsigned i = 0; i < numVertices; ++i)
            {
                blendIndices[i].Push(0);
                blendWeights[i].Push(1.0f);
            }
        }
        else
        {
            for (unsigned i = 0; i < numVertices; ++i)
            {
                blendIndices[i].Push((unsigned char)globalIndex);
                blendWeights[i].Push(1.0f);
            }
        }
        return;
    }

    ufbx_skin_deformer* skin = mesh->skin_deformers.data[0];

    if (model.bones_.Size() > maxBones_)
    {
        if (skin->clusters.count > maxBones_)
        {
            ErrorExit(
                "Geometry (submesh) has over " + String(maxBones_) + " bone influences. Try splitting to more submeshes\n"
                "that each stay at " + String(maxBones_) + " bones or below."
            );
        }

        boneMappings.Resize((unsigned)skin->clusters.count);
        for (size_t i = 0; i < skin->clusters.count; ++i)
        {
            ufbx_skin_cluster* cluster = skin->clusters.data[i];
            String boneName(cluster->bone_node ? cluster->bone_node->name.data : "");
            unsigned globalIndex = FbxGetBoneIndex(model, boneName);
            if (globalIndex == M_MAX_UNSIGNED)
                ErrorExit("Bone " + boneName + " not found");
            boneMappings[i] = globalIndex;
        }

        // Use skin->vertices[]/weights[] for per-vertex data
        // Note: skin->vertices may have fewer entries than mesh vertices
        for (unsigned vi = 0; vi < numVertices; ++vi)
        {
            // Map from deduplicated vertex back to original vertex index
            // We'll handle this mapping at the call site
        }
    }
    else
    {
        // Global bone indices: map cluster index to model bone index
        PODVector<unsigned> clusterToBone;
        clusterToBone.Resize((unsigned)skin->clusters.count);
        for (size_t i = 0; i < skin->clusters.count; ++i)
        {
            ufbx_skin_cluster* cluster = skin->clusters.data[i];
            String boneName(cluster->bone_node ? cluster->bone_node->name.data : "");
            unsigned globalIndex = FbxGetBoneIndex(model, boneName);
            if (globalIndex == M_MAX_UNSIGNED)
                ErrorExit("Bone " + boneName + " not found");
            clusterToBone[i] = globalIndex;
        }

        // Store per-vertex blend data
        // Note: at this point numVertices = mesh->num_vertices (logical vertices)
        // Weights are accessed through skin->vertices[vi]
        for (unsigned vi = 0; vi < numVertices; ++vi)
        {
            if (vi < (unsigned)skin->vertices.count)
            {
                ufbx_skin_vertex sv = skin->vertices.data[vi];
                for (uint32_t wi = 0; wi < sv.num_weights; ++wi)
                {
                    ufbx_skin_weight sw = skin->weights.data[sv.weight_begin + wi];
                    unsigned boneIdx;
                    if (model.bones_.Size() > maxBones_)
                        boneIdx = (unsigned)sw.cluster_index;
                    else
                        boneIdx = clusterToBone[sw.cluster_index];
                    blendIndices[vi].Push((unsigned char)boneIdx);
                    blendWeights[vi].Push((float)sw.weight);
                }
            }
        }
    }

    // Normalize weights, remove excess influences (cap at 4)
    for (unsigned i = 0; i < blendWeights.Size(); ++i)
    {
        while (blendWeights[i].Size() > 4)
        {
            unsigned lowestIndex = 0;
            float lowest = M_INFINITY;
            for (unsigned j = 0; j < blendWeights[i].Size(); ++j)
            {
                if (blendWeights[i][j] < lowest)
                {
                    lowest = blendWeights[i][j];
                    lowestIndex = j;
                }
            }
            blendWeights[i].Erase(lowestIndex);
            blendIndices[i].Erase(lowestIndex);
        }

        float sum = 0.0f;
        for (unsigned j = 0; j < blendWeights[i].Size(); ++j)
            sum += blendWeights[i][j];
        if (sum != 1.0f && sum != 0.0f)
        {
            for (unsigned j = 0; j < blendWeights[i].Size(); ++j)
                blendWeights[i][j] /= sum;
        }
    }
}

// ---------------------------------------------------------------------------
// Vertex elements description
// ---------------------------------------------------------------------------

static PODVector<VertexElement> FbxGetVertexElements(ufbx_mesh* mesh, bool isSkinned)
{
    PODVector<VertexElement> ret;

    ret.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION));

    if (mesh->vertex_normal.exists)
        ret.Push(VertexElement(TYPE_VECTOR3, SEM_NORMAL));

    for (unsigned i = 0; i < (unsigned)mesh->color_sets.count && i < MAX_CHANNELS; ++i)
        ret.Push(VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR, i));

    for (unsigned i = 0; i < (unsigned)mesh->uv_sets.count && i < MAX_CHANNELS; ++i)
        ret.Push(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, i));

    if (mesh->vertex_tangent.exists && mesh->vertex_bitangent.exists)
        ret.Push(VertexElement(TYPE_VECTOR4, SEM_TANGENT));

    if (isSkinned)
    {
        ret.Push(VertexElement(TYPE_VECTOR4, SEM_BLENDWEIGHTS));
        ret.Push(VertexElement(TYPE_UBYTE4, SEM_BLENDINDICES));
    }

    return ret;
}

// ---------------------------------------------------------------------------
// Build and save model
// ---------------------------------------------------------------------------

// Helper: triangulate a mesh part and write vertex/index data into Urho3D buffers
static void FbxProcessMeshPart(
    ufbx_mesh* mesh, ufbx_node* meshNode, ufbx_mesh_part* part,
    FbxModel& model, ufbx_scene* scene,
    bool isSkinned, bool combineBuffers,
    SharedPtr<VertexBuffer>& vb, SharedPtr<IndexBuffer>& ib,
    unsigned& startVertexOffset, unsigned& startIndexOffset,
    BoundingBox& box, unsigned destGeomIndex,
    SharedPtr<Model>& outModel, Vector<PODVector<unsigned> >& allBoneMappings)
{
    if (part->num_triangles == 0)
        return;

    // Get blend data per logical vertex (before triangulation/dedup)
    Vector<PODVector<unsigned char> > blendIndices;
    Vector<PODVector<float> > blendWeights;
    PODVector<unsigned> boneMappings;
    if (isSkinned)
        FbxGetBlendData(model, mesh, meshNode, boneMappings, blendIndices, blendWeights, (unsigned)mesh->num_vertices);

    // Triangulate and build per-corner vertex data
    PODVector<uint32_t> triIndices;
    triIndices.Resize((unsigned)part->num_triangles * 3);

    unsigned triOffset = 0;
    for (size_t fi = 0; fi < part->num_faces; ++fi)
    {
        ufbx_face face = mesh->faces.data[part->face_indices.data[fi]];
        uint32_t numTris = ufbx_triangulate_face(&triIndices[triOffset], triIndices.Size() - triOffset, mesh, face);
        triOffset += numTris * 3;
    }
    unsigned numTriIndices = triOffset;

    // Build vertex data for each triangle corner
    struct RawVertex
    {
        Vector3 position;
        Vector3 normal;
        Vector2 uvs[MAX_CHANNELS];
        Color colors[MAX_CHANNELS];
        Vector4 tangent;
        float blendW[4];
        unsigned char blendI[4];
    };

    unsigned numUVSets = Min((unsigned)mesh->uv_sets.count, MAX_CHANNELS);
    unsigned numColorSets = Min((unsigned)mesh->color_sets.count, MAX_CHANNELS);
    bool hasTangents = mesh->vertex_tangent.exists && mesh->vertex_bitangent.exists;

    PODVector<RawVertex> rawVertices;
    rawVertices.Resize(numTriIndices);

    for (unsigned i = 0; i < numTriIndices; ++i)
    {
        uint32_t idx = triIndices[i]; // index into mesh attribute arrays
        RawVertex& rv = rawVertices[i];

        // Position - use logical vertex index
        uint32_t vertexIndex = mesh->vertex_indices.data[idx];
        rv.position = ToVector3(mesh->vertex_position.values.data[mesh->vertex_position.indices.data[idx]]);

        // Normal
        if (mesh->vertex_normal.exists)
            rv.normal = ToVector3(mesh->vertex_normal.values.data[mesh->vertex_normal.indices.data[idx]]);

        // UVs
        for (unsigned s = 0; s < numUVSets; ++s)
            rv.uvs[s] = ToVector2(mesh->uv_sets.data[s].vertex_uv.values.data[mesh->uv_sets.data[s].vertex_uv.indices.data[idx]]);

        // Colors
        for (unsigned s = 0; s < numColorSets; ++s)
        {
            ufbx_vec4 c = mesh->color_sets.data[s].vertex_color.values.data[mesh->color_sets.data[s].vertex_color.indices.data[idx]];
            rv.colors[s] = Color((float)c.x, (float)c.y, (float)c.z, (float)c.w);
        }

        // Tangent + handedness
        if (hasTangents)
        {
            Vector3 tan = ToVector3(mesh->vertex_tangent.values.data[mesh->vertex_tangent.indices.data[idx]]);
            Vector3 norm = rv.normal;
            Vector3 bitan = ToVector3(mesh->vertex_bitangent.values.data[mesh->vertex_bitangent.indices.data[idx]]);
            float w = (tan.CrossProduct(norm).DotProduct(bitan) < 0.5f) ? -1.0f : 1.0f;
            rv.tangent = Vector4(tan.x_, tan.y_, tan.z_, w);
        }

        // Blend data (from logical vertex)
        memset(rv.blendW, 0, sizeof(rv.blendW));
        memset(rv.blendI, 0, sizeof(rv.blendI));
        if (isSkinned && vertexIndex < (unsigned)blendWeights.Size())
        {
            for (unsigned bi = 0; bi < 4 && bi < blendWeights[vertexIndex].Size(); ++bi)
            {
                rv.blendW[bi] = blendWeights[vertexIndex][bi];
                rv.blendI[bi] = blendIndices[vertexIndex][bi];
            }
        }
    }

    // Deduplicate vertices using ufbx_generate_indices
    PODVector<uint32_t> dedupIndices;
    dedupIndices.Resize(numTriIndices);

    ufbx_vertex_stream stream;
    stream.data = rawVertices.Buffer();
    stream.vertex_count = numTriIndices;
    stream.vertex_size = sizeof(RawVertex);

    ufbx_error err;
    size_t numUniqueVertices = ufbx_generate_indices(&stream, 1, dedupIndices.Buffer(), numTriIndices, nullptr, &err);

    PrintLine("Writing geometry with " + String((unsigned)numUniqueVertices) + " vertices " +
        String(numTriIndices) + " indices");

    if (model.bones_.Size() > 0 && !isSkinned)
        PrintLine("Warning: model has bones but geometry has no skinning information");

    bool largeIndices;
    if (combineBuffers)
        largeIndices = false; // Will be determined by caller
    else
        largeIndices = (numUniqueVertices + startVertexOffset) > 65535;

    // Create buffers if needed
    if (!combineBuffers || !vb)
    {
        PODVector<VertexElement> elements = FbxGetVertexElements(mesh, isSkinned);
        vb = new VertexBuffer(context_);
        ib = new IndexBuffer(context_);
        largeIndices = numUniqueVertices > 65535;
        vb->SetSize((unsigned)numUniqueVertices, elements);
        ib->SetSize(numTriIndices, largeIndices);
        startVertexOffset = 0;
        startIndexOffset = 0;
    }

    // Write index data
    unsigned char* indexData = ib->GetShadowData();
    if (!ib->GetIndexSize() || ib->GetIndexSize() == 2)
    {
        auto* dest = (unsigned short*)indexData + startIndexOffset;
        for (unsigned i = 0; i < numTriIndices; ++i)
            *dest++ = (unsigned short)(dedupIndices[i] + startVertexOffset);
    }
    else
    {
        auto* dest = (unsigned*)indexData + startIndexOffset;
        for (unsigned i = 0; i < numTriIndices; ++i)
            *dest++ = dedupIndices[i] + startVertexOffset;
    }

    // Write vertex data
    PODVector<VertexElement> elements = FbxGetVertexElements(mesh, isSkinned);
    unsigned vertexSize = vb->GetVertexSize();
    unsigned char* vertexData = vb->GetShadowData();
    auto* base = (float*)(vertexData + startVertexOffset * vertexSize);

    for (unsigned vi = 0; vi < numUniqueVertices; ++vi)
    {
        RawVertex& rv = rawVertices[vi];
        auto* dest = (float*)((unsigned char*)base + vi * vertexSize);

        // Position
        Vector3 vertex = rv.position;
        box.Merge(vertex);
        *dest++ = vertex.x_;
        *dest++ = vertex.y_;
        *dest++ = vertex.z_;

        // Normal
        if (mesh->vertex_normal.exists)
        {
            *dest++ = rv.normal.x_;
            *dest++ = rv.normal.y_;
            *dest++ = rv.normal.z_;
        }

        // Colors
        for (unsigned s = 0; s < numColorSets; ++s)
        {
            *((unsigned*)dest) = rv.colors[s].ToUInt();
            ++dest;
        }

        // UVs
        for (unsigned s = 0; s < numUVSets; ++s)
        {
            *dest++ = rv.uvs[s].x_;
            *dest++ = rv.uvs[s].y_;
        }

        // Tangent
        if (hasTangents)
        {
            *dest++ = rv.tangent.x_;
            *dest++ = rv.tangent.y_;
            *dest++ = rv.tangent.z_;
            *dest++ = rv.tangent.w_;
        }

        // Blend data
        if (isSkinned)
        {
            for (unsigned bi = 0; bi < 4; ++bi)
                *dest++ = rv.blendW[bi];

            auto* destBytes = (unsigned char*)dest;
            ++dest;
            for (unsigned bi = 0; bi < 4; ++bi)
                *destBytes++ = rv.blendI[bi];
        }
    }

    // Calculate geometry center
    Vector3 center = Vector3::ZERO;
    if (numTriIndices > 0)
    {
        for (unsigned i = 0; i < numTriIndices; ++i)
            center += rawVertices[dedupIndices[i]].position;
        center /= (float)numTriIndices;
    }

    SharedPtr<Geometry> geom(new Geometry(context_));
    geom->SetIndexBuffer(ib);
    geom->SetVertexBuffer(0, vb);
    geom->SetDrawRange(TRIANGLE_LIST, startIndexOffset, numTriIndices, true);
    outModel->SetNumGeometryLodLevels(destGeomIndex, 1);
    outModel->SetGeometry(destGeomIndex, 0, geom);
    outModel->SetGeometryCenter(destGeomIndex, center);
    if (model.bones_.Size() > maxBones_)
        allBoneMappings.Push(boneMappings);

    startVertexOffset += (unsigned)numUniqueVertices;
    startIndexOffset += numTriIndices;
}

static void FbxBuildAndSaveModel(FbxModel& model, ufbx_scene* scene)
{
    if (!model.rootNode_)
    {
        PrintLine("Null root node for model, skipping model save");
        return;
    }

    String rootNodeName(model.rootNode_->name.data);
    if (!model.meshes_.Size())
    {
        PrintLine("No geometries found starting from node " + rootNodeName + ", skipping model save");
        return;
    }

    PrintLine("Writing model " + rootNodeName);

    // Count total geometries (one per material part per mesh)
    unsigned numGeometries = 0;
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        ufbx_mesh* mesh = model.meshes_[i];
        unsigned numParts = (unsigned)mesh->material_parts.count;
        if (numParts == 0)
            numParts = 1; // Mesh with no materials still needs one geometry
        numGeometries += numParts;
    }

    SharedPtr<Model> outModel(new Model(context_));
    Vector<PODVector<unsigned> > allBoneMappings;
    BoundingBox box;
    bool isSkinned = model.bones_.Size() > 0;

    outModel->SetNumGeometries(numGeometries);

    Vector<SharedPtr<VertexBuffer> > vbVector;
    Vector<SharedPtr<IndexBuffer> > ibVector;

    unsigned destGeomIndex = 0;
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        ufbx_mesh* mesh = model.meshes_[i];
        ufbx_node* meshNode = model.meshNodes_[i];
        unsigned numParts = (unsigned)mesh->material_parts.count;

        if (numParts == 0)
        {
            // Mesh has no material parts - treat as single part covering all faces
            // Build the entire mesh as one geometry
            SharedPtr<VertexBuffer> vb;
            SharedPtr<IndexBuffer> ib;
            unsigned startVertexOffset = 0;
            unsigned startIndexOffset = 0;

            // Create a synthetic mesh_part covering all faces
            ufbx_mesh_part syntheticPart = {};
            syntheticPart.num_faces = mesh->num_faces;
            syntheticPart.num_triangles = mesh->num_triangles;
            // Build face_indices covering all faces
            PODVector<uint32_t> allFaceIndices;
            allFaceIndices.Resize((unsigned)mesh->num_faces);
            for (unsigned fi = 0; fi < (unsigned)mesh->num_faces; ++fi)
                allFaceIndices[fi] = fi;
            syntheticPart.face_indices.data = allFaceIndices.Buffer();
            syntheticPart.face_indices.count = mesh->num_faces;

            FbxProcessMeshPart(mesh, meshNode, &syntheticPart, model, scene,
                isSkinned, false, vb, ib,
                startVertexOffset, startIndexOffset,
                box, destGeomIndex, outModel, allBoneMappings);

            vbVector.Push(vb);
            ibVector.Push(ib);
            ++destGeomIndex;
        }
        else
        {
            for (unsigned p = 0; p < numParts; ++p)
            {
                ufbx_mesh_part* part = &mesh->material_parts.data[p];
                if (part->num_triangles == 0)
                {
                    // Empty part, still need to account for geometry slot
                    // Create minimal empty geometry
                    SharedPtr<VertexBuffer> vb(new VertexBuffer(context_));
                    SharedPtr<IndexBuffer> ib(new IndexBuffer(context_));
                    PODVector<VertexElement> elements = FbxGetVertexElements(mesh, isSkinned);
                    vb->SetSize(0, elements);
                    ib->SetSize(0, false);
                    vbVector.Push(vb);
                    ibVector.Push(ib);
                    SharedPtr<Geometry> geom(new Geometry(context_));
                    geom->SetIndexBuffer(ib);
                    geom->SetVertexBuffer(0, vb);
                    geom->SetDrawRange(TRIANGLE_LIST, 0, 0, true);
                    outModel->SetNumGeometryLodLevels(destGeomIndex, 1);
                    outModel->SetGeometry(destGeomIndex, 0, geom);
                    outModel->SetGeometryCenter(destGeomIndex, Vector3::ZERO);
                    ++destGeomIndex;
                    continue;
                }

                SharedPtr<VertexBuffer> vb;
                SharedPtr<IndexBuffer> ib;
                unsigned startVertexOffset = 0;
                unsigned startIndexOffset = 0;

                FbxProcessMeshPart(mesh, meshNode, part, model, scene,
                    isSkinned, false, vb, ib,
                    startVertexOffset, startIndexOffset,
                    box, destGeomIndex, outModel, allBoneMappings);

                vbVector.Push(vb);
                ibVector.Push(ib);
                ++destGeomIndex;
            }
        }
    }

    // Define model buffers and bounding box
    PODVector<unsigned> emptyMorphRange;
    outModel->SetVertexBuffers(vbVector, emptyMorphRange, emptyMorphRange);
    outModel->SetIndexBuffers(ibVector);
    outModel->SetBoundingBox(box);

    // Build skeleton
    if (model.bones_.Size() && model.rootBone_)
    {
        PrintLine("Writing skeleton with " + String(model.bones_.Size()) + " bones, rootbone " +
            String(model.rootBone_->name.data));

        Skeleton skeleton;
        Vector<Bone>& bones = skeleton.GetModifiableBones();

        for (unsigned i = 0; i < model.bones_.Size(); ++i)
        {
            ufbx_node* boneNode = model.bones_[i];

            Bone newBone;
            newBone.name_ = String(boneNode->name.data);

            ufbx_transform t = boneNode->local_transform;
            newBone.initialPosition_ = ToVector3(t.translation);
            newBone.initialRotation_ = ToQuaternion(t.rotation);
            newBone.initialScale_ = ToVector3(t.scale);

            // If root bone, include transforms between root bone and model root
            if (boneNode == model.rootBone_ && boneNode != model.rootNode_)
            {
                // Compute transform relative to model root
                Matrix3x4 boneWorld = ToMatrix3x4(boneNode->node_to_world);
                Matrix3x4 modelRootInv = ToMatrix3x4(model.rootNode_->node_to_world).Inverse();
                Matrix3x4 relativeTransform = modelRootInv * boneWorld;
                newBone.initialPosition_ = relativeTransform.Translation();
                newBone.initialRotation_ = relativeTransform.Rotation();
                newBone.initialScale_ = relativeTransform.Scale();
            }

            // Get offset matrix from skin clusters
            newBone.offsetMatrix_ = Matrix3x4::IDENTITY;
            for (unsigned mi = 0; mi < model.meshes_.Size(); ++mi)
            {
                ufbx_mesh* mesh = model.meshes_[mi];
                if (mesh->skin_deformers.count == 0)
                    continue;
                ufbx_skin_deformer* skin = mesh->skin_deformers.data[0];
                bool found = false;
                for (size_t ci = 0; ci < skin->clusters.count; ++ci)
                {
                    ufbx_skin_cluster* cluster = skin->clusters.data[ci];
                    if (cluster->bone_node == boneNode)
                    {
                        newBone.offsetMatrix_ = ToMatrix3x4(cluster->geometry_to_bone);
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }

            newBone.radius_ = model.boneRadii_[i];
            newBone.boundingBox_ = model.boneHitboxes_[i];
            newBone.collisionMask_ = BONECOLLISION_SPHERE | BONECOLLISION_BOX;
            newBone.parentIndex_ = i; // Will be fixed below

            bones.Push(newBone);
        }

        // Set bone hierarchy
        for (unsigned i = 1; i < model.bones_.Size(); ++i)
        {
            if (model.bones_[i]->parent)
            {
                const char* parentName = model.bones_[i]->parent->name.data;
                for (unsigned j = 0; j < bones.Size(); ++j)
                {
                    if (bones[j].name_ == parentName)
                    {
                        bones[i].parentIndex_ = j;
                        break;
                    }
                }
            }
        }

        outModel->SetSkeleton(skeleton);
        if (model.bones_.Size() > maxBones_)
            outModel->SetGeometryBoneMappings(allBoneMappings);
    }

    File outFile(context_);
    if (!outFile.Open(model.outName_, FILE_WRITE))
        ErrorExit("Could not open output file " + model.outName_);
    outModel->Save(outFile);

    // Save material list
    if (!noMaterials_ && saveMaterialList_)
    {
        String materialListName = ReplaceExtension(model.outName_, ".txt");
        File listFile(context_);
        if (listFile.Open(materialListName, FILE_WRITE))
        {
            for (unsigned i = 0; i < model.meshes_.Size(); ++i)
            {
                ufbx_mesh* mesh = model.meshes_[i];
                unsigned numParts = (unsigned)mesh->material_parts.count;
                if (numParts == 0)
                {
                    listFile.WriteLine((useSubdirs_ ? "Materials/" : "") + inputName_ + "_Material.xml");
                }
                else
                {
                    for (unsigned p = 0; p < numParts; ++p)
                    {
                        unsigned matIdx = mesh->material_parts.data[p].index;
                        if (matIdx < mesh->materials.count)
                        {
                            String matName = SanitateAssetName(String(mesh->materials.data[matIdx]->name.data));
                            if (matName.Trimmed().Empty())
                                matName = inputName_ + "_Material" + String(matIdx);
                            listFile.WriteLine((useSubdirs_ ? "Materials/" : "") + matName + ".xml");
                        }
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Animations
// ---------------------------------------------------------------------------

static void FbxBuildAndSaveAnimations(ufbx_scene* scene, FbxModel* model)
{
    for (size_t ai = 0; ai < scene->anim_stacks.count; ++ai)
    {
        ufbx_anim_stack* stack = scene->anim_stacks.data[ai];
        ufbx_anim* anim = stack->anim;

        String animName(stack->name.data);
        if (animName.Empty())
            animName = "Anim" + String((unsigned)ai + 1);

        String animOutName;
        if (model)
            animOutName = GetPath(model->outName_) + GetFileName(model->outName_) + "_" + SanitateAssetName(animName) + ".ani";
        else
            animOutName = outPath_ + GetFileName(outName_) + "_" + SanitateAssetName(animName) + ".ani";

        float duration = (float)(stack->time_end - stack->time_begin);
        float thisImportStartTime = importStartTime_;
        float thisImportEndTime = importEndTime_;

        if (thisImportEndTime == 0.0f)
            thisImportEndTime = duration;

        // Adjust to actual animation range
        float animStart = (float)stack->time_begin;
        if (animStart > thisImportStartTime)
            thisImportStartTime = animStart;
        duration = thisImportEndTime - thisImportStartTime;

        SharedPtr<Animation> outAnim(new Animation(context_));
        outAnim->SetAnimationName(animName);
        outAnim->SetLength(duration);

        PrintLine("Writing animation " + animName + " length " + String(duration));

        // Collect all unique key times from all relevant animation curves for each bone
        PODVector<ufbx_node*>& bones = model ? model->bones_ : *(PODVector<ufbx_node*>*)nullptr;
        bool hasModel = (model != nullptr);

        if (!hasModel)
        {
            // Scene-global animations: iterate all nodes
            for (size_t ni = 0; ni < scene->nodes.count; ++ni)
            {
                ufbx_node* node = scene->nodes.data[ni];
                String channelName(node->name.data);
                if (channelName.Empty())
                    continue;

                // Collect key times from all animation curves for this node in this stack
                PODVector<float> keyTimes;
                for (size_t li = 0; li < stack->layers.count; ++li)
                {
                    ufbx_anim_layer* layer = stack->layers.data[li];
                    ufbx_anim_prop_list props = ufbx_find_anim_props(layer, &node->element);
                    for (size_t pi = 0; pi < props.count; ++pi)
                    {
                        ufbx_anim_value* val = props.data[pi].anim_value;
                        for (int ci = 0; ci < 3; ++ci)
                        {
                            if (val->curves[ci])
                            {
                                for (size_t ki = 0; ki < val->curves[ci]->keyframes.count; ++ki)
                                {
                                    float t = (float)val->curves[ci]->keyframes.data[ki].time;
                                    if (t >= thisImportStartTime && t <= thisImportEndTime)
                                        keyTimes.Push(t);
                                }
                            }
                        }
                    }
                }

                if (keyTimes.Empty())
                    continue;

                Sort(keyTimes.Begin(), keyTimes.End());
                // Remove duplicates
                for (unsigned ki = 1; ki < keyTimes.Size();)
                {
                    if (keyTimes[ki] == keyTimes[ki - 1])
                        keyTimes.Erase(ki);
                    else
                        ++ki;
                }

                AnimationTrack* track = outAnim->CreateTrack(channelName);
                track->channelMask_ = CHANNEL_POSITION | CHANNEL_ROTATION | CHANNEL_SCALE;

                for (unsigned ki = 0; ki < keyTimes.Size(); ++ki)
                {
                    float t = keyTimes[ki];
                    ufbx_transform xform = ufbx_evaluate_transform(anim, node, (double)t);

                    AnimationKeyFrame kf;
                    kf.time_ = t - thisImportStartTime;
                    kf.position_ = ToVector3(xform.translation);
                    kf.rotation_ = ToQuaternion(xform.rotation);
                    kf.scale_ = ToVector3(xform.scale);
                    track->keyFrames_.Push(kf);
                }
            }
        }
        else
        {
            // Model-specific animations: only export bones that are in the skeleton
            for (unsigned bi = 0; bi < bones.Size(); ++bi)
            {
                ufbx_node* boneNode = bones[bi];
                String channelName(boneNode->name.data);

                // Collect key times
                PODVector<float> keyTimes;
                for (size_t li = 0; li < stack->layers.count; ++li)
                {
                    ufbx_anim_layer* layer = stack->layers.data[li];
                    ufbx_anim_prop_list props = ufbx_find_anim_props(layer, &boneNode->element);
                    for (size_t pi = 0; pi < props.count; ++pi)
                    {
                        ufbx_anim_value* val = props.data[pi].anim_value;
                        for (int ci = 0; ci < 3; ++ci)
                        {
                            if (val->curves[ci])
                            {
                                for (size_t ki = 0; ki < val->curves[ci]->keyframes.count; ++ki)
                                {
                                    float t = (float)val->curves[ci]->keyframes.data[ki].time;
                                    if (t >= thisImportStartTime && t <= thisImportEndTime)
                                        keyTimes.Push(t);
                                }
                            }
                        }
                    }
                }

                if (keyTimes.Empty())
                    continue;

                Sort(keyTimes.Begin(), keyTimes.End());
                // Remove duplicates
                for (unsigned ki = 1; ki < keyTimes.Size();)
                {
                    if (keyTimes[ki] == keyTimes[ki - 1])
                        keyTimes.Erase(ki);
                    else
                        ++ki;
                }

                // Evaluate transform at each key time
                AnimationTrack* track = outAnim->CreateTrack(channelName);

                // Determine channel mask by checking if values differ from rest pose
                ufbx_transform restTransform = boneNode->local_transform;
                bool hasPos = false, hasRot = false, hasScale = false;

                PODVector<ufbx_transform> transforms;
                transforms.Resize(keyTimes.Size());
                for (unsigned ki = 0; ki < keyTimes.Size(); ++ki)
                {
                    transforms[ki] = ufbx_evaluate_transform(anim, boneNode, (double)keyTimes[ki]);

                    if (!hasPos && !ToVector3(transforms[ki].translation).Equals(ToVector3(restTransform.translation)))
                        hasPos = true;
                    if (!hasRot && !ToQuaternion(transforms[ki].rotation).Equals(ToQuaternion(restTransform.rotation)))
                        hasRot = true;
                    if (!hasScale && !ToVector3(transforms[ki].scale).Equals(ToVector3(restTransform.scale)))
                        hasScale = true;
                }

                if (keyTimes.Size() > 1)
                {
                    hasPos = true;
                    hasRot = true;
                }

                track->channelMask_ = CHANNEL_NONE;
                if (hasPos) track->channelMask_ |= CHANNEL_POSITION;
                if (hasRot) track->channelMask_ |= CHANNEL_ROTATION;
                if (hasScale) track->channelMask_ |= CHANNEL_SCALE;

                // Check for redundant identity scale
                if (track->channelMask_ & CHANNEL_SCALE)
                {
                    bool redundantScale = true;
                    for (unsigned ki = 0; ki < keyTimes.Size(); ++ki)
                    {
                        Vector3 s = ToVector3(transforms[ki].scale);
                        if (fabsf(s.x_ - 1.0f) >= 0.000001f || fabsf(s.y_ - 1.0f) >= 0.000001f || fabsf(s.z_ - 1.0f) >= 0.000001f)
                        {
                            redundantScale = false;
                            break;
                        }
                    }
                    if (redundantScale)
                        track->channelMask_ &= ~CHANNEL_SCALE;
                }

                if (!track->channelMask_)
                {
                    outAnim->RemoveTrack(channelName);
                    continue;
                }

                for (unsigned ki = 0; ki < keyTimes.Size(); ++ki)
                {
                    AnimationKeyFrame kf;
                    kf.time_ = keyTimes[ki] - thisImportStartTime;
                    kf.time_ = Max(kf.time_, 0.0f);

                    ufbx_transform xform = transforms[ki];

                    // If root bone, transform relative to model root
                    if (model && boneNode == model->rootBone_ && boneNode != model->rootNode_)
                    {
                        // Build transform matrix and make relative to model root
                        Matrix3x4 boneLocal(ToVector3(xform.translation), ToQuaternion(xform.rotation), ToVector3(xform.scale));
                        // Walk parent chain from root bone to model root
                        Matrix3x4 derivedTransform = boneLocal;
                        ufbx_node* parent = boneNode->parent;
                        while (parent && parent != model->rootNode_)
                        {
                            Matrix3x4 parentLocal(ToVector3(parent->local_transform.translation),
                                ToQuaternion(parent->local_transform.rotation),
                                ToVector3(parent->local_transform.scale));
                            derivedTransform = parentLocal * derivedTransform;
                            parent = parent->parent;
                        }

                        kf.position_ = derivedTransform.Translation();
                        kf.rotation_ = derivedTransform.Rotation();
                        kf.scale_ = derivedTransform.Scale();
                    }
                    else
                    {
                        kf.position_ = ToVector3(xform.translation);
                        kf.rotation_ = ToQuaternion(xform.rotation);
                        kf.scale_ = ToVector3(xform.scale);
                    }

                    track->keyFrames_.Push(kf);
                }
            }
        }

        // Only save if there are tracks
        if (outAnim->GetNumTracks() > 0)
        {
            File outFile(context_);
            if (!outFile.Open(animOutName, FILE_WRITE))
                ErrorExit("Could not open output file " + animOutName);
            outAnim->Save(outFile);
        }
    }
}

// ---------------------------------------------------------------------------
// Export model command
// ---------------------------------------------------------------------------

static void FbxExportModel(ufbx_scene* scene, const String& outName, ufbx_node* rootNode)
{
    if (outName.Empty())
        ErrorExit("No output file defined");

    FbxModel model;
    model.rootNode_ = rootNode;
    model.outName_ = outName;

    FbxCollectMeshes(model, rootNode);
    FbxCollectBones(model, scene);
    FbxBuildBoneCollisionInfo(model, scene);
    FbxBuildAndSaveModel(model, scene);

    if (!noAnimations_)
        FbxBuildAndSaveAnimations(scene, &model);
}

// ---------------------------------------------------------------------------
// Export animation command
// ---------------------------------------------------------------------------

static void FbxExportAnimation(ufbx_scene* scene, const String& outName, ufbx_node* rootNode)
{
    if (outName.Empty())
        ErrorExit("No output file defined");

    noMaterials_ = true;

    FbxModel model;
    model.rootNode_ = rootNode;
    model.outName_ = outName;

    FbxCollectMeshes(model, rootNode);
    FbxCollectBones(model, scene);

    // If no bones found from skinning, collect all scene nodes as bones
    if (model.bones_.Size() == 0)
    {
        // Walk the node tree and add all nodes as bones
        for (size_t i = 0; i < scene->nodes.count; ++i)
            model.bones_.Push(scene->nodes.data[i]);
        if (scene->nodes.count > 0)
            model.rootBone_ = scene->root_node;
    }

    if (!noAnimations_)
        FbxBuildAndSaveAnimations(scene, &model);
}

// ---------------------------------------------------------------------------
// Scene export
// ---------------------------------------------------------------------------

static void FbxCollectSceneModels(FbxScene& fbxScene, ufbx_scene* scene, ufbx_node* node)
{
    if (node->mesh)
    {
        FbxModel model;
        model.rootNode_ = node;
        model.outName_ = resourcePath_ + (useSubdirs_ ? "Models/" : "") + SanitateAssetName(String(node->name.data)) + ".mdl";

        model.meshes_.Push(node->mesh);
        model.meshNodes_.Push(node);

        // Check uniqueness
        bool unique = true;
        if (checkUniqueModel_)
        {
            for (unsigned i = 0; i < fbxScene.models_.Size(); ++i)
            {
                if (fbxScene.models_[i].meshes_.Size() == model.meshes_.Size())
                {
                    bool same = true;
                    for (unsigned j = 0; j < model.meshes_.Size(); ++j)
                    {
                        if (fbxScene.models_[i].meshes_[j] != model.meshes_[j])
                        {
                            same = false;
                            break;
                        }
                    }
                    if (same)
                    {
                        PrintLine("Added node " + String(node->name.data));
                        fbxScene.nodes_.Push(node);
                        fbxScene.nodeModelIndices_.Push(i);
                        unique = false;
                        break;
                    }
                }
            }
        }

        if (unique)
        {
            PrintLine("Added model " + model.outName_);
            PrintLine("Added node " + String(node->name.data));
            FbxCollectBones(model, scene);
            FbxBuildBoneCollisionInfo(model, scene);

            if (!noAnimations_)
                FbxBuildAndSaveAnimations(scene, &model);

            fbxScene.models_.Push(model);
            fbxScene.nodes_.Push(node);
            fbxScene.nodeModelIndices_.Push(fbxScene.models_.Size() - 1);
        }
    }

    for (size_t i = 0; i < node->children.count; ++i)
        FbxCollectSceneModels(fbxScene, scene, node->children.data[i]);
}

static void FbxCreateHierarchy(Scene* outScene, ufbx_node* srcNode, ufbx_node* rootNode, HashMap<ufbx_node*, Node*>& nodeMapping)
{
    FbxCreateSceneNode(outScene, srcNode, rootNode, nodeMapping);
    for (size_t i = 0; i < srcNode->children.count; ++i)
        FbxCreateHierarchy(outScene, srcNode->children.data[i], rootNode, nodeMapping);
}

static Node* FbxCreateSceneNode(Scene* outScene, ufbx_node* srcNode, ufbx_node* rootNode, HashMap<ufbx_node*, Node*>& nodeMapping)
{
    if (nodeMapping.Contains(srcNode))
        return nodeMapping[srcNode];

    String nodeName(srcNode->name.data);

    if (noHierarchy_)
    {
        Node* outNode = outScene->CreateChild(nodeName, localIDs_ ? LOCAL : REPLICATED);
        Matrix3x4 worldTransform = ToMatrix3x4(srcNode->node_to_world);
        Matrix3x4 rootInv = ToMatrix3x4(rootNode->node_to_world).Inverse();
        Matrix3x4 relative = rootInv * worldTransform;
        outNode->SetTransform(relative.Translation(), relative.Rotation(), relative.Scale());
        nodeMapping[srcNode] = outNode;
        return outNode;
    }

    if (srcNode == rootNode || !srcNode->parent)
    {
        Node* outNode = outScene->CreateChild(nodeName, localIDs_ ? LOCAL : REPLICATED);
        ufbx_transform t = srcNode->local_transform;
        outNode->SetTransform(ToVector3(t.translation), ToQuaternion(t.rotation), ToVector3(t.scale));
        nodeMapping[srcNode] = outNode;
        return outNode;
    }
    else
    {
        if (!nodeMapping.Contains(srcNode->parent))
            FbxCreateSceneNode(outScene, srcNode->parent, rootNode, nodeMapping);

        Node* parent = nodeMapping[srcNode->parent];
        Node* outNode = parent->CreateChild(nodeName, localIDs_ ? LOCAL : REPLICATED);
        ufbx_transform t = srcNode->local_transform;
        outNode->SetTransform(ToVector3(t.translation), ToQuaternion(t.rotation), ToVector3(t.scale));
        nodeMapping[srcNode] = outNode;
        return outNode;
    }
}

static String FbxGetMeshMaterialName(ufbx_mesh* mesh, unsigned partIndex)
{
    if (partIndex < mesh->materials.count)
    {
        String matName = SanitateAssetName(String(mesh->materials.data[partIndex]->name.data));
        if (matName.Trimmed().Empty())
            matName = inputName_ + "_Material" + String(partIndex);
        return (useSubdirs_ ? "Materials/" : "") + matName + ".xml";
    }
    return (useSubdirs_ ? "Materials/" : "") + inputName_ + "_Material.xml";
}

static void FbxBuildAndSaveScene(FbxScene& fbxScene, ufbx_scene* scene, bool asPrefab)
{
    if (!asPrefab)
        PrintLine("Writing scene");
    else
        PrintLine("Writing node hierarchy");

    SharedPtr<Scene> outScene(new Scene(context_));

    if (!asPrefab)
    {
#ifdef URHO3D_PHYSICS
        outScene->CreateComponent<PhysicsWorld>();
#endif
        outScene->CreateComponent<Octree>();
        outScene->CreateComponent<DebugRenderer>();

        if (createZone_)
        {
            Node* zoneNode = outScene->CreateChild("Zone", localIDs_ ? LOCAL : REPLICATED);
            auto* zone = zoneNode->CreateComponent<Zone>();
            zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.f));
            zone->SetAmbientColor(Color(0.25f, 0.25f, 0.25f));

            if (scene->lights.count == 0)
            {
                Node* lightNode = outScene->CreateChild("GlobalLight", localIDs_ ? LOCAL : REPLICATED);
                auto* light = lightNode->CreateComponent<Light>();
                light->SetLightType(LIGHT_DIRECTIONAL);
                lightNode->SetRotation(Quaternion(60.0f, 30.0f, 0.0f));
            }
        }
    }

    auto* cache = context_->GetSubsystem<ResourceCache>();
    HashMap<ufbx_node*, Node*> nodeMapping;

    Node* outRootNode = nullptr;
    if (asPrefab)
        outRootNode = FbxCreateSceneNode(outScene, fbxScene.rootNode_, fbxScene.rootNode_, nodeMapping);
    else
    {
        Matrix3x4 rootTransform = ToMatrix3x4(fbxScene.rootNode_->node_to_world);
        bool rootIsIdentity = rootTransform.Equals(Matrix3x4::IDENTITY);
        bool rootHasModel = false;
        for (unsigned i = 0; i < fbxScene.nodes_.Size(); ++i)
        {
            if (fbxScene.nodes_[i] == fbxScene.rootNode_)
            {
                rootHasModel = true;
                break;
            }
        }
        if (rootIsIdentity && !rootHasModel)
            nodeMapping[fbxScene.rootNode_] = outScene;
    }

    if (!noHierarchy_ && !noEmptyNodes_)
        FbxCreateHierarchy(outScene, fbxScene.rootNode_, fbxScene.rootNode_, nodeMapping);

    // Create geometry nodes
    for (unsigned i = 0; i < fbxScene.nodes_.Size(); ++i)
    {
        const FbxModel& model = fbxScene.models_[fbxScene.nodeModelIndices_[i]];
        Node* modelNode = FbxCreateSceneNode(outScene, fbxScene.nodes_[i], fbxScene.rootNode_, nodeMapping);

        auto* staticModel = static_cast<StaticModel*>(
            model.bones_.Empty() ? modelNode->CreateComponent<StaticModel>() : modelNode->CreateComponent<AnimatedModel>());

        String modelName = (useSubdirs_ ? "Models/" : "") + GetFileNameAndExtension(model.outName_);
        if (!cache->Exists(modelName))
        {
            auto* dummyModel = new Model(context_);
            dummyModel->SetName(modelName);

            unsigned numGeom = 0;
            for (unsigned mi = 0; mi < model.meshes_.Size(); ++mi)
            {
                unsigned parts = (unsigned)model.meshes_[mi]->material_parts.count;
                numGeom += parts > 0 ? parts : 1;
            }
            dummyModel->SetNumGeometries(numGeom);
            cache->AddManualResource(dummyModel);
        }
        staticModel->SetModel(cache->GetResource<Model>(modelName));

        // Set materials
        unsigned matSlot = 0;
        for (unsigned mi = 0; mi < model.meshes_.Size(); ++mi)
        {
            ufbx_mesh* mesh = model.meshes_[mi];
            unsigned numParts = (unsigned)mesh->material_parts.count;
            if (numParts == 0)
            {
                String matName = FbxGetMeshMaterialName(mesh, 0);
                if (!cache->Exists(matName))
                {
                    auto* dummyMat = new Material(context_);
                    dummyMat->SetName(matName);
                    cache->AddManualResource(dummyMat);
                }
                staticModel->SetMaterial(matSlot++, cache->GetResource<Material>(matName));
            }
            else
            {
                for (unsigned p = 0; p < numParts; ++p)
                {
                    String matName = FbxGetMeshMaterialName(mesh, mesh->material_parts.data[p].index);
                    if (!cache->Exists(matName))
                    {
                        auto* dummyMat = new Material(context_);
                        dummyMat->SetName(matName);
                        cache->AddManualResource(dummyMat);
                    }
                    staticModel->SetMaterial(matSlot++, cache->GetResource<Material>(matName));
                }
            }
        }
    }

    // Create lights
    if (!asPrefab)
    {
        for (size_t i = 0; i < scene->lights.count; ++i)
        {
            ufbx_light* light = scene->lights.data[i];
            if (light->instances.count == 0)
                continue;

            ufbx_node* lightNode = light->instances.data[0];
            Node* outNode = FbxCreateSceneNode(outScene, lightNode, fbxScene.rootNode_, nodeMapping);

            auto* outLight = outNode->CreateComponent<Light>();
            outLight->SetColor(Color((float)light->color.x, (float)light->color.y, (float)light->color.z));

            switch (light->type)
            {
            case UFBX_LIGHT_DIRECTIONAL:
                outLight->SetLightType(LIGHT_DIRECTIONAL);
                break;
            case UFBX_LIGHT_SPOT:
                outLight->SetLightType(LIGHT_SPOT);
                outLight->SetFov((float)light->outer_angle);
                break;
            case UFBX_LIGHT_POINT:
                outLight->SetLightType(LIGHT_POINT);
                break;
            default:
                break;
            }
        }
    }

    File file(context_);
    if (!file.Open(fbxScene.outName_, FILE_WRITE))
        ErrorExit("Could not open output file " + fbxScene.outName_);

    Node* output = asPrefab ? outRootNode : outScene;
    if (saveBinary_)
        output->Save(file);
    else if (saveJson_)
        output->SaveJSON(file);
    else
        output->SaveXML(file);
}

static void FbxExportScene(ufbx_scene* scene, const String& outName, ufbx_node* rootNode, bool asPrefab)
{
    FbxScene fbxScene;
    fbxScene.outName_ = outName;
    fbxScene.rootNode_ = rootNode;

    if (useSubdirs_)
        context_->GetSubsystem<FileSystem>()->CreateDir(resourcePath_ + "Models");

    FbxCollectSceneModels(fbxScene, scene, rootNode);

    for (unsigned i = 0; i < fbxScene.models_.Size(); ++i)
        FbxBuildAndSaveModel(fbxScene.models_[i], scene);

    FbxBuildAndSaveScene(fbxScene, scene, asPrefab);
}

// ---------------------------------------------------------------------------
// Materials
// ---------------------------------------------------------------------------

static void FbxExportMaterials(ufbx_scene* scene, HashSet<String>& usedTextures)
{
    if (useSubdirs_)
        context_->GetSubsystem<FileSystem>()->CreateDir(resourcePath_ + "Materials");

    for (size_t i = 0; i < scene->materials.count; ++i)
        FbxBuildAndSaveMaterial(scene, scene->materials.data[i], usedTextures);
}

static String FbxGetTextureFileName(ufbx_texture* texture)
{
    // Prefer relative_filename, fall back to filename
    if (texture->relative_filename.length > 0)
        return GetFileNameAndExtension(String(texture->relative_filename.data));
    if (texture->filename.length > 0)
        return GetFileNameAndExtension(String(texture->filename.data));
    return String::EMPTY;
}

static void FbxBuildAndSaveMaterial(ufbx_scene* scene, ufbx_material* material, HashSet<String>& usedTextures)
{
    String matName = SanitateAssetName(String(material->name.data));
    if (matName.Trimmed().Empty())
        matName = inputName_ + "_Material" + String(material->typed_id);

    XMLFile outMaterial(context_);
    XMLElement materialElem = outMaterial.CreateRoot("material");

    String diffuseTexName;
    String normalTexName;
    String specularTexName;
    String emissiveTexName;
    Color diffuseColor = Color::WHITE;
    Color specularColor;
    Color emissiveColor = Color::BLACK;
    bool hasAlpha = false;
    float specPower = 1.0f;

    // Read FBX material maps
    if (material->fbx.diffuse_color.texture)
        diffuseTexName = FbxGetTextureFileName(material->fbx.diffuse_color.texture);
    if (material->fbx.normal_map.texture)
        normalTexName = FbxGetTextureFileName(material->fbx.normal_map.texture);
    if (material->fbx.specular_color.texture)
        specularTexName = FbxGetTextureFileName(material->fbx.specular_color.texture);
    if (material->fbx.emission_color.texture)
        emissiveTexName = FbxGetTextureFileName(material->fbx.emission_color.texture);

    // Read color values
    if (!noMaterialDiffuseColor_)
    {
        ufbx_vec4 dc = material->fbx.diffuse_color.value_vec4;
        diffuseColor = Color((float)dc.x, (float)dc.y, (float)dc.z, 1.0f);
    }
    {
        ufbx_vec4 sc = material->fbx.specular_color.value_vec4;
        specularColor = Color((float)sc.x, (float)sc.y, (float)sc.z);
    }
    if (!emissiveAO_)
    {
        ufbx_vec4 ec = material->fbx.emission_color.value_vec4;
        emissiveColor = Color((float)ec.x, (float)ec.y, (float)ec.z);
    }

    // Transparency
    float transparency = (float)material->fbx.transparency_factor.value_real;
    if (transparency > 0.0f && transparency < 1.0f)
    {
        hasAlpha = true;
        diffuseColor.a_ = 1.0f - transparency;
    }

    // Specular power
    specPower = (float)material->fbx.specular_exponent.value_real;

    // Build technique name
    String techniqueName = "Techniques/NoTexture";
    if (!diffuseTexName.Empty())
    {
        techniqueName = "Techniques/Diff";
        if (!normalTexName.Empty())
            techniqueName += "Normal";
        if (!specularTexName.Empty())
            techniqueName += "Spec";
        if (normalTexName.Empty() && specularTexName.Empty() && !emissiveTexName.Empty())
            techniqueName += emissiveAO_ ? "AO" : "Emissive";
    }
    if (hasAlpha)
        techniqueName += "Alpha";

    XMLElement techniqueElem = materialElem.CreateChild("technique");
    techniqueElem.SetString("name", techniqueName + ".xml");

    if (!diffuseTexName.Empty())
    {
        XMLElement texElem = materialElem.CreateChild("texture");
        texElem.SetString("unit", "diffuse");
        texElem.SetString("name", (useSubdirs_ ? "Textures/" : "") + diffuseTexName);
        usedTextures.Insert(diffuseTexName);
    }
    if (!normalTexName.Empty())
    {
        XMLElement texElem = materialElem.CreateChild("texture");
        texElem.SetString("unit", "normal");
        texElem.SetString("name", (useSubdirs_ ? "Textures/" : "") + normalTexName);
        usedTextures.Insert(normalTexName);
    }
    if (!specularTexName.Empty())
    {
        XMLElement texElem = materialElem.CreateChild("texture");
        texElem.SetString("unit", "specular");
        texElem.SetString("name", (useSubdirs_ ? "Textures/" : "") + specularTexName);
        usedTextures.Insert(specularTexName);
    }
    if (!emissiveTexName.Empty())
    {
        XMLElement texElem = materialElem.CreateChild("texture");
        texElem.SetString("unit", "emissive");
        texElem.SetString("name", (useSubdirs_ ? "Textures/" : "") + emissiveTexName);
        usedTextures.Insert(emissiveTexName);
    }

    XMLElement diffuseColorElem = materialElem.CreateChild("parameter");
    diffuseColorElem.SetString("name", "MatDiffColor");
    diffuseColorElem.SetColor("value", diffuseColor);
    XMLElement specularElem = materialElem.CreateChild("parameter");
    specularElem.SetString("name", "MatSpecColor");
    specularElem.SetVector4("value", Vector4(specularColor.r_, specularColor.g_, specularColor.b_, specPower));
    XMLElement emissiveColorElem = materialElem.CreateChild("parameter");
    emissiveColorElem.SetString("name", "MatEmissiveColor");
    emissiveColorElem.SetColor("value", emissiveColor);

    auto* fileSystem = context_->GetSubsystem<FileSystem>();
    String outFileName = resourcePath_ + (useSubdirs_ ? "Materials/" : "") + matName + ".xml";
    if (noOverwriteMaterial_ && fileSystem->FileExists(outFileName))
    {
        PrintLine("Skipping save of existing material " + matName);
        return;
    }

    PrintLine("Writing material " + matName);

    File outFile(context_);
    if (!outFile.Open(outFileName, FILE_WRITE))
        ErrorExit("Could not open output file " + outFileName);
    outMaterial.Save(outFile);
}

// ---------------------------------------------------------------------------
// Texture copying
// ---------------------------------------------------------------------------

static void FbxCopyTextures(ufbx_scene* scene, const HashSet<String>& usedTextures, const String& sourcePath)
{
    auto* fileSystem = context_->GetSubsystem<FileSystem>();

    if (useSubdirs_)
        fileSystem->CreateDir(resourcePath_ + "Textures");

    for (HashSet<String>::ConstIterator i = usedTextures.Begin(); i != usedTextures.End(); ++i)
    {
        const String& fileName = *i;

        // Check for embedded textures in ufbx
        bool foundEmbedded = false;
        for (size_t ti = 0; ti < scene->textures.count; ++ti)
        {
            ufbx_texture* tex = scene->textures.data[ti];
            String texFileName = FbxGetTextureFileName(tex);
            if (texFileName == fileName && tex->content.size > 0)
            {
                String fullDestName = resourcePath_ + (useSubdirs_ ? "Textures/" : "") + fileName;
                bool destExists = fileSystem->FileExists(fullDestName);
                if (destExists && noOverwriteTexture_)
                {
                    PrintLine("Skipping copy of existing embedded texture " + fileName);
                    foundEmbedded = true;
                    break;
                }

                PrintLine("Saving embedded texture " + fileName);
                File dest(context_, fullDestName, FILE_WRITE);
                dest.Write(tex->content.data, (unsigned)tex->content.size);
                foundEmbedded = true;
                break;
            }
        }

        if (foundEmbedded)
            continue;

        String fullSourceName = sourcePath + fileName;
        String fullDestName = resourcePath_ + (useSubdirs_ ? "Textures/" : "") + fileName;

        if (!fileSystem->FileExists(fullSourceName))
        {
            PrintLine("Skipping copy of nonexisting material texture " + fileName);
            continue;
        }
        {
            File test(context_, fullSourceName);
            if (!test.GetSize())
            {
                PrintLine("Skipping copy of zero-size material texture " + fileName);
                continue;
            }
        }

        bool destExists = fileSystem->FileExists(fullDestName);
        if (destExists && noOverwriteTexture_)
        {
            PrintLine("Skipping copy of existing texture " + fileName);
            continue;
        }
        if (destExists && noOverwriteNewerTexture_ && fileSystem->GetLastModifiedTime(fullDestName) >
            fileSystem->GetLastModifiedTime(fullSourceName))
        {
            PrintLine("Skipping copying of material texture " + fileName + ", destination is newer");
            continue;
        }

        PrintLine("Copying material texture " + fileName);
        fileSystem->Copy(fullSourceName, fullDestName);
    }
}
