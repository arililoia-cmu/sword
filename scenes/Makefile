.PHONY : all

#n.b. the '-y' sets autoexec scripts to 'on' so that driver expressions will work
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	BLENDER = /Applications/Blender/blender.app/Contents/MacOS/blender -y
#else ifeq ($(UNAME_S),Linux)
#	BLENDER = /usr/bin/blender -y
else 
	BLENDER = blender
endif

EXPORT_MESHES=export-meshes.py
EXPORT_WALKMESHES=export-walkmeshes.py
EXPORT_SCENE=export-scene.py
EXPORT_COLLMESHES=export-collmeshes.py

DIST=../dist

all : \
	$(DIST)/sword.pnct \
	$(DIST)/sword.w \
	$(DIST)/sword.scene \
	$(DIST)/sword.c \

$(DIST)/sword.pnct : sword.blend $(EXPORT_MESHES)
	$(BLENDER) --background --python $(EXPORT_MESHES) -- '$<':Platforms '$@'

$(DIST)/sword.scene : sword.blend $(EXPORT_SCENE)
	$(BLENDER) --background --python $(EXPORT_SCENE) -- '$<':Platforms '$@'

$(DIST)/sword.w : sword.blend $(EXPORT_WALKMESHES)
	$(BLENDER) --background --python $(EXPORT_WALKMESHES) -- '$<':WalkMeshes '$@'

# I am just reusing the walkmesh format for collide meshes. I bet the normals and tri information will come in useful later. 
$(DIST)/sword.c : sword.blend $(EXPORT_COLLMESHES)
	$(BLENDER) --background --python $(EXPORT_COLLMESHES) -- '$<':CollideMeshes '$@'
