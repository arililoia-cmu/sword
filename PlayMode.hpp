#pragma once
#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"
// #include "BehaviorTree.hpp"
#include "load_save_png.hpp"
#include "Collisions.hpp"
#include "Sound.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <deque>
#include <ctime>
#include <iostream>

#include "data_path.hpp"
#include "TextureProgram.hpp"

class BehaviorTree;//Forward Declaration

struct Vert {
		Vert(glm::vec3 const &position_, glm::vec2 const &tex_coord_) : position(position_), tex_coord(tex_coord_) { }
		glm::vec3 position;
		glm::vec2 tex_coord;
	};

class ImagePopup{
    public:
        std::vector< glm::u8vec4 > img_data;
        glm::uvec2 img_size;
        std::vector< glm::u8vec4 > img_tex_data;
        GLuint img_tex = 0;
        GLuint img_buffer = 0;
        GLuint img_vao = 0;
        float previous_display_start_time = 0.0f;
        float display_interval = 5.0f;
        // specify bottom left and bottom right in terms of (-1,1) range 
        // coordinates to make resizing easier
        glm::vec2 img_bottom_left;
        glm::vec2 img_top_right;
        std::vector< Vert > img_attribs;

        ImagePopup(){}

        void Init(std::string const &image_path, glm::vec2 img_bottom_left_, 
                glm::vec2 img_top_right_, float display_interval_){
        
            display_interval = display_interval_;
            img_bottom_left = img_bottom_left_;
            img_top_right = img_top_right_;

            load_png(data_path(image_path), &img_size, &img_data, OriginLocation::UpperLeftOrigin);
            for (int i=img_size.y-1; i>=0; i--){
                for (int j=0; j< (int) img_size.x; j++){
                    glm::u8vec4 pixel_at = img_data.at((i*img_size.x)+j);
                    img_tex_data.push_back(pixel_at);
                }
            }

            glGenTextures(1, &img_tex);
            glBindTexture(GL_TEXTURE_2D, img_tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_size.x, img_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_tex_data.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenBuffers(1, &img_buffer);

            glGenVertexArrays(1, &img_vao);
            glBindVertexArray(img_vao);

            glBindBuffer(GL_ARRAY_BUFFER, img_buffer);
            glVertexAttribPointer(
                texture_program->Position_vec4, //attribute
                3, //size
                GL_FLOAT, //type
                GL_FALSE, //normalized
                sizeof(Vert), //stride
                (GLbyte *)0 + offsetof(Vert, position) //offset
            );
            glEnableVertexAttribArray(texture_program->Position_vec4);

            glVertexAttribPointer(
                texture_program->TexCoord_vec2, //attribute
                2, //size
                GL_FLOAT, //type
                GL_FALSE, //normalized
                sizeof(Vert), //stride
                (GLbyte *)0 + offsetof(Vert, tex_coord) //offset
            );
            glEnableVertexAttribArray(texture_program->TexCoord_vec2);

            glUseProgram(texture_program->program);

            img_attribs.emplace_back(glm::vec3( img_bottom_left.x, img_bottom_left.y, 0.0f), glm::vec2(0.0f, 0.0f)); // 1
            img_attribs.emplace_back(glm::vec3( img_bottom_left.x,  img_top_right.y, 0.0f), glm::vec2(0.0f, 1.0f)); // 2
            img_attribs.emplace_back(glm::vec3( img_top_right.x, img_bottom_left.y, 0.0f), glm::vec2(1.0f, 0.0f)); // 4 
            img_attribs.emplace_back(glm::vec3( img_top_right.x, img_top_right.y, 0.0f), glm::vec2(1.0f, 1.0f)); // 3

        }

        void trigger_draw(){
            previous_display_start_time = ((float)clock())/1000.0f;

        }

        void draw(){
            // clock_t current_time = clock();
            float current_time = ((float)clock())/1000.0f;
            std::cout << "previous_display_start_time: " << previous_display_start_time << std::endl;
            std::cout << "current time: " << current_time << std::endl;
            std::cout << "display interval + prev display stat time: " << display_interval + previous_display_start_time << std::endl;
            if (current_time < display_interval + previous_display_start_time){
                glBindBuffer(GL_ARRAY_BUFFER, img_buffer);
                glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * img_attribs.size(), img_attribs.data(), GL_STREAM_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                
                glUseProgram(texture_program->program);
                glUniformMatrix4fv(texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
                glBindTexture(GL_TEXTURE_2D, img_tex);
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBindVertexArray(img_vao);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)img_attribs.size());
                glDisable(GL_BLEND);
                glBindTexture(GL_TEXTURE_2D, 0);
                glUseProgram(0);
                // GL_ERRORS();
            }
        }

};


class HpBar{
	public:
		int max_hp=0, current_hp=0;
		int empty_x = -1;
		int full_x = 0;
		std::vector<int> fillin_indices;
		std::vector< glm::u8vec4 > tex_data;
		GLuint hp_tex = 0;
		GLuint hp_buffer = 0;
	    GLuint vao = 0;
		glm::uvec2 hp_size;
	    std::vector< glm::u8vec4 > hp_data;
		bool change_enemy_hp = false;

		HpBar(){}

		// going after naming convention in behaviortree
		void Init(int init_hp){
			std::cout << "hpbar initialize start" << std::endl;
			max_hp = init_hp;
			std::cout << "max hp set" << std::endl;
			current_hp = init_hp;
			std::cout << "current hp set" <<init_hp<< std::endl;
		//	std::cin>>current_hp;
		}

		float get_percent_hp_left(){
			return ((float)current_hp / (float)max_hp);
		}

		void change_hp_by(int to_change){
			if (current_hp + to_change > max_hp){
				current_hp = max_hp;
			}
			else if (current_hp + to_change < 0){
				current_hp = 0;
			}
			else{
				current_hp += to_change;
			}
		}

		glm::u8vec4 get_health_color(float hp_bar_transparency){
			glm::u8vec4 health_color = glm::u8vec4(
				0xff * (1.0f - get_percent_hp_left()),
				0xff * get_percent_hp_left(),
				0x00, 0xff * hp_bar_transparency
			);
			return health_color;
		}

};
	





struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	virtual glm::vec2 object_to_window_coordinate(Scene::Transform *object, Scene::Camera *camera, glm::uvec2 const &drawable_size);

	//----- game state -----

	

	
	

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, secondAction, mainAction, dodge;//main=attack second=parry
	bool isMouseVertical=true;
	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	struct Control
	{
		// enum class STANCE
		// {
		// 	IDLE = 0,
		// 	WALK,
		// 	ATTACK_A_FORWARD,
		// 	ATTACK_A_BOUNCE,
		// 	ATTACK_A_BACKWARD,
		// 	PARRY_FORWARD,
		// 	PARRY_BACKWARD,
		// 	DODGE
		// };

		//STANCE stance;

		struct DodgeStanceInfo
		{
			glm::vec3 dir;
			int attackAfter;
		};
		struct AttackStanceInfo
		{
			glm::vec3 dir;
			int attackAfter;
		};
		struct LungeStanceInfo
		{
			glm::vec3 dir;
			int attackAfter;
		};
		struct SweepStanceInfo
		{
			glm::vec3 dir;
			int attackAfter;
		};
		
		union StanceInfo
		{
			DodgeStanceInfo dodge;
			AttackStanceInfo attack;
			SweepStanceInfo sweep;
			LungeStanceInfo lunge;
		} stanceInfo;

		glm::vec3 vel = glm::vec3(0.0f);
		glm::vec3 move = glm::vec3(0.0f); // displacement in world (should be scaled by elapsed)
		float rotate = 0.0f; // angle in world, normal direction, ignore for player 
		uint8_t attack = 0;//1=vertical 2=horizontal;
		uint8_t parry = 0;
		uint8_t dodge = 0;
		

		uint8_t stance = 0; // state variable, 0 is idle, 1 is swing forward, 2 is swing backward (bounce), 3 is swing backward (normal), 
		// 4 is down parry, 5 is up parry, 6 is dodge

		float swingHit = 0.0f; // logged on block
		float swingTime = 0.0f;
	}; 

	//player info:
	struct Pawn {
		WalkPoint at;
		//transform is at pawn's feet 
		Scene::Transform *transform = nullptr;
		//body_transform is at pawn's body
		Scene::Transform *body_transform = nullptr;

		bool is_player = false;
		glm::quat default_rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Describes ``front'' direction with respect to +x
		std::string gameplay_tags="";
		// TODO
		Scene::Transform *arm_transform = nullptr;
		Scene::Transform *wrist_transform = nullptr;
		Scene::Transform *sword_transform = nullptr;

		Control pawn_control;
		HpBar* hp;
		
		// Hp_Bar hp_bar;
		// MyStruct myInstance(int 42);

	};

	void walk_pawn(PlayMode::Pawn &pawn, glm::vec3 movement);

	void processPawnControl(PlayMode::Pawn& pawn, float elapsed);


	struct Enemy : Pawn {
		BehaviorTree* bt;
		// HpBar* hp;
	} enemy;
	Enemy enemyList[5];
	struct Player : Pawn {
		
		//camera_transform joins body_transform and camera->transform
		Scene::Transform *camera_transform = nullptr;
		//camera is attatched to camera_transform and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;

	} player;

	Scene::Transform* groundTransform;
	
	CollisionEngine collEng;

	// sound stuff starts here:
	// set previous clang time to something ridiculous
	clock_t previous_player_sword_clang_time = clock();
	clock_t previous_enemy_sword_clang_time = clock();
	clock_t previous_sword_whoosh_time = clock();
	clock_t previous_footstep_time = clock();
	float min_player_sword_clang_interval = 0.2f;
	float min_enemy_sword_clang_interval = 0.1f;
	float min_sword_whoosh_interval = 0.2f;
	float min_footstep_interval = 0.1f;

	std::shared_ptr< Sound::PlayingSample > w_conv1_sound;
	std::shared_ptr< Sound::PlayingSample > w_conv2_sound;
	std::shared_ptr< Sound::PlayingSample > fast_upswing_sound;
	std::shared_ptr< Sound::PlayingSample > fast_downswing_sound;
	std::shared_ptr< Sound::PlayingSample > footstep_wconv1_sound;
	// sound stuff ends here:

	// random graphics stuff for the hp_bar:
	float hp_bar_transparency = 0.5f; 
	// value between 0 and 1 to set the transparency
	// of the HP bar on the screen
	bool change_player_hp = false;
	bool change_enemy_hp = false;

	int hp_bar_empty_x = -1;
	int hp_bar_full_x = 0;
	

	glm::u8vec4 empty_color = glm::u8vec4(0x00f, 0x00f, 0x00f, 0xff*hp_bar_transparency);


	std::vector<int> hpbar_fillin_indices;
	

};
