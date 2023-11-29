#include "Game.hpp"

#include "PrintUtil.hpp"

#include <stdexcept>

Game::Creature::~Creature() {}

Game::Game()
{
	for(size_t i = 0; i < Game::MAX_CREATURE_COUNT; i++)
	{
		openCreatureSlots.emplace(i, 0);
		std::get<0>(creatures[i]) = nullptr;
	}
}

Game::~Game()
{
	for(size_t i = 0; i < Game::MAX_CREATURE_COUNT; i++)
	{
		auto c = creatures[i];
		if(std::get<0>(c) != nullptr)
		{
			delete std::get<0>(c);
		}
	}
}

void Game::update(float elapsed)
{
	size_t activeCreatures = Game::MAX_CREATURE_COUNT - openCreatureSlots.size();

	//DEBUGOUT << "Iterating through game creatures to update" << std::endl;
	size_t seenSoFar = 0;
	for(size_t i = 0; i < Game::MAX_CREATURE_COUNT; i++)
	{
		auto c = creatures[i];
		if(std::get<0>(c) == nullptr)
		{
			//DEBUGOUT << "Skipping game update because nullptr" << std::endl;
			continue;
		}
		std::get<0>(c)->update(elapsed);
		//DEBUGOUT << "Updated creature, on " << i << "th creature" << std::endl;
		if(++seenSoFar == activeCreatures)
		{
			//DEBUGOUT << "Done updating game after seeing " << activeCreatures << " creatures" << std::endl;
			return;
		}
	}
}

Game::CreatureID Game::spawnCreature(Game::Creature* c)
{
	if(openCreatureSlots.empty())
	{
		DEBUGOUT << "Tried to spawn a creature, but there were not open slots!" << std::endl;
		return CreatureID(Game::MAX_CREATURE_COUNT, 0);
	}

	CreatureID result = openCreatureSlots.front();
	openCreatureSlots.pop();

	std::get<0>(creatures[result.idx]) = c;
	
	return result;
}

// Bounds checking for these following two functions is probably wasteful, but whatever
bool Game::destroyCreature(Game::CreatureID id)
{
	try
	{
		auto& c = creatures.at(id.idx);
		if(std::get<1>(c)== id.gen)
		{
			delete std::get<0>(c);
			std::get<0>(c) = nullptr;

			DEBUGOUT << "Destroyed creature with id gen " << id.gen << " and with idx " << id.idx << std::endl;

			openCreatureSlots.emplace(id.idx, id.gen + 1);
			return true;
		}
		else
		{
			DEBUGOUT << "Tried to destroy a creature with wrong id gen " << id.gen << " and with idx " << id.idx << std::endl;
			return false;
		}
	}
	catch(std::out_of_range const& e)
	{
		DEBUGOUT << "Tried to destroy a creature with out of range id idx "<< id.idx << std::endl;
		return false;
	}
}

Game::Creature* Game::getCreature(Game::CreatureID id)
{
	try
	{
		auto c = creatures.at(id.idx);
		if(std::get<1>(c) == id.gen)
		{
			return std::get<0>(c);
		}
		else
		{
			DEBUGOUT << "Tried to get a creature with wrong id gen " << id.gen << " and with idx " << id.idx << std::endl;
			return nullptr;
		}
	}
	catch(std::out_of_range const& e)
	{
		DEBUGOUT << "Tried to get a creature with out of range id idx " << id.idx << std::endl;
		return nullptr;
	}
}
