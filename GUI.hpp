#ifndef GUI_HPP
#define GUI_HPP

#include "BarTextureProgram.hpp"
#include "TextureProgram.hpp"
#include "Slots.hpp"
#include "gl_errors.hpp"
#include "Scene.hpp"
#include "Game.hpp"

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

	struct Vert
	{
		Vert() : position(0), texCoord(0), val(0) {};
		Vert(glm::vec3 p, glm::vec2 t, float v) : position(p), texCoord(t), val(v) {};
		
		glm::vec3 position;
		glm::vec2 texCoord;
		float val;
	};

	struct Element
	{
		
		virtual ~Element() = 0;
		
		virtual void update(float elapsed) = 0;
		virtual void render(glm::mat4 const& world_to_clip) = 0;
	};

	struct MoveGraphic : Element
	{
		
		MoveGraphic(GLuint t) : tex(t)
			{
				
				corners.emplace_back(glm::vec3(popup_bottom_left.x, popup_bottom_left.y, 1.0f), glm::vec2(0.0f, 0.0f), 0.0f); // 1
				corners.emplace_back(glm::vec3(popup_bottom_left.x, popup_top_right.y, 1.0f), glm::vec2(0.0f, 1.0f), 0.0f); // 2
				corners.emplace_back(glm::vec3(popup_top_right.x, popup_bottom_left.y, 0.0f), glm::vec2(1.0f, 0.0f), 1.0f); // 4 
				corners.emplace_back(glm::vec3(popup_top_right.x, popup_top_right.y, 0.0f), glm::vec2(1.0f, 1.0f), 1.0f); // 3
				
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, corners.size() * sizeof(Vert), corners.data(), GL_STATIC_DRAW);

				glGenVertexArrays(1, &vao);
				glBindVertexArray(vao);
				
				glVertexAttribPointer(texture_program->Position_vec4, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLbyte*)0 + offsetof(Vert, position));
				glEnableVertexAttribArray(texture_program->Position_vec4);

				glVertexAttribPointer(texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLbyte*)0 + offsetof(Vert, texCoord));
				glEnableVertexAttribArray(texture_program->TexCoord_vec2);
				
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);
			}

		virtual ~MoveGraphic()
			{
				glDeleteVertexArrays(1, &vao);
				glDeleteBuffers(1, &vbo);
			}

		void update(float elapsed) override
			{
			}

		void trigger_render(bool render_triggered_){
			render_triggered = render_triggered_;
		}
		
		void render(glm::mat4 const& world_to_clip) override
			{
				if (render_triggered){

					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * corners.size(), corners.data(), GL_STREAM_DRAW);
					glBindBuffer(GL_ARRAY_BUFFER, 0);

					glUseProgram(texture_program->program);
					glUniformMatrix4fv(texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
					glBindTexture(GL_TEXTURE_2D, tex);
					glDisable(GL_DEPTH_TEST);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glBindVertexArray(vao);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)corners.size());
					glDisable(GL_BLEND);
					
					glBindVertexArray(0);
					glBindTexture(GL_TEXTURE_2D, 0);
					glUseProgram(0);

					GL_ERRORS();
				}
			}
		
		GLuint tex; // We have to give this
		std::vector<Vert> corners;
		GLuint vao; // It makes these
		GLuint vbo;
		glm::vec2 popup_bottom_left = glm::vec2(-0.1f, -0.4f);
	    glm::vec2 popup_top_right = glm::vec2(0.1f,-0.3f);
		bool render_triggered = false;
	};

	struct Popup : Element
	{
		
		Popup(GLuint t, float display_interval_, bool initial_visibility) : tex(t)
			{

				visibility_triggered = initial_visibility;
				display_interval = display_interval_;
				
				corners.emplace_back(glm::vec3(popup_bottom_left.x, popup_bottom_left.y, 1.0f), glm::vec2(0.0f, 0.0f), 0.0f); // 1
				corners.emplace_back(glm::vec3(popup_bottom_left.x, popup_top_right.y, 1.0f), glm::vec2(0.0f, 1.0f), 0.0f); // 2
				corners.emplace_back(glm::vec3(popup_top_right.x, popup_bottom_left.y, 0.0f), glm::vec2(1.0f, 0.0f), 1.0f); // 4 
				corners.emplace_back(glm::vec3(popup_top_right.x, popup_top_right.y, 0.0f), glm::vec2(1.0f, 1.0f), 1.0f); // 3
				
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, corners.size() * sizeof(Vert), corners.data(), GL_STATIC_DRAW);

				glGenVertexArrays(1, &vao);
				glBindVertexArray(vao);
				
				glVertexAttribPointer(texture_program->Position_vec4, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLbyte*)0 + offsetof(Vert, position));
				glEnableVertexAttribArray(texture_program->Position_vec4);

				glVertexAttribPointer(texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLbyte*)0 + offsetof(Vert, texCoord));
				glEnableVertexAttribArray(texture_program->TexCoord_vec2);
				
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				if (initial_visibility){
					previous_display_start_time = ((float)clock())/1000.0f;
				}

			}

		virtual ~Popup()
			{
				glDeleteVertexArrays(1, &vao);
				glDeleteBuffers(1, &vbo);
			}

		void update(float elapsed) override
			{	
				// turn off the visibility of the element if it is
				// not turned off after a certain period of time
				if (visibility_triggered){
					float current_time = ((float)clock())/1000.0f;
					if (current_time > (display_interval + previous_display_start_time)){
						visibility_triggered = false;	
					}
				}
			}


		void trigger_visibility(bool tf){
			if (tf){
				visibility_triggered = true;
				previous_display_start_time = ((float)clock())/1000.0f;
			}else{
				visibility_triggered = false;		
			}
		}

		bool get_visible(){
			return visibility_triggered;
		}
		
		void render(glm::mat4 const& world_to_clip) override
			{
				// std::cout << "render popup " << std::endl;
				if (visibility_triggered){

					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * corners.size(), corners.data(), GL_STREAM_DRAW);
					glBindBuffer(GL_ARRAY_BUFFER, 0);

					glUseProgram(texture_program->program);
					glUniformMatrix4fv(texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
					glBindTexture(GL_TEXTURE_2D, tex);
					glDisable(GL_DEPTH_TEST);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glBindVertexArray(vao);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)corners.size());
					glDisable(GL_BLEND);
					
					glBindVertexArray(0);
					glBindTexture(GL_TEXTURE_2D, 0);
					glUseProgram(0);

					GL_ERRORS();
				}
			}
		
		GLuint tex; // We have to give this
		std::vector<Vert> corners;
		GLuint vao; // It makes these
		GLuint vbo;
		bool visibility_triggered;
		glm::vec2 popup_bottom_left = glm::vec2(-1.0f, -1.0f);
	    glm::vec2 popup_top_right = glm::vec2(1.0f, 1.0f);
		float display_interval; 
		float previous_display_start_time = 0.0f;
	};

	struct Bar : Element
	{
		
		Bar(std::function<float(float)> c, GLuint t) : calculateValue(c), tex(t)
			{
				std::vector<Vert> corners;

				glm::vec2 hpbar_bottom_left = glm::vec2(-1.0f, -1.0f);
				glm::vec2 hpbar_top_right = glm::vec2(1.0f, 1.0f);
				
				corners.emplace_back(glm::vec3(hpbar_bottom_left.x, hpbar_bottom_left.y, 1.0f), glm::vec2(0.0f, 0.0f), 0.0f); // 1
				corners.emplace_back(glm::vec3(hpbar_bottom_left.x, hpbar_top_right.y, 1.0f), glm::vec2(0.0f, 1.0f), 0.0f); // 2
				corners.emplace_back(glm::vec3(hpbar_top_right.x, hpbar_bottom_left.y, 0.0f), glm::vec2(1.0f, 0.0f), 1.0f); // 4 
				corners.emplace_back(glm::vec3(hpbar_top_right.x, hpbar_top_right.y, 0.0f), glm::vec2(1.0f, 1.0f), 1.0f); // 3
				
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
		void render(glm::mat4 const& world_to_clip) override
			{
				glUseProgram(bar_texture_program->program);
				glUniform1f(bar_texture_program->PERCENT_float, value);
				glUniform1f(bar_texture_program->ALPHA_float, alpha);
				glUniform4fv(bar_texture_program->CLIPPOS_vec4, 1, glm::value_ptr(glm::vec4(screenPos, 0.0f)));
				glUniform2fv(bar_texture_program->SCALE_vec2, 1, glm::value_ptr(scale));
				glUniform3fv(bar_texture_program->FULLCO_vec3, 1, glm::value_ptr(fullColor));
				glUniform3fv(bar_texture_program->EMPTYCO_vec3, 1, glm::value_ptr(emptyColor));
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

		glm::vec3 fullColor = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 emptyColor = glm::vec3(1.0f, 0.0f, 0.0f);
		
		glm::vec3 screenPos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec2 scale = glm::vec2(1.0f, 1.0f);
		float alpha = 1.0f;

		bool useCreatureID = false;
		Game::CreatureID creatureIDForPos;
	};
	
	Gui() : elements() {};

	~Gui() {} // Slots automatically handles deletion of the Element*
	
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
	void render(glm::mat4 const& world_to_clip)
		{
			for(auto e : elements.slots)
			{
				Element* elem = std::get<0>(e);
				if(elem != nullptr)
				{
					elem->render(world_to_clip);
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
