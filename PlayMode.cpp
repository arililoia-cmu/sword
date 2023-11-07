#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include <SDL.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>
#include "BehaviorTree.hpp"

#include "load_save_png.hpp"
#include "data_path.hpp"

#define M_PI_2f 1.57079632679489661923f
GLuint phonebank_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > phonebank_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("sword.pnct"));
	phonebank_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > phonebank_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("sword.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = phonebank_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = phonebank_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

WalkMesh const* walkmesh = nullptr;
Load< WalkMeshes > phonebank_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("sword.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

CollideMesh const* playerSwordCollMesh = nullptr;
CollideMesh const* enemySwordCollMesh = nullptr;
Load<CollideMeshes> SWORD_COLLIDE_MESHES(LoadTagDefault, []() -> CollideMeshes const*
	{
		CollideMeshes* ret = new CollideMeshes(data_path("sword.c"));
		playerSwordCollMesh = &ret->lookup("PlayerSwordCollMesh");
		enemySwordCollMesh = &ret->lookup("EnemySwordCollMesh");
		return ret;
	});

// in lieu of the 'multisample' object i'd like to make, for the demo,
// i'm loading in the sounds invidually and selecting them randomly to play.
// sound stuff starts here:
Load< Sound::Sample > w_conv1(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sound/sword_clang/w_conv1.wav"));
});

Load< Sound::Sample > w_conv2(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sound/sword_clang/w_conv2.wav"));
});

Load< Sound::Sample > fast_downswing(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sound/sword_whoosh/fast_downswing.wav"));
});

Load< Sound::Sample > fast_upswing(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sound/sword_whoosh/fast_upswing.wav"));
});

Load< Sound::Sample > footstep_wconv1(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sound/footstep/footstep_wconv1.wav"));
});
// sound stuff ends here

// this method taken from my game 2 code:
// https://github.com/arililoia-cmu/15-466-f23-base2/blob/8697e4fed38995ac9b5949fd30c0f75dabe02444/PlayMode.cpp
glm::vec2 PlayMode::object_to_window_coordinate(Scene::Transform *object, Scene::Camera *camera, glm::uvec2 const &drawable_size){

	std::cout << "object_to_window_coordinate: " << std::endl;
	std::cout << "drawable_size.x,y :" << drawable_size.x << " " << drawable_size.y << std::endl;
	

	// glm::vec4 object_position = glm::vec4(object->position.x, object->position.y, object->position.z, 1.0f);
	glm::vec4 object_position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	std::cout << "object_position: " << object_position.x << " " << object_position.y << " " << object_position.z << std::endl;

	glm::mat4x3 object_to_world = object->make_local_to_world();
	// object to world 3x4 x 4x1 coordinates = 3x1 world coordinates
	glm::vec3 op_world = object_to_world * object_position;

	std::cout << "op_world: " << op_world.x << " " << op_world.y << " " << op_world.z << std::endl;

	// make a 4 vector out of op_world
	glm::vec4 op_world_vec4 = glm::vec4(op_world.x, op_world.y, op_world.z, 1.0f);

	assert(camera->transform);
	glm::mat4 world_to_clip = camera->make_projection() * glm::mat4(camera->transform->make_world_to_local());
	glm::vec4 op_clip = world_to_clip * op_world_vec4;

	float ws_x = op_clip.x / op_clip.w;
	float ws_y = op_clip.y / op_clip.w;

	// The next step is to transform from this [-1, 1] space to window-relative coordinate
	// taken from this stackexchange post
	// https://stackoverflow.com/questions/8491247/c-opengl-convert-world-coords-to-screen2d-coords
	float ws_x_real = ((ws_x+1.0f)/2.0f) * (drawable_size.x / 2.0f);
	float ws_y_real = (drawable_size.y / 2.0f) - (((ws_y+1.0f)/2.0f)*(drawable_size.y / 2.0f));
	float ws_y_real2 = abs((drawable_size.y / 2.0f) - ws_y_real);
	return glm::vec2(ws_x_real, ws_y_real2);
}

PlayMode::PlayMode() : scene(*phonebank_scene) {
	//create a player transform:

	for (auto &transform : scene.transforms) {
		if (transform.name == "Player_Body"){
			player.body_transform = &transform;
			player.at = walkmesh->nearest_walk_point(player.body_transform->position + glm::vec3(0.0f, 0.0001f, 0.0f));
			float height = glm::length(player.body_transform->position - walkmesh->to_world_point(player.at));
			player.body_transform->position = glm::vec3(0.0f, 0.0f, height);
		} else if (transform.name == "Enemy_Body"){
			enemy.body_transform = &transform;
			enemy.at = walkmesh->nearest_walk_point(enemy.body_transform->position + glm::vec3(0.0f, 0.0001f, 0.0f));
			float height = glm::length(enemy.body_transform->position - walkmesh->to_world_point(enemy.at));
			enemy.body_transform->position = glm::vec3(0.0f, 0.0f, height);
		} else if (transform.name == "Player_Sword"){
			player.sword_transform = &transform;
		} else if (transform.name == "Player_Wrist"){
			player.wrist_transform = &transform;
		} else if (transform.name == "Enemy_Sword"){
			enemy.sword_transform = &transform;
		} else if (transform.name == "Enemy_Wrist"){
			enemy.wrist_transform = &transform;
		}
	}

	//create player transform at player feet and start player walking at nearest walk point:
	scene.transforms.emplace_back();
	player.transform = &scene.transforms.back();
	player.body_transform->parent = player.transform;
	player.transform->position = walkmesh->to_world_point(player.at);
	player.is_player = true;

	//same for enemy
	scene.transforms.emplace_back();
	enemy.transform = &scene.transforms.back();
	enemy.body_transform->parent = enemy.transform;
	enemy.transform->position = walkmesh->to_world_point(enemy.at);
	enemy.default_rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); //dictates enemy's original rotation wrt +x

	enemy.bt=new BehaviorTree();
	enemy.bt->Init();//AI Initialize
	enemy.bt->SetEnemy(&enemy);
	enemy.bt->SetPlayer(&player);
	//setup camera
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	player.camera = &scene.cameras.front();
	player.camera->fovy = glm::radians(60.0f);
	player.camera->near = 0.01f;

	//between camera and player
	scene.transforms.emplace_back();
	player.camera_transform = &scene.transforms.back();
	player.camera_transform->parent = player.body_transform;
	player.camera_transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); //dictates camera's original rotation wrt +y (not +x)

	player.camera->transform->parent = player.camera_transform;
	player.camera->transform->position = glm::vec3(0.0f, -10.0f, 0.0f);

	//rotate camera facing direction relative to player direction
	player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));


	//arm transform between body and wrist
	scene.transforms.emplace_back();
	player.arm_transform = &scene.transforms.back();
	player.arm_transform->parent = player.body_transform;
	player.wrist_transform->parent = player.arm_transform;
	scene.transforms.emplace_back();
	enemy.arm_transform = &scene.transforms.back();
	enemy.arm_transform->parent = enemy.body_transform;
	enemy.wrist_transform->parent = enemy.arm_transform;
	
	auto playerSwordHit = [this](Scene::Transform* t) -> void
		{
			if(t == enemy.sword_transform)
			{
				if(player.pawn_control.stance == 1)
				{
					player.pawn_control.stance = 2;
					player.pawn_control.swingHit = player.pawn_control.swingTime;
				}
				// else if(player.pawn_control.stance == 4)
				// {
				// 	player.pawn_control.stance = 5;
				// }

				// sound stuff starts here:
				clock_t current_time = clock();
				float elapsed = (float)(current_time - previous_player_sword_clang_time);

				if ((elapsed / CLOCKS_PER_SEC) > min_player_sword_clang_interval){
					w_conv1_sound = Sound::play(*w_conv1, 1.0f, 0.0f);
					previous_player_sword_clang_time = clock();
				}
				// sound stuff ends here

			}

		};

	auto enemySwordHit = [this](Scene::Transform* t) -> void
		{
			if(t == player.sword_transform)
			{
				if(enemy.pawn_control.stance == 1)
				{
					enemy.pawn_control.stance = 2;
					enemy.pawn_control.swingHit = enemy.pawn_control.swingTime;
				}
				// else if(enemy.pawn_control.stance == 4)
				// {
				// 	enemy.pawn_control.stance = 5;
				// }

				clock_t current_time = clock();
				float elapsed = (float)(current_time - previous_enemy_sword_clang_time);

				if ((elapsed / CLOCKS_PER_SEC) > min_enemy_sword_clang_interval){
					w_conv2_sound = Sound::play(*w_conv2, 1.0f, 0.0f);
					previous_enemy_sword_clang_time = clock();
				}
			}
		};
	
	// Create and add colliders
	collEng.cs.emplace_back(player.sword_transform, playerSwordCollMesh, (AABB){glm::vec3(0.0f), glm::vec3(0.0f)}, playerSwordHit);
	collEng.cs.emplace_back(enemy.sword_transform, enemySwordCollMesh, (AABB){glm::vec3(0.0f), glm::vec3(0.0f)}, enemySwordHit);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			mainAction.downs += 1;
			mainAction.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			secondAction.downs += 1;
			secondAction.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			mainAction.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			secondAction.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			glm::vec3 upDir = walkmesh->to_world_smooth_normal(player.at);
			player.transform->rotation = glm::angleAxis(-motion.x * player.camera->fovy, upDir) * player.transform->rotation;

			float pitch = glm::pitch(player.camera->transform->rotation);
			pitch += motion.y * player.camera->fovy;
			//camera looks down -z (basically at the player's feet) when pitch is at zero.
			pitch = std::min(pitch, 0.95f * 3.1415926f);
			pitch = std::max(pitch, 0.05f * 3.1415926f);
			player.camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

			glm::mat4x3 frame = player.camera->transform->make_local_to_parent();
			glm::vec3 forward = -frame[2];
			player.camera->transform->position = -glm::length(player.camera->transform->position) * forward; // Camera distance behind player

			return true;
		}
	}

	return false;
}

void PlayMode::walk_pawn(PlayMode::Pawn &pawn, float elapsed) {

	glm::vec3 remain = pawn.pawn_control.move;
	
	///std::cout << remain.x << "," << remain.y << "," << remain.z << "\n";

	//using a for() instead of a while() here so that if walkpoint gets stuck I
	// some awkward case, code will not infinite loop:
	for (uint32_t iter = 0; iter < 10; ++iter) {
		if (remain == glm::vec3(0.0f)) break;
		WalkPoint end;
		float time;
		walkmesh->walk_in_triangle(pawn.at, remain, &end, &time);
		pawn.at = end;
		if (time == 1.0f) {
			//finished within triangle:
			remain = glm::vec3(0.0f);
			break;
		}
		//some step remains:
		remain *= (1.0f - time);
		//try to step over edge:
		glm::quat rotation;
		if (walkmesh->cross_edge(pawn.at, &end, &rotation)) {
			//stepped to a new triangle:
			pawn.at = end;
			//rotate step to follow surface:
			remain = rotation * remain;
		} else {
			//ran into a wall, bounce / slide along it:
			glm::vec3 const &a = walkmesh->vertices[pawn.at.indices.x];
			glm::vec3 const &b = walkmesh->vertices[pawn.at.indices.y];
			glm::vec3 const &c = walkmesh->vertices[pawn.at.indices.z];
			glm::vec3 along = glm::normalize(b-a);
			glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
			glm::vec3 in = glm::cross(normal, along);

			//check how much 'remain' is pointing out of the triangle:
			float d = glm::dot(remain, in);
			if (d < 0.0f) {
				//bounce off of the wall:
				remain += (-1.25f * d) * in;
			} else {
				//if it's just pointing along the edge, bend slightly away from wall:
				remain += 0.01f * d * in;
			}
		}
	}

	if (remain != glm::vec3(0.0f)) {
		std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
	}

	//update player's position to respect walking:
	pawn.transform->position = walkmesh->to_world_point(pawn.at);

	{ //rotates enemy, not player
		if (!pawn.is_player){
			glm::vec3 upDir = walkmesh->to_world_smooth_normal(player.at);
			pawn.transform->rotation = glm::inverse(pawn.default_rotation) *  glm::angleAxis(pawn.pawn_control.rotate, upDir);  
		}
	}

	{ //update player's rotation to respect local (smooth) up-vector:
		glm::quat adjust = glm::rotation(
			pawn.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
			walkmesh->to_world_smooth_normal(pawn.at) //smoothed up vector at walk location
		);
		pawn.transform->rotation = glm::normalize(adjust * pawn.transform->rotation);
	}

	// attacking
	{	
		// this variable is so that we can play the appropriate sound 
		// when the stance changes, and we don't need to worry about 
		// coordinating timers for sounds with animations
		bool stance_changed_in_attack = false;

		uint8_t& stance = pawn.pawn_control.stance;
		float& st = pawn.pawn_control.swingTime;
		if (stance == 0){ // idle
			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 1.0f);
			pawn.arm_transform->rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
			pawn.wrist_transform->rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			if (pawn.pawn_control.attack){
				if (stance != 1){ stance_changed_in_attack = true; }
				stance = 1;
			} else if (pawn.pawn_control.parry){
				if (stance != 4){ stance_changed_in_attack = true; }
				stance = 4;
			}
		} else if (stance == 1 || stance == 3){ // fast downswing, fast upswing, slow upswing
			const float dur = (stance < 3) ? 0.4f : 1.0f; //total time of downswing/upswing
			float rt = st / dur;
			float adt_time = 1 - (1-rt)*(1-rt)*(1-rt) * (2 - (1-rt)*(1-rt)*(1-rt)); // Time scaling, imitates acceleration

			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 1.0f - adt_time * 1.5f);
			pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * adt_time / 2.0f, glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * -adt_time * 1.2f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));

			if (stance == 1){ 
				st += elapsed;
				if (st > dur){
					if (stance != 3){ stance_changed_in_attack = true; }
					stance = 3;
					st = 1.0f; 
				}


			} else {

				st -= elapsed;
				if (st < 0.0f){
					if (stance != 0){ stance_changed_in_attack = true; }
					stance = 0;
				}
			} 
		} else if (stance == 2) {

			const float dur = 0.4f;
			const float delay = 0.4f;
			float dt = pawn.pawn_control.swingHit;
			float rt = (st + delay) / dur * (dt) / (dt + delay);

			float adt_time = 1 - (1-rt)*(1-rt)*(1-rt) * (2 - (1-rt)*(1-rt)*(1-rt)); // Time scaling, imitates acceleration

			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 1.0f - adt_time * 1.5f);
			pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * adt_time / 2.0f, glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * -adt_time * 1.2f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));

			st -= elapsed;
			if (st < -delay){
				stance = 0;
				st = 0.0f;
			}

			{
				clock_t current_time = clock();
				float elapsed = (float)(current_time - previous_sword_whoosh_time);
				if ((elapsed / CLOCKS_PER_SEC) > min_sword_whoosh_interval){
					fast_upswing_sound = Sound::play(*fast_upswing, 1.0f, 0.0f);
					previous_sword_whoosh_time = clock();
				}
			}
		} else if (stance == 4 || stance == 5){ // parry
			const float dur = 0.25f; //total time of down parry, total parry time is thrice due to holding 
			float rt = (st < dur) ? st / dur : 1.0f;
			float adt_time = 1 - (1-rt)*(1-rt)*(1-rt) * (2 - (1-rt)*(1-rt)*(1-rt)); // Time scaling, imitates acceleration

			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 1.0f - adt_time / 2.0f);
			pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * adt_time * 1.5f, glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * adt_time / 1.3f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));

			if (stance == 4){ 
				st += elapsed;
				// Hold parry stance for additional duration
				if (st > 2 * dur){ 
					if (stance != 5){ stance_changed_in_attack = true; }
					stance = 5;
				}
			} else {
				if (st > dur){
					st = dur;
				}
				st -= elapsed;
				if (st < 0.0f){
					if (stance != 0){ stance_changed_in_attack = true; }
					stance = 0;
				}
			}
		}

		if (stance_changed_in_attack){
			// fast downswing
			if (stance == 1) {
				fast_downswing_sound = Sound::play(*fast_downswing, 1.0f, 0.0f);
			}
			// fast upswing
			if (stance == 2) {
				fast_upswing_sound = Sound::play(*fast_upswing, 1.0f, 0.0f);
			}
		}
	}
}

void PlayMode::update(float elapsed) {


	//player walking:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 5.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		if ((left.pressed != right.pressed) || (down.pressed != up.pressed)){
			clock_t current_time = clock();
			float footstep_elapsed = (float)(current_time - previous_footstep_time);
			if ((footstep_elapsed / CLOCKS_PER_SEC) > min_footstep_interval){
				footstep_wconv1_sound = Sound::play(*footstep_wconv1, 1.0f, 0.0f);
				previous_footstep_time = clock();
			}
		}

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//get move in world coordinate system:
		glm::vec3 pmove = player.camera_transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);
		player.pawn_control.move = pmove;

		player.pawn_control.attack = mainAction.pressed; // Attack input control
		player.pawn_control.parry = secondAction.pressed; // Attack input control

		walk_pawn(player, elapsed);	



		enemy.bt->tick();// AI Thinking
		PlayMode::Control& enemy_control=enemy.bt->GetControl();
		//simple enemy that walks toward player
		enemy.pawn_control.move = enemy_control.move*elapsed;
		enemy_control.move=glm::vec3(0,0,0);//like a consumer pattern
		//and locks on player
		enemy.pawn_control.rotate = enemy_control.rotate;

		//Todo:  enemy_control.attack=0;
		//Todo:   enemy_control.parry=0;

		enemy.pawn_control.attack = enemy_control.attack; // mainAction.pressed; // For demonstration purposes bound to player attack
		enemy.pawn_control.parry = enemy_control.parry; //secondAction.pressed; 

		walk_pawn(enemy, elapsed);	

		/*
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
		*/
	}

	// VERY TEMPORARY
	collEng.broadPhase(collEng.cs);

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*player.camera);

	/* In case you are wondering if your walkmesh is lining up with your scene, try:
	{
		glDisable(GL_DEPTH_TEST);
		DrawLines lines(player.camera->make_projection() * glm::mat4(player.camera->transform->make_world_to_local()));
		for (auto const &tri : walkmesh->triangles) {
			lines.draw(walkmesh->vertices[tri.x], walkmesh->vertices[tri.y], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.y], walkmesh->vertices[tri.z], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.z], walkmesh->vertices[tri.x], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
		}
	}
	*/

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion looks; WASD moves; space attack, E blocks, escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion looks; WASD moves; space attack, E blocks, escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();


	// STUFF ADDED STARTS HERE
	std::vector< glm::u8vec4 > enemy_hp_tex_data;
	static GLuint enemy_hp_tex = 0;
	static GLuint enemy_hp_buffer = 0;
	static GLuint enemy_vao = 0;

	for (int i=0; i<25; i++){
		enemy_hp_tex_data.push_back(glm::u8vec4(0xff, 0x00, 0x00, 0x7f));
	}

	if (enemy_hp_tex == 0) {
		glGenTextures(1, &enemy_hp_tex);
		glBindTexture(GL_TEXTURE_2D, enemy_hp_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 5, 5, 0, GL_RGBA, GL_UNSIGNED_BYTE, enemy_hp_tex_data.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (enemy_hp_buffer == 0){
		glGenBuffers(1, &enemy_hp_buffer);
	}

	if (enemy_vao == 0) {
		//based on PPU466.cpp

		glGenVertexArrays(1, &enemy_vao);
		glBindVertexArray(enemy_vao);

		glBindBuffer(GL_ARRAY_BUFFER, enemy_hp_buffer);

		glVertexAttribPointer(
			lit_color_texture_program->Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vert), //stride
			(GLbyte *)0 + offsetof(Vert, position) //offset
		);
		glEnableVertexAttribArray(lit_color_texture_program->Position_vec4);

		glVertexAttribPointer(
			lit_color_texture_program->TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vert), //stride
			(GLbyte *)0 + offsetof(Vert, tex_coord) //offset
		);
		glEnableVertexAttribArray(lit_color_texture_program->TexCoord_vec2);

	}

	glUseProgram(lit_color_texture_program->program);

	// step 2: figure out where in the window to draw the thing
	glm::vec2 enemy_o2wc = object_to_window_coordinate(enemy.body_transform, player.camera, drawable_size);
	float enemy_window_x = ((enemy_o2wc.x / (drawable_size.x/2.0f)) * 2.0f) - 1.0f;
	float enemy_window_y = ((enemy_o2wc.y / (drawable_size.y/2.0f)) * 2.0f) - 1.0f;
	float block_size = 0.1f;
	// sqrm = square making ratio
	float sqrm = ((float)drawable_size.y / (float)drawable_size.x);

	std::vector< Vert > attribs;
	attribs.emplace_back(glm::vec3( enemy_window_x - block_size*sqrm, enemy_window_y - block_size, 0.0f), glm::vec2(0.0f, 0.0f));
	attribs.emplace_back(glm::vec3( enemy_window_x - block_size*sqrm,  enemy_window_y + block_size, 0.0f), glm::vec2(0.0f, 1.0f));
	attribs.emplace_back(glm::vec3( enemy_window_x + block_size*sqrm, enemy_window_y - block_size, 0.0f), glm::vec2(1.0f, 0.0f));
	attribs.emplace_back(glm::vec3( enemy_window_x + block_size*sqrm, enemy_window_y + block_size, 0.0f), glm::vec2(1.0f, 1.0f));

	glBindBuffer(GL_ARRAY_BUFFER, enemy_hp_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * attribs.size(), attribs.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(lit_color_texture_program->program);
	glUniformMatrix4fv(lit_color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glBindTexture(GL_TEXTURE_2D, enemy_hp_tex);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(enemy_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, attribs.size());
	// glBindVertexArray(0);
	glDisable(GL_BLEND);
	// glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();
	// STUFF I ADDED ENDS HERE


	// DRAWING THE HP BAR FROM FILE
	// reference: https://gamedev.stackexchange.com/questions/59078/sdl-function-for-loading-pngs
	// do this later
	// for now: drawing HP bar on top of the screen

	static std::vector< glm::u8vec4 > hpbar_data;
	static glm::uvec2 hpbar_size;
	static std::vector< glm::u8vec4 > hpbar_tex_data;

	// static GLuint enemy_hp_tex = 0;
	static GLuint hpbar_tex = 0;
	// static GLuint enemy_hp_buffer = 0;
	static GLuint hpbar_buffer = 0;
	// static GLuint enemy_vao = 0;
	static GLuint hpbar_vao = 0;

	if (hpbar_data.empty()){
		load_png(data_path("graphics/healthbar2.png"), &hpbar_size, &hpbar_data, OriginLocation::LowerLeftOrigin);
		for (int i=0; i<hpbar_size.y; i++){
			for (int j=0; j<hpbar_size.x; j++){
				// hpbar_tex_data.push_back(hpbar_data.at((i*hpbar_size.x)+j));
				
				// hpbar_tex_data.push_back(glm::vec4(
				// 	hpbar_tex_data.at((i*hpbar_size.x)+j)[0],
				// 	hpbar_tex_data.at((i*hpbar_size.x)+j)[1],
				// 	hpbar_tex_data.at((i*hpbar_size.x)+j)[2],
				// 	0.5f
				// ));
				hpbar_tex_data.push_back(glm::u8vec4(0xff, 0x00, 0x00, 0x7f));

			}
		}
	}else{
		std::cout << hpbar_size.x << " " << hpbar_size.y << std::endl;
	}

	if (hpbar_tex == 0) {
		glGenTextures(1, &hpbar_tex);
		glBindTexture(GL_TEXTURE_2D, hpbar_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hpbar_size.x, hpbar_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, hpbar_tex_data.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (hpbar_buffer == 0){
		glGenBuffers(1, &hpbar_buffer);
	}

	if (hpbar_vao == 0) {
		//based on PPU466.cpp

		glGenVertexArrays(1, &hpbar_vao);
		glBindVertexArray(hpbar_vao);

		glBindBuffer(GL_ARRAY_BUFFER, hpbar_buffer);
		// glVertexAttrib4f(lit_color_texture_program->Color_vec4, 1.0f, 1.0f, 1.0f, 1.0f);
		glVertexAttribPointer(
			lit_color_texture_program->Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vert), //stride
			(GLbyte *)0 + offsetof(Vert, position) //offset
		);
		glEnableVertexAttribArray(lit_color_texture_program->Position_vec4);

		glVertexAttribPointer(
			lit_color_texture_program->TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vert), //stride
			(GLbyte *)0 + offsetof(Vert, tex_coord) //offset
		);
		glEnableVertexAttribArray(lit_color_texture_program->TexCoord_vec2);

	}

	glUseProgram(lit_color_texture_program->program);

	// step 2: where to draw the hp bar
	static glm::vec2 hpbar_bottom_left = glm::vec2(-0.9, 0.6);
	static glm::vec2 hpbar_top_right = glm::vec2(0.9, 0.9);

	std::vector< Vert > hpbar_attribs;
	hpbar_attribs.emplace_back(glm::vec3( hpbar_bottom_left.x, hpbar_bottom_left.y, 0.0f), glm::vec2(0.0f, 0.0f)); // 1
	hpbar_attribs.emplace_back(glm::vec3( hpbar_top_right.x, hpbar_bottom_left.y, 0.0f), glm::vec2(1.0f, 1.0f)); // 4 
	hpbar_attribs.emplace_back(glm::vec3( hpbar_bottom_left.x,  hpbar_top_right.y, 0.0f), glm::vec2(0.0f, 1.0f)); // 2
	hpbar_attribs.emplace_back(glm::vec3( hpbar_top_right.x, hpbar_top_right.y, 0.0f), glm::vec2(1.0f, 0.0f)); // 3
	

	glBindBuffer(GL_ARRAY_BUFFER, hpbar_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * hpbar_attribs.size(), hpbar_attribs.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glUseProgram(lit_color_texture_program->program);
	glVertexAttrib4f(lit_color_texture_program->Color_vec4, 1.0f, 1.0f, 1.0f, 1.0f);
	glUniformMatrix4fv(lit_color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glBindTexture(GL_TEXTURE_2D, hpbar_tex);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(hpbar_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, hpbar_attribs.size());
	// glBindVertexArray(0);
	glDisable(GL_BLEND);
	// glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();




}
