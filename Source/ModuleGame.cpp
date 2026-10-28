#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

#include <algorithm>
#include <iostream>

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
	Ball(ModulePhysics* physics, int x, int y, Texture2D& texture)
		: PhysicsEntity(physics->CreateCircle(x, y, 8))
		, texture(texture)
	{}

	void Update() override {
		int x, y;
		body->GetPosition(x, y);
		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { (float)x, (float)y, (float)texture.width, (float)texture.height };
		Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
		DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, WHITE);
	}
private:
	Texture2D& texture;
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
private:
	ModulePhysics* physics;
	b2Joint* joint;
	bool reversed;
public:

	Spring(ModulePhysics* physics, int x, int y)
		: PhysicsEntity(physics->CreateStaticRectangle(x, y, 24, 6)), physics(physics),
		joint(NULL),
		reversed(false)
	{
	};

	void Update() override {}
	void CreateJoint(Kicker* kicker, Spring* spring) {
		b2DistanceJointDef def;
		b2Body* spring_body = body->body;          
		b2Body* kicker_body = kicker->body->body;  
		def.Initialize( spring_body,kicker_body,spring_body->GetWorldCenter(),kicker_body->GetWorldCenter());

		def.length = 1.0f;
		def.stiffness = 50.0f;
		def.damping = 2.0f;

		joint = (b2DistanceJoint*)physics->GetWorld()->CreateJoint(&def);
	}
	
};

class Paddle : public PhysicsEntity {
	static constexpr int points[10] = {
		0, 0,
		2, -4,
		24, -1,
		24, 1,
		2, 4,
	};

public:
	Paddle(ModulePhysics* physics, int x, int y, Texture2D& texture)
		: PhysicsEntity(physics->CreatePolygon(x, y, points, 10))
		, texture(texture)
	{}

	void Update() override {
		int x, y;
		body->GetPosition(x, y);
		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { (float)x, (float)y, (float)texture.width, (float)texture.height };
		Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
		DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, WHITE);
	}
private:
	Texture2D& texture;
};


class Flipper : public PhysicsEntity {
protected:
	const float MAX_MOTOR_TORQUE = 50.0f;
	const float LIFT_TORQUE = 35.0f;
	const float REST_TORQUE = 15.0f;
	const float LIMIT_ANGLE = 35.0f;

public:
	Flipper(ModulePhysics* physics, int x, int y, bool reverse)
		: PhysicsEntity(physics->CreateStaticRectangle(x, y, 2, 2))
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
		def.maxMotorTorque = MAX_MOTOR_TORQUE;
		joint = reinterpret_cast<b2RevoluteJoint*>(physics->CreateJoint(def));
	}

	void SetLifted(bool lift) {
		joint->SetMotorSpeed((lift ? -LIFT_TORQUE : REST_TORQUE) * (reversed ? -1.0f : 1.0f));
	}

private:
	ModulePhysics* physics;
	b2RevoluteJoint* joint;
	bool reversed;
};


class Bumper : public PhysicsEntity {
public:
	static constexpr float KICK_SPEED = 9.8f; // BOUNCE SPEED / fORCE

	Bumper(ModulePhysics* physics, Module* listener, int x, int y, int radius) : PhysicsEntity(physics->CreateStaticCircle(x, y, radius)), radius(radius)
	{
		body->listener = listener; 


	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

	}

	void Hit(PhysBody* ball)
	{
		
		b2Vec2 dir = ball->body->GetPosition() - body->body->GetPosition();
		dir.Normalize();
		ball->body->SetLinearVelocity(KICK_SPEED * dir);

	}




private:
	int radius;
};



ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled), leftFlipper(NULL), rightFlipper(NULL)
{

	points = 0;
}


ModuleGame::~ModuleGame()
{}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");

	boardTexture = LoadTexture("Assets/board.png");
	leftFlipperTexture = LoadTexture("Assets/flipper_left.png");
	rightFlipperTexture = LoadTexture("Assets/flipper_right.png");
	ballTexture = LoadTexture("Assets/ball.png");

	App->renderer->camera.x = App->renderer->camera.y = 0;

	float leftFlipperX = 56.0f;
	float rightFlipperX = 106.0f;
	float flippersY = 259.0f;
	float paddleOffset = 6.0f;

	float baseX = 150.0f;
	float baseY = 70.0f;
	float KickerX = 150.0f;
	float KickerY = 50.0f;

	leftFlipper = new Flipper(App->physics, leftFlipperX, flippersY, false);
	Paddle* paddle0 = new Paddle(App->physics, leftFlipperX - paddleOffset, flippersY, leftFlipperTexture);
	entities.push_back(leftFlipper);
	entities.push_back(paddle0);
	leftFlipper->CreateJoint(paddle0);

	rightFlipper = new Flipper(App->physics, rightFlipperX, flippersY, true);
	Paddle* paddle1 = new Paddle(App->physics, rightFlipperX - paddleOffset, flippersY, rightFlipperTexture);
	entities.push_back(rightFlipper);
	entities.push_back(paddle1);
	rightFlipper->CreateJoint(paddle1);

	Spring*springbase = new Spring(App->physics, baseX, baseY);
	Kicker* springKicker = new Kicker(App->physics, KickerX, KickerY);
	springbase->CreateJoint(springKicker,springbase);

	//Bouncer
	entities.push_back(new Bumper(App->physics, this, 24, 24, 24));
	entities.push_back(new  Bumper(App->physics, this, 22, 183, 10));
	entities.push_back(new  Bumper(App->physics, this, 138, 180, 10));


	return true;
}

// Unload assets
bool ModuleGame::CleanUp()
{
	LOG("Unloading Intro scene");

	UnloadTexture(boardTexture);
	UnloadTexture(leftFlipperTexture);
	UnloadTexture(rightFlipperTexture);
	UnloadTexture(ballTexture);

	DestroyAllEntities();

	return true;
}

// Update: draw background
update_status ModuleGame::Update()
{
	DrawTexture(boardTexture, 0, 0, RAYWHITE);

	for (PhysicsEntity* entity : entities)
	{
		entity->Update();

	}

	leftFlipper->SetLifted(IsKeyDown(KEY_LEFT));
	rightFlipper->SetLifted(IsKeyDown(KEY_RIGHT));

	if (IsKeyPressed(KEY_SPACE)) {
		short scale = App->renderer->scale;
		Rectangle& targetDest = App->renderer->targetDest;
		entities.push_back(new Ball(App->physics, std::min(std::max((GetMouseX() - targetDest.x) / scale, 0.0f), (float)GetScreenWidth()), std::min(std::max((GetMouseY() - targetDest.y) / scale, 0.0f), (float)GetScreenHeight()), ballTexture));
	}
	

	DrawText(TextFormat("Points: %d", points), 30, 30, 2, BLACK);

	return UPDATE_CONTINUE;
}

void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	for (PhysicsEntity* entity : entities)
	{
		Bumper* bumper = dynamic_cast<Bumper*>(entity);
		if (bumper != NULL && bumper->body == bodyA)
		{
			bumper->Hit(bodyB);
			points += 100;
		}
	}
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
