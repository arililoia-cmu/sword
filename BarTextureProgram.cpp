// All credit to Jim McCann, September 2021

#include "BarTextureProgram.hpp"

#include "GL.hpp"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Scene::Drawable::Pipeline bar_texture_program_pipeline;

Load< BarTextureProgram > bar_texture_program(LoadTagEarly, []() -> BarTextureProgram const * {
	BarTextureProgram *ret = new BarTextureProgram();

	//----- build the pipeline template -----
	bar_texture_program_pipeline.program = ret->program;

	bar_texture_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;

	//make a 1-pixel white texture to bind by default:
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	bar_texture_program_pipeline.textures[0].texture = tex;
	bar_texture_program_pipeline.textures[0].target = GL_TEXTURE_2D;

	return ret;
});

BarTextureProgram::BarTextureProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 OBJECT_TO_CLIP;\n"
		"in vec4 Position;\n"
		"in vec2 TexCoord;\n"
		"in float Val;\n"
		"out vec2 texCoord;\n"
		"out float val;\n"
		"void main() {\n"
		"	gl_Position = OBJECT_TO_CLIP * Position;\n"
		"	texCoord = TexCoord;\n"
		"   val = Val;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"uniform float PERCENT; \n"
		"in vec2 texCoord;\n"
		"in float val;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"   vec4 co = texture(TEX, texCoord);\n"
		"   if(co.rgb == vec3(0, 0, 0) && co.a > 0)\n"
		"   {\n"
		"      if(val <= PERCENT)\n"
		"      {\n"
		"         fragColor = vec4(1.0 * (1.0 - PERCENT), 1.0 * PERCENT, 0.0, co.a);"
		"      }\n"
		"      else\n"
		"      {\n"
		"	      fragColor = co; \n"
		"      }\n"
		"   }\n"
		"   else\n"
		"   {\n"
		"	   fragColor = co;\n"
		"   }\n"
		"}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	Position_vec4 = glGetAttribLocation(program, "Position");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");
	Val_f = glGetAttribLocation(program, "Val");

	//look up the locations of uniforms:
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");
	PERCENT_float = glGetUniformLocation(program, "PERCENT");

	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

BarTextureProgram::~BarTextureProgram() {
	glDeleteProgram(program);
	program = 0;
}

