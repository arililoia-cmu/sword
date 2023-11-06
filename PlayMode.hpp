#pragma once
#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"
// #include "BehaviorTree.hpp"
#include "Collisions.hpp"
#include "Sound.hpp"
#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <ctime>
#include <iostream>

class BehaviorTree;//Forward Declaration

class HpBar{
public:
	int current_hp;
	HpBar(int init_hp){
		current_hp = init_hp;
	};

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
	} left, right, down, up, secondAction, mainAction;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	struct Control {
		glm::vec3 move = glm::vec3(0.0f); // displacement in world (should be scaled by elapsed)
		float rotate = 0.0f; // angle in world, normal direction, ignore for player 
		uint8_t attack = 0;
		uint8_t parry = 0;

		uint8_t stance = 0; // state variable, 0 is idle, 1 is swing forward, 2 is swing backward (bounce), 3 is swing backward (normal), 
		// 4 is down parry, 5 is up parry

		float swingHit = 0.0f; // logged on block
		float swingTime = 0.0f;
	}; 

	// struct MyStruct {
	// 	// Member variables
	// 	int data;
	// 	// Constructor
	// 	MyStruct(int value) : data(value) {}

	// };


	//player info:
	struct Pawn {
		WalkPoint at;
		//transform is at pawn's feet 
		Scene::Transform *transform = nullptr;
		//body_transform is at pawn's body
		Scene::Transform *body_transform = nullptr;

		bool is_player = false;
		glm::quat default_rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Describes ``front'' direction with respect to +x

		// TODO
		Scene::Transform *arm_transform = nullptr;
		Scene::Transform *wrist_transform = nullptr;
		Scene::Transform *sword_transform = nullptr;

		Control pawn_control;
		// HpBar hpbar(10);
		// Hp_Bar hp_bar;
		// MyStruct myInstance(int 42);

	};

	void walk_pawn(PlayMode::Pawn &pawn, float elapsed);


	struct Enemy : Pawn {
		BehaviorTree* bt;
	} enemy;

	struct Player : Pawn {
		
		//camera_transform joins body_transform and camera->transform
		Scene::Transform *camera_transform = nullptr;
		//camera is attatched to camera_transform and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;

	} player;	
	
	Collisions collEng;

	// sound stuff starts here:
	// set previous clang time to something ridiculous
	clock_t previous_player_sword_clang_time = clock();
	clock_t previous_enemy_sword_clang_time = clock();
	clock_t previous_sword_whoosh_time = clock();
	clock_t previous_footstep_time = clock();
	float min_player_sword_clang_interval = 0.2;
	float min_enemy_sword_clang_interval = 0.1;
	float min_sword_whoosh_interval = 0.2;
	float min_footstep_interval = 0.1;

	std::shared_ptr< Sound::PlayingSample > w_conv1_sound;
	std::shared_ptr< Sound::PlayingSample > w_conv2_sound;
	std::shared_ptr< Sound::PlayingSample > fast_upswing_sound;
	std::shared_ptr< Sound::PlayingSample > fast_downswing_sound;
	std::shared_ptr< Sound::PlayingSample > footstep_wconv1_sound;
	// sound stuff ends here:
};
