#pragma once

#include "Globals.h"
#include "Module.h"

#include "p2Point.h"

#include "raylib.h"
#include <vector>

class PhysBody;
class PhysicsEntity;
class Flipper;
class Kicker;
class Ball;

class ModuleGame : public Module
{
protected:
	Ball* ball;
	Flipper* leftFlipper;
	Flipper* rightFlipper;
	Kicker* kicker;

	Texture2D boardTexture;
	Texture2D leftFlipperTexture;
	Texture2D rightFlipperTexture;
	Texture2D ballTexture;
	Texture2D bumperTexture;
	Texture2D bumperHitTexture;
	Texture2D lifeTexture;

	Vector2 spawnPoint;

public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start();
	update_status Update();
	bool CleanUp();

	void OnCollision(PhysBody* bodyA, PhysBody* bodyB) override;

	void DestroyEntity(PhysicsEntity* entity);
	void DestroyAllEntities();

	int lives;
	int points;
	int combo;
	int highscore;
	std::vector<PhysicsEntity*> entities;

private:
	bool ballResetQueued;
};
