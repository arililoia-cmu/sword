#include "WalkMesh.hpp"

#include "glm/gtx/quaternion.hpp"
#include "read_write_chunk.hpp"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <string>

WalkMesh::WalkMesh(std::vector< glm::vec3 > const &vertices_, std::vector< glm::vec3 > const &normals_, std::vector< glm::uvec3 > const &triangles_)
	: vertices(vertices_), normals(normals_), triangles(triangles_) {

	//construct next_vertex map (maps each edge to the next vertex in the triangle):
	next_vertex.reserve(triangles.size()*3);
	auto do_next = [this](uint32_t a, uint32_t b, uint32_t c) {
		auto ret = next_vertex.insert(std::make_pair(glm::uvec2(a,b), c));
		assert(ret.second);
	};
	for (auto const &tri : triangles) {
		do_next(tri.x, tri.y, tri.z);
		do_next(tri.y, tri.z, tri.x);
		do_next(tri.z, tri.x, tri.y);
	}

	//DEBUG: are vertex normals consistent with geometric normals?
	for (auto const &tri : triangles) {
		glm::vec3 const &a = vertices[tri.x];
		glm::vec3 const &b = vertices[tri.y];
		glm::vec3 const &c = vertices[tri.z];
		glm::vec3 out = glm::normalize(glm::cross(b-a, c-a));

		float da = glm::dot(out, normals[tri.x]);
		float db = glm::dot(out, normals[tri.y]);
		float dc = glm::dot(out, normals[tri.z]);

		assert(da > 0.1f && db > 0.1f && dc > 0.1f);
	}
}

//project pt to the plane of triangle a,b,c and return the barycentric weights of the projected point:
glm::vec3 barycentric_weights(glm::vec3 const &a, glm::vec3 const &b, glm::vec3 const &c, glm::vec3 const &pt)
{
	glm::vec3 const ba = b - a; // First edge
	glm::vec3 const ca = c - a; // Second edge

	glm::vec3 const normal = glm::cross(ba, ca); // Respecting CCW order of vertices, this should give us
	float const normal2 = glm::dot(normal, normal); // Get length squared, so we can normalize the coordinates

	glm::vec3 const pa = pt - a; // We will calculate coord for a using the other two

	float const opB = glm::dot(glm::cross(pa, ca), normal) / normal2;
	float const opC = glm::dot(glm::cross(ba, pa), normal) / normal2;
	float const opA = 1.0f - (opB + opC);
	
	return glm::vec3(opA, opB, opC);
}

WalkPoint WalkMesh::nearest_walk_point(glm::vec3 const &world_point) const {
	assert(!triangles.empty() && "Cannot start on an empty walkmesh");

	WalkPoint closest;
	float closest_dis2 = std::numeric_limits< float >::infinity();

	for (auto const &tri : triangles) {
		//find closest point on triangle:

		glm::vec3 const &a = vertices[tri.x];
		glm::vec3 const &b = vertices[tri.y];
		glm::vec3 const &c = vertices[tri.z];

		//get barycentric coordinates of closest point in the plane of (a,b,c):
		glm::vec3 coords = barycentric_weights(a,b,c, world_point);

		//is that point inside the triangle?
		if (coords.x >= 0.0f && coords.y >= 0.0f && coords.z >= 0.0f) {
			//yes, point is inside triangle.
			float dis2 = glm::length2(world_point - to_world_point(WalkPoint(tri, coords)));
			if (dis2 < closest_dis2) {
				closest_dis2 = dis2;
				closest.indices = tri;
				closest.weights = coords;
			}
		} else {
			//check triangle vertices and edges:
			auto check_edge = [&world_point, &closest, &closest_dis2, this](uint32_t ai, uint32_t bi, uint32_t ci) {
				glm::vec3 const &a = vertices[ai];
				glm::vec3 const &b = vertices[bi];

				//find closest point on line segment ab:
				float along = glm::dot(world_point-a, b-a);
				float max = glm::dot(b-a, b-a);
				glm::vec3 pt;
				glm::vec3 coords;
				if (along < 0.0f) {
					pt = a;
					coords = glm::vec3(1.0f, 0.0f, 0.0f);
				} else if (along > max) {
					pt = b;
					coords = glm::vec3(0.0f, 1.0f, 0.0f);
				} else {
					float amt = along / max;
					pt = glm::mix(a, b, amt);
					coords = glm::vec3(1.0f - amt, amt, 0.0f);
				}

				float dis2 = glm::length2(world_point - pt);
				if (dis2 < closest_dis2) {
					closest_dis2 = dis2;
					closest.indices = glm::uvec3(ai, bi, ci);
					closest.weights = coords;
				}
			};
			check_edge(tri.x, tri.y, tri.z);
			check_edge(tri.y, tri.z, tri.x);
			check_edge(tri.z, tri.x, tri.y);
		}
	}
	assert(closest.indices.x < vertices.size());
	assert(closest.indices.y < vertices.size());
	assert(closest.indices.z < vertices.size());
	return closest;
}

// Referenced code from @stroucki on discord (image in game5 channel)
void WalkMesh::walk_in_triangle(WalkPoint const &start, glm::vec3 const &step, WalkPoint *end_, float *time_) const {
	assert(end_);
	auto& end = *end_;

	assert(time_);
	auto& time = *time_;

	glm::vec3 const& a = vertices[start.indices.x];
	glm::vec3 const& b = vertices[start.indices.y];
	glm::vec3 const& c = vertices[start.indices.z];

	glm::vec3 stepEndBary;
	{ //project 'step' into a barycentric-coordinates direction:
		glm::vec3 stepEnd = to_world_point(start) + step;
		stepEndBary = barycentric_weights(a, b, c, stepEnd);
	}

	float nearestEdgeTime = std::numeric_limits<float>::infinity(); // Starting at impossible time
	uint32_t nearestEdgeOpVert = 3; // Starting at invalid value

	//figure out which edge (if any) is crossed first.
	// set time and end appropriately.
	for(size_t i = 0; i < 3; i++)
	{
		float startBary = start.weights[i];
		if(stepEndBary[i] <= 0.0f) // We exited on the side opposite this vert
		{
			float timeToReachOpEdge = startBary / (startBary - stepEndBary[i]);
			
			if(timeToReachOpEdge < nearestEdgeTime)
			{
				nearestEdgeTime = timeToReachOpEdge;
				nearestEdgeOpVert = i;
			}
		}
	}

	time = std::min(1.0f, nearestEdgeTime);

	glm::vec3 endWeights = start.weights + time * (stepEndBary - start.weights);

	if(nearestEdgeOpVert != 3)
	{
		for(size_t i = 0; i < 3; i++)
		{
			end.indices[i] = start.indices[(i + nearestEdgeOpVert + 1) % 3];
			end.weights[i] = endWeights[(i + nearestEdgeOpVert + 1) % 3];
		}
		end.weights[2] = 0.0f;
	}
	else
	{
		end.indices = start.indices;
		end.weights = endWeights;
	}
}

bool WalkMesh::cross_edge(WalkPoint const& start, WalkPoint* end_, glm::quat* rotation_) const
{
	assert(end_);
	auto& end = *end_;

	assert(rotation_);
	auto& rotation = *rotation_;

	assert(start.weights.z == 0.0f); // This is our convention
	glm::uvec2 edge = glm::uvec2(start.indices); // The edge from the POV of the current triangle
	glm::uvec2 opEdge = glm::uvec2(edge.y, edge.x); // The edge from the POV of the opposite triangle, regardless of whether that exists

	// Checking if the edge we're on is a boundary edge:
	// Since we maintained counter clock wise order in the triangle we started in, we can
	// reverse the order of the edge endpoints to discover the third point of the adjacent
	// triangle that we want to cross over into.
	// We're using find here since there SHOULDN'T be an entry if this is a boundary edge.
	auto opEdgeIt = next_vertex.find(opEdge);
	if(opEdgeIt != next_vertex.end())
	{
		//make 'end' represent the same (world) point, but on triangle (edge.y, edge.x, [other point]):
		end = WalkPoint(glm::uvec3(opEdge.x, opEdge.y, opEdgeIt->second), glm::vec3(start.weights.y, start.weights.x, 0.0f));

		//make 'rotation' the rotation that takes (start.indices)'s normal to (end.indices)'s normal:
		glm::vec3 const startN = to_world_triangle_normal(start);
		glm::vec3 const endN = to_world_triangle_normal(end);

		rotation = glm::rotation(startN, endN);

		return true;
	}
	else
	{
		end = start;
		rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
		return false;
	}
}


WalkMeshes::WalkMeshes(std::string const &filename) {
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
		std::cerr << "WARNING: trailing data in walkmesh file '" << filename << "'" << std::endl;
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

		auto ret = meshes.emplace(name, WalkMesh(wm_vertices, wm_normals, wm_triangles));
		if (!ret.second) {
			throw std::runtime_error("WalkMesh with duplicated name '" + name + "' in '" + filename + "'");
		}

	}
}

WalkMesh const &WalkMeshes::lookup(std::string const &name) const {
	auto f = meshes.find(name);
	if (f == meshes.end()) {
		throw std::runtime_error("WalkMesh with name '" + name + "' not found.");
	}
	return f->second;
}
