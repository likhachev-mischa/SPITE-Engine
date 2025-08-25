#pragma once

namespace spite
{
	class EntityManager;

	//Draws full scene hierchy
	void inspectUIEntityHierchy(EntityManager& entityManager);

	// Draws the full inspector UI for a given entity.
	void inspectUIEntityComponents(EntityManager& entityManager);
}

