import bpy
import math
import mathutils
import os
import sys

# Argument parsing
argv = sys.argv
argv = argv[argv.index("--") + 1:]

# Skybox definition
sky = argv[0]
filename = argv[1]
resolution = int(argv[2])
destination = argv[3]

# Scene setup
scene = bpy.context.scene
node_tree = scene.world.node_tree
tree_nodes = node_tree.nodes
tree_nodes.clear()
node_background = tree_nodes.new(type='ShaderNodeBackground')
node_environment = tree_nodes.new('ShaderNodeTexEnvironment')

# Load HDR sky file
node_environment.image = bpy.data.images.load(sky)
node_environment.location = -300,0

# Link up all nodes
node_output = tree_nodes.new(type='ShaderNodeOutputWorld')
node_output.location = 200,0
links = node_tree.links
link = links.new(node_environment.outputs["Color"], node_background.inputs["Color"])
link = links.new(node_background.outputs["Background"], node_output.inputs["Surface"])

# Setup the camera
camera = bpy.data.objects.new('SkyCam', bpy.data.cameras.new('SkyCam'))
bpy.context.collection.objects.link(camera)
scene.camera = camera

# Configure lens
bpy.context.scene.render.resolution_x = resolution
bpy.context.scene.render.resolution_y = resolution
camera.data.lens_unit = 'FOV'
camera.data.angle = 1.5708 # 90 deg
scene.render.image_settings.file_format = 'PNG'

# Render down cubeface
camera.rotation_euler = mathutils.Euler((0, 0, 0))
scene.render.filepath = os.path.join(destination, filename + "_NegY.png")
bpy.ops.render.render(write_still = 1)

# Render front cubeface
camera.rotation_euler = mathutils.Euler((math.pi / 2, 0, 0))
scene.render.filepath = os.path.join(destination, filename + "_PosZ.png")
bpy.ops.render.render(write_still = 1)

# Render up cubeface
camera.rotation_euler = mathutils.Euler((math.pi, 0, 0))
scene.render.filepath = os.path.join(destination, filename + "_PosY.png")
bpy.ops.render.render(write_still = 1)

# Render back cubeface
camera.rotation_euler = mathutils.Euler((math.pi * 3 / 2, math.pi, 0))
scene.render.filepath = os.path.join(destination, filename + "_NegZ.png")
bpy.ops.render.render(write_still = 1)

# Render right cubeface
camera.rotation_euler = mathutils.Euler((- math.pi / 2, math.pi, -math.pi / 2))
scene.render.filepath = os.path.join(destination, filename + "_NegX.png")
bpy.ops.render.render(write_still = 1)

# Render left cubeface
camera.rotation_euler = mathutils.Euler((math.pi / 2, 0, -math.pi / 2))
scene.render.filepath = os.path.join(destination, filename + "_PosX.png")
bpy.ops.render.render(write_still = 1)

# Cleanup
objs = bpy.data.objects
objs.remove(camera, do_unlink=True)
