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
	{
	}

public:
	virtual ~PhysicsEntity() = default;

	// Draw the entity at the position of its body
	virtual void Update() = 0;

public:
	PhysBody* body;
};

class FlipperAnchor : public PhysicsEntity {
public:
	FlipperAnchor(ModulePhysics* physics, int x, int y)
		: PhysicsEntity(physics->CreateStaticRectangle(x, y, 16, 16))
	{
		b2Filter filter = b2Filter();
		filter.categoryBits = 0x0000; // do not collide with anything
		for (b2Fixture* fixture = body->body->GetFixtureList(); fixture; fixture = fixture->GetNext())
		{
			fixture->SetFilterData(filter);

		}
	}

	void CreateJoint(Flipper* flipper) {
		b2RevoluteJointDef def;
		def.Initialize(body->body, flipper->body->body, body->body->GetWorldCenter());
		// TODO: parameters
		world->CreateJoint(&def);
	}

	void Update() override {

	}

private:
	b2World* world;
	b2RevoluteJoint* joint;
};

class Flipper : public PhysicsEntity {
	static constexpr int points[8] = {
		-1, 0,
		11, -26,
		132, 0,
		26, 26,
	};

public:
	Flipper(ModulePhysics* physics, int x, int y)
		: PhysicsEntity(physics->CreatePolygon(x, y, points, 8))
	{
		
	}

	void Update() override {
		
	}
};

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
}

ModuleGame::~ModuleGame()
{}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");

	App->renderer->camera.x = App->renderer->camera.y = 0;
	FlipperAnchor* anchor0 = new FlipperAnchor(App->physics, 80, 80);
	Flipper* flipper0 = new Flipper(App->physics, 50, 80);
	entities.push_back(anchor0);
	entities.push_back(flipper0);
	anchor0->CreateJoint(flipper0);

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
