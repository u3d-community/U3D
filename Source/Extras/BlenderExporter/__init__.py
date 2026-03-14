import bpy
from bpy.props import (
    StringProperty,
    BoolProperty,
    FloatProperty,
    EnumProperty,
)
from bpy_extras.io_utils import ExportHelper, ImportHelper
import os
import logging

log = logging.getLogger("ExportLogger")


class ImportU3D(bpy.types.Operator, ImportHelper):
    """Import a U3D model file"""
    bl_idname = "import_mesh.u3d"
    bl_label = "Import U3D"
    bl_options = {'PRESET', 'UNDO'}

    filename_ext = ".mdl"
    filter_glob: StringProperty(default="*.mdl", options={'HIDDEN'})

    import_skeleton: BoolProperty(
        name="Skeleton",
        description="Import skeleton/bones",
        default=True)
    import_animations: BoolProperty(
        name="Animations",
        description="Import .ani files from the same directory",
        default=True)
    object_scale: FloatProperty(
        name="Scale",
        description="Scale factor for import",
        default=1.0,
        min=0.001,
        max=1000.0)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False
        layout.prop(self, 'import_skeleton')
        layout.prop(self, 'import_animations')
        layout.prop(self, 'object_scale')

    def execute(self, context):
        from .importer import ImportModel
        try:
            ImportModel(
                self.filepath,
                import_skeleton=self.import_skeleton,
                import_animations=self.import_animations,
                scale=self.object_scale)
            self.report({'INFO'}, "Imported: {:s}".format(self.filepath))
        except Exception as e:
            self.report({'ERROR'}, str(e))
            return {'CANCELLED'}
        return {'FINISHED'}


class ExportU3D(bpy.types.Operator, ExportHelper):
    """Export selection to U3D model format"""
    bl_idname = "export_mesh.u3d"
    bl_label = "Export U3D"
    bl_options = {'PRESET'}

    filename_ext = ".mdl"
    filter_glob: StringProperty(default="*.mdl", options={'HIDDEN'})

    export_meshes: BoolProperty(
        name="Meshes",
        description="Export mesh geometry",
        default=True)
    export_normals: BoolProperty(
        name="Normals",
        description="Export vertex normals",
        default=True)
    export_tangents: BoolProperty(
        name="Tangents",
        description="Export vertex tangents",
        default=True)
    export_uvs: BoolProperty(
        name="UV Coordinates",
        description="Export UV coordinates",
        default=True)
    export_uv2: BoolProperty(
        name="Second UV Set",
        description="Export second UV set (lightmap)",
        default=False)
    export_colors: BoolProperty(
        name="Vertex Colors",
        description="Export vertex colors",
        default=False)
    export_skeleton: BoolProperty(
        name="Skeleton",
        description="Export skeleton/bones",
        default=True)
    export_weights: BoolProperty(
        name="Bone Weights",
        description="Export bone weights",
        default=True)
    export_animations: BoolProperty(
        name="Animations",
        description="Export skeletal animations",
        default=True)
    export_all_actions: BoolProperty(
        name="All Actions",
        description="Export all actions, not just the active one",
        default=True)
    export_morphs: BoolProperty(
        name="Shape Keys",
        description="Export shape keys as morphs",
        default=True)
    export_materials: BoolProperty(
        name="Materials",
        description="Export materials as XML",
        default=True)
    export_material_list: BoolProperty(
        name="Material List",
        description="Export material list file",
        default=True)
    apply_modifiers: BoolProperty(
        name="Apply Modifiers",
        description="Apply modifiers before exporting",
        default=True)
    only_selected: BoolProperty(
        name="Only Selected",
        description="Export only selected objects",
        default=False)
    only_deform_bones: BoolProperty(
        name="Only Deform Bones",
        description="Export only bones with the Deform flag",
        default=False)
    global_origin: BoolProperty(
        name="Use Global Origin",
        description="Use world space coordinates",
        default=True)
    object_scale: FloatProperty(
        name="Scale",
        description="Scale factor for export",
        default=1.0,
        min=0.001,
        max=1000.0)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False

        box = layout.box()
        box.label(text="Geometry")
        box.prop(self, 'export_meshes')
        box.prop(self, 'export_normals')
        box.prop(self, 'export_tangents')
        box.prop(self, 'export_uvs')
        box.prop(self, 'export_uv2')
        box.prop(self, 'export_colors')

        box = layout.box()
        box.label(text="Skeleton")
        box.prop(self, 'export_skeleton')
        box.prop(self, 'export_weights')
        box.prop(self, 'only_deform_bones')

        box = layout.box()
        box.label(text="Animation")
        box.prop(self, 'export_animations')
        box.prop(self, 'export_all_actions')
        box.prop(self, 'export_morphs')

        box = layout.box()
        box.label(text="Materials")
        box.prop(self, 'export_materials')
        box.prop(self, 'export_material_list')

        box = layout.box()
        box.label(text="Transform")
        box.prop(self, 'apply_modifiers')
        box.prop(self, 'only_selected')
        box.prop(self, 'global_origin')
        box.prop(self, 'object_scale')

    def execute(self, context):
        from .decompose import (
            TData, TOptions,
            DecomposeArmature, DecomposeActions, DecomposeMesh)
        from .exporter import (
            UrhoExport, UrhoWriteModel, UrhoWriteAnimation,
            UrhoWriteMaterial, UrhoWriteMaterialList)

        filepath = self.filepath
        directory = os.path.dirname(filepath)
        basename = os.path.splitext(os.path.basename(filepath))[0]

        tOptions = TOptions()
        tOptions.applyModifiers = self.apply_modifiers
        tOptions.onlySelected = self.only_selected
        tOptions.globalOrigin = self.global_origin
        tOptions.scale = self.object_scale
        tOptions.doBones = self.export_skeleton
        tOptions.doOnlyDeformBones = self.only_deform_bones
        tOptions.doAnimations = self.export_animations
        tOptions.doAllActions = self.export_all_actions
        tOptions.doCurrentAction = not self.export_all_actions
        tOptions.doGeometryPos = self.export_meshes
        tOptions.doGeometryNor = self.export_normals
        tOptions.doGeometryTan = self.export_tangents
        tOptions.doGeometryUV = self.export_uvs
        tOptions.doGeometryUV2 = self.export_uv2
        tOptions.doGeometryCol = self.export_colors
        tOptions.doGeometryWei = self.export_weights
        tOptions.doMorphs = self.export_morphs
        tOptions.doMaterials = self.export_materials

        scene = context.scene
        depsgraph = context.evaluated_depsgraph_get()

        if self.only_selected:
            objects = context.selected_objects
        else:
            objects = list(scene.collection.all_objects)
        meshObjects = [obj for obj in objects if obj.type == 'MESH']
        armatureObjects = [obj for obj in objects if obj.type == 'ARMATURE']

        if not meshObjects and not armatureObjects:
            self.report({'ERROR'}, "No mesh or armature objects to export")
            return {'CANCELLED'}

        allArmatures = list(armatureObjects)
        for obj in meshObjects:
            if obj.parent and obj.parent.type == 'ARMATURE':
                if obj.parent not in allArmatures:
                    allArmatures.append(obj.parent)

        print("U3D Export: {:d} meshes, {:d} armatures".format(
            len(meshObjects), len(allArmatures)))
        for obj in meshObjects:
            print("  Mesh: {:s} (parent: {:s})".format(
                obj.name, obj.parent.name if obj.parent else "None"))
        for obj in allArmatures:
            print("  Armature: {:s} ({:d} bones)".format(
                obj.name, len(obj.data.bones)))

        tData = TData()
        tData.objectName = basename

        savedPoses = {}
        if allArmatures:
            for armObj in allArmatures:
                savedPoses[armObj] = armObj.data.pose_position
                armObj.data.pose_position = 'REST'
            depsgraph.update()

        if tOptions.doBones and allArmatures:
            for armObj in allArmatures:
                DecomposeArmature(armObj, tData, tOptions)

        if not meshObjects:
            self.report({'WARNING'}, "No mesh objects found, exporting skeleton only")

        for meshObj in meshObjects:
            tOptions.lodUpdatedGeometryIndices = set()
            tOptions.lodDistance = 0.0
            tData.materialGeometryMap = {}
            DecomposeMesh(depsgraph, meshObj, tData, tOptions)

        if allArmatures:
            for armObj in allArmatures:
                armObj.data.pose_position = savedPoses[armObj]
            depsgraph.update()

        if tOptions.doAnimations and tData.bonesMap:
            DecomposeActions(scene, allArmatures, tData, tOptions)

        uModel, uAnimations, uMaterials = UrhoExport(tData)

        UrhoWriteModel(uModel, filepath)
        self.report({'INFO'}, "Exported model: {:s}".format(filepath))

        for uAnimation in uAnimations:
            aniPath = os.path.join(directory, uAnimation.name + ".ani")
            UrhoWriteAnimation(uAnimation, aniPath)
            self.report({'INFO'}, "Exported animation: {:s}".format(aniPath))

        if self.export_materials and uMaterials:
            matDir = os.path.join(directory, "Materials")
            os.makedirs(matDir, exist_ok=True)
            for uMaterial in uMaterials:
                matPath = os.path.join(matDir, uMaterial.name + ".xml")
                UrhoWriteMaterial(uMaterial, matPath)

            if self.export_material_list:
                matListPath = os.path.splitext(filepath)[0] + ".txt"
                UrhoWriteMaterialList(uMaterials, matListPath)

        return {'FINISHED'}


def menu_func_import(self, context):
    self.layout.operator(ImportU3D.bl_idname, text="U3D (.mdl)")


def menu_func_export(self, context):
    self.layout.operator(ExportU3D.bl_idname, text="U3D (.mdl)")


def register():
    bpy.utils.register_class(ImportU3D)
    bpy.utils.register_class(ExportU3D)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    bpy.utils.unregister_class(ExportU3D)
    bpy.utils.unregister_class(ImportU3D)
