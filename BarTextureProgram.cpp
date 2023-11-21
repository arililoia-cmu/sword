// All credit to Jim McCann, September 2021

#include "BarTextureProgram.hpp"

#include "GL.hpp"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< BarTextureProgram > bar_texture_program(LoadTagEarly, []() -> BarTextureProgram const * {
	BarTextureProgram *ret = new BarTextureProgram();
	return ret;
});

BarTextureProgram::BarTextureProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform vec4 CLIPPOS;\n"
		"uniform vec2 SCALE;\n"
		"in vec4 Position;\n"
		"in vec2 TexCoord;\n"
		"in float Val;\n"
		"out vec2 texCoord;\n"
		"out float val;\n"
		"void main() {\n"
		"	gl_Position = vec4(Position.x * SCALE.x, Position.y * SCALE.y, Position.zw) + CLIPPOS;\n"
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
	PERCENT_float = glGetUniformLocation(program, "PERCENT");
	CLIPPOS_vec4 = glGetUniformLocation(program, "CLIPPOS");
	SCALE_vec2 = glGetUniformLocation(program, "SCALE");

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

