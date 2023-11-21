// All credit to Jim McCann, September 2021

#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

//Shader program that draws transformed, lit, textured vertices tinted with vertex colors:
struct BarTextureProgram {
	BarTextureProgram();
	~BarTextureProgram();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	GLuint Position_vec4 = -1U;
	GLuint TexCoord_vec2 = -1U;
	GLuint Val_f = -1U;

	//Uniform (per-invocation variable) locations:
	GLuint OBJECT_TO_CLIP_mat4 = -1U;
	GLuint PERCENT_float = -1U;

	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

extern Load<BarTextureProgram> bar_texture_program;

//For convenient scene-graph setup, copy this object:
// NOTE: by default, has texture bound to 1-pixel white texture -- so it's okay to use with vertex-color-only meshes.
extern Scene::Drawable::Pipeline bar_texture_program_pipeline;
