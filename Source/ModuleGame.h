#pragma once

#include "Globals.h"
#include "Module.h"

#include "p2Point.h"

#include "raylib.h"
#include <vector>

class PhysBody;
class PhysicsEntity;
class Flipper;
class Spring;

class ModuleGame : public Module
{
protected:
	Flipper* leftFlipper;
	Flipper* rightFlipper;

	Texture2D boardTexture;
	Texture2D leftFlipperTexture;
	Texture2D rightFlipperTexture;
	Texture2D ballTexture;
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start();
	update_status Update();
	bool CleanUp();

	void OnCollision(PhysBody* bodyA, PhysBody* bodyB) override;

	void DestroyEntity(PhysicsEntity* entity);
	void DestroyAllEntities();
	
	std::vector<PhysicsEntity*> entities;
};
