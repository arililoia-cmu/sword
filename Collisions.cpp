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

	glm::vec3 da = d * awtl;
	glm::vec3 db = -d * bwtl;

	glm::vec3 simplex[4] = {};

	uint8_t simpType = 1;
	simplex[0] = (altw * glm::vec4(a.farthest(da), 1.0f)) - (bltw * glm::vec4(b.farthest(db), 1.0f));

	d = glm::vec3(0.0f) - simplex[0];

	while(true)
	{
		da = d * awtl;
		db = -d * bwtl;
		
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
			// This is the actual tetra case, where we except we can contain the origin

			glm::vec3 AD = simplex[3] - simplex[0];
			glm::vec3 AC = simplex[3] - simplex[1];
			glm::vec3 AB = simplex[3] - simplex[2];
			glm::vec3 AO = simplex[3] - glm::vec3(0.0f);

			glm::vec3 triACD = glm::cross(AD, glm::cross(AC, glm::vec3(0.0f)));
			glm::vec3 triABC = glm::cross(AC, glm::cross(AB, glm::vec3(0.0f)));
			glm::vec3 triABD = glm::cross(AB, glm::cross(AD, glm::vec3(0.0f)));

			if(glm::dot(triACD, AO) > 0)
			{
				simplex[2] = simplex[3];
				simpType = 2;
				d = glm::normalize(triACD);
			}
			else if(glm::dot(triABD, AO) > 0)
			{
				simplex[1] = simplex[2];
				simplex[2] = simplex[3];
				simpType = 2;
				d = glm::normalize(triABD);
			}
			else if(glm::dot(triABC, AO) > 0)
			{
				simplex[0] = simplex[1];
				simplex[1] = simplex[2];
				simplex[2] = simplex[3];
				simpType = 2;
				d = glm::normalize(triABC);
			}
			else
			{
				return true;
			}
		}
	}
}

CollideMesh::CollideMesh(std::vector<glm::vec3> const& vertices_, std::vector<glm::vec3> const& normals_, std::vector<glm::uvec3> const& triangles_)
	: vertices(vertices_), normals(normals_), triangles(triangles_) {}

CollideMeshes::CollideMeshes(std::string const& filename)
{
	std::ifstream file(filename, std::ios::binary);

	std::vector< glm::vec3 > vertices;
	read_chunk(file, "p...", &vertices);

	std::vector< glm::vec3 > normals;
	read_chunk(file, "n...", &normals);

	std::vector< glm::uvec3 > triangles;
	read_chunk(file, "tri0", &triangles);

	std::vector< char > names;
	read_chunk(file, "str0", &names);

	struct IndexEntry {
		uint32_t name_begin, name_end;
		uint32_t vertex_begin, vertex_end;
		uint32_t triangle_begin, triangle_end;
	};

	std::vector< IndexEntry > index;
	read_chunk(file, "idxA", &index);

	if (file.peek() != EOF) {
		std::cerr << "WARNING: trailing data in collidemesh file '" << filename << "'" << std::endl;
	}

	//-----------------

	if (vertices.size() != normals.size()) {
		throw std::runtime_error("Mis-matched position and normal sizes in '" + filename + "'");
	}

	for (auto const &e : index) {
		if (!(e.name_begin <= e.name_end && e.name_end <= names.size())) {
			throw std::runtime_error("Invalid name indices in index of '" + filename + "'");
		}
		if (!(e.vertex_begin <= e.vertex_end && e.vertex_end <= vertices.size())) {
			throw std::runtime_error("Invalid vertex indices in index of '" + filename + "'");
		}
		if (!(e.triangle_begin <= e.triangle_end && e.triangle_end <= triangles.size())) {
			throw std::runtime_error("Invalid triangle indices in index of '" + filename + "'");
		}

		//copy vertices/normals:
		std::vector< glm::vec3 > wm_vertices(vertices.begin() + e.vertex_begin, vertices.begin() + e.vertex_end);
		std::vector< glm::vec3 > wm_normals(normals.begin() + e.vertex_begin, normals.begin() + e.vertex_end);

		//remap triangles:
		std::vector< glm::uvec3 > wm_triangles; wm_triangles.reserve(e.triangle_end - e.triangle_begin);
		for (uint32_t ti = e.triangle_begin; ti != e.triangle_end; ++ti) {
			if (!( (e.vertex_begin <= triangles[ti].x && triangles[ti].x < e.vertex_end)
			    && (e.vertex_begin <= triangles[ti].y && triangles[ti].y < e.vertex_end)
			    && (e.vertex_begin <= triangles[ti].z && triangles[ti].z < e.vertex_end) )) {
				throw std::runtime_error("Invalid triangle in '" + filename + "'");
			}
			wm_triangles.emplace_back(
				triangles[ti].x - e.vertex_begin,
				triangles[ti].y - e.vertex_begin,
				triangles[ti].z - e.vertex_begin
			);
		}
		
		std::string name(names.begin() + e.name_begin, names.begin() + e.name_end);

		auto ret = meshes.emplace(name, CollideMesh(wm_vertices, wm_normals, wm_triangles));
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

void Collisions::broadPhase(std::vector<Collider>& colliders)
{
	for(size_t i = 0; i < colliders.size() - 1; i++)
	{
		for(size_t j = i + 1; j < colliders.size(); j++)
		{
			if(GJK(colliders[i], colliders[j]))
			{
				colliders[i].callback(colliders[j].transform);
				colliders[j].callback(colliders[i].transform);
				//std::cout << "collider [" << i << "] hit collider [" << j << "]" << std::endl;
			}
			// AABB& ciaabb = colliders[i].aabb;
			// AABB& cjaabb = colliders[j].aabb;

			// bool aabbIntersect = !(ciaabb.min[0] > cjaabb.max[0]
			// 					   || cjaabb.min[0] > ciaabb.max[0]
			// 					   || ciaabb.min[1] > cjaabb.max[1]
			// 					   || cjaabb.min[1] > ciaabb.max[1]
			// 					   || ciaabb.min[2] > cjaabb.max[2]
			// 					   || cjaabb.min[2] > ciaabb.max[2]);

			// // If wanting to split to multiple phases, we'd pass off this work to be done, maybe batched
			// if(aabbIntersect)
			// {
				
			// }
		}
	}
}
