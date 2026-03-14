import bpy
import bmesh
import math
from mathutils import Vector, Matrix, Quaternion
from collections import OrderedDict
import logging

log = logging.getLogger("ExportLogger")


class TVertex:
    def __init__(self):
        self.blenderIndex = None
        self.pos = None
        self.normal = None
        self.color = None
        self.uv = None
        self.uv2 = None
        self.tangent = None
        self.bitangent = None
        self.weights = None

    def __eq__(self, other):
        return (self.pos == other.pos and
                self.normal == other.normal and
                self.uv == other.uv and
                self.color == other.color)

    def __hash__(self):
        hashValue = 0
        if self.pos:
            hashValue ^= hash(self.pos.x) ^ hash(self.pos.y) ^ hash(self.pos.z)
        if self.normal:
            hashValue ^= hash(self.normal.x) ^ hash(self.normal.y) ^ hash(self.normal.z)
        if self.uv:
            hashValue ^= hash(self.uv.x) ^ hash(self.uv.y)
        return hashValue


class TLodLevel:
    def __init__(self):
        self.distance = 0.0
        self.indexSet = set()
        self.triangleList = []


class TGeometry:
    def __init__(self):
        self.lodLevels = []
        self.materialName = None


class TMorph:
    def __init__(self, name):
        self.name = name
        self.indexSet = set()
        self.triangleList = []
        self.vertexMap = {}


class TMaterial:
    def __init__(self, name):
        self.name = name
        self.diffuseColor = None
        self.specularColor = None
        self.specularHardness = None
        self.emitColor = None
        self.emitIntensity = None
        self.opacity = None
        self.twoSided = False
        self.diffuseTexName = None
        self.normalTexName = None
        self.specularTexName = None
        self.emitTexName = None
        self.metallicTexName = None
        self.roughness = None
        self.metallic = None

    def __eq__(self, other):
        if hasattr(other, 'name'):
            return self.name == other.name
        return self.name == other


class TBone:
    def __init__(self, index, parentName, position, rotation, scale, transform, length):
        self.index = index
        self.parentName = parentName
        self.bindPosition = position
        self.bindRotation = rotation
        self.bindScale = scale
        self.worldTransform = transform
        self.length = length


class TFrame:
    def __init__(self, time, position, rotation, scale):
        self.time = time
        self.position = position
        self.rotation = rotation
        self.scale = scale

    def hasMoved(self, other):
        return (self.position != other.position or
                self.rotation != other.rotation or
                self.scale != other.scale)


class TTrack:
    def __init__(self, name):
        self.name = name
        self.frames = []


class TAnimation:
    def __init__(self, name):
        self.name = name
        self.tracks = []


class TData:
    def __init__(self):
        self.objectName = None
        self.verticesList = []
        self.geometriesList = []
        self.morphsList = []
        self.materialsList = []
        self.materialGeometryMap = {}
        self.bonesMap = OrderedDict()
        self.animationsList = []
        self.firstRootBoneName = None
        self.reparentedRoots = set()


class TOptions:
    def __init__(self):
        self.lodUpdatedGeometryIndices = set()
        self.lodDistance = None
        self.mergeObjects = False
        self.onlySelected = False
        self.orientation = Quaternion((1.0, 0.0, 0.0, 0.0))
        self.scale = 1.0
        self.globalOrigin = True
        self.applyModifiers = True
        self.doBones = True
        self.doOnlyDeformBones = False
        self.doAnimations = True
        self.doAllActions = True
        self.doCurrentAction = False
        self.doTimeline = False
        self.doAnimationExtraFrame = True
        self.doAnimationPos = True
        self.doAnimationRot = True
        self.doAnimationSca = True
        self.filterSingleKeyFrames = False
        self.doGeometryPos = True
        self.doGeometryNor = True
        self.doGeometryCol = False
        self.doGeometryColAlpha = False
        self.doGeometryUV = True
        self.doGeometryUV2 = False
        self.doGeometryTan = True
        self.doGeometryWei = True
        self.doMorphs = True
        self.doMorphNor = True
        self.doMorphTan = True
        self.doMaterials = True


def GenerateTangents(tLodLevels, tVertexList):
    if not tVertexList:
        return

    for tLodLevel in reversed(tLodLevels):
        if not tLodLevel.indexSet or not tLodLevel.triangleList:
            tLodLevels.remove(tLodLevel)
            continue
        for vertexIndex in tLodLevel.indexSet:
            vertex = tVertexList[vertexIndex]
            if vertex.pos is None or vertex.normal is None or vertex.uv is None:
                return
            vertex.tangent = Vector((0.0, 0.0, 0.0))
            vertex.bitangent = Vector((0.0, 0.0, 0.0))

    for tLodLevel in tLodLevels:
        for triangle in tLodLevel.triangleList:
            vertex1 = tVertexList[triangle[0]]
            vertex2 = tVertexList[triangle[1]]
            vertex3 = tVertexList[triangle[2]]

            x1 = vertex2.pos.x - vertex1.pos.x
            y1 = vertex2.pos.y - vertex1.pos.y
            z1 = vertex2.pos.z - vertex1.pos.z
            u1 = vertex2.uv.x - vertex1.uv.x
            v1 = vertex2.uv.y - vertex1.uv.y

            x2 = vertex3.pos.x - vertex1.pos.x
            y2 = vertex3.pos.y - vertex1.pos.y
            z2 = vertex3.pos.z - vertex1.pos.z
            u2 = vertex3.uv.x - vertex1.uv.x
            v2 = vertex3.uv.y - vertex1.uv.y

            d = u1 * v2 - u2 * v1
            if d == 0:
                continue

            t = Vector(((v2 * x1 - v1 * x2) / d,
                        (v2 * y1 - v1 * y2) / d,
                        (v2 * z1 - v1 * z2) / d))
            b = Vector(((u1 * x2 - u2 * x1) / d,
                        (u1 * y2 - u2 * y1) / d,
                        (u1 * z2 - u2 * z1) / d))

            vertex1.tangent += t
            vertex2.tangent += t
            vertex3.tangent += t
            vertex1.bitangent += b
            vertex2.bitangent += b
            vertex3.bitangent += b

    for tLodLevel in tLodLevels:
        for vertexIndex in tLodLevel.indexSet:
            vertex = tVertexList[vertexIndex]
            if len(vertex.tangent) == 4:
                continue
            tLen = vertex.tangent.length
            if tLen < 1e-6:
                vertex.tangent = Vector((1.0, 0.0, 0.0, 1.0))
                continue
            tOrtho = (vertex.tangent - vertex.normal * vertex.normal.dot(vertex.tangent)).normalized()
            bOrtho = vertex.normal.cross(vertex.tangent).normalized()
            w = 1.0 if bOrtho.dot(vertex.bitangent) >= 0.0 else -1.0
            vertex.bitangent = bOrtho
            vertex.tangent = Vector((tOrtho.x, tOrtho.y, tOrtho.z, w))


def DecomposeArmature(armatureObj, tData, tOptions):
    bonesMap = tData.bonesMap
    armature = armatureObj.data

    if not armature.bones:
        log.warning("Armature {:s} has no bones".format(armatureObj.name))
        return

    log.info("Decomposing armature: {:s} ({:d} bones)".format(
        armatureObj.name, len(armature.bones)))

    originMatrix = Matrix.Identity(4)

    bonesList = []

    def Traverse(bone, parent):
        if tOptions.doOnlyDeformBones and not bone.use_deform:
            hasDeformDescendant = False
            for child in bone.children:
                if Traverse(child, parent):
                    hasDeformDescendant = True
            return hasDeformDescendant
        bonesList.append((bone, parent))
        for child in bone.children:
            Traverse(child, bone)
        return True

    for bone in armature.bones.values():
        if bone.parent is None:
            Traverse(bone, None)

    if not bonesList:
        log.warning("Armature {:s} has no bone to export".format(armatureObj.name))
        return

    rootBones = [bone for bone, parent in bonesList if parent is None]

    armWorld = armatureObj.matrix_world

    if len(rootBones) > 1:
        synthName = armatureObj.name
        tData.firstRootBoneName = synthName
        for bone, parent in bonesList:
            if parent is None:
                tData.reparentedRoots.add(bone.name)

        synthMatrix = originMatrix @ armWorld
        if tOptions.orientation:
            synthMatrix = tOptions.orientation.to_matrix().to_4x4() @ synthMatrix
        synthMatrix = Matrix.Rotation(math.radians(-90.0), 4, 'X') @ synthMatrix
        if tOptions.scale != 1.0:
            synthMatrix.translation *= tOptions.scale
        st = synthMatrix.to_translation()
        sq = synthMatrix.to_quaternion()
        ss = synthMatrix.to_scale()

        sml = armWorld.copy()
        if tOptions.orientation:
            sml = tOptions.orientation.to_matrix().to_4x4() @ sml
        if tOptions.scale != 1.0:
            sml.translation *= tOptions.scale
        (sml[1][:], sml[2][:]) = (sml[2][:], sml[1][:])
        sml[0][2] = -sml[0][2]
        sml[1][2] = -sml[1][2]
        sml[2][2] = -sml[2][2]

        synthBone = TBone(len(bonesMap), None,
                          Vector((st.x, st.y, -st.z)),
                          Quaternion((sq.w, -sq.x, -sq.y, sq.z)),
                          Vector((ss.x, ss.y, ss.z)),
                          sml, 0.0)
        bonesMap[synthName] = synthBone

    for bone, parent in bonesList:
        boneMatrix = bone.matrix_local.copy()

        if bone.name in tData.reparentedRoots:
            pass
        elif parent:
            boneMatrix = parent.matrix_local.inverted() @ boneMatrix
        else:
            boneMatrix = originMatrix @ armWorld @ boneMatrix
            if tOptions.orientation:
                boneMatrix = tOptions.orientation.to_matrix().to_4x4() @ boneMatrix
            boneMatrix = Matrix.Rotation(math.radians(-90.0), 4, 'X') @ boneMatrix

        if tOptions.scale != 1.0:
            boneMatrix.translation *= tOptions.scale

        t = boneMatrix.to_translation()
        q = boneMatrix.to_quaternion()
        s = boneMatrix.to_scale()

        tl = Vector((t.x, t.y, -t.z))
        ql = Quaternion((q.w, -q.x, -q.y, q.z))
        sl = Vector((s.x, s.y, s.z))

        ml = armWorld @ bone.matrix_local
        if tOptions.orientation:
            ml = tOptions.orientation.to_matrix().to_4x4() @ ml
        if tOptions.scale != 1.0:
            ml.translation *= tOptions.scale
        (ml[1][:], ml[2][:]) = (ml[2][:], ml[1][:])
        ml[0][2] = -ml[0][2]
        ml[1][2] = -ml[1][2]
        ml[2][2] = -ml[2][2]

        if bone.name in tData.reparentedRoots:
            parentName = tData.firstRootBoneName
        else:
            parentName = parent and parent.name
        tBone = TBone(len(bonesMap), parentName, tl, ql, sl, ml,
                       bone.length * tOptions.scale)

        if bone.name not in bonesMap:
            bonesMap[bone.name] = tBone
        else:
            log.warning("Bone {:s} already present in the map".format(bone.name))


def DecomposeActions(scene, armatureObjects, tData, tOptions):
    bonesMap = tData.bonesMap
    animationsList = tData.animationsList

    if not isinstance(armatureObjects, (list, tuple)):
        armatureObjects = [armatureObjects]

    armatureObjects = [a for a in armatureObjects if a.animation_data]
    if not armatureObjects:
        return

    originMatrix = Matrix.Identity(4)
    savedFrame = scene.frame_current
    savedState = {}
    for armObj in armatureObjects:
        savedState[armObj] = (
            armObj.animation_data.action,
            armObj.animation_data.use_nla)

    boneArmature = {}
    for armObj in armatureObjects:
        for boneName in armObj.pose.bones.keys():
            if boneName in bonesMap:
                boneArmature[boneName] = armObj

    hasNla = False
    for armObj in armatureObjects:
        if armObj.animation_data.nla_tracks:
            hasNla = True
            break

    if hasNla:
        _decomposeNla(scene, armatureObjects, tData, tOptions,
                       originMatrix, boneArmature)
    else:
        _decomposeAllActions(scene, armatureObjects, tData, tOptions,
                              originMatrix, boneArmature)

    for armObj in armatureObjects:
        action, use_nla = savedState[armObj]
        armObj.animation_data.action = action
        armObj.animation_data.use_nla = use_nla
    scene.frame_set(savedFrame)


def _collectNlaSegments(armatureObjects):
    segments = {}
    for armObj in armatureObjects:
        if not armObj.animation_data:
            continue
        for track in armObj.animation_data.nla_tracks:
            for strip in track.strips:
                start = int(strip.frame_start)
                end = int(strip.frame_end)
                key = (start, end)
                if key not in segments:
                    segments[key] = strip.name
    return segments


def _sampleBones(scene, bonesMap, boneArmature, tOptions,
                  originMatrix, startframe, endframe,
                  reparentedRoots=None):
    boneInfos = []
    for boneName in bonesMap.keys():
        if boneName not in boneArmature:
            continue
        armObj = boneArmature[boneName]
        poseBone = armObj.pose.bones[boneName]
        parent = poseBone.parent
        while parent and parent.name not in bonesMap:
            parent = parent.parent
        parentName = parent.name if parent else None
        boneInfos.append((boneName, armObj.name, poseBone.name, parentName))

    trackMap = OrderedDict()
    for boneName, _, _, _ in boneInfos:
        trackMap[boneName] = TTrack(boneName)

    depsgraph = None

    for frameTime in range(startframe, endframe, scene.frame_step):
        isLastFrame = (frameTime >= endframe - scene.frame_step)
        scene.frame_set(frameTime)
        depsgraph = bpy.context.evaluated_depsgraph_get()

        for boneName, armObjName, poseBoneName, parentName in boneInfos:
            armObj = scene.objects[armObjName]
            evalObj = armObj.evaluated_get(depsgraph)
            poseBone = evalObj.pose.bones[poseBoneName]
            poseMatrix = poseBone.matrix.copy()
            if reparentedRoots and boneName in reparentedRoots:
                pass
            elif parentName:
                parent = evalObj.pose.bones[parentName]
                poseMatrix = parent.matrix.inverted() @ poseMatrix
            else:
                poseMatrix = evalObj.matrix_world @ poseMatrix
                if tOptions.orientation:
                    poseMatrix = tOptions.orientation.to_matrix().to_4x4() @ poseMatrix
                poseMatrix = Matrix.Rotation(math.radians(-90.0), 4, 'X') @ originMatrix @ poseMatrix

            if tOptions.scale != 1.0:
                poseMatrix.translation *= tOptions.scale

            t = poseMatrix.to_translation()
            q = poseMatrix.to_quaternion()
            s = poseMatrix.to_scale()

            tl = Vector((t.x, t.y, -t.z))
            ql = Quaternion((q.w, -q.x, -q.y, q.z))
            sl = Vector((s.x, s.y, s.z))

            if not tOptions.doAnimationPos:
                tl = None
            if not tOptions.doAnimationRot:
                ql = None
            if not tOptions.doAnimationSca:
                sl = None

            tFrame = TFrame((frameTime - startframe) / scene.render.fps, tl, ql, sl)

            tTrack = trackMap[boneName]
            if not tTrack.frames or isLastFrame or tTrack.frames[-1].hasMoved(tFrame):
                tTrack.frames.append(tFrame)

    tracks = []
    for tTrack in trackMap.values():
        if tTrack.frames and (not tOptions.filterSingleKeyFrames or len(tTrack.frames) > 1):
            tracks.append(tTrack)

    return tracks


def _decomposeNla(scene, armatureObjects, tData, tOptions,
                   originMatrix, boneArmature):
    bonesMap = tData.bonesMap
    animationsList = tData.animationsList

    segments = _collectNlaSegments(armatureObjects)
    if not segments:
        return

    for armObj in armatureObjects:
        armObj.data.pose_position = 'POSE'
        armObj.animation_data.use_nla = True
        armObj.animation_data.action = None

    scene.frame_set(scene.frame_current)

    for (startframe, endframe), name in sorted(segments.items()):
        log.info("Decomposing NLA segment: {:s} (frames {:d} {:d})".format(
            name, startframe, endframe))

        aniEnd = endframe
        if tOptions.doAnimationExtraFrame:
            aniEnd += scene.frame_step

        tracks = _sampleBones(scene, bonesMap, boneArmature,
                               tOptions, originMatrix, startframe, aniEnd,
                               tData.reparentedRoots)

        if tracks:
            tAnimation = TAnimation(name)
            tAnimation.tracks = tracks
            animationsList.append(tAnimation)


def _decomposeAllActions(scene, armatureObjects, tData, tOptions,
                          originMatrix, boneArmature):
    bonesMap = tData.bonesMap
    animationsList = tData.animationsList

    actionsPerArm = {}
    for armObj in armatureObjects:
        armBoneNames = set(armObj.pose.bones.keys())
        actions = []
        if tOptions.doAllActions:
            for action in bpy.data.actions:
                if action in actions:
                    continue
                for fcurve in action.fcurves:
                    dp = fcurve.data_path
                    if dp.startswith('pose.bones["'):
                        bname = dp.split('"')[1]
                        if bname in armBoneNames:
                            actions.append(action)
                            break
        if tOptions.doCurrentAction:
            current = armObj.animation_data.action
            if current and current not in actions:
                actions.append(current)
        actionsPerArm[armObj] = actions

    exported = set()
    for armObj, actions in actionsPerArm.items():
        for action in actions:
            if action.name in exported:
                continue
            exported.add(action.name)

            (startframe, endframe) = action.frame_range
            startframe = int(startframe)
            endframe = int(endframe + 1)
            if tOptions.doAnimationExtraFrame:
                endframe += scene.frame_step

            log.info("Decomposing action: {:s} (frames {:.1f} {:.1f})".format(
                action.name, startframe, endframe - 1))

            for a in armatureObjects:
                a.data.pose_position = 'POSE'
                a.animation_data.use_nla = False
                a.animation_data.action = None
            armObj.animation_data.action = action
            scene.frame_set(scene.frame_current)

            tracks = _sampleBones(scene, bonesMap, boneArmature,
                                   tOptions, originMatrix, startframe, endframe,
                                   tData.reparentedRoots)

            if tracks:
                tAnimation = TAnimation(action.name)
                tAnimation.tracks = tracks
                animationsList.append(tAnimation)


def _get_principled_bsdf(material):
    if not material.use_nodes or not material.node_tree:
        return None
    for node in material.node_tree.nodes:
        if node.type == 'BSDF_PRINCIPLED':
            return node
    return None


def _get_image_from_input(input_socket):
    if not input_socket.is_linked:
        return None
    linked_node = input_socket.links[0].from_node
    if linked_node.type == 'TEX_IMAGE' and linked_node.image:
        return linked_node.image.name
    return None


def DecomposeMesh(depsgraph, meshObj, tData, tOptions):
    verticesList = tData.verticesList
    geometriesList = tData.geometriesList
    materialsList = tData.materialsList
    materialGeometryMap = tData.materialGeometryMap
    morphsList = tData.morphsList
    bonesMap = tData.bonesMap

    verticesMap = {}

    shapeKeys = meshObj.data.shape_keys
    keyBlocks = []
    if shapeKeys and len(shapeKeys.key_blocks) > 0:
        keyBlocks = shapeKeys.key_blocks
    shapeKeysOldValues = []
    for j, block in enumerate(keyBlocks):
        shapeKeysOldValues.append(block.value)
        if j == 0:
            continue
        block.value = 0

    evalObj = meshObj.evaluated_get(depsgraph) if tOptions.applyModifiers else meshObj
    mesh = evalObj.to_mesh()

    log.info("Decomposing mesh: {:s} ({:d} vertices)".format(meshObj.name, len(mesh.vertices)))

    mesh.calc_loop_triangles()

    posMatrix = Matrix.Identity(4)
    normalMatrix = Matrix.Identity(4)

    if tOptions.globalOrigin:
        posMatrix = meshObj.matrix_world
        normalMatrix = meshObj.matrix_world.inverted().transposed()

    if tOptions.orientation:
        posMatrix = tOptions.orientation.to_matrix().to_4x4() @ posMatrix
        normalMatrix = tOptions.orientation.to_matrix().to_4x4() @ normalMatrix

    if tOptions.scale != 1.0:
        posMatrix = Matrix.Scale(tOptions.scale, 4) @ posMatrix

    uvLayer = None
    uvLayer2 = None
    if tOptions.doGeometryUV and mesh.uv_layers:
        uvLayer = mesh.uv_layers.active
        if not uvLayer and len(mesh.uv_layers) > 0:
            uvLayer = mesh.uv_layers[0]
        if tOptions.doGeometryUV2 and len(mesh.uv_layers) > 1:
            for uv in mesh.uv_layers:
                if uv != uvLayer:
                    uvLayer2 = uv
                    break
    if tOptions.doGeometryUV and not uvLayer:
        log.warning("Object {:s} has no UV data".format(meshObj.name))

    colorLayer = None
    if tOptions.doGeometryCol:
        if mesh.color_attributes:
            colorLayer = mesh.color_attributes.active_color
        if not colorLayer and hasattr(mesh, 'vertex_colors') and mesh.vertex_colors:
            colorLayer = mesh.vertex_colors.active
    if tOptions.doGeometryCol and not colorLayer:
        log.warning("Object {:s} has no color data".format(meshObj.name))

    meshVertexGroups = meshObj.vertex_groups

    if tOptions.doMaterials:
        for material in mesh.materials:
            if not material:
                continue
            materialName = material.name
            if materialName in materialsList:
                continue
            tMaterial = TMaterial(materialName)
            materialsList.append(tMaterial)

            tMaterial.twoSided = not material.use_backface_culling

            principled = _get_principled_bsdf(material)
            if principled:
                base_color_input = principled.inputs.get('Base Color')
                if base_color_input:
                    tMaterial.diffuseTexName = _get_image_from_input(base_color_input)
                    if not base_color_input.is_linked:
                        c = base_color_input.default_value
                        tMaterial.diffuseColor = (c[0], c[1], c[2])

                normal_input = principled.inputs.get('Normal')
                if normal_input and normal_input.is_linked:
                    normal_node = normal_input.links[0].from_node
                    if normal_node.type == 'NORMAL_MAP':
                        color_input = normal_node.inputs.get('Color')
                        if color_input:
                            tMaterial.normalTexName = _get_image_from_input(color_input)

                metallic_input = principled.inputs.get('Metallic')
                if metallic_input:
                    tMaterial.metallicTexName = _get_image_from_input(metallic_input)
                    if not metallic_input.is_linked:
                        tMaterial.metallic = metallic_input.default_value

                roughness_input = principled.inputs.get('Roughness')
                if roughness_input:
                    if not roughness_input.is_linked:
                        tMaterial.roughness = roughness_input.default_value

                spec_input = principled.inputs.get('Specular IOR Level')
                if not spec_input:
                    spec_input = principled.inputs.get('Specular')
                if spec_input:
                    tMaterial.specularTexName = _get_image_from_input(spec_input)

                emission_input = principled.inputs.get('Emission Color')
                if not emission_input:
                    emission_input = principled.inputs.get('Emission')
                if emission_input:
                    tMaterial.emitTexName = _get_image_from_input(emission_input)
                    if not emission_input.is_linked:
                        c = emission_input.default_value
                        tMaterial.emitColor = (c[0], c[1], c[2])

                emission_strength = principled.inputs.get('Emission Strength')
                if emission_strength:
                    tMaterial.emitIntensity = emission_strength.default_value

                alpha_input = principled.inputs.get('Alpha')
                if alpha_input and not alpha_input.is_linked:
                    tMaterial.opacity = alpha_input.default_value
            else:
                tMaterial.diffuseColor = (
                    material.diffuse_color[0],
                    material.diffuse_color[1],
                    material.diffuse_color[2])

    updatedGeometryIndices = set()

    faceVertexMap = {}

    for tri in mesh.loop_triangles:
        material = None
        if mesh.materials and len(mesh.materials) > tri.material_index:
            material = mesh.materials[tri.material_index]

        materialName = material and material.name
        mapMaterialName = materialName

        try:
            geometryIndex = materialGeometryMap[mapMaterialName]
        except KeyError:
            geometryIndex = len(geometriesList)
            newGeometry = TGeometry()
            newGeometry.materialName = materialName
            geometriesList.append(newGeometry)
            materialGeometryMap[mapMaterialName] = geometryIndex

        geometry = geometriesList[geometryIndex]

        lodLevelIndex = len(geometry.lodLevels)
        if not geometry.lodLevels or geometryIndex not in tOptions.lodUpdatedGeometryIndices:
            tLodLevel = TLodLevel()
            tLodLevel.distance = tOptions.lodDistance or 0.0
            geometry.lodLevels.append(tLodLevel)
            tOptions.lodUpdatedGeometryIndices.add(geometryIndex)
        else:
            tLodLevel = geometry.lodLevels[-1]

        updatedGeometryIndices.add(geometryIndex)
        indexSet = tLodLevel.indexSet
        triangleList = tLodLevel.triangleList

        tempList = []

        for i in range(3):
            loopIndex = tri.loops[i]
            vertexIndex = tri.vertices[i]

            vertex = mesh.vertices[vertexIndex]
            position = posMatrix @ vertex.co

            normal = mesh.corner_normals[loopIndex].vector
            normal = normalMatrix @ normal

            tVertex = TVertex()
            tVertex.blenderIndex = vertexIndex

            if tOptions.doGeometryPos:
                tVertex.pos = Vector((position.x, position.z, position.y))

            if tOptions.doGeometryNor:
                tVertex.normal = Vector((normal.x, normal.z, normal.y))

            if tOptions.doGeometryUV and uvLayer:
                uv = uvLayer.data[loopIndex].uv
                tVertex.uv = Vector((uv[0], 1.0 - uv[1]))

            if tOptions.doGeometryUV2 and uvLayer2:
                uv2 = uvLayer2.data[loopIndex].uv
                tVertex.uv2 = Vector((uv2[0], 1.0 - uv2[1]))

            if tOptions.doGeometryCol and colorLayer:
                if hasattr(colorLayer, 'data') and len(colorLayer.data) > loopIndex:
                    col = colorLayer.data[loopIndex].color
                    tVertex.color = (
                        int(col[0] * 255),
                        int(col[1] * 255),
                        int(col[2] * 255),
                        int(col[3] * 255) if len(col) > 3 else 255)

            if tOptions.doGeometryWei and bonesMap and meshVertexGroups:
                weights = []
                for group in vertex.groups:
                    if group.weight < 0.001:
                        continue
                    groupName = meshVertexGroups[group.group].name
                    if groupName in bonesMap:
                        boneIndex = bonesMap[groupName].index
                        weights.append((boneIndex, group.weight))
                if weights:
                    tVertex.weights = weights

            if (tVertex.weights is None and tOptions.doGeometryWei and bonesMap
                    and meshObj.parent_type == 'BONE'
                    and meshObj.parent_bone in bonesMap):
                boneIndex = bonesMap[meshObj.parent_bone].index
                tVertex.weights = [(boneIndex, 1.0)]


            vertexKey = (vertexIndex, loopIndex)
            if vertexKey in faceVertexMap:
                existingIndex = faceVertexMap[vertexKey]
                existing = verticesList[existingIndex]
                if existing == tVertex:
                    tempList.append(existingIndex)
                    continue

            tVertexIndex = len(verticesList)
            verticesList.append(tVertex)
            faceVertexMap[vertexKey] = tVertexIndex
            tempList.append(tVertexIndex)

        for idx in tempList:
            indexSet.add(idx)

        triangleList.append((tempList[0], tempList[2], tempList[1]))

    if tOptions.doGeometryTan:
        for geometryIndex in updatedGeometryIndices:
            geometry = geometriesList[geometryIndex]
            GenerateTangents(geometry.lodLevels, verticesList)

    if tOptions.doMorphs and keyBlocks:
        for j, block in enumerate(keyBlocks):
            if j == 0:
                continue
            block.value = 1.0
            morphEvalObj = meshObj.evaluated_get(depsgraph) if tOptions.applyModifiers else meshObj
            morphMesh = morphEvalObj.to_mesh()

            tMorph = TMorph(block.name)

            for tVertexIndex, tVertex in enumerate(verticesList):
                if tVertex.blenderIndex is None:
                    continue
                blIdx = tVertex.blenderIndex
                if blIdx >= len(morphMesh.vertices):
                    continue

                morphVert = morphMesh.vertices[blIdx]
                morphPos = posMatrix @ morphVert.co
                morphPosConverted = Vector((morphPos.x, morphPos.z, morphPos.y))

                if tVertex.pos and (morphPosConverted - tVertex.pos).length > 1e-6:
                    morphVertex = TVertex()
                    morphVertex.blenderIndex = blIdx
                    morphVertex.pos = morphPosConverted
                    if tVertex.normal:
                        morphVertex.normal = tVertex.normal.copy()
                    tMorph.vertexMap[tVertexIndex] = morphVertex
                    tMorph.indexSet.add(tVertexIndex)

            morphEvalObj.to_mesh_clear()
            block.value = 0

            if tMorph.vertexMap:
                morphsList.append(tMorph)

    for j, block in enumerate(keyBlocks):
        block.value = shapeKeysOldValues[j]

    evalObj.to_mesh_clear()
