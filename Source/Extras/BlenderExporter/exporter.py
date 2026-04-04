from .utils import BinaryFileWriter, FloatToString, XmlToPrettyString
from mathutils import Vector, Quaternion
from xml.etree import ElementTree as ET
from collections import defaultdict
import operator
import os
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
ELEMENT_BLEND = 0x0300

MORPH_ELEMENTS = ELEMENT_POSITION | ELEMENT_NORMAL | ELEMENT_TANGENT

BONE_BOUNDING_SPHERE = 0x0001
BONE_BOUNDING_BOX = 0x0002

TRACK_POSITION = 0x0001
TRACK_ROTATION = 0x0002
TRACK_SCALE = 0x0004

TRIANGLE_LIST = 0

MAX_SKIN_MATRICES = 64
BONES_PER_VERTEX = 4

EPSILON = 1e-6

TYPE_VECTOR2 = 2
TYPE_VECTOR3 = 3
TYPE_VECTOR4 = 4
TYPE_UBYTE4 = 5
TYPE_UBYTE4_NORM = 6

SEM_POSITION = 0
SEM_NORMAL = 1
SEM_TANGENT = 3
SEM_TEXCOORD = 4
SEM_COLOR = 5
SEM_BLENDWEIGHTS = 6
SEM_BLENDINDICES = 7


class BoundingBox:
    def __init__(self):
        self.min = None
        self.max = None

    def merge(self, point):
        if self.min is None:
            self.min = point.copy()
            self.max = point.copy()
            return
        if point.x < self.min.x:
            self.min.x = point.x
        if point.y < self.min.y:
            self.min.y = point.y
        if point.z < self.min.z:
            self.min.z = point.z
        if point.x > self.max.x:
            self.max.x = point.x
        if point.y > self.max.y:
            self.max.y = point.y
        if point.z > self.max.z:
            self.max.z = point.z


class UrhoVertex:
    def __init__(self, tVertex):
        self.mask = 0
        self.index = None
        self.pos = tVertex.pos
        if tVertex.pos:
            self.mask |= ELEMENT_POSITION
        self.normal = tVertex.normal
        if tVertex.normal:
            self.mask |= ELEMENT_NORMAL
        self.color = tVertex.color
        if tVertex.color:
            self.mask |= ELEMENT_COLOR
        self.uv = tVertex.uv
        if tVertex.uv:
            self.mask |= ELEMENT_UV1
        self.uv2 = tVertex.uv2
        if tVertex.uv2:
            self.mask |= ELEMENT_UV2
        self.tangent = tVertex.tangent
        if tVertex.tangent:
            self.mask |= ELEMENT_TANGENT
        self.weights = [(0.0, 0, None)] * BONES_PER_VERTEX
        if tVertex.weights is not None:
            sortedList = sorted(tVertex.weights, key=operator.itemgetter(1), reverse=True)
            sortedList = sortedList[:BONES_PER_VERTEX]
            totalWeight = sum([t[1] for t in sortedList])
            if totalWeight > 0:
                for i, t in enumerate(sortedList):
                    self.weights[i] = (t[1] / totalWeight, t[0], None)
                self.mask |= ELEMENT_BLEND

    def AlmostEqual(self, other):
        if self.pos and other.pos:
            for a, b in zip(self.pos, other.pos):
                if abs(a - b) > EPSILON:
                    return False
        elif self.pos != other.pos:
            return False
        if self.normal and other.normal:
            for a, b in zip(self.normal, other.normal):
                if abs(a - b) > EPSILON:
                    return False
        elif self.normal != other.normal:
            return False
        if self.color != other.color:
            return False
        if self.uv and other.uv:
            for a, b in zip(self.uv, other.uv):
                if abs(a - b) > EPSILON:
                    return False
        elif self.uv != other.uv:
            return False
        for a, b in zip(self.weights, other.weights):
            if a[1] != b[1]:
                return False
            if abs(a[0] - b[0]) > EPSILON:
                return False
        return True

    def __hash__(self):
        hashValue = 0
        if self.pos:
            hashValue ^= hash(self.pos.x) ^ hash(self.pos.y) ^ hash(self.pos.z)
        return hashValue

    def subtract(self, other, mask):
        if mask & ELEMENT_POSITION:
            self.pos = self.pos - other.pos
        if mask & ELEMENT_NORMAL:
            self.normal = self.normal - other.normal
        if mask & ELEMENT_TANGENT:
            self.tangent = Vector((
                self.tangent.x - other.tangent.x,
                self.tangent.y - other.tangent.y,
                self.tangent.z - other.tangent.z,
                0))


class UrhoVertexBuffer:
    def __init__(self):
        self.elementMask = None
        self.morphMinIndex = 0
        self.morphMaxIndex = 0
        self.vertices = []

    def updateMask(self, vertexMask):
        if self.elementMask is None:
            self.elementMask = vertexMask
        elif self.elementMask != vertexMask:
            if (self.elementMask & vertexMask) == self.elementMask:
                self.elementMask = vertexMask


class UrhoIndexBuffer:
    def __init__(self):
        self.indexSize = 0
        self.indexes = []


class UrhoLodLevel:
    def __init__(self):
        self.distance = 0.0
        self.primitiveType = 0
        self.vertexBuffer = 0
        self.indexBuffer = 0
        self.startIndex = 0
        self.countIndex = 0


class UrhoGeometry:
    def __init__(self):
        self.boneMap = []
        self.lodLevels = []
        self.center = Vector((0.0, 0.0, 0.0))
        self.uMaterialName = None


class UrhoVertexMorph:
    def __init__(self):
        self.name = None
        self.vertexBufferMap = {}


class UrhoBone:
    def __init__(self):
        self.name = None
        self.parentIndex = None
        self.position = None
        self.rotation = None
        self.scale = Vector((1.0, 1.0, 1.0))
        self.matrix = None
        self.inverseMatrix = None
        self.derivedPosition = None
        self.collisionMask = 0
        self.radius = None
        self.boundingBox = BoundingBox()
        self.length = 0


class UrhoModel:
    def __init__(self):
        self.name = None
        self.vertexBuffers = []
        self.indexBuffers = []
        self.geometries = []
        self.morphs = []
        self.bones = []
        self.boundingBox = BoundingBox()


class UrhoKeyframe:
    def __init__(self, tFrame):
        self.mask = 0
        self.time = tFrame.time
        self.position = tFrame.position
        if tFrame.position:
            self.mask |= TRACK_POSITION
        self.rotation = tFrame.rotation
        if tFrame.rotation:
            self.mask |= TRACK_ROTATION
        self.scale = tFrame.scale
        if tFrame.scale:
            self.mask |= TRACK_SCALE


class UrhoTrack:
    def __init__(self):
        self.name = ""
        self.elementMask = None
        self.keyframes = []

    def updateMask(self, keyframeMask):
        if self.elementMask is None:
            self.elementMask = keyframeMask
        elif self.elementMask != keyframeMask:
            self.elementMask &= keyframeMask


class UrhoAnimation:
    def __init__(self):
        self.name = ""
        self.length = 0.0
        self.tracks = []


class UrhoMaterial:
    def __init__(self):
        self.name = None
        self.techniqueName = None
        self.diffuseColor = None
        self.specularColor = None
        self.emissiveColor = None
        self.diffuseTexName = None
        self.normalTexName = None
        self.specularTexName = None
        self.emissiveTexName = None
        self.twoSided = False
        self.psdefines = ""
        self.vsdefines = ""


def UrhoExport(tData):
    uModel = UrhoModel()
    uModel.name = tData.objectName

    for boneName, bone in tData.bonesMap.items():
        uBoneIndex = len(uModel.bones)
        uBone = UrhoBone()
        uModel.bones.append(uBone)
        uBone.name = boneName
        if bone.parentName:
            uBone.parentIndex = tData.bonesMap[bone.parentName].index
        else:
            uBone.parentIndex = uBoneIndex
        uBone.position = bone.bindPosition
        uBone.rotation = bone.bindRotation
        uBone.scale = bone.bindScale
        uBone.matrix = bone.worldTransform
        uBone.inverseMatrix = uBone.matrix.inverted()
        uBone.derivedPosition = uBone.matrix.to_translation()
        uBone.length = bone.length

    totalVertices = len(tData.verticesList)

    maxLodVertices = 0
    for tGeometry in tData.geometriesList:
        for tLodLevel in tGeometry.lodLevels:
            vertexCount = len(tLodLevel.indexSet)
            if vertexCount > maxLodVertices:
                maxLodVertices = vertexCount

    useOneBuffer = True
    if totalVertices > 65535 and maxLodVertices <= 65535:
        useOneBuffer = False

    vertexBuffer = None
    indexBuffer = None
    modelIndexMap = {}

    for tGeometry in tData.geometriesList:
        uGeometry = UrhoGeometry()
        uModel.geometries.append(uGeometry)
        geomIndex = len(uModel.geometries) - 1
        uGeometry.uMaterialName = tGeometry.materialName

        center = Vector((0.0, 0.0, 0.0))
        remappedVertices = set()

        for lodIndex, tLodLevel in enumerate(tGeometry.lodLevels):
            uLodLevel = UrhoLodLevel()
            uGeometry.lodLevels.append(uLodLevel)
            uLodLevel.distance = tLodLevel.distance
            uLodLevel.primitiveType = TRIANGLE_LIST

            if vertexBuffer is None or (lodIndex == 0 and not useOneBuffer):
                vertexBuffer = UrhoVertexBuffer()
                uModel.vertexBuffers.append(vertexBuffer)
                uVerticesMap = {}

            if indexBuffer is None or (lodIndex == 0 and not useOneBuffer):
                indexBuffer = UrhoIndexBuffer()
                uModel.indexBuffers.append(indexBuffer)
                uLodLevel.startIndex = 0
            else:
                uLodLevel.startIndex = len(indexBuffer.indexes)

            uLodLevel.countIndex = len(tLodLevel.triangleList) * 3
            uLodLevel.vertexBuffer = len(uModel.vertexBuffers) - 1
            uLodLevel.indexBuffer = len(uModel.indexBuffers) - 1

            indexMap = {}

            for tVertexIndex in tLodLevel.indexSet:
                tVertex = tData.verticesList[tVertexIndex]
                uVertex = UrhoVertex(tVertex)
                vertexBuffer.updateMask(uVertex.mask)

                uVertexHash = hash(uVertex)
                try:
                    uVerticesMapList = uVerticesMap[uVertexHash]
                except KeyError:
                    uVerticesMapList = []
                    uVerticesMap[uVertexHash] = uVerticesMapList

                uVertexIndex = None
                for ivl in uVerticesMapList:
                    if vertexBuffer.vertices[ivl].AlmostEqual(uVertex):
                        uVertexIndex = ivl
                        break

                if uVertexIndex is None:
                    uVertexIndex = len(vertexBuffer.vertices)
                    vertexBuffer.vertices.append(uVertex)
                    uVerticesMapList.append(uVertexIndex)

                if tVertexIndex not in indexMap:
                    indexMap[tVertexIndex] = uVertexIndex

                if vertexBuffer.elementMask and (vertexBuffer.elementMask & ELEMENT_POSITION):
                    uModel.boundingBox.merge(uVertex.pos)

            for oldIndex, newIndex in indexMap.items():
                try:
                    vbviSet = modelIndexMap[oldIndex]
                except KeyError:
                    vbviSet = set()
                    modelIndexMap[oldIndex] = vbviSet
                vbviSet.add((uLodLevel.vertexBuffer, newIndex))

            centerCount = 0
            for triangle in tLodLevel.triangleList:
                for tVertexIndex in triangle:
                    uVertexIndex = indexMap[tVertexIndex]
                    indexBuffer.indexes.append(uVertexIndex)
                    if (lodIndex == 0) and vertexBuffer.elementMask and (vertexBuffer.elementMask & ELEMENT_POSITION):
                        centerCount += 1
                        center += vertexBuffer.vertices[uVertexIndex].pos

            if lodIndex == 0 and centerCount:
                uGeometry.center = center / centerCount

            if (len(uModel.bones) > MAX_SKIN_MATRICES and
                    vertexBuffer.elementMask and
                    (vertexBuffer.elementMask & ELEMENT_BLEND) == ELEMENT_BLEND):
                for uVertexIndex in indexMap.values():
                    if uVertexIndex in remappedVertices:
                        continue
                    remappedVertices.add(uVertexIndex)
                    vertex = vertexBuffer.vertices[uVertexIndex]
                    for j, (weight, boneIndex, _) in enumerate(vertex.weights):
                        if weight < EPSILON:
                            continue
                        try:
                            remappedBoneIndex = uGeometry.boneMap.index(boneIndex)
                        except ValueError:
                            remappedBoneIndex = len(uGeometry.boneMap)
                            if remappedBoneIndex < MAX_SKIN_MATRICES:
                                uGeometry.boneMap.append(boneIndex)
                            else:
                                remappedBoneIndex = 0
                                weight = 0.0
                        vertex.weights[j] = (weight, boneIndex, remappedBoneIndex)

    if tData.geometriesList and uModel.boundingBox.min is None:
        uModel.boundingBox.min = Vector((0.0, 0.0, 0.0))
        uModel.boundingBox.max = Vector((0.0, 0.0, 0.0))

    for uIndexBuffer in uModel.indexBuffers:
        if len(uIndexBuffer.indexes) > 65535:
            uIndexBuffer.indexSize = 4
        else:
            uIndexBuffer.indexSize = 2

    for uVertexBuffer in uModel.vertexBuffers:
        if not uVertexBuffer.elementMask:
            continue
        if (uVertexBuffer.elementMask & ELEMENT_BLEND) != ELEMENT_BLEND:
            continue
        for uVertex in uVertexBuffer.vertices:
            vertexPos = uVertex.pos
            for weight, boneIndex, _ in uVertex.weights:
                if weight > 0.33:
                    uBone = uModel.bones[boneIndex]
                    bonePos = uBone.derivedPosition
                    distance = (bonePos - vertexPos).length
                    if uBone.radius is None or distance > uBone.radius:
                        uBone.collisionMask |= BONE_BOUNDING_SPHERE
                        uBone.radius = distance
                    boneVertexPos = uBone.inverseMatrix @ vertexPos
                    uBone.collisionMask |= BONE_BOUNDING_BOX
                    uBone.boundingBox.merge(boneVertexPos)

    for tMorph in tData.morphsList:
        uMorph = UrhoVertexMorph()
        uMorph.name = tMorph.name
        uModel.morphs.append(uMorph)

        for tVertexIndex, tMorphVertex in tMorph.vertexMap.items():
            if tVertexIndex not in modelIndexMap:
                continue
            vbviSet = modelIndexMap[tVertexIndex]
            for uVertexBufferIndex, uVertexIndex in vbviSet:
                try:
                    uMorphVertexBuffer = uMorph.vertexBufferMap[uVertexBufferIndex]
                except KeyError:
                    uMorphVertexBuffer = UrhoVertexBuffer()
                    uMorph.vertexBufferMap[uVertexBufferIndex] = uMorphVertexBuffer

                uMorphVertex = UrhoVertex(tMorphVertex)
                uMorphVertexBuffer.updateMask(uMorphVertex.mask)

                uVertexBuffer = uModel.vertexBuffers[uVertexBufferIndex]
                uVertex = uVertexBuffer.vertices[uVertexIndex]

                morphMask = uMorphVertexBuffer.elementMask or uMorphVertex.mask
                uMorphVertex.subtract(uVertex, morphMask & MORPH_ELEMENTS)
                uMorphVertex.index = uVertexIndex
                uMorphVertexBuffer.vertices.append(uMorphVertex)

                if uVertexBuffer.morphMinIndex == 0 and uVertexBuffer.morphMaxIndex == 0:
                    uVertexBuffer.morphMinIndex = uVertexIndex
                    uVertexBuffer.morphMaxIndex = uVertexIndex
                elif uVertexIndex < uVertexBuffer.morphMinIndex:
                    uVertexBuffer.morphMinIndex = uVertexIndex
                elif uVertexIndex > uVertexBuffer.morphMaxIndex:
                    uVertexBuffer.morphMaxIndex = uVertexIndex

    uAnimations = []
    for tAnimation in tData.animationsList:
        uAnimation = UrhoAnimation()
        uAnimation.name = tAnimation.name
        uAnimation.length = 0.0

        for tTrack in tAnimation.tracks:
            uTrack = UrhoTrack()
            uTrack.name = tTrack.name

            for tFrame in tTrack.frames:
                uKeyframe = UrhoKeyframe(tFrame)
                uTrack.updateMask(uKeyframe.mask)
                uTrack.keyframes.append(uKeyframe)

            uTrack.keyframes.sort(key=operator.attrgetter('time'))

            if uTrack.keyframes and uTrack.elementMask:
                uAnimation.tracks.append(uTrack)
                length = uTrack.keyframes[-1].time
                if uAnimation.length < length:
                    uAnimation.length = length

        if uAnimation.tracks:
            uAnimations.append(uAnimation)

    uMaterials = []
    for tMaterial in tData.materialsList:
        uMaterial = UrhoMaterial()
        uMaterial.name = tMaterial.name

        alpha = 1.0
        if tMaterial.opacity is not None and tMaterial.opacity < 1.0:
            alpha = tMaterial.opacity

        technique = "NoTexture"
        if tMaterial.diffuseTexName:
            technique = "Diff"
            if tMaterial.normalTexName:
                technique += "Normal"
            if tMaterial.specularTexName:
                technique += "Spec"
            if tMaterial.emitTexName:
                technique += "Emissive"
        if alpha < 1.0:
            technique += "Alpha"

        uMaterial.techniqueName = technique

        if tMaterial.diffuseColor:
            uMaterial.diffuseColor = (
                tMaterial.diffuseColor[0],
                tMaterial.diffuseColor[1],
                tMaterial.diffuseColor[2],
                alpha)

        if tMaterial.specularColor:
            hardness = tMaterial.specularHardness or 50.0
            uMaterial.specularColor = (
                tMaterial.specularColor[0],
                tMaterial.specularColor[1],
                tMaterial.specularColor[2],
                hardness)

        if tMaterial.emitColor:
            uMaterial.emissiveColor = (
                tMaterial.emitColor[0],
                tMaterial.emitColor[1],
                tMaterial.emitColor[2])

        uMaterial.diffuseTexName = tMaterial.diffuseTexName
        uMaterial.normalTexName = tMaterial.normalTexName
        uMaterial.specularTexName = tMaterial.specularTexName
        uMaterial.emissiveTexName = tMaterial.emitTexName
        uMaterial.twoSided = tMaterial.twoSided

        uMaterials.append(uMaterial)

    return uModel, uAnimations, uMaterials


def UrhoWriteModel(model, filename):
    if not model.vertexBuffers or not model.indexBuffers or not model.geometries:
        log.error("No model data to export in {:s}".format(filename))
        return

    fw = BinaryFileWriter()
    fw.open(filename)

    fw.writeAsciiStr("UMD2")

    fw.writeUInt(len(model.vertexBuffers))
    for buffer in model.vertexBuffers:
        fw.writeUInt(len(buffer.vertices))
        mask = buffer.elementMask
        elements = []
        if mask & ELEMENT_POSITION:
            elements.append(TYPE_VECTOR3 | (SEM_POSITION << 8))
        if mask & ELEMENT_NORMAL:
            elements.append(TYPE_VECTOR3 | (SEM_NORMAL << 8))
        if mask & ELEMENT_COLOR:
            elements.append(TYPE_UBYTE4_NORM | (SEM_COLOR << 8))
        if mask & ELEMENT_UV1:
            elements.append(TYPE_VECTOR2 | (SEM_TEXCOORD << 8))
        if mask & ELEMENT_UV2:
            elements.append(TYPE_VECTOR2 | (SEM_TEXCOORD << 8) | (1 << 16))
        if mask & ELEMENT_TANGENT:
            elements.append(TYPE_VECTOR4 | (SEM_TANGENT << 8))
        if mask & ELEMENT_BWEIGHTS:
            elements.append(TYPE_VECTOR4 | (SEM_BLENDWEIGHTS << 8))
        if mask & ELEMENT_BINDICES:
            elements.append(TYPE_UBYTE4 | (SEM_BLENDINDICES << 8))
        fw.writeUInt(len(elements))
        for elem in elements:
            fw.writeUInt(elem)
        fw.writeUInt(buffer.morphMinIndex)
        if buffer.morphMaxIndex != 0:
            fw.writeUInt(buffer.morphMaxIndex - buffer.morphMinIndex + 1)
        else:
            fw.writeUInt(0)
        for vertex in buffer.vertices:
            if mask & ELEMENT_POSITION:
                fw.writeVector3(vertex.pos)
            if mask & ELEMENT_NORMAL:
                fw.writeVector3(vertex.normal)
            if mask & ELEMENT_COLOR:
                for i in range(4):
                    fw.writeUByte(vertex.color[i])
            if mask & ELEMENT_UV1:
                fw.writeFloat(vertex.uv[0])
                fw.writeFloat(vertex.uv[1])
            if mask & ELEMENT_UV2:
                fw.writeFloat(vertex.uv2[0])
                fw.writeFloat(vertex.uv2[1])
            if mask & ELEMENT_TANGENT:
                fw.writeVector3(vertex.tangent)
                fw.writeFloat(vertex.tangent.w if hasattr(vertex.tangent, 'w') else vertex.tangent[3])
            if mask & ELEMENT_BWEIGHTS:
                for i in range(BONES_PER_VERTEX):
                    fw.writeFloat(vertex.weights[i][0])
            if mask & ELEMENT_BINDICES:
                for i in range(BONES_PER_VERTEX):
                    boneIndex = vertex.weights[i][1]
                    remappedBoneIndex = vertex.weights[i][2]
                    if remappedBoneIndex is not None:
                        boneIndex = remappedBoneIndex
                    fw.writeUByte(boneIndex)

    fw.writeUInt(len(model.indexBuffers))
    for buffer in model.indexBuffers:
        fw.writeUInt(len(buffer.indexes))
        fw.writeUInt(buffer.indexSize)
        for i in buffer.indexes:
            if buffer.indexSize == 2:
                fw.writeUShort(i)
            else:
                fw.writeUInt(i)

    fw.writeUInt(len(model.geometries))
    for geometry in model.geometries:
        fw.writeUInt(len(geometry.boneMap))
        for bone in geometry.boneMap:
            fw.writeUInt(bone)
        fw.writeUInt(len(geometry.lodLevels))
        for lod in geometry.lodLevels:
            fw.writeFloat(lod.distance)
            fw.writeUInt(lod.primitiveType)
            fw.writeUInt(lod.vertexBuffer)
            fw.writeUInt(lod.indexBuffer)
            fw.writeUInt(lod.startIndex)
            fw.writeUInt(lod.countIndex)

    fw.writeUInt(len(model.morphs))
    for morph in model.morphs:
        fw.writeAsciiStr(morph.name)
        fw.writeUByte(0)
        fw.writeUInt(len(morph.vertexBufferMap))
        for morphBufferIndex, morphBuffer in sorted(morph.vertexBufferMap.items()):
            fw.writeUInt(morphBufferIndex)
            mask = (morphBuffer.elementMask or 0) & MORPH_ELEMENTS
            fw.writeUInt(mask)
            fw.writeUInt(len(morphBuffer.vertices))
            for vertex in morphBuffer.vertices:
                fw.writeUInt(vertex.index)
                if mask & ELEMENT_POSITION:
                    fw.writeVector3(vertex.pos)
                if mask & ELEMENT_NORMAL:
                    fw.writeVector3(vertex.normal)
                if mask & ELEMENT_TANGENT:
                    fw.writeVector3(vertex.tangent)

    fw.writeUInt(len(model.bones))
    for bone in model.bones:
        fw.writeAsciiStr(bone.name)
        fw.writeUByte(0)
        fw.writeUInt(bone.parentIndex)
        fw.writeVector3(bone.position)
        fw.writeQuaternion(bone.rotation)
        fw.writeVector3(bone.scale)
        for row in bone.inverseMatrix[:3]:
            for v in row:
                fw.writeFloat(v)
        fw.writeUByte(bone.collisionMask)
        if bone.collisionMask & BONE_BOUNDING_SPHERE:
            fw.writeFloat(bone.radius)
        if bone.collisionMask & BONE_BOUNDING_BOX:
            fw.writeVector3(bone.boundingBox.min)
            fw.writeVector3(bone.boundingBox.max)

    fw.writeVector3(model.boundingBox.min)
    fw.writeVector3(model.boundingBox.max)

    for geometry in model.geometries:
        fw.writeVector3(geometry.center)

    fw.close()


def UrhoWriteAnimation(animation, filename):
    if not animation.tracks:
        log.error("No animation data to export in {:s}".format(filename))
        return

    fw = BinaryFileWriter()
    fw.open(filename)

    fw.writeAsciiStr("UANI")
    fw.writeAsciiStr(animation.name)
    fw.writeUByte(0)
    fw.writeFloat(animation.length)

    fw.writeUInt(len(animation.tracks))
    for track in animation.tracks:
        fw.writeAsciiStr(track.name)
        fw.writeUByte(0)
        mask = track.elementMask
        fw.writeUByte(track.elementMask)
        fw.writeUInt(len(track.keyframes))
        for keyframe in track.keyframes:
            fw.writeFloat(keyframe.time)
            if mask & TRACK_POSITION:
                fw.writeVector3(keyframe.position)
            if mask & TRACK_ROTATION:
                fw.writeQuaternion(keyframe.rotation)
            if mask & TRACK_SCALE:
                fw.writeVector3(keyframe.scale)

    fw.close()


def UrhoWriteMaterial(uMaterial, filename):
    materialElem = ET.Element('material')

    technique = uMaterial.techniqueName
    techElem = ET.SubElement(materialElem, 'technique')
    techElem.set('name', 'Techniques/{:s}.xml'.format(technique))

    texUnitNames = ['diffuse', 'normal', 'specular', 'emissive']
    textures = [
        uMaterial.diffuseTexName,
        uMaterial.normalTexName,
        uMaterial.specularTexName,
        uMaterial.emissiveTexName]

    for i, texName in enumerate(textures):
        if texName:
            texElem = ET.SubElement(materialElem, 'texture')
            texElem.set('unit', texUnitNames[i])
            texElem.set('name', 'Textures/{:s}'.format(texName))

    if uMaterial.diffuseColor:
        paramElem = ET.SubElement(materialElem, 'parameter')
        paramElem.set('name', 'MatDiffColor')
        paramElem.set('value', '{:g} {:g} {:g} {:g}'.format(*uMaterial.diffuseColor))

    if uMaterial.specularColor:
        paramElem = ET.SubElement(materialElem, 'parameter')
        paramElem.set('name', 'MatSpecColor')
        paramElem.set('value', '{:g} {:g} {:g} {:g}'.format(*uMaterial.specularColor))

    if uMaterial.emissiveColor:
        paramElem = ET.SubElement(materialElem, 'parameter')
        paramElem.set('name', 'MatEmissiveColor')
        paramElem.set('value', '{:g} {:g} {:g}'.format(*uMaterial.emissiveColor))

    if uMaterial.twoSided:
        cullElem = ET.SubElement(materialElem, 'cull')
        cullElem.set('value', 'none')
        shadowCullElem = ET.SubElement(materialElem, 'shadowcull')
        shadowCullElem.set('value', 'none')

    xmlStr = XmlToPrettyString(materialElem)
    with open(filename, 'w') as f:
        f.write(xmlStr)


def UrhoWriteMaterialList(materials, filename):
    with open(filename, 'w') as f:
        for mat in materials:
            f.write('Materials/{:s}.xml\n'.format(mat.name))
