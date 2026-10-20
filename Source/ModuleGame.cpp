#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModulePhysics.h"

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{}

ModuleGame::~ModuleGame()
{}

class Circle : public PhysicsEntity
{
public:
	Circle(ModulePhysics* physics, int _x, int _y, Texture2D _texture)
		: PhysicsEntity(physics->CreateCircle(_x, _y, 25))
		, texture(_texture)
	{
	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

		// Red when the mouse is inside the body
		Color tint = body->Contains(GetMouseX(), GetMouseY()) ? RED : WHITE;

		// Box2D gives the center of the body, so draw the texture around its center
		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { (float)x, (float)y, (float)texture.width, (float)texture.height };
		Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
		DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, tint);
	}

private:
	Texture2D texture;
};


class Box : public PhysicsEntity
{
public:
	Box(ModulePhysics* physics, int _x, int _y)
		: PhysicsEntity(physics->CreateStaticRectangle(_x, _y, half_width, half_height))
	{
	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

		DrawRectangle(x - half_width, y - half_height, half_width * 2, half_height * 2, WHITE);
	}

private:
	static constexpr int half_width = 50;
	static constexpr int half_height = 12;
};


// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");

	App->renderer->camera.x = App->renderer->camera.y = 0;

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

	if (IsKeyPressed(KEY_ONE))
	{
		Circle* c = new Circle(App->physics, GetMouseX(), GetMouseY(), circle);
		c->body->listener = this;
		entities.push_back(c);
	}

	if (IsKeyPressed(KEY_TWO))
	{
		Box* b = new Box(App->physics, GetMouseX(), GetMouseY());
		b->body->listener = this;
		entities.push_back(b);
	}

	return UPDATE_CONTINUE;
}

void ModuleGame::OnCollision(PhysicsBody* bodyA, PhysicsBody* bodyB)
{}

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

