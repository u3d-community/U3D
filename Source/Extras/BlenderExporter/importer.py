import struct
import os
import bpy
import bmesh
from mathutils import Vector, Matrix, Quaternion
import math
import logging

log = logging.getLogger("ExportLogger")

ELEMENT_POSITION = 0x0001
ELEMENT_NORMAL = 0x0002
ELEMENT_COLOR = 0x0004
ELEMENT_UV1 = 0x0008
ELEMENT_UV2 = 0x0010
ELEMENT_TANGENT = 0x0080
ELEMENT_BWEIGHTS = 0x0100
ELEMENT_BINDICES = 0x0200

BONE_BOUNDING_SPHERE = 0x0001
BONE_BOUNDING_BOX = 0x0002

TRACK_POSITION = 0x0001
TRACK_ROTATION = 0x0002
TRACK_SCALE = 0x0004


class BinaryFileReader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def readUInt(self):
        v = struct.unpack_from("<I", self.data, self.pos)[0]
        self.pos += 4
        return v

    def readUShort(self):
        v = struct.unpack_from("<H", self.data, self.pos)[0]
        self.pos += 2
        return v

    def readUByte(self):
        v = struct.unpack_from("<B", self.data, self.pos)[0]
        self.pos += 1
        return v

    def readFloat(self):
        v = struct.unpack_from("<f", self.data, self.pos)[0]
        self.pos += 4
        return v

    def readVector3(self):
        v = struct.unpack_from("<3f", self.data, self.pos)
        self.pos += 12
        return Vector(v)

    def readQuaternion(self):
        v = struct.unpack_from("<4f", self.data, self.pos)
        self.pos += 16
        return Quaternion(v)

    def readAsciiStr(self):
        end = self.data.index(b'\x00', self.pos)
        s = self.data[self.pos:end].decode('ascii', errors='replace')
        self.pos = end + 1
        return s

    def readBytes(self, count):
        v = self.data[self.pos:self.pos + count]
        self.pos += count
        return v


class MdlVertexBuffer:
    def __init__(self):
        self.vertexCount = 0
        self.elementMask = 0
        self.morphMinIndex = 0
        self.morphCount = 0
        self.positions = []
        self.normals = []
        self.colors = []
        self.uvs = []
        self.uv2s = []
        self.tangents = []
        self.bweights = []
        self.bindices = []


class MdlIndexBuffer:
    def __init__(self):
        self.indexCount = 0
        self.indexSize = 0
        self.indices = []


class MdlLodLevel:
    def __init__(self):
        self.distance = 0.0
        self.primitiveType = 0
        self.vertexBuffer = 0
        self.indexBuffer = 0
        self.startIndex = 0
        self.countIndex = 0


class MdlGeometry:
    def __init__(self):
        self.boneMap = []
        self.lodLevels = []


class MdlBone:
    def __init__(self):
        self.name = ""
        self.parentIndex = 0
        self.position = Vector()
        self.rotation = Quaternion()
        self.scale = Vector((1, 1, 1))
        self.inverseMatrix = []
        self.collisionMask = 0
        self.radius = 0.0
        self.bbMin = Vector()
        self.bbMax = Vector()


class MdlModel:
    def __init__(self):
        self.vertexBuffers = []
        self.indexBuffers = []
        self.geometries = []
        self.bones = []
        self.bbMin = Vector()
        self.bbMax = Vector()
        self.geomCenters = []


def ReadModel(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    fr = BinaryFileReader(data)
    magic = fr.readBytes(4)
    if magic not in (b'UMDL', b'UMD2'):
        raise ValueError("Not a valid Urho3D model file")

    isUMD2 = (magic == b'UMD2')
    model = MdlModel()

    UMD2_TYPE_SIZES = {0: 4, 1: 4, 2: 8, 3: 12, 4: 16, 5: 4, 6: 4}

    numVB = fr.readUInt()
    for _ in range(numVB):
        vb = MdlVertexBuffer()
        vb.vertexCount = fr.readUInt()
        elements = []

        if isUMD2:
            numElements = fr.readUInt()
            mask = 0
            for _ in range(numElements):
                desc = fr.readUInt()
                elemType = desc & 0xFF
                elemSemantic = (desc >> 8) & 0xFF
                elemIndex = (desc >> 16) & 0xFF
                elemSize = UMD2_TYPE_SIZES.get(elemType, 0)
                elements.append((elemSemantic, elemIndex, elemSize))
                if elemSemantic == 0:
                    mask |= ELEMENT_POSITION
                elif elemSemantic == 1:
                    mask |= ELEMENT_NORMAL
                elif elemSemantic == 3:
                    mask |= ELEMENT_TANGENT
                elif elemSemantic == 4:
                    if elemIndex == 0:
                        mask |= ELEMENT_UV1
                    elif elemIndex == 1:
                        mask |= ELEMENT_UV2
                elif elemSemantic == 5:
                    mask |= ELEMENT_COLOR
                elif elemSemantic == 6:
                    mask |= ELEMENT_BWEIGHTS
                elif elemSemantic == 7:
                    mask |= ELEMENT_BINDICES
            vb.elementMask = mask
        else:
            vb.elementMask = fr.readUInt()

        vb.morphMinIndex = fr.readUInt()
        vb.morphCount = fr.readUInt()

        if isUMD2:
            for _ in range(vb.vertexCount):
                for semantic, elemIndex, elemSize in elements:
                    if semantic == 0:
                        vb.positions.append(fr.readVector3())
                    elif semantic == 1:
                        vb.normals.append(fr.readVector3())
                    elif semantic == 5:
                        vb.colors.append((fr.readUByte(), fr.readUByte(), fr.readUByte(), fr.readUByte()))
                    elif semantic == 4 and elemIndex == 0:
                        vb.uvs.append((fr.readFloat(), fr.readFloat()))
                    elif semantic == 4 and elemIndex == 1:
                        vb.uv2s.append((fr.readFloat(), fr.readFloat()))
                    elif semantic == 3:
                        vb.tangents.append((fr.readFloat(), fr.readFloat(), fr.readFloat(), fr.readFloat()))
                    elif semantic == 6:
                        vb.bweights.append((fr.readFloat(), fr.readFloat(), fr.readFloat(), fr.readFloat()))
                    elif semantic == 7:
                        vb.bindices.append((fr.readUByte(), fr.readUByte(), fr.readUByte(), fr.readUByte()))
                    else:
                        fr.readBytes(elemSize)
        else:
            for _ in range(vb.vertexCount):
                if vb.elementMask & ELEMENT_POSITION:
                    vb.positions.append(fr.readVector3())
                if vb.elementMask & ELEMENT_NORMAL:
                    vb.normals.append(fr.readVector3())
                if vb.elementMask & ELEMENT_COLOR:
                    vb.colors.append((fr.readUByte(), fr.readUByte(), fr.readUByte(), fr.readUByte()))
                if vb.elementMask & ELEMENT_UV1:
                    vb.uvs.append((fr.readFloat(), fr.readFloat()))
                if vb.elementMask & ELEMENT_UV2:
                    vb.uv2s.append((fr.readFloat(), fr.readFloat()))
                if vb.elementMask & ELEMENT_TANGENT:
                    vb.tangents.append((fr.readFloat(), fr.readFloat(), fr.readFloat(), fr.readFloat()))
                if vb.elementMask & ELEMENT_BWEIGHTS:
                    vb.bweights.append((fr.readFloat(), fr.readFloat(), fr.readFloat(), fr.readFloat()))
                if vb.elementMask & ELEMENT_BINDICES:
                    vb.bindices.append((fr.readUByte(), fr.readUByte(), fr.readUByte(), fr.readUByte()))

        model.vertexBuffers.append(vb)

    numIB = fr.readUInt()
    for _ in range(numIB):
        ib = MdlIndexBuffer()
        ib.indexCount = fr.readUInt()
        ib.indexSize = fr.readUInt()
        for _ in range(ib.indexCount):
            if ib.indexSize == 2:
                ib.indices.append(fr.readUShort())
            else:
                ib.indices.append(fr.readUInt())
        model.indexBuffers.append(ib)

    numGeom = fr.readUInt()
    for _ in range(numGeom):
        geom = MdlGeometry()
        boneMapCount = fr.readUInt()
        for _ in range(boneMapCount):
            geom.boneMap.append(fr.readUInt())
        numLods = fr.readUInt()
        for _ in range(numLods):
            lod = MdlLodLevel()
            lod.distance = fr.readFloat()
            lod.primitiveType = fr.readUInt()
            lod.vertexBuffer = fr.readUInt()
            lod.indexBuffer = fr.readUInt()
            lod.startIndex = fr.readUInt()
            lod.countIndex = fr.readUInt()
            geom.lodLevels.append(lod)
        model.geometries.append(geom)

    numMorphs = fr.readUInt()
    for _ in range(numMorphs):
        fr.readAsciiStr()
        numBufs = fr.readUInt()
        for _ in range(numBufs):
            fr.readUInt()
            morphMask = fr.readUInt()
            morphVertCount = fr.readUInt()
            for _ in range(morphVertCount):
                fr.readUInt()
                if morphMask & ELEMENT_POSITION:
                    fr.readVector3()
                if morphMask & ELEMENT_NORMAL:
                    fr.readVector3()
                if morphMask & ELEMENT_TANGENT:
                    fr.readVector3()

    numBones = fr.readUInt()
    for _ in range(numBones):
        bone = MdlBone()
        bone.name = fr.readAsciiStr()
        bone.parentIndex = fr.readUInt()
        bone.position = fr.readVector3()
        bone.rotation = fr.readQuaternion()
        bone.scale = fr.readVector3()
        bone.inverseMatrix = []
        for _ in range(12):
            bone.inverseMatrix.append(fr.readFloat())
        bone.collisionMask = fr.readUByte()
        if bone.collisionMask & BONE_BOUNDING_SPHERE:
            bone.radius = fr.readFloat()
        if bone.collisionMask & BONE_BOUNDING_BOX:
            bone.bbMin = fr.readVector3()
            bone.bbMax = fr.readVector3()
        model.bones.append(bone)

    model.bbMin = fr.readVector3()
    model.bbMax = fr.readVector3()

    for _ in range(numGeom):
        if fr.pos + 12 > len(fr.data):
            break
        model.geomCenters.append(fr.readVector3())

    return model


def ReadAnimation(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    fr = BinaryFileReader(data)
    magic = fr.readBytes(4)
    if magic != b'UANI':
        raise ValueError("Not a valid Urho3D animation file")

    animName = fr.readAsciiStr()
    animLength = fr.readFloat()

    tracks = []
    numTracks = fr.readUInt()
    for _ in range(numTracks):
        trackName = fr.readAsciiStr()
        channelMask = fr.readUByte()
        numKeyframes = fr.readUInt()
        keyframes = []
        for _ in range(numKeyframes):
            time = fr.readFloat()
            pos = rot = scale = None
            if channelMask & TRACK_POSITION:
                pos = fr.readVector3()
            if channelMask & TRACK_ROTATION:
                rot = fr.readQuaternion()
            if channelMask & TRACK_SCALE:
                scale = fr.readVector3()
            keyframes.append((time, pos, rot, scale))
        tracks.append((trackName, channelMask, keyframes))

    return animName, animLength, tracks


def ImportModel(filepath, import_skeleton=True, import_animations=True, scale=1.0):
    model = ReadModel(filepath)
    basename = os.path.splitext(os.path.basename(filepath))[0]
    directory = os.path.dirname(filepath)

    armatureObj = None
    if import_skeleton and model.bones:
        armatureObj = _create_armature(basename, model, scale)

    meshObj = _create_mesh(basename, model, armatureObj, scale)

    if import_animations and armatureObj:
        bindData = {}
        for bone in model.bones:
            bindData[bone.name] = (bone.position, bone.rotation, bone.scale)
        for f in sorted(os.listdir(directory)):
            if f.endswith('.ani'):
                aniPath = os.path.join(directory, f)
                try:
                    _import_animation(armatureObj, aniPath, scale, bindData)
                except Exception as e:
                    log.warning("Failed to import animation {:s}: {:s}".format(f, str(e)))

    return meshObj


def _create_armature(name, model, scale):
    armature = bpy.data.armatures.new(name + "_Armature")
    armatureObj = bpy.data.objects.new(name + "_Armature", armature)
    bpy.context.collection.objects.link(armatureObj)
    bpy.context.view_layer.objects.active = armatureObj

    bpy.ops.object.mode_set(mode='EDIT')

    for bone in model.bones:
        eb = armature.edit_bones.new(bone.name)
        eb.head = Vector((0, 0, 0))
        eb.tail = Vector((0, 0.05 * scale, 0))

    editBones = list(armature.edit_bones)

    for i, bone in enumerate(model.bones):
        eb = editBones[i]

        invMat = bone.inverseMatrix
        mat4 = Matrix((
            (invMat[0], invMat[1], invMat[2], invMat[3]),
            (invMat[4], invMat[5], invMat[6], invMat[7]),
            (invMat[8], invMat[9], invMat[10], invMat[11]),
            (0, 0, 0, 1)))
        boneMat = mat4.inverted()

        boneMat[0][2] = -boneMat[0][2]
        boneMat[1][2] = -boneMat[1][2]
        boneMat[2][2] = -boneMat[2][2]
        (boneMat[1][:], boneMat[2][:]) = (boneMat[2][:], boneMat[1][:])

        if scale != 1.0:
            boneMat.translation *= scale

        eb.matrix = boneMat
        boneLength = max(0.02 * scale, bone.radius * scale if bone.radius else 0.02 * scale)
        eb.tail = eb.head + (eb.matrix.to_3x3() @ Vector((0, boneLength, 0)))

    for i, bone in enumerate(model.bones):
        if bone.parentIndex != i and bone.parentIndex < len(editBones):
            editBones[i].parent = editBones[bone.parentIndex]

    bpy.ops.object.mode_set(mode='OBJECT')
    return armatureObj


def _create_mesh(name, model, armatureObj, scale):
    mesh = bpy.data.meshes.new(name)
    meshObj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(meshObj)

    allVerts = []
    allFaces = []
    allNormals = {}
    allUVs = {}
    allUV2s = {}
    allColors = {}
    geomMaterials = []
    faceMaterialIndices = []

    vertOffset = 0

    for geomIdx, geom in enumerate(model.geometries):
        if not geom.lodLevels:
            continue
        lod = geom.lodLevels[0]

        vb = model.vertexBuffers[lod.vertexBuffer]
        ib = model.indexBuffers[lod.indexBuffer]

        usedVerts = set()
        faces = []
        for i in range(lod.startIndex, lod.startIndex + lod.countIndex, 3):
            if i + 2 >= len(ib.indices):
                break
            i0 = ib.indices[i]
            i1 = ib.indices[i + 1]
            i2 = ib.indices[i + 2]
            usedVerts.update((i0, i1, i2))
            faces.append((i0, i1, i2))

        localToGlobal = {}
        for vi in sorted(usedVerts):
            if vi < len(vb.positions):
                pos = vb.positions[vi]
                blenderPos = Vector((pos.x, -pos.z, pos.y)) * scale
                globalIdx = len(allVerts)
                allVerts.append(blenderPos)
                localToGlobal[vi] = globalIdx

        for face in faces:
            g0 = localToGlobal.get(face[0])
            g1 = localToGlobal.get(face[1])
            g2 = localToGlobal.get(face[2])
            if g0 is not None and g1 is not None and g2 is not None:
                faceIdx = len(allFaces)
                allFaces.append((g0, g1, g2))
                faceMaterialIndices.append(geomIdx)

                if vb.elementMask & ELEMENT_NORMAL and vb.normals:
                    for loopOff, vi in enumerate((face[0], face[1], face[2])):
                        loopIdx = faceIdx * 3 + loopOff
                        if vi < len(vb.normals):
                            n = vb.normals[vi]
                            allNormals[loopIdx] = (n.x, -n.z, n.y)

                if vb.elementMask & ELEMENT_UV1 and vb.uvs:
                    for loopOff, vi in enumerate((face[0], face[1], face[2])):
                        loopIdx = faceIdx * 3 + loopOff
                        if vi < len(vb.uvs):
                            allUVs[loopIdx] = (vb.uvs[vi][0], 1.0 - vb.uvs[vi][1])

                if vb.elementMask & ELEMENT_UV2 and vb.uv2s:
                    for loopOff, vi in enumerate((face[0], face[1], face[2])):
                        loopIdx = faceIdx * 3 + loopOff
                        if vi < len(vb.uv2s):
                            allUV2s[loopIdx] = (vb.uv2s[vi][0], 1.0 - vb.uv2s[vi][1])

                if vb.elementMask & ELEMENT_COLOR and vb.colors:
                    for loopOff, vi in enumerate((face[0], face[1], face[2])):
                        loopIdx = faceIdx * 3 + loopOff
                        if vi < len(vb.colors):
                            c = vb.colors[vi]
                            allColors[loopIdx] = (c[0]/255.0, c[1]/255.0, c[2]/255.0, c[3]/255.0)

        if armatureObj and model.bones and vb.elementMask & ELEMENT_BWEIGHTS:
            for vi in sorted(usedVerts):
                globalIdx = localToGlobal.get(vi)
                if globalIdx is None:
                    continue
                if vi >= len(vb.bweights) or vi >= len(vb.bindices):
                    continue
                weights = vb.bweights[vi]
                indices = vb.bindices[vi]
                for j in range(4):
                    if weights[j] < 0.001:
                        continue
                    boneIdx = indices[j]
                    if geom.boneMap:
                        if boneIdx < len(geom.boneMap):
                            boneIdx = geom.boneMap[boneIdx]
                        else:
                            continue
                    if boneIdx < len(model.bones):
                        boneName = model.bones[boneIdx].name
                        if boneName not in meshObj.vertex_groups:
                            meshObj.vertex_groups.new(name=boneName)
                        meshObj.vertex_groups[boneName].add([globalIdx], weights[j], 'ADD')

    mesh.from_pydata(allVerts, [], allFaces)
    mesh.update()

    if allNormals:
        loopNormals = [(0, 0, 1)] * len(mesh.loops)
        for loopIdx, normal in allNormals.items():
            if loopIdx < len(loopNormals):
                loopNormals[loopIdx] = normal
        mesh.normals_split_custom_set(loopNormals)

    if allUVs:
        uvLayer = mesh.uv_layers.new(name="UVMap")
        for loopIdx, uv in allUVs.items():
            if loopIdx < len(uvLayer.data):
                uvLayer.data[loopIdx].uv = uv

    if allUV2s:
        uvLayer2 = mesh.uv_layers.new(name="UVMap2")
        for loopIdx, uv in allUV2s.items():
            if loopIdx < len(uvLayer2.data):
                uvLayer2.data[loopIdx].uv = uv

    if allColors:
        colorLayer = mesh.color_attributes.new(name="Color", type='BYTE_COLOR', domain='CORNER')
        for loopIdx, col in allColors.items():
            if loopIdx < len(colorLayer.data):
                colorLayer.data[loopIdx].color = col

    if len(model.geometries) > 1:
        for i in range(len(model.geometries)):
            mat = bpy.data.materials.new(name="{:s}_Mat{:d}".format(name, i))
            mesh.materials.append(mat)
        for polyIdx, matIdx in enumerate(faceMaterialIndices):
            if polyIdx < len(mesh.polygons):
                mesh.polygons[polyIdx].material_index = matIdx

    if armatureObj:
        mod = meshObj.modifiers.new(name="Armature", type='ARMATURE')
        mod.object = armatureObj
        meshObj.parent = armatureObj

    mesh.update()
    return meshObj


def _urho_trs_to_matrix(pos, rot, scale):
    mat = Matrix.Translation(pos) @ rot.to_matrix().to_4x4()
    if scale:
        for i in range(3):
            for j in range(3):
                mat[j][i] *= scale[i]
    return mat


def _import_animation(armatureObj, filepath, scale, bindData=None):
    animName, animLength, tracks = ReadAnimation(filepath)

    if not tracks:
        return

    action = bpy.data.actions.new(name=animName)
    if not armatureObj.animation_data:
        armatureObj.animation_data_create()
    armatureObj.animation_data.action = action

    fps = bpy.context.scene.render.fps

    for trackName, channelMask, keyframes in tracks:
        if trackName not in armatureObj.pose.bones:
            continue

        poseBone = armatureObj.pose.bones[trackName]

        restMat = None
        if bindData and trackName in bindData:
            bPos, bRot, bScale = bindData[trackName]
            restMat = _urho_trs_to_matrix(bPos, bRot, bScale)

        for time, pos, rot, scl in keyframes:
            frame = time * fps + 1

            if restMat:
                animPos = pos if pos else (bindData[trackName][0] if bindData and trackName in bindData else Vector())
                animRot = rot if rot else (bindData[trackName][1] if bindData and trackName in bindData else Quaternion())
                animScale = scl if scl else (bindData[trackName][2] if bindData and trackName in bindData else Vector((1, 1, 1)))
                animMat = _urho_trs_to_matrix(animPos, animRot, animScale)
                delta = restMat.inverted() @ animMat
                dt = delta.to_translation()
                dr = delta.to_quaternion()
                ds = delta.to_scale()

                if channelMask & TRACK_POSITION:
                    poseBone.location = dt
                    poseBone.keyframe_insert(data_path="location", frame=frame)
                if channelMask & TRACK_ROTATION:
                    poseBone.rotation_quaternion = dr
                    poseBone.keyframe_insert(data_path="rotation_quaternion", frame=frame)
                if channelMask & TRACK_SCALE:
                    poseBone.scale = ds
                    poseBone.keyframe_insert(data_path="scale", frame=frame)
            else:
                if pos and (channelMask & TRACK_POSITION):
                    poseBone.location = Vector(pos)
                    poseBone.keyframe_insert(data_path="location", frame=frame)
                if rot and (channelMask & TRACK_ROTATION):
                    poseBone.rotation_quaternion = rot
                    poseBone.keyframe_insert(data_path="rotation_quaternion", frame=frame)
                if scl and (channelMask & TRACK_SCALE):
                    poseBone.scale = Vector(scl)
                    poseBone.keyframe_insert(data_path="scale", frame=frame)
