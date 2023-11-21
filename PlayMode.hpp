#pragma once
#include "Mode.hpp"

#include "Game.hpp"
#include "Scene.hpp"
#include "WalkMesh.hpp"
#include "Pawn.hpp"
#include "GUI.hpp"
#include "Collisions.hpp"
#include "Sound.hpp"
#include "Slots.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <ctime>
#include <iostream>

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// Input, update, render
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	
	//----- game state -----	
	//input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, secondAction, mainAction, dodge;

	Player* player;
	std::array<Enemy*, 5> enemies;
	std::array<Gui::GuiID, 5> enemyHpBars;
	
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
};
