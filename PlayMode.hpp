#pragma once
#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"
//#include "BehaviorTree.hpp"
#include <glm/glm.hpp>

#include <vector>
#include <deque>
class BehaviorTree;//Forward Declaration
struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, mainAction;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	struct Control {
		glm::vec3 move = glm::vec3(0.0f); // displacement in world (should be scaled by elapsed)
		float rotate = 0.0f; // angle in world, normal direction, ignore for player 
		uint8_t attack = 0;
		uint8_t parry = 0;

		float const swingCooldown = 1.0f;
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

		// TODO
		Scene::Transform *wrist_transform = nullptr;
		Scene::Transform *sword_transform = nullptr;

		Control pawn_control;
	};
	void walk_pawn(PlayMode::Pawn &pawn);

	struct Enemy : Pawn {
		BehaviorTree* bt;
	} enemy;

	struct Player : Pawn {
		
		//camera_transform joins body_transform and camera->transform
		Scene::Transform *camera_transform = nullptr;
		//camera is attatched to camera_transform and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;
	} player;	
	

};
