#pragma once

#include "ModulePhysics.h"

class PhysicsEntity
{
protected:
	PhysicsEntity(PhysicsBody* _body)
		: body(_body)
	{}

public:
	virtual ~PhysicsEntity() = default;
	virtual void Update() = 0;

	PhysicsBody* body;
};