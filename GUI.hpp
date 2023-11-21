#ifndef GUI_HPP
#define GUI_HPP

#include "BarTextureProgram.hpp"
#include "Slots.hpp"
#include "gl_errors.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include "GL.hpp"

#include <functional>
#include <vector>
#include <cstddef>
#include <set>

struct Gui
{	
	struct Element
	{
		virtual ~Element() = 0;
		
		virtual void update(float elapsed) = 0;
		virtual void render() = 0;
	};
	
	struct Bar : Element
	{
		struct Vert
		{
			Vert(glm::vec3 p, glm::vec2 t, float v) : position(p), texCoord(t), val(v) {};
			
			glm::vec3 position;
			glm::vec2 texCoord;
			float val;
		};
		
		Bar(std::function<float(float)> c, GLuint t) : calculateValue(c), tex(t)
			{
				std::vector<Vert> corners;

				glm::vec2 hpbar_bottom_left = glm::vec2(-0.9f, 0.6f);
				glm::vec2 hpbar_top_right = glm::vec2(0.9f, 0.9f);
				
				corners.emplace_back(glm::vec3( hpbar_bottom_left.x, hpbar_bottom_left.y, 0.0f), glm::vec2(0.0f, 0.0f), 0.0f); // 1
				corners.emplace_back(glm::vec3( hpbar_bottom_left.x,  hpbar_top_right.y, 0.0f), glm::vec2(0.0f, 1.0f), 0.0f); // 2
				corners.emplace_back(glm::vec3( hpbar_top_right.x, hpbar_bottom_left.y, 0.0f), glm::vec2(1.0f, 0.0f), 1.0f); // 4 
				corners.emplace_back(glm::vec3( hpbar_top_right.x, hpbar_top_right.y, 0.0f), glm::vec2(1.0f, 1.0f), 1.0f); // 3
				
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, corners.size() * sizeof(Vert), corners.data(), GL_STATIC_DRAW);

				glGenVertexArrays(1, &vao);
				glBindVertexArray(vao);
				
				glVertexAttribPointer(bar_texture_program->Position_vec4, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLbyte*)0 + offsetof(Vert, position));
				glEnableVertexAttribArray(bar_texture_program->Position_vec4);

				glVertexAttribPointer(bar_texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLbyte*)0 + offsetof(Vert, texCoord));
				glEnableVertexAttribArray(bar_texture_program->TexCoord_vec2);

				glVertexAttribPointer(bar_texture_program->Val_f, 1, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLbyte*)0 + offsetof(Vert, val));
				glEnableVertexAttribArray(bar_texture_program->Val_f);
				
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);
			}

		virtual ~Bar()
			{
				glDeleteVertexArrays(1, &vao);
				glDeleteBuffers(1, &vbo);
			}

		void update(float elapsed) override
			{
				value = calculateValue(elapsed);
			}
		void render() override
			{
				glUseProgram(bar_texture_program->program);
				glUniformMatrix4fv(bar_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
				glUniform1f(bar_texture_program->PERCENT_float, value);
				glBindTexture(GL_TEXTURE_2D, tex);
				
				glDisable(GL_DEPTH_TEST);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				
				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				
				glEnable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
				
				glBindVertexArray(0);
				glBindTexture(GL_TEXTURE_2D, 0);
				glUseProgram(0);

				GL_ERRORS();
			}
		
		std::function<float(float)> calculateValue; // Function that calculates the value for this bar
		float value; // From 0 to 1, don't touch

		GLuint tex; // We have to give this
		
		GLuint vao; // It makes these
		GLuint vbo;
	};
	
	Gui() : elements() {};
	
	void update(float elapsed)
		{
			for(auto e : elements.slots)
			{
				Element* elem = std::get<0>(e);
				if(elem != nullptr)
				{
					elem->update(elapsed);
				}
			}
		}
	void render()
		{
			for(auto e : elements.slots)
			{
				Element* elem = std::get<0>(e);
				if(elem != nullptr)
				{
					elem->render();
				}
			}
		}

	typedef SlotID GuiID;

	GuiID addElement(Element* e)
		{
			return elements.spawn(e);
		}
	bool removeElement(GuiID id)
		{
			return elements.destroy(id);
		}
	Element* getElement(GuiID id)
		{
			return elements.get(id);
		}
	
	Slots<Element*, 128> elements;
};

#endif
