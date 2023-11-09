#!/usr/bin/env python

#based on 'export-sprites.py' and 'glsprite.py' from TCHOW Rainbow; code used is released into the public domain.

#Note: Script meant to be executed from within blender, as per:
#blender --background --python export-meshes.py -- <infile.blend>[:collection] <outfile.w>

import sys,re

args = []
for i in range(0,len(sys.argv)):
        if sys.argv[i] == '--':
                args = sys.argv[i+1:]

if len(args) < 2 or len(args) > 3:
        print("\n\nUsage:\nblender --background --python export-collmeshes.py -- <infile.blend>[:collection] [pattern] <outfile.w>\nExports the meshes with names matching regex /pattern/ (default /.*/) referenced by all objects in collection to a binary blob, in collmesh format, indexed by the names of the objects that reference them.\n")
        exit(1)

infile = args[0]

collection_name = None
m = re.match(r'^(.*):([^:]+)$', infile)
if m:
        infile = m.group(1)
        collection_name = m.group(2)

if len(args) == 3:
        pattern = args[1]
        outfile = args[2]
else:
        pattern = '.*'
        outfile = args[1]

assert outfile.endswith(".c")

print("Will export meshes referenced from ",end="")
if collection_name:
        print("collection '" + collection_name + "'",end="")
else:
        print('master collection',end="")
print(" of '" + infile + "', matching pattern /" + pattern + "/ to '" + outfile + "'.")


import bpy
import struct
import re

bpy.ops.wm.open_mainfile(filepath=infile)


if collection_name:
        if not collection_name in bpy.data.collections:
                print("ERROR: Collection '" + collection_name + "' does not exist in scene.")
                exit(1)
        collection = bpy.data.collections[collection_name]
else:
        collection = bpy.context.scene.collection

#meshes to write:
to_write = set()
did_collections = set()
def add_meshes(from_collection):
        global to_write
        global did_collections
        if from_collection in did_collections:
                return
        did_collections.add(from_collection)

        for obj in from_collection.objects:
                if obj.type == 'MESH':
                        if re.match(pattern, obj.data.name):
                                to_write.add(obj.data)
                if obj.instance_collection:
                        add_meshes(obj.instance_collection)
        for child in from_collection.children:
                add_meshes(child)

add_meshes(collection)

print("Added meshes from: ", did_collections)

#set all collections visible: (so that meshes can be selected for triangulation)
def set_visible(layer_collection):
        layer_collection.exclude = False
        layer_collection.hide_viewport = False
        layer_collection.collection.hide_viewport = False
        for child in layer_collection.children:
                set_visible(child)

set_visible(bpy.context.view_layer.layer_collection)

#vertex (as vec3), normal (as vec3), and triangle (as uvec3) data from the meshes:
positions = b''

#strings contains the mesh names:
strings = b''

#containingRads contains the radius that encloses all the vertices in the mesh from the center
# containingRads = b''

#index gives offsets into the data (and names) for each mesh:
index = b''

position_count = 0

for obj in bpy.data.objects:
        if obj.data in to_write:
                to_write.remove(obj.data)
        else:
                continue

        obj.hide_select = False
        mesh = obj.data
        name = mesh.name

        print("Writing '" + name + "'...")

        if bpy.context.object:
                bpy.ops.object.mode_set(mode='OBJECT') #get out of edit mode (just in case)

        #select the object and make it the active object:
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.mode_set(mode='OBJECT')

        #apply all modifiers (?):
        bpy.ops.object.convert(target='MESH')
        
        vertex_begin = position_count
        
        for vertex in mesh.vertices:
                positions += struct.pack('fff', *vertex.co)
                position_count += 1

        vertex_end = position_count

        #record mesh name, vertex range
        name_begin = len(strings)
        strings += bytes(name, "utf8")
        name_end = len(strings)
        index += struct.pack('II', name_begin, name_end)
        index += struct.pack('II', vertex_begin, vertex_end)
        # index += struct.pack('II', containingRadsAt)


#check that we wrote as much data as anticipated:
assert(position_count * 3*4 == len(positions))

#write the data chunk and index chunk to an output blob:
blob = open(outfile, 'wb')

def write_chunk(magic, data):
        blob.write(struct.pack('4s',magic)) #type
        blob.write(struct.pack('I', len(data))) #length
        blob.write(data)

#first chunk: the positions
write_chunk(b'p...', positions)
write_chunk(b'str0', strings)
# write_chunk(b'rad0', containingRads)
write_chunk(b'idxA', index)
wrote = blob.tell()
blob.close()

print("Wrote " + str(wrote) + " bytes [== " +
        str(len(positions)+8) + " bytes of positions + " +
        str(len(strings)+8) + " bytes of strings + " +
        # str(len(containingRads)+8) + " bytes of containingRads + " +
        str(len(index)+8) + " bytes of index] to '" + outfile + "'")
