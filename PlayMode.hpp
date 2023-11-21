#pragma once
#include "Mode.hpp"

#include "Game.hpp"
#include "Scene.hpp"
#include "WalkMesh.hpp"
#include "Pawn.hpp"
#include "GUI.hpp"
// #include "BehaviorTree.hpp"
#include "Collisions.hpp"
#include "Sound.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <ctime>
#include <iostream>

class HpBar
{
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


struct Vert
{
	Vert(glm::vec3 const &position_, glm::vec2 const &tex_coord_) : position(position_), tex_coord(tex_coord_) { }
	glm::vec3 position;
	glm::vec2 tex_coord;
};

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// Input, update, render
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	virtual glm::vec2 object_to_window_coordinate(Scene::Transform *object, Scene::Camera *camera, glm::uvec2 const &drawable_size);
	
	//----- game state -----	
	//input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, secondAction, mainAction, dodge;

	Player* player;
	std::array<Enemy*, 5> enemies;
	
	Game game;
	
	Scene scene;
	CollisionEngine collEng;
	Gui gui;

	void walk_pawn(Pawn& pawn, glm::vec3 movement);
	void processPawnControl(Pawn& pawn, float elapsed);

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
