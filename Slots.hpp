#ifndef SLOTS_HPP
#define SLOTS_HPP

#include <cstddef>
#include <vector>
#include <array>
#include <queue>
#include <tuple>
#include <deque>
#include <ctime>
#include <exception>
#include <iostream>

typedef size_t slots_gen_t;

struct SlotID
{
	SlotID() : idx(0), gen(0) {};
	SlotID(size_t i, slots_gen_t g) : idx(i), gen(g) {};
	
	size_t idx;
	slots_gen_t gen; 
};

template<typename T, slots_gen_t N>
struct Slots
{
	Slots()
		{
			for(size_t i = 0; i < N; i++)
			{
				openSlots.emplace(i, 0);
				std::get<0>(slots[i]) = nullptr;
			}
		}
	~Slots()
		{
			for(size_t i = 0; i < N; i++)
			{
				auto c = slots[i];
				if(std::get<0>(c) != nullptr)
				{
					delete std::get<0>(c);
				}
			}
		}
	
	SlotID spawn(T c)
		{
			if(openSlots.empty())
			{
				return SlotID(N, 0);
			}

			SlotID result = openSlots.front();
			openSlots.pop();

			std::get<0>(slots[result.idx]) = c;
	
			return result;
		}

	bool destroy(SlotID id)
		{
			try
			{
				auto& c = slots.at(id.idx);
				if(std::get<1>(c) == id.gen)
				{
					delete std::get<0>(c);
					std::get<0>(c) = nullptr;

					openSlots.emplace(id.idx, id.gen + 1);
					return true;
				}
				else
				{
					return false;
				}
			}
			catch(std::out_of_range const& e)
			{
				return false;
			}
		}

	T get(SlotID id)
		{
			try
			{
				auto c = slots.at(id.idx);
				if(std::get<1>(c) == id.gen)
				{
					return std::get<0>(c);
				}
				else
				{
					return nullptr;
				}
			}
			catch(std::out_of_range const& e)
			{
				return nullptr;
			}
		}

	std::array<std::tuple<T, slots_gen_t>, N> slots;
	std::queue<SlotID> openSlots; 
};

#endif
