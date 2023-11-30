#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "PrintUtil.hpp"
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
		walkmesh = &ret->lookup("WalkMesh");
		// TODO: line 43 results in a segfault on Mac M1, line
		// 44 throws an error on other computers. change this so that 
		// it checks if either walkmesh or walkmesh.021 exists?
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

Load<GLuint> roll_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/roll.png"), false, true, true));
	});

Load<GLuint> slice_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/slice.png"), false, true, true));
	});

Load<GLuint> titlecard_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/titlecard.png"), false, true, true));
	});

Load<GLuint> blood_graphic_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/blood_1.png"), false, true, true));
	});

Load<GLuint> game_over_tex(LoadTagDefault,
	[]()
	{
		return new GLuint(load_texture(data_path("graphics/game_over.png"), false, true, true));
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
				if(transform->name.length() >= 5 && transform->name.substr(0, 5) == "Enemy")
				{
					return;
				}
				
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

// type should be 0 1 or 2
void PlayMode::setupEnemy(Game::CreatureID myEnemyID, glm::vec3 pos, float maxhp, int type)
{
	Enemy* enemy = static_cast<Enemy*>(game.getCreature(myEnemyID));
	
	// Assume enemy has literally nothing set up right now
	scene.transforms.emplace_back();
	enemy->transform = &scene.transforms.back();
	
	scene.transforms.emplace_back();
	enemy->body_transform = &scene.transforms.back();
	*(enemy->body_transform) = enemyPresets[type].body_transform;
	enemy->body_transform->parent = enemy->transform;

	scene.transforms.emplace_back();
	enemy->arm_transform = &scene.transforms.back();
	enemy->arm_transform->parent = enemy->body_transform;

	scene.transforms.emplace_back();
	enemy->wrist_transform = &scene.transforms.back();
	*(enemy->wrist_transform) = enemyPresets[type].wrist_transform;
	enemy->wrist_transform->parent = enemy->arm_transform;

	scene.transforms.emplace_back();
	enemy->sword_transform = &scene.transforms.back();
	*(enemy->sword_transform) = enemyPresets[type].sword_transform;
	enemy->sword_transform->parent = enemy->wrist_transform;

	// Customize
	enemy->hp = maxhp;
	enemy->maxhp = maxhp;
	enemy->stamina = 100.0f;
	enemy->maxstamina = 100.0f;
	enemy->staminaRegenRate = 10.0f;
	enemy->is_player = false;
	enemy->type = type;

	enemy->swordDamage = 7.5f;
	enemy->walkCollRad = 1.0f;
	enemy->hitInvulnTime = 0.3f;

	enemy->bt=new BehaviorTree();
	enemy->bt->Init();//AI Initialize
	enemy->bt->SetEnemy(enemy);
	enemy->bt->SetPlayer(player);
	enemy->bt->SetEnemyType(type); // Customize
	enemy->bt->InitInterrupt();

	enemy->body_transform->position = pos; // CUSTOMIZE
	
	enemy->at = walkmesh->nearest_walk_point(enemy->body_transform->position + glm::vec3(0.0f, 0.0001f, 0.0f));
	glm::vec3 worldPoint = walkmesh->to_world_point(enemy->at);
	float height = 1.4f;
	enemy->body_transform->position = glm::vec3(0.0f, 0.0f, height);
	enemy->transform->position = worldPoint;
	enemy->default_rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); //dictates enemy's original rotation wrt +x

	auto enemySwordHit = [this, myEnemyID](Game::CreatureID c, Scene::Transform* t) -> void
		{
			if(t == player->sword_transform)
			{
				if(player->pawn_control.stance == 4)
				{
					Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(myEnemyID));

					if(!enemyPtr)
					{
						DEBUGOUT << "Enemy no longer exists on enemySwordHit!" << std::endl;
						return;
					}
							
					if(enemyPtr->pawn_control.stance == 1)
					{
						enemyPtr->pawn_control.stance = 2;
						enemyPtr->pawn_control.swingHit = enemyPtr->pawn_control.swingTime;

						DEBUGOUT << "Player parried enemy, flagging to swap enemy to broken sword, enemy was stance 1!" << std::endl;
						enemyPtr->flagToBreakSword = true;// Can't just switch since collisions still be processed and don't want to deregister
						// This code is really blegh cuz it's hidden that dergistering doesn't work while collisions being processed
					}
					else if(enemyPtr->pawn_control.stance == 9)
					{
						enemyPtr->pawn_control.stance = 10;
						enemyPtr->pawn_control.swingHit = enemyPtr->pawn_control.swingTime;

						DEBUGOUT << "Player parried enemy, flagging to swap  enemy to broken sword, enemy was stance 9!" << std::endl;
						enemyPtr->flagToBreakSword = true;// Can't just switch since collisions still be processed and don't want to deregister
						// This code is really blegh cuz it's hidden that dergistering doesn't work while collisions being processed
					}

					clock_t current_time = clock();
					float elapsed = (float)(current_time - previous_enemy_sword_clang_time);

					if ((elapsed / CLOCKS_PER_SEC) > min_enemy_sword_clang_interval){
						w_conv2_sound = Sound::play(*w_conv2, 1.0f, 0.0f);
						previous_enemy_sword_clang_time = clock();
					}
				}
			}
		};
	auto enemyHit = [this, myEnemyID](Game::CreatureID c, Scene::Transform* t) -> void
		{
			if(t == player->sword_transform)
			{
				Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(myEnemyID));
						
				if(!enemyPtr)
				{
					DEBUGOUT << "Enemy no longer exists in enemyISwordHit!" << std::endl;
					return;
				}


				if(player->pawn_control.stance == 1 || player->pawn_control.stance == 7 || player->pawn_control.stance == 9)
				{
					if(std::find_if(enemyPtr->recentHitters.begin(), enemyPtr->recentHitters.end(),
									 [this](Pawn::RecentHitter& rh) -> bool { return rh.hitter == player->sword_transform; }) == enemyPtr->recentHitters.end())
					{
						enemyPtr->hp -= player->swordDamage;
						enemyPtr->recentHitters.push_front(Pawn::RecentHitter(enemyPtr->hitInvulnTime, player->sword_transform));
						DEBUGOUT << "ENEMY HIT WITH SWORD while player was in stance " << player->pawn_control.stance << std::endl;
					}
				}
			}
		};

	enemy->swordCollider = collEng.registerCollider(myEnemyID, enemy->sword_transform, enemySwordCollMesh, enemySwordCollMesh->containingRadius, enemySwordHit, CollisionEngine::Layer::ENEMY_SWORD_LAYER);
	enemy->bodyCollider = collEng.registerCollider(myEnemyID, enemy->body_transform, enemyCollMesh, enemyCollMesh->containingRadius, enemyHit, CollisionEngine::Layer::ENEMY_BODY_LAYER);

	auto enemyHpBarCalculate = [this, myEnemyID](float elapsed) -> float
		{
			Pawn* p = static_cast<Pawn*>(game.getCreature(myEnemyID));
			if(p)
			{
				return (float)p->hp / (float)p->maxhp;
			}
			return 0.0f;
		};

	auto* enemyHpBar = new Gui::Bar(enemyHpBarCalculate, *heart_tex);
	enemyHpBar->scale = glm::vec2(0.05f, 0.08f);
	enemyHpBar->alpha = 0.5f;
	enemyHpBar->fullColor = glm::vec3(0.0f, 1.0f, 0.0f);
	enemyHpBar->emptyColor = glm::vec3(1.0f, 0.0f, 0.0f);
	enemyHpBar->useCreatureID = true;
	enemyHpBar->creatureIDForPos = myEnemyID;
			
	enemyHpBars.push_back(gui.addElement(enemyHpBar));

	auto addDrawable = [this](Scene::Transform* tform, std::string mesh_name) -> void
		{
			Mesh const& mesh = G_MESHES->lookup(mesh_name);
	
			scene.drawables.emplace_back(tform);
			Scene::Drawable& drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;
			drawable.pipeline.vao = G_LIT_COLOR_TEXTURE_PROGRAM_VAO;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
		};

	addDrawable(enemy->body_transform, "Player" + enemyPresets[type].postfix);
	addDrawable(enemy->wrist_transform, "Wrist" + enemyPresets[type].postfix);
	addDrawable(enemy->sword_transform, "Sword" + enemyPresets[type].postfix);
}

void PlayMode::swapEnemyToBrokenSword(Game::CreatureID myEnemyID)
{
	Enemy* enemy = static_cast<Enemy*>(game.getCreature(myEnemyID));

	if(!enemy)
	{
		return;
	}
	
	auto pertainsToEnemySword = [enemy](Scene::Drawable& d) -> bool
		{
			if(d.transform == enemy->sword_transform)
			{
				return true;
			}
			return false;
		};
	scene.drawables.remove_if(pertainsToEnemySword);
	collEng.unregisterCollider(enemy->swordCollider);

	auto addDrawable = [this](Scene::Transform* tform, std::string mesh_name) -> void
		{
			Mesh const& mesh = G_MESHES->lookup(mesh_name);
	
			scene.drawables.emplace_back(tform);
			Scene::Drawable& drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;
			drawable.pipeline.vao = G_LIT_COLOR_TEXTURE_PROGRAM_VAO;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
		};

	//addDrawable(enemy->sword_transform, "Player" + enemyPresets[enemy->type].postfix); // SWAP ME
	 addDrawable(enemy->sword_transform, "Sword_Broken" + enemyPresets[enemy->type].postfix); 

	auto enemySwordHit = [this, myEnemyID](Game::CreatureID c, Scene::Transform* t) -> void
		{
			if(t == player->sword_transform)
			{
				if(player->pawn_control.stance == 4)
				{
					Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(myEnemyID));

					if(!enemyPtr)
					{
						DEBUGOUT << "Enemy no longer exists on enemySwordHit!" << std::endl;
						return;
					}
							
					if(enemyPtr->pawn_control.stance == 1)
					{
						enemyPtr->pawn_control.stance = 2;
						enemyPtr->pawn_control.swingHit = enemyPtr->pawn_control.swingTime;
					}
					else if(enemyPtr->pawn_control.stance == 9)
					{
						enemyPtr->pawn_control.stance = 10;
						enemyPtr->pawn_control.swingHit = enemyPtr->pawn_control.swingTime;
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
	
	enemy->swordCollider = collEng.registerCollider(myEnemyID, enemy->sword_transform, enemySwordBrokenCollMesh, enemySwordBrokenCollMesh->containingRadius, enemySwordHit, CollisionEngine::Layer::ENEMY_SWORD_LAYER);
}

// Right now, this makes a proper fully copy of the scene, which is fine, but
// there's no reason to keep the global scene around if we're only using it
// like this. Unsure what to do.
PlayMode::PlayMode() : scene(*G_SCENE)
{
	for(size_t i = 0; i < enemyPresets.size(); i++)
	{
		enemyPresets[i].postfix = ".00" + std::to_string(i + 1);
	}
	
	plyr = game.spawnCreature(new Player());
	player = static_cast<Player*>(game.getCreature(plyr));
	
	auto tformit = scene.transforms.begin();
	while(tformit != scene.transforms.end())
	{
		Scene::Transform& transform = *tformit;
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
		else if(transform.name.length() >= 5 && transform.name.substr(0, 5) == "Enemy")
		{
			// Yes, we are adding them and then removing them again, no it doesn't matter this is startup cost
			for(size_t i = 0; i < enemyPresets.size(); i++)
			{
				if(transform.name == ("Enemy_Body" + enemyPresets[i].postfix))
				{
					enemyPresets[i].body_transform = transform;
				}
				else if(transform.name == ("Enemy_Sword" + enemyPresets[i].postfix))
				{
					enemyPresets[i].sword_transform = transform;
				}
				else if(transform.name == ("Enemy_Wrist" + enemyPresets[i].postfix))
				{
					enemyPresets[i].wrist_transform = transform;
				}
			}
			auto toDestroyit = tformit++;
			scene.transforms.erase(toDestroyit);
			continue;
		}
		tformit++;
	}

	// Create player transform at player feet and start player walking at nearest walk point:
	// TODO this can also be done per pawn
	scene.transforms.emplace_back();
	player->transform = &scene.transforms.back();
	player->body_transform->parent = player->transform;
	player->transform->position = walkmesh->to_world_point(player->at);
	player->is_player = true;
	player->hp = 100.0f;
	player->maxhp = 100.0f;
	player->stamina = 100.0f;
	player->maxstamina = 100.0f;
	player->staminaRegenRate = 10.0f;

	player->walkCollRad = 1.0f;
	player->swordDamage = 34.0f;
	player->hitInvulnTime = 0.3f;

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
	// scene.transforms.emplace_back();

	// SETTING UP COLLIDERS
	{	// Because some objects reuse the same colliders
		playerSwordCollMesh = &G_COLLIDEMESHES->lookup("PlayerSwordCollMesh");
		enemySwordCollMesh = &G_COLLIDEMESHES->lookup("EnemySwordCollMesh");
		enemySwordBrokenCollMesh = &G_COLLIDEMESHES->lookup("EnemySwordBrokenCollMesh");
		// enemySwordBrokenCollMesh = &G_COLLIDEMESHES->lookup("EnemyCollMesh"); // Replace when broken sword is ready
		enemyCollMesh = &G_COLLIDEMESHES->lookup("EnemyCollMesh");
		playerCollMesh = &G_COLLIDEMESHES->lookup("PlayerCollMesh");
	
		auto playerSwordHit = [this](Game::CreatureID c, Scene::Transform* t) -> void
			{
				for(Game::CreatureID enemyID : enemiesId)
				{
					Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(enemyID));
					
					if(t == enemyPtr->sword_transform)
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
				for(Game::CreatureID enemyID : enemiesId)
				{
					Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(enemyID));
					
					if(t == enemyPtr->sword_transform &&
					   (std::find_if(player->recentHitters.begin(), player->recentHitters.end(),
									 [enemyPtr](Pawn::RecentHitter& rh) -> bool { return rh.hitter == enemyPtr->sword_transform; }) == player->recentHitters.end()))
					{
						if(enemyPtr->pawn_control.stance == 1 || enemyPtr->pawn_control.stance == 7 || enemyPtr->pawn_control.stance == 9)
						{
							// Could add damage based on stance (best done by actually having a table inside each pawn that says
							// how much damage it does in each stance).
							// Generalize stances to moves?
							player->hp -= enemyPtr->swordDamage;
							player->recentHitters.push_back(Pawn::RecentHitter(player->hitInvulnTime, enemyPtr->sword_transform));
							DEBUGOUT << "Player hit with sword while enemy was in stance " << enemyPtr->pawn_control.stance << std::endl;
						}
					}
				}
			};
	
		player->swordCollider = collEng.registerCollider(plyr, player->sword_transform, playerSwordCollMesh, playerSwordCollMesh->containingRadius, playerSwordHit, CollisionEngine::Layer::PLAYER_SWORD_LAYER);
		player->bodyCollider = collEng.registerCollider(plyr, player->body_transform, playerCollMesh, playerCollMesh->containingRadius, playerHit, CollisionEngine::Layer::PLAYER_BODY_LAYER);
	}

	// SETTING UP UI BARS
	{
		auto playerHpBarCalculate = [this](float elapsed) -> float
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

		auto playerStamBarCalculate = [this](float elapsed) -> float
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
	}

	for(size_t i = 0; i < 5; i++)
	{
		enemiesId.push_back(game.spawnCreature(new Enemy()));
		if(i==0){
			setupEnemy(enemiesId.back(), glm::vec3(80.0f, 5.0f * i - 10.0f, 0.001f), 100.0f, 1);
		}
		if(i==1){
			setupEnemy(enemiesId.back(), glm::vec3(60.0f, 5.0f * i - 10.0f, 0.001f), 100.0f, 2);
		}
		if(i==2){
			setupEnemy(enemiesId.back(), glm::vec3(0.0f, 5.0f * i - 10.0f, 0.001f), 100.0f, 0);
		}
		if(i==3){
			setupEnemy(enemiesId.back(), glm::vec3(40.0f, 5.0f * i - 10.0f, 0.001f), 100.0f, 1);
		}
		if(i==4){
			setupEnemy(enemiesId.back(), glm::vec3(20.0f, 5.0f * i - 10.0f, 0.001f), 100.0f, 2);
		}
		//setupEnemy(enemiesId.back(), glm::vec3(i*20.0f, 5.0f * i - 10.0f, 0.001f), 100.0f, i % 3);
	}

	// SETTING UP POPUPS
	auto graphic_setup = [this](Load<GLuint> move_graphic_tex, const std::vector<int>& corresponding_stances){
		auto* moveGraphic = new Gui::MoveGraphic(*move_graphic_tex);
		Gui::GuiID move_popup_ID = gui.addElement(moveGraphic);
		for (int i=0; i< (int) corresponding_stances.size(); i++){
			stanceGuiIDMap[corresponding_stances[i]] = move_popup_ID;
		}
	};

	// register how stances correspond with which textures will be shown
	graphic_setup(dodge_tex, {6});
	graphic_setup(parry_tex, {4,5});
	graphic_setup(attack_tex, {1,3});
	graphic_setup(roll_tex, {7});
	graphic_setup(slice_tex, {9});

	auto* titleGraphic = new Gui::Popup(*titlecard_tex, 10000000.0f, true);
	titlecard_ID = gui.addElement(titleGraphic);

	auto* gameOverGraphic = new Gui::Popup(*game_over_tex, std::numeric_limits<float>::max(), false);
	gameOver_ID = gui.addElement(gameOverGraphic);

	auto* bloodGraphic = new Gui::Popup(*blood_graphic_tex, 20.0f, false);
	bloodGraphic_ID = gui.addElement(bloodGraphic);

		// for (int i=0; i< (int) corresponding_stances.size(); i++){
		// 	stanceGuiIDMap[corresponding_stances[i]] = move_popup_ID;
		// }
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

			// remove title card
			Gui::Element* elem = gui.getElement(titlecard_ID);
			if (elem){
				Gui::Popup* grabbed = static_cast<Gui::Popup*>(elem);
				if (grabbed->get_visible()){
					grabbed->trigger_visibility(false);
					return true;
				}
			}

			if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
				SDL_SetRelativeMouseMode(SDL_TRUE);
			}

			if(evt.button.button==SDL_BUTTON_LEFT){
				mainAction.downs += 1;
				mainAction.pressed = true;
				isMouseVertical=true;
				return true;
			}else if(evt.button.button==SDL_BUTTON_RIGHT){
				//secondAction.downs += 1;
				//secondAction.pressed = true;
				mainAction.downs+=1;
				mainAction.pressed=true;
				isMouseVertical=false;
				return true;
			}


	}else if(evt.type==SDL_MOUSEBUTTONUP){
			if(evt.button.button==SDL_BUTTON_LEFT){
				mainAction.pressed = false;
				return true;
			}else if(evt.button.button==SDL_BUTTON_RIGHT){
				//secondAction.pressed = false;
				mainAction.pressed=false;
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
		/*	if(abs(motion.x)>abs(motion.y)){
				isMouseVertical=false;
			}else{
				isMouseVertical=true;
			}*/

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
			/*
			if (pitch > 0.5f * 3.1415926f) {
				camera_length = std::min(camera_length, 0.9f * (player->player_height + player->camera_transform->position.z) / sin(pitch - 0.5f * 3.1415926f));
			}
			*/
			player->camera->transform->position = - camera_length * forward; // -glm::length(player.camera->transform->position) * forward; // Camera distance behind player
			// glm::vec3 original = player->camera->transform->position;

			glm::vec3 wld = player->camera->transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); 
			glm::vec3 proj = walkmesh->to_world_point(walkmesh->nearest_walk_point(wld));

			float adj_height = (wld.z < proj.z) ? proj.z + 0.1f : wld.z;
			glm::vec4 newworld = glm::vec4(proj.x, proj.y, adj_height, 1.0f);

			player->camera->transform->position = glm::vec3(0.0f);
			glm::vec3 newlocal = player->camera_transform->make_world_to_local() * newworld; 
			//std::cout << newlocal.x << " " << newlocal.y << " " << newlocal.z << "\n\n";

			player->camera->transform->position = newlocal;

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
				if(pawn.is_player && pawn.stamina <= 10.0f)
				{
					DEBUGOUT << "Player doesn't have enough stamina to attack" << std::endl;
				}
				else
				{
					if (stance != 1){ stance_changed_in_attack = true; }
					pawn.pawn_control.stanceInfo.attack.dir = pawn.pawn_control.move;
					pawn.gameplay_tags="attack";

					if(pawn.pawn_control.attack==1){// Here you can change whether the pawn is casting vertical(stance=1) or horizontal(stance=9)
						stance = 1;
						pawn.stamina -= 10.0f;
						pawn.pawn_control.attack=0;
					}
					if(pawn.pawn_control.attack==2){
						stance = 9;
						pawn.stamina -= 10.0f;
						pawn.pawn_control.attack=0;
					}

					pawn.pawn_control.stanceInfo.attack.attackAfter = 0;
				}
			}
			else if (pawn.pawn_control.parry)
			{
				if(pawn.is_player && pawn.stamina <= 20.0f)
				{
					DEBUGOUT << "Player doesn't have enough stamina to parry" << std::endl;
				}
				else
				{
					if(stance != 4){ stance_changed_in_attack = true; }
					pawn.gameplay_tags="parry";
					stance = 4;
					pawn.pawn_control.parry=0;

					pawn.stamina -= 20.0f;
				}
			}
			else if(pawn.pawn_control.dodge)
			{
				if(pawn.is_player && pawn.stamina <= 15.0f)
				{
					DEBUGOUT << "Player doesn't have enough stamina to dodge" << std::endl;
				}
				else
				{
					if(glm::length2(pawn.pawn_control.move) > 0.001f)
					{
						pawn.gameplay_tags = "dodge";
						stance = 6;
						pawn.pawn_control.stanceInfo.dodge.dir = glm::normalize(pawn.pawn_control.move);
						pawn.pawn_control.stanceInfo.dodge.attackAfter = 0;

						pawn.stamina -= 15.0f;
					}
				
					pawn.pawn_control.dodge = 0;
				}
			}
		} else if (stance == 1 || stance == 3){ // fast downswing, fast upswing, slow upswing
			const float dur = (stance < 3) ? 0.4f : 1.0f; //total time of downswing/upswing

			static auto interpolateWeapon = [](float x) -> float
				{
					return (float)(1.0f - 1.0f / (1.0f + pow((2.0f * x) / (1.0f - x), 3)));
				};

			float rt = st / dur;
			float adt_time = interpolateWeapon(rt); //1 - (1-rt)*(1-rt)*(1-rt) * (2 - (1-rt)*(1-rt)*(1-rt)); // Time scaling, imitates acceleration


			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 0.5f - adt_time * 0.8f);
			pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * adt_time / 2.0f, glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * -adt_time * 1.0f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));

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

			pawn.arm_transform->position = glm::vec3(0.0f, 0.0f, 0.5f - adt_time * 0.6f);
			pawn.arm_transform->rotation = glm::angleAxis(M_PI_2f * adt_time / 2.0f, glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
			pawn.wrist_transform->rotation = glm::angleAxis(M_PI_2f * -adt_time * 1.0f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));

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
			const float dur = 0.50f;
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

			movement = pawn.pawn_control.stanceInfo.dodge.dir * (distance * amount + 0.5f * PlayerSpeed * elapsed);

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
					if(pawn.is_player && pawn.stamina <= 10.0f)
					{
						DEBUGOUT << "Player doesn't have enough stamina to lunge" << std::endl;
						stance = 0;
					}
					else
					{
						pawn.pawn_control.stanceInfo.lunge.dir = pawn.pawn_control.stanceInfo.dodge.dir;
						stance = 7;

						pawn.stamina -= 10.0f;
					}
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

void PlayMode::walk_pawn(Pawn& pawn, glm::vec3 movement)
{
	glm::vec3 remain = movement;

	for(Game::CreatureID enemyID : enemiesId)
	{
		Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(enemyID));
		if(enemyPtr && (enemyPtr != &pawn))
		{
			glm::vec3 toOther = enemyPtr->transform->position - pawn.transform->position;
			float toOtherLength = glm::length(toOther);
			if(toOtherLength < (enemyPtr->walkCollRad + pawn.walkCollRad))
			{
				float remainLength = glm::length(remain);
				if(remainLength * toOtherLength > 0.0001f)
				{
					remain = remain - remain * glm::dot(toOther / toOtherLength, remain / remainLength);
				}
			}
		}
	}
	if(&pawn != player)
	{
		glm::vec3 toOther = player->transform->position - pawn.transform->position;
		float toOtherLength = glm::length(toOther);
		if(toOtherLength < (player->walkCollRad + pawn.walkCollRad))
		{
			float remainLength = glm::length(remain);
			if(remainLength * toOtherLength > 0.0001f)
			{
				remain = remain - remain * glm::dot(toOther / toOtherLength, remain / remainLength);
			}
		}
	}
	
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
	// Clearing 0 HP enemies
	{
		auto enemyIDit = enemiesId.begin();
		while(enemyIDit != enemiesId.end())
		{
			Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(*enemyIDit));

			if(!enemyPtr)
			{
				DEBUGOUT << "Trying to check enemy hps to see if they need to be deleted, but an enemy didn't eixst!" << std::endl;
				enemyIDit++;
			}
			else
			{
				auto pertainsToEnemy = [enemyPtr](Scene::Drawable& d) -> bool
					{
						if(d.transform == enemyPtr->body_transform || d.transform == enemyPtr->sword_transform || d.transform == enemyPtr->wrist_transform)
						{
							return true;
						}
						return false;
					};

				auto pertainsToEnemyTForm = [enemyPtr](Scene::Transform& d) -> bool
					{
						if(&d == enemyPtr->body_transform ||
						   &d == enemyPtr->transform ||
						   &d == enemyPtr->arm_transform ||
						   &d == enemyPtr->wrist_transform ||
						   &d == enemyPtr->sword_transform)
						{
							return true;
						}
						return false;
					};
			
				if(enemyPtr->hp <= 0.0f)
				{
					DEBUGOUT << "Started deleting enemy" << std::endl;
					
					collEng.unregisterCollider(enemyPtr->bodyCollider); 
					collEng.unregisterCollider(enemyPtr->swordCollider);

					DEBUGOUT << "Deleting enemy, colliders deregistered" << std::endl;
					
					scene.drawables.remove_if(pertainsToEnemy);

					DEBUGOUT << "Deleting enemy, drawables removed" << std::endl;

					// This is ridiculously inefficient since we have pointers already, but we never stored iterators, and I don't want to add it rn
					scene.transforms.remove_if(pertainsToEnemyTForm); // Whatever, we will just leave transforms allocated, who cares
					delete enemyPtr->bt;

					DEBUGOUT << "Deleting enemy, bt deleted" << std::endl;
					
					game.destroyCreature(*enemyIDit); // Automatically calls delete (yes I know bad design but we don't have time to fix)

					DEBUGOUT << "Deleting enemy, creature destroyed" << std::endl;
					
					auto toDestroyit = enemyIDit++;
					enemiesId.erase(toDestroyit);

					DEBUGOUT << "Finished deleting enemy" << std::endl;
				}
				else
				{
					enemyIDit++;
				}
			}
		}
	}
	
	// Proccessing previous sword breaks
	{
		for(Game::CreatureID myEnemyID : enemiesId)
		{
			Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(myEnemyID));
			if(enemyPtr->flagToBreakSword == true)
			{
				swapEnemyToBrokenSword(myEnemyID);
				enemyPtr->swordDamage = 3.5f;
				enemyPtr->flagToBreakSword = false;
			}
		}
	}

	// Also processing invuln timer decrements
	{
		for(Game::CreatureID myEnemyID : enemiesId)
		{
			Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(myEnemyID));
			for(auto it = enemyPtr->recentHitters.begin(); it != enemyPtr->recentHitters.end();)
			{
				if(it->timeLeft <= 0.0f)
				{
					it = enemyPtr->recentHitters.erase(it);
				}
				else
				{
					it->timeLeft -= elapsed;
					it++;
				}
			}
		}

		for(auto it = player->recentHitters.begin(); it != player->recentHitters.end();)
		{
			if(it->timeLeft <= 0.0f)
			{
				it = player->recentHitters.erase(it);
			}
			else
			{
				it->timeLeft -= elapsed;
				it++;
			}
		}
	}

	// Replenishing player stamina
	{
		for(Game::CreatureID myEnemyID : enemiesId)
		{
			Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(myEnemyID));

					
			enemyPtr->stamina += enemyPtr->staminaRegenRate * elapsed;
			if(enemyPtr->stamina >= enemyPtr->maxstamina)
			{
				enemyPtr->stamina = enemyPtr->maxstamina;
			}
		}
		
		player->stamina += player->staminaRegenRate * elapsed;
		if(player->stamina >= player->maxstamina)
		{
			player->stamina = player->maxstamina;
		}
	}
	
	// Handle the input we've received this update
	// We don't put this in player's own update since we need to get the input
	// which is the mode's job
	{
		//combine inputs into a move:
		
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

		//reset button press counters:
		left.downs = 0;
		right.downs = 0;
		up.downs = 0;
		down.downs = 0;
		dodge.downs = 0;
		mainAction.downs = 0;
		secondAction.downs = 0;

		auto trigger_move_graphic = [this](int prev_stance, int new_stance){

			if (stanceGuiIDMap.find(prev_stance) != stanceGuiIDMap.end()){
				Gui::Element* elem = gui.getElement(stanceGuiIDMap[prev_stance]);
				if (elem){
					Gui::MoveGraphic* grabbed = static_cast<Gui::MoveGraphic*>(elem);
					grabbed->trigger_render(false);
				}	
			}

			if (stanceGuiIDMap.find(new_stance) != stanceGuiIDMap.end()){
				Gui::Element* elem = gui.getElement(stanceGuiIDMap[new_stance]);
				if (elem){
					Gui::MoveGraphic* grabbed = static_cast<Gui::MoveGraphic*>(elem);
					grabbed->trigger_render(true);
				}	
			}
		};
		
		static int prev_stance = player->pawn_control.stance;
		processPawnControl(*player, elapsed);
		if (player->pawn_control.stance != prev_stance){
			trigger_move_graphic(prev_stance, player->pawn_control.stance);
		}
		prev_stance = player->pawn_control.stance;

		for(Game::CreatureID enemyID : enemiesId)
		{
			Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(enemyID));
			if(!enemyPtr)
			{
				DEBUGOUT << "Trying to iterate enemy control, but an enemy didn't exist..." << std::endl;
				continue;
			}
			enemyPtr->bt->tick();// AI Thinking
			PawnControl& enemy_control=enemyPtr->bt->GetControl();
			enemyPtr->pawn_control.move = enemy_control.move;
			enemy_control.move=glm::vec3(0,0,0);//like a consumer pattern
			enemyPtr->pawn_control.rotate = enemy_control.rotate;
			enemyPtr->pawn_control.attack = enemy_control.attack; // mainAction.pressed; // For demonstration purposes bound to player attack
			enemyPtr->pawn_control.parry = enemy_control.parry; //secondAction.pressed; 
			enemy_control.attack=0;
			enemy_control.parry=0;
			
			processPawnControl(*enemyPtr, elapsed);	
		}
	}

	Pawn* p = static_cast<Pawn*>(game.getCreature(plyr));
	if(p)
	{
		if (p->hp <= 0.0f){
			Gui::Element* elem = gui.getElement(gameOver_ID);
			if (elem){
				Gui::Popup* grabbed = static_cast<Gui::Popup*>(elem);
				if (!grabbed->get_visible()){
					grabbed->trigger_visibility(true);
					Sound::stop_all_samples();
				}
			}

		}
	}


	// Updates the systems
	collEng.update(elapsed);
	//DEBUGOUT << "Finished collision update" << std::endl;
	game.update(elapsed);
	//DEBUGOUT << "Finished game update" << std::endl;
	gui.update(elapsed);
	//DEBUGOUT << "Finished gui update" << std::endl;
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

	//DEBUGOUT << "Started drawing gui bars" << std::endl;
	
	// Positioning enemy HP bars (yes I know we can do this much more efficiently on the GPU)
	auto enemyHpBarit = enemyHpBars.begin();
	while(enemyHpBarit != enemyHpBars.end())
	{	
		Gui::GuiID enemyHpBar = *enemyHpBarit;
		Gui::Element* elem = gui.getElement(enemyHpBar);
		if(elem)
		{	
			Gui::Bar* bar = static_cast<Gui::Bar*>(elem);

			if(bar->useCreatureID)
			{
				Enemy* enemyPtr = static_cast<Enemy*>(game.getCreature(bar->creatureIDForPos));
				if(!enemyPtr)
				{
					DEBUGOUT << "Tried to set position for hp bar, and it exists, but it's enemy for positioning doesn't, deleting" << std::endl;
					gui.removeElement(enemyHpBar);
					auto hpBarToDeleteit = enemyHpBarit++;
					enemyHpBars.erase(hpBarToDeleteit);
					continue;
				}
				else
				{
					enemyHpBarit++;
				}
				glm::vec4 clipPos = world_to_clip * glm::mat4(enemyPtr->body_transform->make_local_to_world()) * glm::vec4(0.0f, 0.0f, 2.0f, 1.0f);

				// Cursed ass cpu clipping LMAO
				bar->screenPos = glm::vec3(clipPos.x / clipPos.w, clipPos.y / clipPos.w, (clipPos.w > 0) ? 0.0f : 1.0f);
			}
		}
		else
		{
			enemyHpBarit++;
			DEBUGOUT << "Enemy hp bar doesn't exist, but it's still in the list!" << std::endl;
		}
	}
	
	gui.render(world_to_clip);

	//DEBUGOUT << "Finished drawing gui" << std::endl;

	GL_ERRORS();
}
