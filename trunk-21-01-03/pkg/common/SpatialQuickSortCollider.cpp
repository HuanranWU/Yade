/*************************************************************************
*  Copyright (C) 2008 by Sergei Dorofeenko				 *
*  sega@users.berlios.de                                                 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#include "SpatialQuickSortCollider.hpp"
#include <core/BodyContainer.hpp>
#include <core/Scene.hpp>
#include <algorithm>

namespace yade { // Cannot have #include directive inside.

YADE_PLUGIN((SpatialQuickSortCollider));

void SpatialQuickSortCollider::action()
{
	if (scene->isPeriodic) {
		throw runtime_error("SpatialQuickSortCollider doesn't handle periodic boundaries.");
	}

	// update bounds
	boundDispatcher->scene = scene;
	boundDispatcher->action();

	const shared_ptr<BodyContainer>& bodies = scene->bodies;

	// This collider traverses all interactions at every step, therefore all interactions
	// that were requested for erase might be erased here and will be recreated if necessary.
	scene->interactions->eraseNonReal();

	size_t nbElements = bodies->size();
	if (nbElements != rank.size()) {
		size_t n = rank.size();
		rank.resize(nbElements);
		for (; n < nbElements; ++n)
			rank[n] = shared_ptr<AABBBound>(new AABBBound);
	}

	Vector3r min, max;
	int      n = 0;
	for (const auto& b : *bodies) {
		if (!b->bound)
			continue;
		min          = b->bound->min;
		max          = b->bound->max;
		rank[n]->id  = b->getId();
		rank[n]->min = min;
		rank[n]->max = max;
		n++;
	}

	const shared_ptr<InteractionContainer>& interactions = scene->interactions;
	scene->interactions->iterColliderLastRun             = scene->iter;

	sort(rank.begin(), rank.end(), xBoundComparator()); // sorting along X

	int                     id, id2;
	size_t                  j;
	shared_ptr<Interaction> interaction;
	for (int i = 0, e = nbElements - 1; i < e; ++i) {
		id  = rank[i]->id;
		min = rank[i]->min;
		max = rank[i]->max;
		j   = i;
		while (++j < nbElements) {
			if (rank[j]->min[0] > max[0])
				break; // skip all others, because it's sorted along X
			id2 = rank[j]->id;
#ifdef YADE_MPI
			if (not Collider::mayCollide(Body::byId(id, scene).get(), Body::byId(id2, scene).get(), scene->subdomain))
#else
			if (not Collider::mayCollide(Body::byId(id, scene).get(), Body::byId(id2, scene).get()))
#endif
				continue; // skip this pair id ↔ id2, because it cannot collide for whatever reasons, e.g. a groupMask
			if (rank[j]->min[1] < max[1] && rank[j]->max[1] > min[1] && rank[j]->min[2] < max[2] && rank[j]->max[2] > min[2]) {
				if ((interaction = interactions->find(Body::id_t(id), Body::id_t(id2))) == 0) {
					interaction = shared_ptr<Interaction>(new Interaction(id, id2));
					interactions->insert(interaction);
				}
				interaction->iterLastSeen = scene->iter;
			}
		}
	}
}

} // namespace yade
