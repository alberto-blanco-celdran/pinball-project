#pragma once

#include "PhysicsEntity.h"

class Ball : PhysicsEntity {
public:
	Ball(ModulePhysics* physics, int x, int y, Texture2D _texture)
		: PhysicsEntity(physics->CreateCircle(x, y, 25))
		, texture(_texture) {}
	
	void Update() override;

private:
	Texture2D texture;
};