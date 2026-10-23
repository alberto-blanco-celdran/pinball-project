#pragma once

#include "Globals.h"
#include "Module.h"
#include "PhysicsEntity.h"

#include <vector>

class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start();
	update_status Update();
	bool CleanUp();

	void OnCollision(PhysicsBody* bodyA, PhysicsBody* bodyB) override;

	void DestroyEntity(PhysicsEntity* entity);
	void DestroyAllEntities();

public:



	Texture2D circle;
	Texture2D box;

	uint32 bonus_fx;


	std::vector<PhysicsEntity*> entities;
};
