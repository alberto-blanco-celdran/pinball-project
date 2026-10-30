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
	{
	}

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
		: PhysicsEntity(physics->CreateCircle(x, y, 4))
		, texture(texture)
	{
	}

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
		: PhysicsEntity(physics->CreateRectangle(x, y, 5.5, 2))
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
		: PhysicsEntity(physics->CreateStaticRectangle(x, y, 5.5, 2)), physics(physics),
		joint(NULL),
		reversed(false)
	{
	};

	void Update() override {}
	void CreateJoint(Kicker* kicker, Spring* spring) {
		b2DistanceJointDef def;
		b2Body* spring_body = body->body;
		b2Body* kicker_body = kicker->body->body;
		def.Initialize(spring_body, kicker_body, spring_body->GetWorldCenter(), kicker_body->GetWorldCenter());

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
	{
	}

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
	const float LIFT_TORQUE = 45.0f;
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



// Chain abierta (NO cerrada: el primer y el ultimo punto no se unen, eso es un hueco)
static PhysBody* CreateOpenChain(ModulePhysics* physics, const int* points, int size)
{
	PhysBody* pbody = new PhysBody();

	b2BodyDef body;
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = physics->GetWorld()->CreateBody(&body);

	int count = size / 2;
	b2Vec2* vertices = new b2Vec2[count];
	for (int i = 0; i < count; ++i)
	{
		vertices[i].x = PIXEL_TO_METERS(points[i * 2 + 0]);
		vertices[i].y = PIXEL_TO_METERS(points[i * 2 + 1]);
	}

	b2Vec2 prev = vertices[0] + (vertices[0] - vertices[1]);
	b2Vec2 next = vertices[count - 1] + (vertices[count - 1] - vertices[count - 2]);

	b2ChainShape shape;
	shape.CreateChain(vertices, count, prev, next);
	delete[] vertices; // CreateChain copia los vertices

	b2FixtureDef fixture;
	fixture.shape = &shape;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

//Paredes del tablero.Cada pared es UNA chain(una linea hecha de puntos, en pixeles : x, y, x, y...)
// loop = false: chain abierta, el primer y el ultimo punto NO se unen (eso es un agujero)
// loop = true:  chain cerrada, el ultimo punto se une con el primero (islas, obstaculos...)
// El tamano del array se calcula solo, no hace falta contar los puntos.
class Wall : public PhysicsEntity {
public:
	template <int N>
	Wall(ModulePhysics* physics, const int(&points)[N], bool loop = false)
		: PhysicsEntity(loop ? physics->CreateChain(0, 0, points, N) : CreateOpenChain(physics, points, N))
	{
	}

	//Las paredes estan dibujadas en board.png
	void Update() override {}
};

// Parte izquierda del tablero: desde lo alto del arco hasta el lado izquierdo del agujero
static const int wall_left[] = {
	// Arco
	100, 6, 90, 5, 74, 6, 62, 8,
	50, 12, 43, 16, 37, 20, 32, 24,
	28, 28, 24, 32, 21, 36, 19, 40,
	16, 44, 14, 48, 13, 52, 11, 56,
	10, 60, 9, 64, 8, 72, 7, 80,
	// Pared izq
	6, 88, 6, 108, 8, 120, 9, 128,
	11, 136, 13, 144, 17, 152, 22, 160,
	24, 164, 24, 192, 20, 196, 12, 200,
	10, 204, 8, 208, 8, 244,
	// Embudo izq
	10, 248, 15, 252, 24, 256, 30, 260,
	36, 264, 42, 268, 48, 272, 54, 277,
};

// Parte derecha del tablero: desde el lado derecho del drenaje, subiendo por el tablero, muelle y el arco superior
static const int wall_right[] = {
	// Embudo derecho
	105, 277, 111, 272, 117, 268, 123, 264,
	129, 260, 135, 256, 144, 252, 149, 248,
	151, 244,
	// Pared derecha del tablero (cara izquierda del divisor)
	151, 208, 147, 200, 139, 196, 135, 192,
	135, 164, 137, 160, 142, 152, 146, 144,
	148, 136, 150, 128, 151, 120, 153, 108,
	153, 90, 153, 80, 152, 70, 150, 62,
	148, 54, 145, 46,
	// Punta del divisor y cara derecha (carril del muelle)
	148, 48, 151, 54, 155, 62, 158, 70,
	159, 78, 161, 86, 161, 277,
	// Fondo del carril y pared exterior
	175, 277, 175, 80, 174, 72, 172, 62,
	167, 50, 161, 41, 155, 35, 148, 27,
	136, 17, 124, 11, 112, 8,
	// Vuelta al principio del arco superior
	100, 6,
};

static const int big_island_right[] = {
	114, 34, 125, 45, 133, 57, 135, 69,
	138, 82, 138, 114, 134, 133, 132, 143,
	127, 151, 123, 154, 112, 148, 115, 137,
	116, 129, 111, 125, 91, 113, 91, 98,
	113, 85, 116, 83, 121, 76, 121, 68,
	119, 63, 114, 53
};


static const int big_island_left[]{
	49, 32,
	49, 54,
	42, 64,
	39, 72,
	43, 81,
	47, 84,
	73, 97,
	73, 113,
	53, 123,
	47, 126,
	44, 133,
	46, 139,
	47, 141,
	49, 149,
	39, 154,
	34, 149,
	26, 132,
	25, 122,
	23, 114,
	23, 82,
	24, 77,
	26, 68,
	30, 55,
	37, 43
};
//static const int up_island_right[]{};
//static const int up_island_left[]{};
//static const int lil_island_right[]{};
//static const int lil_island_left[]{};
//static const int long_island_right[]{};
//static const int long_island_left[]{};

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled), leftFlipper(NULL), rightFlipper(NULL)
{
	points = 0;
}


ModuleGame::~ModuleGame()
{
}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");

	boardTexture = LoadTexture("Assets/board.png");
	leftFlipperTexture = LoadTexture("Assets/flipper_left.png");
	rightFlipperTexture = LoadTexture("Assets/flipper_right.png");
	ballTexture = LoadTexture("Assets/ball.png");

	App->renderer->camera.x = App->renderer->camera.y = 0;

	// Walls of the table (the hole between them is the drain)
	entities.push_back(new Wall(App->physics, wall_left));
	entities.push_back(new Wall(App->physics, wall_right));
	entities.push_back(new Wall(App->physics, big_island_right));
	entities.push_back(new Wall(App->physics, big_island_left));

	float leftFlipperX = 56.0f;
	float rightFlipperX = 106.0f;
	float flippersY = 259.0f;
	float paddleOffset = 6.0f;

	float baseX = 168.0f;
	float baseY = 260.0f;
	float KickerX = 168.0f;
	float KickerY = 270.0f;

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

	Spring* springbase = new Spring(App->physics, baseX, baseY);
	Kicker* springKicker = new Kicker(App->physics, KickerX, KickerY);
	springbase->CreateJoint(springKicker, springbase);

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

	DrawText(TextFormat("%d", points), 63, 177, 5, BLACK);

	bool extraBall = false;

	if (extraBall) {

		//life += 1;
		extraBall = false;

	}

	//if (life == 0 || life < 0) {
	//	points = 0;
	//}

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
			if (points > 100000) {
				points = points;
			}
			else {
				points += 100;
			}
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