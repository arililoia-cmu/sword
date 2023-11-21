#include "Game.hpp"

#include "PrintUtil.hpp"

#include <stdexcept>

Game::Game()
{
	for(size_t i = 0; i < Game::MAX_CREATURE_COUNT; i++)
	{
		openCreatureSlots.emplace(i, 0);
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
		if(std::get<1>(creatures.at(id.idx)) == id.gen)
		{
			// TODO creature destruction cleanup code!
			DEBUGOUT << "TODO creature destruction cleanup code!" << std::endl;

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
		DEBUGOUT << "Tried to destroy a creature with out of range id idx " << id.idx << std::endl;
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
