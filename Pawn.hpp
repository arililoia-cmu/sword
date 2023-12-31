#ifndef PAWN_HPP
#define PAWN_HPP

#include "Game.hpp"
#include "Collisions.hpp"
#include <list>

class BehaviorTree;

struct PawnControl
{
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
	uint8_t attack = 0;
	uint8_t parry = 0;
	uint8_t dodge = 0;

	uint8_t stance = 0; // state variable, 0 is idle, 1 is swing forward, 2 is swing backward (bounce), 3 is swing backward (normal), 
	// 4 is down parry, 5 is up parry, 6 is dodge

	float swingHit = 0.0f; // logged on block
	float swingTime = 0.0f;
}; 

struct Pawn : public Game::Creature
{
	Pawn() { recentHitters.clear(); };
	virtual ~Pawn() {};

	void update(float elapsed) override {};
	void render() override {};
		
	WalkPoint at;
	//transform is at pawn's feet 
	Scene::Transform* transform = nullptr;
	//body_transform is at pawn's body
	Scene::Transform* body_transform = nullptr;

	bool is_player = false;
	glm::quat default_rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Describes ``front'' direction with respect to +x
	std::string gameplay_tags="";
	// TODO
	Scene::Transform* arm_transform = nullptr;
	Scene::Transform* wrist_transform = nullptr;
	Scene::Transform* sword_transform = nullptr;

	PawnControl pawn_control;
	//HpBar* hp;

	float hp;
	float maxhp;

	float stamina;
	float maxstamina;
	float staminaRegenRate = 0.0f;

	CollisionEngine::ID swordCollider;
	CollisionEngine::ID bodyCollider;

	float previous_sword_whoosh_time = 0.0f;

	float walkCollRad = 1.0f;

	float swordDamage = 1.0f; // This is how much damage we do to others

	struct RecentHitter
	{
		RecentHitter(float tl, Scene::Transform* tf) : timeLeft(tl), hitter(tf) {};
		
		float timeLeft = 0.0f;
		Scene::Transform* hitter = nullptr;
	};
	
	std::list<RecentHitter> recentHitters;
	float hitInvulnTime = 1.0f; // On a per enemy basis
};

struct Enemy : public Pawn
{
	BehaviorTree* bt;

	bool flagToBreakSword = false;
	int type = 0;
	float previous_sword_clang_time = 0.0f;
};
	
struct Player : public Pawn
{	
	//camera_transform joins body_transform and camera->transform
	Scene::Transform *camera_transform = nullptr;
	//camera is attatched to camera_transform and will be pitched by mouse up/down motion:
	Scene::Camera *camera = nullptr;
	float player_height = 10.0f; // read during setup
};

#endif
