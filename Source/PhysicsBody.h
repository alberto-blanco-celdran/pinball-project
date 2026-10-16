#pragma once

#include "ModulePhysics.h"

class PhysicsBody
{
public:
	PhysicsBody()
		: body(NULL), listener(NULL)
	{}

	void GetPosition(int& x, int& y) const;
	float GetRotation() const;
	bool Contains(int x, int y) const;
	int RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;

public:
	b2Body* body;
	Module* listener;
};