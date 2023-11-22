#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "TextureProgram.hpp"

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

GLuint G_LIT_COLOR_TEXTURE_PROGRAM_VAO = 0;

// Contains all the meshes for the scene
Load<MeshBuffer> G_MESHES(LoadTagDefault,
	[]() -> MeshBuffer const*
	{
		MeshBuffer const* ret = new MeshBuffer(data_path("sword.pnct"));
		// If we add more shader programs, we're going to need to make VAOs for them as well here
		G_LIT_COLOR_TEXTURE_PROGRAM_VAO = ret->make_vao_for_program(lit_color_texture_program->program);
		return ret;
	});

// Stores all the relevant walkmeshes
WalkMesh const* walkmesh = nullptr;
Load<WalkMeshes> G_WALKMESHES(LoadTagDefault,
	[]() -> WalkMeshes const*
	{
		WalkMeshes* ret = new WalkMeshes(data_path("sword.w"));
		// TODO: move this out, this is only relevant for specific scenes
		walkmesh = &ret->lookup("WalkMesh");
		return ret;
	});

// Stores all the relevant collidemeshes
Load<CollideMeshes> G_COLLIDEMESHES(LoadTagDefault,
	[]() -> CollideMeshes const*
	{
		CollideMeshes* ret = new CollideMeshes(data_path("sword.c"));
		return ret;
	});

// Adapted from 2018 code Jim mentioned
GLuint load_texture(std::string const &filename, bool mirror, bool sharp, bool alpha = false)
{
	glm::uvec2 size;
	std::vector< glm::u8vec4 > data;
	load_png(filename, &size, &data, LowerLeftOrigin);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	GLint internalformat = GL_RGB;
	if(alpha)
	{
		internalformat = GL_RGBA8;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	if(sharp)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	if (mirror) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();

	return tex;
}

// TODO move this into an atlas (like meshbuffer or walkmeshes or collidemeshes)
Load<GLuint> grass_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("textures/grass.png"), true, false));
	});

Load<GLuint> tile_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("textures/tile.png"), false, false));
	});

Load<GLuint> rock_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("textures/rock.png"), true, false));
	});

Load<GLuint> path_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("textures/path.png"), false, false));
	});

Load<GLuint> hp_bar_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/healthbar_base.png"), false, true, true));
	});

Load<GLuint> heart_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/enemy-hp.png"), false, true, true));
	});

Load<GLuint> dodge_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/dodge.png"), false, true, true));
	});

Load<GLuint> parry_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/parry.png"), false, true, true));
	});

Load<GLuint> attack_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/attack.png"), false, true, true));
	});

// Here we set up the drawables correctly
// Note that this is basically initializing the "mesh rendering" system
// So other systems that are initialized can follow this same pattern
// Note also that "scene" in this case does not fully describe what we might
// think of as the "scene" for gameplay purposes -- it basically only describes
// the mesh rendering part of it. The rest is set up when playmode is actually
// entered. I guess the idea here is that if we wanted to reload the level we
// already have it in memory here, and then we have a mutable copy of that which
// is used during gameplay in the mode.
Load<Scene> G_SCENE(LoadTagDefault,
	[]() -> Scene const*
	{
		return new Scene(data_path("sword.scene"),
			[&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name)
			{
				Mesh const& mesh = G_MESHES->lookup(mesh_name);

				scene.drawables.emplace_back(transform);
				Scene::Drawable& drawable = scene.drawables.back();

				drawable.pipeline = lit_color_texture_program_pipeline;
				drawable.pipeline.vao = G_LIT_COLOR_TEXTURE_PROGRAM_VAO;
				drawable.pipeline.type = mesh.type;
				drawable.pipeline.start = mesh.start;
				drawable.pipeline.count = mesh.count;

				if(transform->name.length() >= 5 && transform->name.substr(0, 5) == "Plane")
				{
					drawable.pipeline.textures[0].texture = *grass_tex;
				}
				else if(transform->name == "Arena")
				{
					drawable.pipeline.textures[0].texture = *tile_tex;
				}
				else if(transform->name.length() >= 8 && transform->name.substr(0, 8) == "Mountain")
				{
					drawable.pipeline.textures[0].texture = *rock_tex;
				}
				else if(transform->name.length() >= 4 && transform->name.substr(0, 4) == "Path")
				{
					drawable.pipeline.textures[0].texture = *path_tex;
				}
			});
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

// Right now, this makes a proper fully copy of the scene, which is fine, but
// there's no reason to keep the global scene around if we're only using it
// like this. Unsure what to do.
PlayMode::PlayMode() : scene(*G_SCENE)
{
	// Binding transforms
	// TODO: this is a lot of duplicated code, set up something that does this automatically
	// for pawns

	// Create the pawns that we want for this scene
	
	// for(auto& transform : scene.transforms)
	// {
	// 	std::string& name = transform.name;

	// 	// This if chain is inefficent but it probably doesn't matter since this is done once
	// 	if(name.rfind("Player_", 0) == 0)
	// 	{
			
	// 	}
	// 	else if(name.rfind("Enemy_", 0) == 0)
	// 	{
			
	// 	}
	// }

	Game::CreatureID plyr = game.spawnCreature(new Player());
	player = static_cast<Player*>(game.getCreature(plyr));

	std::array<Game::CreatureID, 5> enemiesId;
	for(size_t i = 0; i < enemies.size(); i++)
	{
		enemiesId[i] = game.spawnCreature(new Enemy());
		enemies[i] = static_cast<Enemy*>(game.getCreature(enemiesId[i]));
	}
	
	for(auto& transform : scene.transforms)
	{
		if(transform.name == "Player_Body")
		{
			player->body_transform = &transform;
			player->at = walkmesh->nearest_walk_point(player->body_transform->position + glm::vec3(0.0f, 0.0001f, 0.0f));
			player->player_height = glm::length(player->body_transform->position - walkmesh->to_world_point(player->at));
			player->body_transform->position = glm::vec3(0.0f, 0.0f, player->player_height);
		}
		else if(transform.name == "Player_Sword")
		{
			player->sword_transform = &transform;
		}
		else if(transform.name == "Player_Wrist")
		{
			player->wrist_transform = &transform;
		}
		
		
		for(int i=0;i<5;++i)
		{
			char num[1]={'\0'};
			num[0]='0'+(char)i;
			std::string snum(num);
			if(transform.name=="Enemy_Body"+snum){
				enemies[i]->body_transform = &transform;
				enemies[i]->at = walkmesh->nearest_walk_point(enemies[i]->body_transform->position + glm::vec3(0.0f, 0.0001f, 0.0f));
				float height = glm::length(enemies[i]->body_transform->position - walkmesh->to_world_point(enemies[i]->at));
				enemies[i]->body_transform->position = glm::vec3(0.0f, 0.0f, height);
			}
			if(transform.name=="Enemy_Sword"+snum){
				enemies[i]->sword_transform = &transform;
			}
			if(transform.name=="Enemy_Wrist"+snum){
				enemies[i]->wrist_transform = &transform;
			}
		}
		for(int i=0;i<5;++i){
			char num[5]={'.','0','0','0','\0'};
			num[3]=(char)(i+48+1);
			std::string snum(num);
			if(transform.name=="Enemy_Body"+snum){
				enemies[i]->body_transform = &transform;
				enemies[i]->at = walkmesh->nearest_walk_point(enemies[i]->body_transform->position + glm::vec3(0.0f, 0.0001f, 0.0f));
				float height = glm::length(enemies[i]->body_transform->position - walkmesh->to_world_point(enemies[i]->at));
				enemies[i]->body_transform->position = glm::vec3(0.0f, 0.0f, height);
			}
			if(transform.name=="Enemy_Sword"+snum){
				enemies[i]->sword_transform = &transform;
			}
			if(transform.name=="Enemy_Wrist"+snum){
				enemies[i]->wrist_transform = &transform;
			}
		}
	}

	// Create player transform at player feet and start player walking at nearest walk point:
	// TODO this can also be done per pawn
	scene.transforms.emplace_back();
	player->transform = &scene.transforms.back();
	player->body_transform->parent = player->transform;
	player->transform->position = walkmesh->to_world_point(player->at);
	player->is_player = true;
	player->hp = 100;
	player->maxhp = 100;
	player->stamina = 100;
	player->maxstamina = 100;

	scene.transforms.emplace_back();
	for(int i=0;i<5;++i){
			//same for enemy
	 	scene.transforms.emplace_back();

		enemies[i]->transform = &scene.transforms.back();
		enemies[i]->body_transform->parent = enemies[i]->transform;
		enemies[i]->transform->position = walkmesh->to_world_point(enemies[i]->at);
		enemies[i]->default_rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); //dictates enemy's original rotation wrt +x

		enemies[i]->hp = 100;
		enemies[i]->maxhp = 100;
	}

	for(int i=0;i<5;++i)
	{
		enemies[i]->bt=new BehaviorTree();
		enemies[i]->bt->Init();//AI Initialize
		enemies[i]->bt->SetEnemy(enemies[i]);
		enemies[i]->bt->SetPlayer(player);
	}

	// TODO This should probably be done by setting the camera to match the properties from the blender camera but this is OK
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	player->camera = &scene.cameras.front();
	player->camera->fovy = glm::radians(60.0f);
	player->camera->near = 0.01f;

	//between camera and player
	scene.transforms.emplace_back();
	player->camera_transform = &scene.transforms.back();
	player->camera_transform->parent = player->body_transform;
	player->camera_transform->position = glm::vec3(0.0f, 0.0f, 1.0f);
	player->camera_transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); //dictates camera's original rotation wrt +y (not +x)

	player->camera->transform->parent = player->camera_transform;
	player->camera->transform->position = glm::vec3(0.0f, -6.0f, 0.0f);

	//rotate camera facing direction relative to player direction
	player->camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	
	// TODO this should be done in a phase that adds all the transforms that make a pawn
	// Should be done with the ones before that add feet
	scene.transforms.emplace_back();
	player->arm_transform = &scene.transforms.back();
	player->arm_transform->parent = player->body_transform;
	player->wrist_transform->parent = player->arm_transform;
	scene.transforms.emplace_back();

	// SETTING UP COLLIDERS
	{	// Because some objects reuse the same colliders
		CollideMesh const* playerSwordCollMesh = nullptr;
		CollideMesh const* enemySwordCollMesh = nullptr;
		CollideMesh const* enemyCollMesh = nullptr;
		CollideMesh const* playerCollMesh = nullptr;

		playerSwordCollMesh = &G_COLLIDEMESHES->lookup("PlayerSwordCollMesh");
		enemySwordCollMesh = &G_COLLIDEMESHES->lookup("EnemySwordCollMesh");
		enemyCollMesh = &G_COLLIDEMESHES->lookup("EnemyCollMesh");
		playerCollMesh = &G_COLLIDEMESHES->lookup("PlayerCollMesh");

		// Here we register the pawns with the collision system
		for(int i=0;i<5;++i)
		{
			scene.transforms.emplace_back();
			enemies[i]->arm_transform = &scene.transforms.back();
			enemies[i]->arm_transform->parent = enemies[i]->body_transform;
			enemies[i]->wrist_transform->parent = enemies[i]->arm_transform;

			auto enemyISwordHit = [this, i](Game::CreatureID c, Scene::Transform* t) -> void
				{
					if(t == player->sword_transform)
					{
						if(player->pawn_control.stance == 4)
						{
							if(enemies[i]->pawn_control.stance == 1)
							{
								enemies[i]->pawn_control.stance = 2;
								enemies[i]->pawn_control.swingHit = enemies[i]->pawn_control.swingTime;
							}
							else if(enemies[i]->pawn_control.stance == 9)
							{
								enemies[i]->pawn_control.stance = 10;
								enemies[i]->pawn_control.swingHit = enemies[i]->pawn_control.swingTime;
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
					}
				};

			auto enemyIHit = [this, i](Game::CreatureID c, Scene::Transform* t) -> void
				{
					if(t == player->sword_transform)
					{
						enemies[i]->hp -= 1;
						//enemies[i]->hp->change_enemy_hp = true;
						std::cout << "ENEMY " << i << " HIT WITH SWORD" << std::endl;
						// exit(0);
					}
				};

			collEng.registerCollider(enemiesId[i], enemies[i]->sword_transform, enemySwordCollMesh, enemySwordCollMesh->containingRadius, enemyISwordHit, CollisionEngine::Layer::ENEMY_SWORD_LAYER);
			collEng.registerCollider(enemiesId[i], enemies[i]->body_transform, enemyCollMesh, enemyCollMesh->containingRadius, enemyIHit, CollisionEngine::Layer::ENEMY_BODY_LAYER);
		}
	
		auto playerSwordHit = [this](Game::CreatureID c, Scene::Transform* t) -> void
			{
				for(int i = 0; i < 5; i++)
				{
					if(t == enemies[i]->sword_transform)
					{
						if(player->pawn_control.stance == 1)
						{
							player->pawn_control.stance = 2;
							player->pawn_control.swingHit = player->pawn_control.swingTime;
						}
						else if(player->pawn_control.stance == 9)
						{
							player->pawn_control.stance = 10;
							player->pawn_control.swingHit = player->pawn_control.swingTime;
						}
						
						// sound stuff starts here:
						clock_t current_time = clock();
						float elapsed = (float)(current_time - previous_player_sword_clang_time);

						if ((elapsed / CLOCKS_PER_SEC) > min_player_sword_clang_interval){
							w_conv1_sound = Sound::play(*w_conv1, 1.0f, 0.0f);
							previous_player_sword_clang_time = clock();
						}
						// sound stuff ends here

						return;
					}
				}
			};


		auto playerHit = [this](Game::CreatureID c, Scene::Transform* t) -> void
			{		
				for(int i = 0; i < 5; i++){
					if(t == enemies[i]->sword_transform){
						player->hp -= 1;
						//change_player_hp = true;
					}
				}
			};
	
		collEng.registerCollider(plyr, player->sword_transform, playerSwordCollMesh, playerSwordCollMesh->containingRadius, playerSwordHit, CollisionEngine::Layer::PLAYER_SWORD_LAYER);
		collEng.registerCollider(plyr, player->body_transform, playerCollMesh, playerCollMesh->containingRadius, playerHit, CollisionEngine::Layer::PLAYER_BODY_LAYER);
	}

	// SETTING UP UI BARS
	{
		auto playerHpBarCalculate = [this, plyr](float elapsed) -> float
			{
				Pawn* p = static_cast<Pawn*>(game.getCreature(plyr));
				if(p)
				{
					return (float)p->hp / (float)p->maxhp;
				}
				return 0.0f;
			};
		auto* playerHpBar = new Gui::Bar(playerHpBarCalculate, *hp_bar_tex);
		playerHpBar->screenPos = glm::vec3(0.0f, 0.8f, 0.0f);
		playerHpBar->scale = glm::vec2(0.9f, 0.1f);
		playerHpBar->alpha = 0.5f;
		playerHpBar->fullColor = glm::vec3(0.0f, 1.0f, 0.0f);
		playerHpBar->emptyColor = glm::vec3(1.0f, 0.0f, 0.0f);
		gui.addElement(playerHpBar);

		auto playerStamBarCalculate = [this, plyr](float elapsed) -> float
			{
				Pawn* p = static_cast<Pawn*>(game.getCreature(plyr));
				if(p)
				{
					return (float)p->stamina / (float)p->maxstamina;
				}
				return 0.0f;
			};
		auto* playerStamBar = new Gui::Bar(playerStamBarCalculate, *hp_bar_tex);
		playerStamBar->screenPos = glm::vec3(0.0f, 0.6f, 0.0f);
		playerStamBar->scale = glm::vec2(0.9f, 0.1f);
		playerStamBar->alpha = 0.5f;
		playerStamBar->fullColor = glm::vec3(1.0f, 0.0f, 1.0f);
		playerStamBar->emptyColor = glm::vec3(0.0f, 1.0f, 1.0f);
		gui.addElement(playerStamBar);

		for(size_t i = 0; i < enemiesId.size(); i++)
		{
			auto enemyId = enemiesId[i];
			auto enemyHpBarCalculate = [this, enemyId](float elapsed) -> float
				{
					Pawn* p = static_cast<Pawn*>(game.getCreature(enemyId));
					if(p)
					{
						return (float)p->hp / (float)p->maxhp;
					}
					return 0.0f;
				};

			auto* enemyHpBar = new Gui::Bar(enemyHpBarCalculate, *heart_tex);
			enemyHpBar->scale = glm::vec2(0.08f, 0.08f);
			enemyHpBar->alpha = 0.5f;
			enemyHpBar->fullColor = glm::vec3(0.0f, 1.0f, 0.0f);
			enemyHpBar->emptyColor = glm::vec3(1.0f, 0.0f, 0.0f);
		
			enemyHpBars[i] = gui.addElement(enemyHpBar);
		}
	}

	// SETTING UP POPUPS
	auto* dodgeGraphic = new Gui::Popup(*dodge_tex, 
		glm::vec2(-0.1, -0.4), glm::vec2(0.1,-0.3), 500.0f
	);
	dodge_popup_ID = gui.addElement(dodgeGraphic);

	auto* parryGraphic = new Gui::Popup(*parry_tex, 
		glm::vec2(-0.1, -0.4), glm::vec2(0.1,-0.3), 800.0f
	);
	parry_popup_ID = gui.addElement(parryGraphic);

	auto* attackGraphic = new Gui::Popup(*attack_tex, 
		glm::vec2(-0.1, -0.4), glm::vec2(0.1,-0.3), 800.0f
	);
	attack_popup_ID = gui.addElement(attackGraphic);

	DEBUGOUT << "end of loading scene" << std::endl;
}

PlayMode::~PlayMode()
{
	
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
			dodge.downs += 1;
			dodge.pressed = true;
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
			dodge.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			secondAction.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {

			if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
				SDL_SetRelativeMouseMode(SDL_TRUE);
			}
		//std::cout<<"sssssssssssssssssssssssssssssssssssss"<<std::endl;
			if(evt.button.button==SDL_BUTTON_LEFT){
				mainAction.downs += 1;
				mainAction.pressed = true;
				return true;
			}else if(evt.button.button==SDL_BUTTON_RIGHT){
				secondAction.downs += 1;
				secondAction.pressed = true;
				return true;
			}


	}else if(evt.type==SDL_MOUSEBUTTONUP){
			if(evt.button.button==SDL_BUTTON_LEFT){
				mainAction.pressed = false;
				return true;
			}else if(evt.button.button==SDL_BUTTON_RIGHT){
				secondAction.pressed = false;
				return true;
			}
	}
	
	else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			
			//deciding attacking direction
			if(abs(motion.x)>abs(motion.y)){
				isMouseVertical=false;
			}else{
				isMouseVertical=true;
			}

			glm::vec3 upDir = walkmesh->to_world_smooth_normal(player->at);
			player->transform->rotation = glm::angleAxis(-motion.x * player->camera->fovy, upDir) * player->transform->rotation;

			float pitch = glm::pitch(player->camera->transform->rotation);
			pitch += motion.y * player->camera->fovy;
			//camera looks down -z (basically at the player's feet) when pitch is at zero.
			pitch = std::min(pitch, 0.95f * 3.1415926f);
			pitch = std::max(pitch, 0.05f * 3.1415926f);
			player->camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

			glm::mat4x3 frame = player->camera->transform->make_local_to_parent();
			glm::vec3 forward = -frame[2];
			float camera_length = 6.0f;
			if (pitch > 0.5f * 3.1415926f) {
				camera_length = std::min(camera_length, 0.9f * (player->player_height + player->camera_transform->position.z) / sin(pitch - 0.5f * 3.1415926f));
			}
			player->camera->transform->position = - camera_length * forward; // -glm::length(player.camera->transform->position) * forward; // Camera distance behind player

			return true;
		}
	}

	return false;
}

void PlayMode::processPawnControl(Pawn& pawn, float elapsed)
{	
	// Control& control = pawn.pawn_control;

	glm::vec3 movement = glm::vec3(0.0f);

	{	
		// this variable is so that we can play the appropriate sound 
		// when the stance changes, and we don't need to worry about 
		// coordinating timers for sounds with animations
		bool stance_changed_in_attack = false;

		uint8_t& stance = pawn.pawn_control.stance;
		float& st = pawn.pawn_control.swingTime;

		float moveLen2 = glm::length2(pawn.pawn_control.move);
		if(stance != 6 && stance != 1)
		{
			static auto accelInterpolate = [](float x) -> float
				{
					return 0.5f + cos(x * M_PI_2f) * 0.5f;
				};
			
			if(moveLen2 > 0.001f)
			{
				float moveKept = accelInterpolate(glm::dot(pawn.pawn_control.vel, pawn.pawn_control.move) / moveLen2);
			
				pawn.pawn_control.vel = pawn.pawn_control.move * moveKept;
			}
		}
		if(moveLen2 <= 0.001f)
		{
			pawn.pawn_control.vel = pawn.pawn_control.vel * 0.5f;
		}

		movement = pawn.pawn_control.vel * elapsed;
		
		if (stance == 0)
		{ // idle
		//	std::cout<<"EEEEEEEEEEEEEEEEE"<<std::endl;

			//pawn.pawn_control.vel -= glm::normalize(pawn.pawn_control.vel) * glm::length2(pawn.pawn_control.vel);
			
			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 0.5f);
			pawn.arm_transform->rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
			pawn.wrist_transform->rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			if(pawn.pawn_control.attack)
			{
				if (stance != 1){ stance_changed_in_attack = true; }
				pawn.pawn_control.stanceInfo.attack.dir = pawn.pawn_control.move;
				pawn.gameplay_tags="attack";

				if(pawn.pawn_control.attack==1){// Here you can change whether the pawn is casting vertical(stance=1) or horizontal(stance=9)
					stance = 1;
					pawn.pawn_control.attack=0;
				}
				if(pawn.pawn_control.attack==2){
					stance = 9;
				//	std::cout<<"DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"<<std::endl;
					pawn.pawn_control.attack=0;
				}


				pawn.pawn_control.stanceInfo.attack.attackAfter = 0;
			}
			else if (pawn.pawn_control.parry)
			{
				if(stance != 4){ stance_changed_in_attack = true; }
				pawn.gameplay_tags="parry";
				stance = 4;
				pawn.pawn_control.parry=0;
			}
			else if(pawn.pawn_control.dodge)
			{
				if(glm::length2(pawn.pawn_control.move) > 0.001f)
				{
					pawn.gameplay_tags = "dodge";
					stance = 6;
					pawn.pawn_control.stanceInfo.dodge.dir = glm::normalize(pawn.pawn_control.move);
					pawn.pawn_control.stanceInfo.dodge.attackAfter = 0;
				}
				
				pawn.pawn_control.dodge = 0;
			}
		} else if (stance == 1 || stance == 3){ // fast downswing, fast upswing, slow upswing
			const float dur = (stance < 3) ? 0.4f : 1.0f; //total time of downswing/upswing

			static auto interpolateWeapon = [](float x) -> float
				{
					return (float)(1.0f - 1.0f / (1.0f + pow((2.0f * x) / (1.0f - x), 3)));
				};

			float rt = st / dur;
			float adt_time = interpolateWeapon(rt); //1 - (1-rt)*(1-rt)*(1-rt) * (2 - (1-rt)*(1-rt)*(1-rt)); // Time scaling, imitates acceleration


			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 0.5f - adt_time * 1.5f);
			pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * adt_time / 2.0f, glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * -adt_time * 1.2f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));

			if (stance == 1)
			{
				static auto interpolate = [](float x) -> float
					{
						float b = 0.5f;
						float a = 0.35f;
						if(x <= a)
						{
							return b / a * x;
						}
						else
						{
							return 1.0f - (1.0f - b) * powf((1.0f - x) / (1.0f - a), (b * (1.0f - a)) / (a * (1.0f - b)));
						}
					};
			
				// 0 to 1
				float before = interpolate(st / dur);
				
				st += elapsed;
				if(st >= dur)
				{
					st = dur;
				}

				float after = interpolate(st / dur);
				float amount = after - before;
				movement = pawn.pawn_control.stanceInfo.attack.dir * amount * dur;

				if(pawn.pawn_control.attack)
				{
					if(st > 0.2f)
					{
						pawn.pawn_control.stanceInfo.attack.attackAfter = 1;
					}
					pawn.pawn_control.attack = 0;
				}

				if(st == dur)
				{
					if (stance != 3){ stance_changed_in_attack = true; }
					stance = 3;
					st = 1.0f; 
				}

			} else {
			if(stance==3){
				pawn.gameplay_tags="";//clear gameplay tag for AI
			//	std::cout<<"ddddddddddddddddddddddddddddddddddddd"<<std::endl;
			}
			if(pawn.pawn_control.attack)
				{
					pawn.pawn_control.stanceInfo.attack.attackAfter =0;// 1;  Pearson Comment: don't actually need input buffer in attacking.
					pawn.pawn_control.attack = 0;
				}
			
				st -= elapsed;
				if (st < 0.0f){
					if (stance != 0){ stance_changed_in_attack = true; }
					st = 0.0f;
					if(pawn.pawn_control.stanceInfo.attack.attackAfter)
					{
					//	stance = 9;
						pawn.pawn_control.stanceInfo.sweep.dir = pawn.pawn_control.stanceInfo.attack.dir;
					}
					else
					{
						stance = 0;
					}
				}
			} 
		} else if (stance == 2) {

			if(stance==2){
				pawn.gameplay_tags="";//clear gameplay tag for AI
			}
			const float dur = 0.4f;
			const float delay = 0.4f;
			float dt = pawn.pawn_control.swingHit;
			float rt = (st + delay) / dur * (dt) / (dt + delay);

			float adt_time = 1 - (1-rt)*(1-rt)*(1-rt) * (2 - (1-rt)*(1-rt)*(1-rt)); // Time scaling, imitates acceleration

			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 0.5f - adt_time * 0.8f);
			pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * adt_time / 2.0f, glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * -adt_time * 1.2f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));

			st -= elapsed;
			if (st < -delay){
				stance = 0;
				st = 0.0f;
			}

			{
				clock_t current_time = clock();
				elapsed = (float)(current_time - previous_sword_whoosh_time);
				if ((elapsed / CLOCKS_PER_SEC) > min_sword_whoosh_interval){
					fast_upswing_sound = Sound::play(*fast_upswing, 1.0f, 0.0f);
					previous_sword_whoosh_time = clock();
				}
			}
		} else if (stance == 4 || stance == 5){ // parry
			if(stance==5){
				pawn.gameplay_tags="";//clear gameplay tag for AI
			//	std::cout<<"eeeaaa"<<std::endl;
			}
			const float dur = 0.4f; //total time of down parry, total parry time is thrice due to holding 
			float rt = (st < dur) ? st / dur : 1.0f;
			float adt_time = 1 - (1-rt)*(1-rt)*(1-rt) * (2 - (1-rt)*(1-rt)*(1-rt)); // Time scaling, imitates acceleration

			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 0.5f - adt_time / 4.0f);
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
		else if(stance == 6) // DODGE ROLL (implemented as attack since it similarly precludes you from taking any action and modifies your transform)
		{
			const float dur = 0.20f;
			const float distance = 3.0f;

			static auto interpolate = [](float x) -> float
				{
					return (float)(1.0f - 1.0f / (1.0f + pow((2.0f * x) / (1.0f - x), 3)));
				};
			
			// 0 to 1
			float before = interpolate(st / dur);

			st += elapsed;
			if(st >= dur)
			{
				st = dur;
			}

			float after = interpolate(st / dur);
				
			float amount = after - before;

			movement = pawn.pawn_control.stanceInfo.dodge.dir * distance * amount;

			if(pawn.pawn_control.attack)
			{
				pawn.pawn_control.stanceInfo.dodge.attackAfter = 1;
				pawn.pawn_control.attack = 0;
			}

			if(st == dur)
			{
				st = 0.0f;
				if(pawn.pawn_control.stanceInfo.dodge.attackAfter)
				{
					pawn.pawn_control.stanceInfo.lunge.dir = pawn.pawn_control.stanceInfo.dodge.dir;
					stance = 7;
				}
				else
				{
					stance = 0;
				}
			}
		}
		else if(stance == 7 || stance == 8)
		{
			// Lunge attack
			const float dur = (stance < 8) ? 0.4f : 0.4f; //total time of downswing/upswing
			const float distance = 1.5f;

			static auto interpolateWeapon = [](float x) -> float
				{
					return (float)( 1.0f - 1.0f / (1.0f + pow((2.0f * x) / (1.0f - x), 3)));
				};
			
			static auto interpolate = [](float x) -> float
				{
					return (float)(1.0f - 1.0f / (1.0f + pow((1.0f * x) / (1.0f - x), 3)));
				};
			
			float rt = st / dur;
			float adt_time = interpolateWeapon(rt);
			
			pawn.arm_transform->position = glm::vec3(0.0f - adt_time * 1.5f, -adt_time * 0.4f, 0.5f);
			//pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * interpolate(st / dur) / 2.0f, glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * -adt_time * 1.1f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));

			if(stance == 7)
			{
				// 0 to 1
				float before = interpolate(st / dur);
				
				st += elapsed;
				if(st >= dur)
				{
					st = dur;
				}

				float after = interpolate(st / dur);
				float amount = after - before;
				movement = pawn.pawn_control.stanceInfo.lunge.dir * amount * distance;

				if(st == dur)
				{
					if (stance != 8){ stance_changed_in_attack = true; }
					stance = 8;
					st = 0.4f; 
				}

			} else {
			if(stance==8){
				pawn.gameplay_tags="";//clear gameplay tag for AI
			//	std::cout<<"ddddddddddddddddddddddddddddddddddddd"<<std::endl;
			}
				st -= elapsed;
				if (st < 0.0f){
					if (stance != 0){ stance_changed_in_attack = true; }
					stance = 0;
				}
			} 
		}
		else if(stance == 9 || stance == 10)
		{
			// Sweep left attack
			const float dur = (stance < 8) ? 0.5f : 1.0f; //total time of downswing/upswing
			const float distance = 0.5f;

			static auto interpolateWeapon = [](float x) -> float
				{
					return (float)(1.0f - 1.0f / (1.0f + pow((1.0f * x) / (1.0f - x), 3)));
				};
			static auto interpolateWeaponFast = [](float x) -> float
				{
					return (float)(1.0f - 1.0f / (1.0f + pow((2.0f * x) / (1.0f - x), 3)));
				};
			
			static auto interpolate = [](float x) -> float
				{
					return (float)(1.0f - 1.0f / (1.0f + pow((0.7f * x) / (1.0f - x), 3)));
				};
			
			float rt = st / dur;
			float adt_time = interpolateWeapon(rt);
			
			pawn.arm_transform->position = glm::vec3(0.0f, -adt_time * 0.4f, 0.5f);
			pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * adt_time * 2.0f, glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * -interpolateWeaponFast(rt) * 1.0f, glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)));

			if(stance == 9)
			{
				// 0 to 1
				float before = interpolate(st / dur);
				
				st += elapsed;
				if(st >= dur)
				{
					st = dur;
				}

				float after = interpolate(st / dur);
				float amount = after - before;
				movement = pawn.pawn_control.stanceInfo.sweep.dir * amount * distance;

				if(st == dur)
				{
					if (stance != 10){ stance_changed_in_attack = true; }
					stance = 10;
					st = 1.0f; 
				}

			} else {
			if(stance==10){
				pawn.gameplay_tags="";//clear gameplay tag for AI

			}
				st -= elapsed;
				if (st < 0.0f){
										st = 0.0f;

					if (stance != 0){ stance_changed_in_attack = true; }
					stance = 0;
					//				std::cout<<"JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ"<<std::endl;
					//				int temp;
					//				std::cin>>temp;
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

	walk_pawn(pawn, movement);
}

void PlayMode::walk_pawn(Pawn &pawn, glm::vec3 movement)
{
	glm::vec3 remain = movement;

	///std::cout << remain.x << "," << remain.y << "," << remain.z << "\n";

	//using a for() instead of a while() here so that if walkpoint gets stuck I
	// some awkward case, code will not infinite loop:
	for(uint32_t iter = 0; iter < 10; ++iter)
	{
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
			glm::vec3 upDir = walkmesh->to_world_smooth_normal(player->at);
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
}

void PlayMode::update(float elapsed)
{
	// Handle the input we've received this update
	// We don't put this in player's own update since we need to get the input
	// which is the mode's job
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 5.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if(left.pressed && !right.pressed) move.x =-1.0f;
		if(!left.pressed && right.pressed) move.x = 1.0f;
		if(down.pressed && !up.pressed) move.y =-1.0f;
		if(!down.pressed && up.pressed) move.y = 1.0f;

		if((left.pressed != right.pressed) || (down.pressed != up.pressed))
		{
			clock_t current_time = clock();
			float footstep_elapsed = (float)(current_time - previous_footstep_time);
			if ((footstep_elapsed / CLOCKS_PER_SEC) > min_footstep_interval){
				footstep_wconv1_sound = Sound::play(*footstep_wconv1, 1.0f, 0.0f);
				previous_footstep_time = clock();
			}
		}
		
		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed;

		//get move in world coordinate system:
		glm::vec3 pmove = player->camera_transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);
		
		player->pawn_control.move = pmove;
		
		if(mainAction.pressed){
			if(isMouseVertical){
				player->pawn_control.attack=1;
			}else{
				player->pawn_control.attack=2;
			}
			mainAction.pressed=false;
		}

		//player.pawn_control.attack = mainAction.pressed; // Attack input control
		player->pawn_control.parry = secondAction.pressed; // Parry input control
		player->pawn_control.dodge = dodge.pressed;

		auto trigger_popups = [this](int downs, Gui::GuiID popup_ID) -> void
		{	
			if (downs > 0){
				Gui::Element* elem = gui.getElement(popup_ID);
				if (elem){
					Gui::Popup* grabbed = static_cast<Gui::Popup*>(elem);
					grabbed->trigger_render();
				}	
			}
		};

		trigger_popups(dodge.downs, dodge_popup_ID);
		trigger_popups(secondAction.downs, parry_popup_ID);
		trigger_popups(mainAction.downs, attack_popup_ID);
	
		//reset button press counters:
		left.downs = 0;
		right.downs = 0;
		up.downs = 0;
		down.downs = 0;
		dodge.downs = 0;
		mainAction.downs = 0;
		secondAction.downs = 0;
		
		processPawnControl(*player, elapsed);

		for(int i=0;i<5;++i)
		{
			enemies[i]->bt->tick();// AI Thinking
			PawnControl& enemy_control=enemies[i]->bt->GetControl();
			enemies[i]->pawn_control.move = enemy_control.move;
			enemy_control.move=glm::vec3(0,0,0);//like a consumer pattern
			enemies[i]->pawn_control.rotate = enemy_control.rotate;
			enemies[i]->pawn_control.attack = enemy_control.attack; // mainAction.pressed; // For demonstration purposes bound to player attack
			enemies[i]->pawn_control.parry = enemy_control.parry; //secondAction.pressed; 
			enemy_control.attack=0;
			enemy_control.parry=0;
			
			processPawnControl(*enemies[i], elapsed);	
		}
	}

	// Updates the systems
	collEng.update(elapsed);
	game.update(elapsed);
	gui.update(elapsed);
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	player->camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	// glUseProgram(0);

	glClearColor(140.0f / 256.0f, 190.0f / 256.0f, 250.0f / 256.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	glm::mat4 world_to_clip; // Just want to reuse this lol
	scene.draw(*player->camera, world_to_clip);

	// Positioning enemy HP bars (yes I know we can do this much more efficiently on the GPU)
	for(size_t i = 0; i < enemyHpBars.size(); i++)
	{
		Gui::Element* elem = gui.getElement(enemyHpBars[i]);
		if(elem)
		{
			Gui::Bar* bar = static_cast<Gui::Bar*>(elem);
			glm::vec4 clipPos = world_to_clip * glm::mat4(enemies[i]->body_transform->make_local_to_world()) * glm::vec4(0.0f, 0.0f, 2.0f, 1.0f);

			// Cursed ass cpu clipping LMAO
			bar->screenPos = glm::vec3(clipPos.x / clipPos.w, clipPos.y / clipPos.w, (clipPos.w > 0) ? 0.0f : 1.0f);
		}
	}
	
	gui.render(world_to_clip);

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
}
