#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

#include <algorithm>
#include <iostream>

// Base class for everything in the scene that has a physics body.
class PhysicsEntity
{
protected:

	PhysicsEntity(PhysBody* _body)
		: body(_body), markedForDeletion(false)
	{
		_body->owner = this;
	}

public:
	virtual ~PhysicsEntity() = default;

	// Draw the entity at the position of its body
	virtual void Update() = 0;

	PhysBody* body;
	bool markedForDeletion;
};


class Ball : public PhysicsEntity {
public:
	Ball(ModulePhysics* physics, int x, int y, Texture2D& texture)
		: PhysicsEntity(physics->CreateCircle(x, y, 4)), texture(texture)
		
	{
		b2Filter filter = b2Filter();
		filter.categoryBits = 0x0002;
		for (b2Fixture* fixture = body->body->GetFixtureList(); fixture; fixture = fixture->GetNext())
		{
			fixture->SetFilterData(filter);
		}
		body->body->SetBullet(true); // increase collisions precision
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
protected:
	const float CHARGE_TIME = 0.8f;
	const float MAX_FORCE = 1600.0f;

private:
	float chargeTime = 0.0f;

public:
	Kicker(ModulePhysics* physics, ModuleAudio* audio, int x, int y, uint32 sound)
		: PhysicsEntity(physics->CreateRectangle(x, y, 6, 2))
		, pressed(false), Audio(audio), Sound(sound)
	{
		body->body->SetFixedRotation(true);
	}

	void Update() override
	{
		if (!pressed) {
			if (chargeTime > 0.0f) {
				Audio->PlayFx(Sound);
				body->body->ApplyForceToCenter(b2Vec2 { 0, 0 }, true); // HACK: sometimes it gets stuck (???)
			}
			chargeTime = 0.0f;
			return;
		}
		chargeTime += GetFrameTime();
		body->body->ApplyForceToCenter(b2Vec2{ 0.0f, MAX_FORCE * std::min(chargeTime / CHARGE_TIME, 1.0f)}, true);
	}

	bool pressed;
	ModuleAudio* Audio;
	uint32 Sound;
};


class Spring : public PhysicsEntity {
private:
	ModulePhysics* physics;
	b2DistanceJoint* joint;
public:
	Spring(ModulePhysics* physics, int x, int y)
		: PhysicsEntity(physics->CreateStaticRectangle(x, y, 6, 2)), physics(physics),
		joint(NULL)
	{};

	void Update() override {}

	void CreateJoint(Kicker* kicker) {
		b2Body* springBody = body->body;
		b2Body* kickerBody = kicker->body->body;
		b2DistanceJointDef def;
		def.Initialize(springBody, kickerBody, springBody->GetWorldCenter(), kickerBody->GetWorldCenter());
		def.collideConnected = true;
		def.minLength = 0.3f;
		def.maxLength = 1.3f;
		def.length = def.maxLength;
		b2LinearStiffness(def.stiffness, def.damping, 25.0f, 0.2f, def.bodyA, def.bodyB);
		joint = reinterpret_cast<b2DistanceJoint*>(physics->CreateJoint(def));
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
		b2Filter filter = b2Filter();
		filter.maskBits = 0x0002; // only collide with the ball
		for (b2Fixture* fixture = body->body->GetFixtureList(); fixture; fixture = fixture->GetNext())
		{
			fixture->SetFilterData(filter);
		}
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
	static constexpr float MAX_MOTOR_TORQUE = 50.0f;
	static constexpr float LIFT_TORQUE = 45.0f;
	static constexpr float REST_TORQUE = 15.0f;
	static constexpr float LIMIT_ANGLE = 35.0f;

public:
	Flipper(ModulePhysics* physics, ModuleAudio* audio, int x, int y, bool reverse, uint sound)
		: PhysicsEntity(physics->CreateStaticRectangle(x, y, 2, 2))
		, physics(physics), joint(NULL), reversed(reverse), Audio(audio), Sound(sound), lifted(false)
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
		if (lifted != lift) {
			lifted = lift;
			if (lift) {
				Audio->PlayFx(Sound);
			}
		}

		joint->SetMotorSpeed((lift ? -LIFT_TORQUE : REST_TORQUE) * (reversed ? -1.0f : 1.0f));
	}

private:
	ModulePhysics* physics;
	b2RevoluteJoint* joint;
	ModuleAudio* Audio;
	uint32 Sound;
	bool reversed;
	bool lifted;
};


class Bumper : public PhysicsEntity {
protected:
	static constexpr float KICK_SPEED = 7.5f; // BOUNCE SPEED / FORCE
	static constexpr float HIT_TIME = 0.1f;
	float hitTimer;

public:
	Bumper(ModulePhysics* physics, Module* listener, int x, int y, int radius) 
		: PhysicsEntity(physics->CreateStaticCircle(x, y, radius))
		, hitTimer(0.0f)
	{
		body->listener = listener;
	}

	void Update() override
	{
		hitTimer = std::max(hitTimer - GetFrameTime(), 0.0f);
	}

	void Hit(PhysBody* ball)
	{
		b2Vec2 dir = ball->body->GetPosition() - body->body->GetPosition();
		dir.Normalize();

		ball->body->SetLinearVelocity(KICK_SPEED * dir);
		hitTimer = HIT_TIME;
	}
};


class KibyBumper : public Bumper {
public:
	KibyBumper(ModulePhysics* physics, Module* listener, int x, int y, int radius, Texture2D& texture, Texture2D& hitTexture)
		: Bumper(physics, listener, x, y, radius)
		, texture(texture), hitTexture(hitTexture)
	{};

	void Update() override
	{
		Bumper::Update(); // update timer

		// draw kiby sprite
		int x, y;
		body->GetPosition(x, y);
		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { (float)x, (float)y, (float)texture.width, (float)texture.height };
		Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
		DrawTexturePro(hitTimer > 0.0f ? hitTexture : texture, source, dest, origin, body->GetRotation() * RAD2DEG, WHITE);
	}

private:
	Texture2D& texture;
	Texture2D& hitTexture;
};


class BallSensor : public PhysicsEntity {
public:
	BallSensor(ModulePhysics* physics, Module* listener, int x, int y, int w, int h)
		: PhysicsEntity(physics->CreateRectangleSensor(x, y, w, h))
	{
		body->listener = listener;
	};
	void Update() override {};
};

//Paredes del tablero.Cada pared es UNA chain(una linea hecha de puntos, en pixeles : x, y, x, y...)
// loop = false: chain abierta, el primer y el ultimo punto NO se unen (eso es un agujero)
// loop = true:  chain cerrada, el ultimo punto se une con el primero (islas, obstaculos...)
// El tamano del array se calcula solo, no hace falta contar los puntos.
class Wall_1 : public PhysicsEntity {
public:
	template <int N>
	Wall_1(ModulePhysics* physics, const int(&points)[N], bool loop = false)
		: PhysicsEntity(loop ? physics->CreateChain(0, 0, points, N) : physics->CreateOpenChain(0, 0, points, N))
	{
	}

	//Las paredes estan dibujadas en board.png
	void Update() override {}
};

class Wall_2 : public PhysicsEntity {
public:
	template <int N>
	Wall_2(ModulePhysics* physics, const int(&points)[N], bool loop = true)
		: PhysicsEntity(loop ? physics->CreateChain(0, 0, points, N) : physics->CreateOpenChain(0, 0, points, N))
	{
	}

	//Las paredes estan dibujadas en board.png
	void Update() override {}
};

//WALL 1
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

//WALL 2
static const int big_island_right[] = {
	114, 34, 125, 45, 133, 57, 135, 69,
	138, 82, 138, 114, 134, 133, 132, 143,
	127, 151, 123, 154, 112, 148, 115, 137,
	116, 129, 111, 125, 91, 113, 91, 98,
	113, 85, 116, 83, 121, 76, 121, 68,
	119, 63, 114, 53
};


static const int big_island_left[]{
	49, 32, 49, 54, 42, 64, 39, 72,
	43, 81, 47, 84, 73, 97, 73, 113,
	53, 123, 47, 126, 44, 133, 46, 139,
	47, 141, 49, 149, 39, 154, 34, 149,
	26, 132, 25, 122, 23, 114, 23, 82,
	24, 77, 26, 68, 30, 55, 37, 43
};


static const int long_island_left[]{
	141, 206, 140, 241,

	111, 261, 105, 252,

	133, 236, 136, 234,
	137, 231, 137, 206
};

static const int long_island_right[]{
	25, 207, 24, 230,
	25, 233, 29, 236,

	54, 252, 49, 259,

	22, 240, 21, 207
};

static const int up_island_right[]{
	72, 24, 72, 41, 66, 41, 66, 24
};

static const int up_island_left[]{
	96, 24, 96, 40, 90, 41, 90, 24
};

static const int lil_island_right[]{
	36, 209, 39, 210,52, 234, 49, 238, 39, 229, 37, 226
};
static const int lil_island_left[]{
	125, 209, 126, 228, 112, 237, 112, 233, 123, 209
};

ModuleGame::ModuleGame(Application* app, bool start_enabled)
	: Module(app, start_enabled)
	, leftFlipper(NULL), rightFlipper(NULL)
	, spawnPoint(Vector2 { 168, 250 })
	, lives(3), points(0), combo(0), previous(0)
{
}

ModuleGame::~ModuleGame()
{
}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");

	// load textures
	boardTexture = LoadTexture("Assets/board.png");
	leftFlipperTexture = LoadTexture("Assets/flipper_left.png");
	rightFlipperTexture = LoadTexture("Assets/flipper_right.png");
	ballTexture = LoadTexture("Assets/ball.png");
	bumperTexture = LoadTexture("Assets/kiby.png");
	bumperHitTexture = LoadTexture("Assets/kibyquemado.png");
	lifeTexture = LoadTexture("Assets/life.png");

	App->renderer->camera.x = App->renderer->camera.y = 0;

	//Audio
	
	KickerAudio = App->audio->LoadFx("Assets/Flicker.wav");
	DeathAudio = App->audio->LoadFx("Assets/Kicker.wav");
	BumperAudio = App->audio->LoadFx("Assets/Death.wav");
	FlickerAudio = App->audio->LoadFx("Assets/Bumper.wav");
	App->audio->LoadFx("Assets/bonus.wav"); //Sin este audio todo peta por alguna razon



	// WALL 1
	entities.push_back(new Wall_1(App->physics, wall_left));
	entities.push_back(new Wall_1(App->physics, wall_right));

	//WALL 2
	entities.push_back(new Wall_2(App->physics, big_island_right));
	entities.push_back(new Wall_2(App->physics, big_island_left));
	entities.push_back(new Wall_2(App->physics, up_island_right));
	entities.push_back(new Wall_2(App->physics, up_island_left));
	entities.push_back(new Wall_2(App->physics, lil_island_right));
	entities.push_back(new Wall_2(App->physics, lil_island_left));
	entities.push_back(new Wall_2(App->physics, long_island_right));
	entities.push_back(new Wall_2(App->physics, long_island_left));

	float leftFlipperX = 58.0f;
	float rightFlipperX = 103.0f;
	float flippersY = 259.0f;
	float paddleOffset = 6.0f;

	float springX = 168.0f;
	float springY = 288.0f;
	float kickerY = springY - 10.0f;

	// flippers
	leftFlipper = new Flipper(App->physics, App->audio, leftFlipperX, flippersY, false, FlickerAudio);
	Paddle* paddle0 = new Paddle(App->physics, leftFlipperX - paddleOffset, flippersY, leftFlipperTexture);
	entities.push_back(leftFlipper);
	entities.push_back(paddle0);
	leftFlipper->CreateJoint(paddle0);

	rightFlipper = new Flipper(App->physics, App->audio, rightFlipperX, flippersY, true, FlickerAudio);
	Paddle* paddle1 = new Paddle(App->physics, rightFlipperX - paddleOffset, flippersY, rightFlipperTexture);
	entities.push_back(rightFlipper);
	entities.push_back(paddle1);
	rightFlipper->CreateJoint(paddle1);

	// kicker
	Spring* spring = new Spring(App->physics, springX, springY);
	kicker = new Kicker(App->physics, App->audio, springX, kickerY, KickerAudio);
	entities.push_back(spring);
	entities.push_back(kicker);
	spring->CreateJoint(kicker);
	
	// bumpers
	entities.push_back(new KibyBumper(App->physics, this, 82, 55, 6, bumperTexture, bumperHitTexture));
	entities.push_back(new KibyBumper(App->physics, this, 62, 75, 6, bumperTexture, bumperHitTexture));
	entities.push_back(new KibyBumper(App->physics, this, 102, 75, 6, bumperTexture, bumperHitTexture));
	entities.push_back(new KibyBumper(App->physics, this, 81, 142, 6, bumperTexture, bumperHitTexture));

	entities.push_back(new  Bumper(App->physics, this, 22, 183, 10));
	entities.push_back(new  Bumper(App->physics, this, 138, 180, 10));

	// sensor
	entities.push_back(new BallSensor(App->physics, this, 0, SCREEN_HEIGHT + 24, SCREEN_WIDTH, 16));

	// ball
	ball = new Ball(App->physics, spawnPoint.x, spawnPoint.y, ballTexture);
	entities.push_back(ball);

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
	UnloadTexture(bumperTexture);
	UnloadTexture(bumperHitTexture);

	DestroyAllEntities();

	return true;
}

// Update: draw background
update_status ModuleGame::Update()
{
	DrawTexture(boardTexture, 0, 0, RAYWHITE);

	// Score and lives
	for (int i = 1; i <= lives; i++) {
		DrawTexture(lifeTexture, 60 + 8 * (i - 1), 186, WHITE);
	}
	DrawText(TextFormat("%d", points), 63, 173, 5, Color{ 72, 75, 82, 255 });

	// Update
	for (PhysicsEntity* entity : entities)
	{
		if (entity->markedForDeletion) {
			DestroyEntity(entity);
			continue;
		}
		entity->Update();
	}

	// Game over
	if (lives < 0) {
		if (IsKeyDown(KEY_ENTER)) {
			previous = points;
			points = 0;
			lives = 3;
			return UPDATE_CONTINUE;
		}

		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{ 0, 0, 0, 200 });
		DrawText("GAME OVER", 35, 112, 20, RED);
		DrawText(TextFormat("PREVIOUS SCORE: %d", previous), 35, 144, 5, DARKGRAY);
		DrawText(TextFormat("YOUR SCORE: %d", points), 35, 156, 5, WHITE);
		
		Color highscoreCol = GRAY;
		if (points >= highscore && highscore > 0) {
			highscoreCol = GOLD;
			DrawText("NEW HIGHSCORE!", 50, 192, 5, WHITE);
		}
		DrawText(TextFormat("HIGHSCORE: %d", highscore), 35, 168, 5, highscoreCol);

		DrawText("PRESS [ENTER] TO TRY AGAIN", 16, 216, 5, YELLOW);
		return UPDATE_CONTINUE;
	}

	// Ball resetting
	if (ballResetQueued) {
		ball->body->body->SetAngularVelocity(0.0f);
		ball->body->body->SetLinearVelocity(b2Vec2{ 0, 0 });
		ball->body->body->SetTransform(b2Vec2{ PIXEL_TO_METERS(spawnPoint.x), PIXEL_TO_METERS(spawnPoint.y) }, 0.0f);
		ballResetQueued = false;
	}

	// Input
	leftFlipper->SetLifted(IsKeyDown(KEY_LEFT));
	rightFlipper->SetLifted(IsKeyDown(KEY_RIGHT));
	kicker->pressed = IsKeyDown(KEY_DOWN);

	if (App->physics->debug && IsKeyPressed(KEY_ONE)) {
		Vector2 mousePosition = App->renderer->GetMousePosition();
		entities.push_back(new Ball(App->physics, mousePosition.x, mousePosition.y, ballTexture));
	}





	return UPDATE_CONTINUE;
}

void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	if (Bumper* bumper = dynamic_cast<Bumper*>(bodyA->owner)) {
		if (bumper->body != bodyA) {
			return;
		}
		App->audio->PlayFx(BumperAudio);
		bumper->Hit(bodyB);

		// HACK: this shouldn't go here, but it will for now
		int bonus = 100;
		points = std::min(points + bonus, 999999);
		highscore = std::max(points, highscore); // update highscore if current score is greater
		combo += bonus;
		if (combo >= 1000) {
			lives = std::min(lives + 1, 5);
			combo = 0;
		}
		return;
	}

	if (BallSensor* sensor = dynamic_cast<BallSensor*>(bodyA->owner)) {
		if (sensor->body != bodyA) {
			return;
		}

		// destroy additional balls
		if (Ball* cheatBall = dynamic_cast<Ball*>(bodyB->owner)) {
			if (cheatBall != ball) {
				cheatBall->markedForDeletion = true;
				return;
			}
		}

		// HACK: this shouldn't go here, but it will for now
		ballResetQueued = true;
		App->audio->PlayFx(DeathAudio);
		lives -= 1;
		combo = 0;
		return;
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