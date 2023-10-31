#ifndef COLLISIONS_HPP
#define COLLISIONS_HPP

#include <glm/glm.hpp>

#include "Mesh.hpp"
#include "Scene.hpp"

#include <vector>

struct AABB
{
	glm::vec3 min;
	glm::vec3 max;
};

struct CollideMesh
{
	// Same as walk mesh will keep track of triangles, vertices:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals; // normals for interpolated 'up' direction
	std::vector<glm::uvec3> triangles; // CCW-oriented

	CollideMesh(std::vector<glm::vec3> const& vertices_, std::vector<glm::vec3> const& normals_, std::vector<glm::uvec3> const& triangles_);
};

struct CollideMeshes
{
	//load a list of named WalkMeshes from a file:
	CollideMeshes(std::string const& filename);

	//retrieve a WalkMesh by name:
	CollideMesh const& lookup(std::string const& name) const;

	//internals:
	std::unordered_map<std::string, CollideMesh> meshes;
};

// MUST BE CONVEX
struct Collider
{
	Collider(Scene::Transform* t, CollideMesh const* m, AABB ab) : transform(t), mesh(m), aabb(ab) {};
	
	// Finds the farthest point in the direction d
	// (This obviously is only guaranteed to be useful if we're convex)
	// In a convex mesh, this is always guaranteed to be a vertex of a mesh
	glm::vec3 farthest(glm::vec3& d);

	Scene::Transform* transform;
	CollideMesh const* mesh;
	AABB aabb;
};

struct Collisions
{
	// set up this way so if we need to multithread to achieve performance it is not as hard, but for
	// simplicity right now putting it all in one phase
	void broadPhase(std::vector<Collider>& colliders);

	std::vector<Collider> cs;
};

#endif
