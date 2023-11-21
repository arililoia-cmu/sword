#ifndef COLLISIONS_HPP
#define COLLISIONS_HPP

#include <functional>
#include <glm/glm.hpp>

#include "Mesh.hpp"
#include "Scene.hpp"
#include "Game.hpp"

#include <array>
#include <vector>

struct CollideMesh
{
	// Same as walk mesh will keep track of triangles, vertices:
	std::vector<glm::vec3> vertices;

	float containingRadius;

	CollideMesh(std::vector<glm::vec3> const& vertices_, float cr);
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

// struct MeshCollider
// {
// 	MeshCollider(Scene::Transform* t, CollideMesh const* m, float br, std::function<void(Scene::Transform*)> c) : transform(t), mesh(m), broadRadius(br), callback(c) {};

// 	// Finds the farthest point in the direction d
// 	// (This obviously is only guaranteed to be useful if we're convex)
// 	// In a convex mesh, this is always guaranteed to be a vertex of a mesh
// 	glm::vec3 farthest(glm::vec3& d);

// 	Scene::Transform* transform;
// 	CollideMesh const* mesh;
// 	float broadRadius;

// 	std::function<void(Scene::Transform*)> callback;
// }

// MUST BE CONVEX
struct Collider
{
	Collider(Game::CreatureID cid, Scene::Transform* t, CollideMesh const* m, float br, std::function<void(Game::CreatureID, Scene::Transform*)> c) : cId(cid), transform(t), mesh(m), broadRadius(br), callback(c) {};
	
	// Finds the farthest point in the direction d
	// (This obviously is only guaranteed to be useful if we're convex)
	// In a convex mesh, this is always guaranteed to be a vertex of a mesh
	glm::vec3 farthest(glm::vec3& d);

	Game::CreatureID cId;
	Scene::Transform* transform;
	CollideMesh const* mesh;
	float broadRadius;

	std::function<void(Game::CreatureID, Scene::Transform*)> callback;
};

struct CollisionEngine
{
public:
	// A simple ID for managing registering and unregistering colliders
	typedef uint64_t ID;

	enum Layer : size_t
	{
		PLAYER_SWORD_LAYER = 0,
		PLAYER_BODY_LAYER,
		ENEMY_SWORD_LAYER,
		ENEMY_BODY_LAYER,
		LAYER_COUNT
	};

	bool LayerMatrix[LAYER_COUNT][LAYER_COUNT] =
	{
		// Player sword layer interacts with:
		{false, false, true, true},
		{false, false, true, true},
		{true, true, false, false},
		{true, true, false, false}
	};
	
	CollisionEngine();
	
	// Register a collider so the engine can check for its collisions
	// Requires a radius (for broad phase testing / acceleration structures)
	// A transform so we know where the collider is
	// A actual collider we can do full testing against
	// And some meta information in the form of layers that describes what this collider should be tested against

	// Also note that this does not forward the arguments, so it can be slower than optimal, but I'll fix
	// it if it's a problem
	ID registerCollider(Game::CreatureID cid, Scene::Transform* t, CollideMesh const* m, float br, std::function<void(Game::CreatureID, Scene::Transform*)> c, Layer l);

	// Deregister a collider so the engine can forget about it
	void unregisterCollider(ID id);
	
	// Checks for collisions and sends out collision events
	// Makes a list and then sends out events
	void update(float elapsed);
	
private:
	ID nextID;
	
	std::array<std::vector<Collider>, LAYER_COUNT> colliders;
	std::unordered_map<ID, std::pair<Layer, size_t>> fromID;
};

#endif
