#ifndef CREATURE_HPP
#define CREATURE_HPP

#include "Collisions.hpp"

#include <vector>

struct Creature
{
	// Component for walking (registers with the walksystem)
	// Reason we have this is mostly so that we don't need to worry about moving our walkpoint around
	// and can just say "go in a direction"
	// We need this because we don't want to intersect with other creatures (or other things) on the walking mesh
	// And so the walkmesh needs to know about all the walkers so it can disentangle them (so we don't have to do it
	// via the physics system). Both are gonna use the collision system though. This just makes it a bit easaier for me
	// to program "fake" physics collision because I can restrict the orientations of the rigid bodies and I can
	// resitrct their movement to a 2d manifold lol.
	// It's extremely possible that we use a different collider for this -- just a 2D collider system where this represents the
	// area the creature is taking up on the walkmesh. Decolliding objects in 2D (especially circles) is quite easy and this wouldn't
	// be that hard to do. The edge cases where it matters that we aren't doing 3D collision for walking only matter if our creatures
	// are in wild animations, in which case we probably want them to respond with physics anyway, so this doesn't really hurt that.

	// Component for collision (body)
	// Can add weapon collision in a specialized way
	// Done this way since every creature will have a body that can be hit (otherwise it's not a creature lmao)
	// but not all of them will have "just 1 sword" as a weapon (some might have something different than that
	// So it makes sense to just defer that to the specific creature and have the body be handled here
	// We might have a composite collider for our body (think torso + leg mesh colliders) so it pays to
	// have a list of colliders that we start with and cleanup at the end

	// Again, we only need these for cleanup, we could have this be a lambda that we preset
	// But this code eventually needs to go somewhere and I'd rather just put it here for now
	// But yeah, no reason for us to really touch these unless we are changing our colliders in the middle
	// of the game (ie we like, had our arm cut off or something, lol)
	// This could include colliders that are registered not just for the body but for the weapon

	// Component for rendering (this is already done by scene, but we need some link back so we can modify it)
	// This is useful for visual effects where we want to swap which shader program the thing is using, we'll preload
	// all the shaders and then we'll swap VAOs and start sending new uniforms when the time comes

	// Components for meta information such as HP or whatnot
	// The idea is that we don't give this thing an HP bar: we simply have a UI system that creates a
	// HP bar, and we give it a lambda or some other thing that allows it to synchronize what it's
	// rendering with the HP we have. IDC about performance right now, it's just a nice way to do it
	// I think. NOTE: when we create the hp bar, we have to handle the situation where the creature has been removed
	// or destroyed or something (it can figure out when it has died from 0 hp lol). Talked about later.
	// Oh also, the HP bar is going to need other stuff besides HP, it's gonna need our position (maybe a "head position")
	// it's going to need our name if we have one, etc. So it's going to just have a link to our creature and it's going
	// to have to deal with when we are destroyed, not the creature itself.

	// For HP specifically, I'm thinking we have an hp resource that can be expended, and then we have
	// immunity timers PER ENEMY (or thing hitting us). This way getting hit by 1 enemy sword slash gives us a bit
	// of immunity, but if we get hit by 2 at the same time we take both their damage
	
	// The big "feature" of a creature is that it can take actions
	// And only certain actions based on its current state
	// So right now all pawns (enemies and player) have the same set of actions they can take based on their stance
	// And we're kind of cosntricted from making more types of enemies because we haven't divorced that from rendering and stuff

	// Note that creatures register with a lot of systems, or a lot of systems are build to observe them
	// They register with the collider system, they register with the walking system, and UI systems rely on them existing
	// Because of this we have to be responsible when we are destroyed
	// We have to deregister our colliders, we have to deregister from the walk system, etc.
	// Thus we should make it so that we can only destroy or add new creatures at the end of an update
	// This means we need a system for overlooking all of the creatures and we can flag ourselves to it to be destroyed. This system
	// is probably also gonna be the authority for getting new ids or accessing by ids. This system should try to be fast though so idk.
	// Need to optimize probably.

	// This is called when we need to be cleaned up
	// Yes, we are implicitly assuming that the systems we registered with will still exist, so if breaking this
	// assumption (we probably shouldn't) something would need to massively change with this architecture
	void cleanup();
};

#endif
