#pragma once

#include "Module.h"
#include "Globals.h"

#include "box2d/box2d.h"

// Gravity in m/s^2. Screen Y grows DOWNWARDS and we convert meters <-> pixels without flipping Y,
// so a POSITIVE Y gravity makes bodies fall down on screen. That is why the world receives -GRAVITY_Y.
#define GRAVITY_X 0.0f
#define GRAVITY_Y -7.0f

// Units: Box2D works in meters (float), raylib draws in pixels (int)
#define PIXELS_PER_METER 50.0f // if touched change METER_PER_PIXEL too
#define METER_PER_PIXEL 0.02f  // this is 1 / PIXELS_PER_METER !

#define METERS_TO_PIXELS(m) ((int) floor(PIXELS_PER_METER * (m)))
#define PIXEL_TO_METERS(p)  ((float) METER_PER_PIXEL * (p))

class PhysicsBody
{
public:
	PhysicsBody()
		: body(NULL), listener(NULL)
	{
	}

	void GetPosition(int& x, int& y) const;
	float GetRotation() const;
	bool Contains(int x, int y) const;
	int RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;

public:
	b2Body* body;
	Module* listener;
};

// Module --------------------------------------
class ModulePhysics : public Module, public b2ContactListener
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	PhysicsBody* CreateCircle(int x, int y, int radius);
	PhysicsBody* CreateStaticCircle(int x, int y, int radius);
	PhysicsBody* CreateRectangle(int x, int y, int half_width, int half_height);
	PhysicsBody* CreateStaticRectangle(int x, int y, int half_width, int half_height);
	PhysicsBody* CreateRectangleSensor(int x, int y, int half_width, int half_height);
	PhysicsBody* CreateChain(int x, int y, const int* points, int size);


	void DestroyBody(PhysicsBody* pbody);
	void BeginContact(b2Contact* contact) override;

private:
	bool debug;
	b2World* world;
};
