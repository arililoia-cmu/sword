#ifndef GAME_HPP
#define GAME_HPP

#include "Scene.hpp"
#include "WalkMesh.hpp"
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

struct Game
{
	Game();
	~Game();

	// This is the main feature of game, it manages creatures that register with systems
	struct Creature
	{
		virtual void update(float elapsed) = 0; // These are for "fire and forget" creatures
		virtual void render() = 0; // These are for "fire and forget" creatures
		virtual ~Creature() = 0;
	};

	void update(float elapsed);
	
	typedef size_t creature_gen_t;
	static size_t const MAX_CREATURE_COUNT = 128; // Arbitrary
	std::array<std::tuple<Creature*, creature_gen_t>, MAX_CREATURE_COUNT> creatures; // Fixed amount, we have the memory, who cares
	
	// This of cours breaks if we spawn gazillions of entities, but I don't really care
	// The nice thing here is that we can pass CreatureID around and know it refers to a single thing -- if the
	// creature it refers to is destroyed, it still refers to that creature (but obviously that creature is now inaccessible)
	struct CreatureID 
	{
		CreatureID() : idx(MAX_CREATURE_COUNT), gen(0) {};
		CreatureID(size_t i, creature_gen_t g) : idx(i), gen(g) {};
		
		size_t idx; // What slot are we in?
		creature_gen_t gen; // What generation of inhabitant of this slot are we?
	};

	// TODO make this better (only iterate thru updates till the last creature, and add to the open list in priority so that if we
	// prefer to fill slots before the last used creature slot so that we don't have to iterate thru whole list
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
