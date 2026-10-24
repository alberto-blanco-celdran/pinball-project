#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

#include <algorithm>

// Base class for everything in the scene that has a physics body
class PhysicsEntity
{
protected:

	PhysicsEntity(PhysBody* _body)
		: body(_body)
	{}

public:
	virtual ~PhysicsEntity() = default;

	// Draw the entity at the position of its body
	virtual void Update() = 0;

public:
	PhysBody* body;
};

class Ball : public PhysicsEntity {
public:
	Ball(ModulePhysics* physics, int x, int y)
		: PhysicsEntity(physics->CreateCircle(x, y, 24))
	{}

	void Update() override {}
};

class Kicker : public PhysicsEntity {
public:
	Kicker(ModulePhysics* physics, int x, int y)
		: PhysicsEntity(physics->CreateRectangle(x, y, 24, 6))
	{
	}

	void Update() override {}

};

class Spring : public PhysicsEntity {
public:

	Spring(ModulePhysics* physics, int x, int y)
		: PhysicsEntity(physics->CreateStaticRectangle(x, y, 24, 6))
		,physics(physics)
	{
	}
	
	void Update() override {}

	
	void CreateJoint(Kicker* kicker, Spring* spring) {
		b2DistanceJointDef def;
		b2Body* a = body->body;          
		b2Body* b = kicker->body->body;  
		def.Initialize( a,b,a->GetWorldCenter(), b->GetWorldCenter());
		joint = (b2DistanceJoint*)physics->world->CreateJoint(&def);
	}
	
};

class Paddle : public PhysicsEntity {
	static constexpr int points[10] = {
		0, 0,
		16, -24,
		128, -2,
		128, 2,
		16, 24,
	};

public:
	Paddle(ModulePhysics* physics, int x, int y)
		: PhysicsEntity(physics->CreatePolygon(x, y, points, 10))
	{}

	void Update() override {}
};


class Flipper : public PhysicsEntity {
protected:
	const float LIFT_TORQUE = 1000.0;
	const float REST_TORQUE = 100.0;
	const float LIMIT_ANGLE = 35.0f;

public:
	Flipper(ModulePhysics* physics, int x, int y, bool reverse)
		: PhysicsEntity(physics->CreateStaticRectangle(x, y, 16, 16))
		, physics(physics), joint(NULL), reversed(reverse)
	{
		b2Filter filter = b2Filter();
		filter.categoryBits = 0x0000; // do not collide with anything
		for (b2Fixture* fixture = body->body->GetFixtureList(); fixture; fixture = fixture->GetNext())
		{
			fixture->SetFilterData(filter);
		}
	}

	void Update() override {}

	void CreateJoint(Paddle* paddle) {
		float offset = reversed ? 180.0f : 0.0f;
		b2RevoluteJointDef def;
		def.Initialize(body->body, paddle->body->body, body->body->GetWorldCenter());
		def.enableLimit = true;
		def.lowerAngle = DEGTORAD * (offset - LIMIT_ANGLE);
		def.upperAngle = DEGTORAD * (LIMIT_ANGLE + offset);
		def.enableMotor = true;
		joint = reinterpret_cast<b2RevoluteJoint*>(physics->CreateJoint(def));
	}

	void SetLifted(bool lift) {
		float mul = reversed ? -1.0f : 1.0f;
		if (lift) {
			joint->SetMaxMotorTorque(LIFT_TORQUE);
			joint->SetMotorSpeed(-LIFT_TORQUE * mul);
		}
		else {
			joint->SetMaxMotorTorque(REST_TORQUE);
			joint->SetMotorSpeed(REST_TORQUE * mul);
		}
	}

private:
	ModulePhysics* physics;
	b2RevoluteJoint* joint;
	bool reversed;
};

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled), leftFlipper(NULL), rightFlipper(NULL)
{}


ModuleGame::~ModuleGame()
{}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");

	App->renderer->camera.x = App->renderer->camera.y = 0;

	float leftFlipperX = 80.0f;
	float rightFlipperX = 300.0f;
	float flippersY = 480.0f;
	float paddleOffset = 24.0f;

	float baseX = 900.0f;
	float baseY = 450.0f;
	float KickerX = 950.0f;
	float KickerY = 500.0f;

	leftFlipper = new Flipper(App->physics, leftFlipperX, flippersY, false);
	Paddle* paddle0 = new Paddle(App->physics, leftFlipperX - paddleOffset, flippersY);
	entities.push_back(leftFlipper);
	entities.push_back(paddle0);
	leftFlipper->CreateJoint(paddle0);

	rightFlipper = new Flipper(App->physics, rightFlipperX, flippersY, true);
	Paddle* paddle1 = new Paddle(App->physics, rightFlipperX - paddleOffset, flippersY);
	entities.push_back(rightFlipper);
	entities.push_back(paddle1);
	rightFlipper->CreateJoint(paddle1);

	springbase = new Spring(App->physics, baseX, baseY);
	Kicker* springKicker = new Kicker(App->physics, KickerX, KickerY);
	springbase->CreateJoint(springKicker);


	return true;
}

// Unload assets
bool ModuleGame::CleanUp()
{
	LOG("Unloading Intro scene");

	DestroyAllEntities();

	return true;
}

// Update: draw background
update_status ModuleGame::Update()
{
	for (PhysicsEntity* entity : entities)
	{
		entity->Update();

	}

	leftFlipper->SetLifted(IsKeyDown(KEY_LEFT));
	rightFlipper->SetLifted(IsKeyDown(KEY_RIGHT));

	if (IsKeyPressed(KEY_SPACE)) {
		entities.push_back(new Ball(App->physics, GetMouseX(), GetMouseY()));
	}
	//if (IsKeyPressed(KEY_UP)) {
		//entities.push_back(new Spring(App->physics, GetMouseX(), GetMouseY()));
	

	return UPDATE_CONTINUE;
}

void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
}

void ModuleGame::DestroyEntity(PhysicsEntity* entity)
{
	entities.erase(std::find(entities.begin(), entities.end(), entity));
	App->physics->DestroyBody(entity->body);
	delete entity;
}

void ModuleGame::DestroyAllEntities()
{
	for (PhysicsEntity* entity : entities)
	{
		App->physics->DestroyBody(entity->body);
		delete entity;
	}
	entities.clear();
}
