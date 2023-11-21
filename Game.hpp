#ifndef GAME_HPP
#define GAME_HPP

#include "Scene.hpp"
#include "WalkMesh.hpp"
#include "Collisions.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <queue>
#include <tuple>
#include <deque>
#include <ctime>
#include <exception>
#include <iostream>

class BehaviorTree;

struct Game
{
	Game();

	// This is the main feature of game, it manages creatures that register with systems
	class Creature
	{
		virtual ~Creature() = 0;
	};
	
	typedef size_t creature_gen_t;
	static size_t const MAX_CREATURE_COUNT = 128; // Arbitrary
	std::array<std::tuple<Creature*, creature_gen_t>, MAX_CREATURE_COUNT> creatures; // Fixed amount, we have the memory, who cares
	
	// This of cours breaks if we spawn gazillions of entities, but I don't really care
	struct CreatureID // Wraps the creature index so it's easier to identify creatures (can't get lucky and accidentally delete wrong one)
	{
		CreatureID(size_t i, creature_gen_t g) : idx(i), gen(g) {};
		
		size_t idx; // What slot are we in?
		creature_gen_t gen; // What generation of inhabitant of this slot are we?
	};
	
	std::queue<CreatureID> openCreatureSlots; // What slots are open in the creature list? Queue so don't reuse same slots too much

	// Returns an id of MAX_CREATURE_COUNT if fails to spawn, can check this before using
	// It's on you to create the creature, but we'll delete it... we don't know what you want yet!
	// so use this function like spawnCreature(new whatever);
	CreatureID spawnCreature(Creature* c);

	// Returns true if successfully destroyed, returns falls if doesn't exist (ie gen is wrong!)
	bool destroyCreature(CreatureID id);

	// Gets creature, checking id to make sure gen is correct (ensures this is the correct creature, and it hasn't been destroyed!)
	// Returns nullptr if this creature doesn't exist
	Creature* getCreature(CreatureID id);
};

#endif
