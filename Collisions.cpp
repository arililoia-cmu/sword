#include "Collisions.hpp"

#include "read_write_chunk.hpp"

#include <limits>
#include <iostream>
#include <fstream>
#include <algorithm>

// Code in this file is very "stupid code" and should be refactored after
// prototype phase

// For example, we only support convex meshes. Obviously we can represent
// more primitive things like boxes and stuff as convex meshes but we can
// also handle them more efficiently if we don't generalize.
// Breaking concave meshes into convex meshes is hard and I'm not gonna do it
// here, we can do that in blender or something.

// I used https://www.youtube.com/watch?v=ajv46BSqcK4 to help visualize this algorithm
bool GJK(Collider& a, Collider& b)
{
	glm::mat4x3 altw = a.transform->make_local_to_world();
	glm::mat4x3 bltw = b.transform->make_local_to_world();

	glm::vec3 d = glm::normalize(bltw * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) - altw * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	
	glm::mat4x3 awtl = a.transform->make_world_to_local();
	glm::mat4x3 bwtl = b.transform->make_world_to_local();

	glm::vec3 da = awtl * glm::vec4(d, 0.0f);
	glm::vec3 db = bwtl * glm::vec4(-d, 0.0f);

	glm::vec3 simplex[4] = {};

	uint8_t simpType = 1;
	simplex[0] = (altw * glm::vec4(a.farthest(da), 1.0f)) - (bltw * glm::vec4(b.farthest(db), 1.0f));

	d = glm::vec3(0.0f) - simplex[0];

	while(true)
	{
		da = awtl * glm::vec4(d, 0.0f);
		db = bwtl * glm::vec4(-d, 0.0f);
		
		glm::vec3 next = (altw * glm::vec4(a.farthest(da), 1.0f)) - (bltw * glm::vec4(b.farthest(db), 1.0f));

		if(glm::dot(next, d) < 0)
		{
			return false;
		}

		simplex[simpType] = next;
		
		if(simpType == 1)
		{
			// Line case, we assume origin not on line
			simpType = 2;

			glm::vec3 B = simplex[0];
			glm::vec3 A = simplex[1];

			glm::vec3 AB = B - A;
			glm::vec3 AO = glm::vec3(0.0f) - A;

			d = glm::normalize(glm::cross(AB, glm::cross(AO, AB)));

			continue;
		}
		else if(simpType == 2)
		{
			// Triangle case, we assume origin not on triangle
			simpType = 3;

			glm::vec3 C = simplex[0];
			glm::vec3 B = simplex[1];
			glm::vec3 A = simplex[2];

			glm::vec3 AB = B - A;
			glm::vec3 AC = C - A;
			glm::vec3 AO = glm::vec3(0.0f) - A;

			d = glm::normalize(glm::cross(AC, glm::cross(AB, glm::vec3(0.0f))));
			if(glm::dot(d, AO) < 0)
			{
				d *= -1.0f;
			}
			
			continue;
		}
		else
		{
			// This is the actual tetra case, where we can contain the origin

			glm::vec3 AD = simplex[3] - simplex[0];
			glm::vec3 AC = simplex[3] - simplex[1];
			glm::vec3 AB = simplex[3] - simplex[2];
			glm::vec3 AO = simplex[3] - glm::vec3(0.0f);

			glm::vec3 triABC = glm::cross(AB, AC);
			if(glm::dot(triABC, AD) > 0) { triABC = -triABC; }

			glm::vec3 triABD = glm::cross(AB, AD);
			if(glm::dot(triABD, AC) > 0) { triABD = -triABD; }

			glm::vec3 triACD = glm::cross(AC, AD);
			if(glm::dot(triACD, AB) > 0) { triACD = -triACD; }

			if(glm::dot(triACD, AO) < 0)
			{
				simplex[2] = simplex[3];
				d = glm::normalize(triACD);
			}
			else if(glm::dot(triABD, AO) < 0)
			{
				simplex[1] = simplex[2];
				simplex[2] = simplex[3];
				d = glm::normalize(triABD);
			}
			else if(glm::dot(triABC, AO) < 0)
			{
				simplex[0] = simplex[1];
				simplex[1] = simplex[2];
				simplex[2] = simplex[3];
				d = glm::normalize(triABC);
			}
			else
			{
				return true;
			}
		}
	}
}

CollideMesh::CollideMesh(std::vector<glm::vec3> const& vertices_,  float cr)
	: vertices(vertices_), containingRadius(cr) {}

CollideMeshes::CollideMeshes(std::string const& filename)
{
	std::ifstream file(filename, std::ios::binary);

	std::vector<glm::vec3> vertices;
	read_chunk(file, "p...", &vertices);

	std::vector<char> names;
	read_chunk(file, "str0", &names);

	// TODO: SEE THE BELOW TODO ABOUT MAKING THE SCRIPT WORK
	// std::vector<float> containingRads;
	// read_chunk(file, "rad0", &containingRads);

	struct IndexEntry
	{
		uint32_t name_begin, name_end;
		uint32_t vertex_begin, vertex_end;
		//uint32_t containingRadsAt;
	};

	std::vector<IndexEntry> index;
	read_chunk(file, "idxA", &index);

	if(file.peek() != EOF)
	{
		std::cerr << "WARNING: trailing data in collidemesh file '" << filename << "'" << std::endl;
	}

	//-----------------

	for(auto const& e : index)
	{
		if(!(e.name_begin <= e.name_end && e.name_end <= names.size()))
		{
			throw std::runtime_error("Invalid name indices in index of '" + filename + "'");
		}
		if(!(e.vertex_begin <= e.vertex_end && e.vertex_end <= vertices.size()))
		{
			throw std::runtime_error("Invalid vertex indices in index of '" + filename + "'");
		}
		
		//copy vertices/normals:
		std::vector<glm::vec3> wm_vertices(vertices.begin() + e.vertex_begin, vertices.begin() + e.vertex_end);		
		std::string name(names.begin() + e.name_begin, names.begin() + e.name_end);

		// TODO: I HAVEN'T ACTUALLY MODIFIED THE SCRIPT FOR GENERATING THE SCENE TO ACTUALLY CALCULATE THE CONTAINING RADIUS
		// SO I AM DOING IT HERE
		// IN A WAY THAT IS EASY TO CHANGE INTO WORKING WHEN WE HAVE THE SCRIPT WORKING

		// auto ret = meshes.emplace(name, CollideMesh(wm_vertices, wm_normals, wm_triangles, containingRads[e.containingRadsAt]));
		auto ret = meshes.emplace(name, CollideMesh(wm_vertices, std::numeric_limits<float>().infinity()));
		if (!ret.second) {
			throw std::runtime_error("CollideMesh with duplicated name '" + name + "' in '" + filename + "'");
		}

	}
}

CollideMesh const& CollideMeshes::lookup(std::string const& name) const
{
	auto f = meshes.find(name);
	if(f == meshes.end())
	{
		throw std::runtime_error("CollideMesh with name '" + name + "' not found.");
	}
	return f->second;
}

glm::vec3 Collider::farthest(glm::vec3& d)
{
	float farthestSoFar = 0.0f;
	glm::vec3 farthestSoFarPoint;
	
	for(glm::vec3 v : mesh->vertices)
	{	
		float dist = glm::dot(v, d);
		if(dist > farthestSoFar)
		{
			farthestSoFar = dist;
			farthestSoFarPoint = v;
		}
	}

	return farthestSoFarPoint;
}

////////////////////////////////////
// CollisionEngine Implementation //
////////////////////////////////////

CollisionEngine::CollisionEngine() : nextID(0), colliders(), fromID() {}

CollisionEngine::ID CollisionEngine::registerCollider(Scene::Transform* t, CollideMesh const* m, float br, std::function<void(Scene::Transform*)> c)
{
	colliders.emplace_back(t, m, br, c);

	fromID.emplace(nextID, colliders.size());
	
	return nextID++;
}

void CollisionEngine::unregisterCollider(CollisionEngine::ID id)
{
	fromID.erase(id);
}

void CollisionEngine::update(float elapsed)
{
	struct CollisionOccurence
	{
		CollisionOccurence(size_t i, size_t j) : a(i), b(j) {};
		
		size_t a;
		size_t b;
	};

	std::vector<CollisionOccurence> collisionOccurences;

	// We split up checking for collisions and sending them out so that it's
	// more amenable to parallelization in the future. Note that this also does
	// a little bit to prevent segfaults (ie, if we delete the transform when handling a collision,
	// then that's ok because it's not like we need to derefence it after that)
	for(size_t i = 0; i < colliders.size() - 1; i++)
	{
		for(size_t j = i + 1; j < colliders.size(); j++)
		{
			if(GJK(colliders[i], colliders[j]))
			{
				collisionOccurences.emplace_back(i, j);
			}
		}
	}

	for(auto& c : collisionOccurences)
	{
		colliders[c.a].callback(colliders[c.b].transform);
		colliders[c.b].callback(colliders[c.a].transform);
	}
}
