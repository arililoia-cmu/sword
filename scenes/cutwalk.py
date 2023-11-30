import bpy
import bmesh

# bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.object.select_all(action='DESELECT')
if bpy.context.scene.objects.get("WalkMesh"):
    bpy.data.objects['WalkMesh'].select_set(True)
    bpy.context.view_layer.objects.active = bpy.data.objects['WalkMesh']
    bpy.ops.object.delete()

objectToSelect = bpy.data.objects["Walk"]
objectToSelect.select_set(True)    
bpy.context.view_layer.objects.active = objectToSelect

src_obj = bpy.context.active_object
new_obj = src_obj.copy()
new_obj.data = src_obj.data.copy()
bpy.data.collections['WalkMeshes'].objects.link(new_obj)
new_obj.name = 'WalkMesh'
new_obj.data.name = 'WalkMesh'

bpy.ops.object.select_all(action='DESELECT')

WalkMesh = bpy.data.collections["WalkMeshes"].all_objects["WalkMesh"]
Platforms = bpy.data.collections["Platforms"]
Extra = bpy.data.collections["Extra"]
bpy.context.view_layer.objects.active = WalkMesh

for obj in Platforms.all_objects:
    objname = obj.name
    if (objname[0:3] == "Box" or objname[0:11] == "TreeCollide" or objname[0:14] == "PillarcCollide"):
        print(objname)
        bool_op = WalkMesh.modifiers.new(type="BOOLEAN", name=objname)
        bool_op.object = obj
        bool_op.operation = "DIFFERENCE"
        bool_op.solver = "FAST"
        if objname[0:14] == "PillarcCollide":
            bool_op.solver = "EXACT"
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=objname)

for obj in Extra.all_objects:
    objname = obj.name
    if (objname[0:3] == "Box"):
        print(objname)
        bool_op = WalkMesh.modifiers.new(type="BOOLEAN", name=objname)
        bool_op.object = obj
        bool_op.operation = "DIFFERENCE"
        bool_op.solver = "FAST"
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=objname)

