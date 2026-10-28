#include "Globals.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRender.h"
#include <math.h>
#include <iostream>

ModuleRender::ModuleRender(Application* app, bool start_enabled) : Module(app, start_enabled)
{
    background = RAYWHITE;
}

// Destructor
ModuleRender::~ModuleRender()
{}

// Called before render is available
bool ModuleRender::Init()
{
	LOG("Creating Renderer context");
	
    int width = App->window->GetWidth();
    int height = App->window->GetHeight();
	
    target = LoadRenderTexture(width, height);
    targetSrc = Rectangle{ 0, 0, (float)width, -(float)height };

	return true;
}

// PreUpdate: clear buffer
update_status ModuleRender::PreUpdate()
{
	// NOTE: Draw the game into the render target so we can scale it properly later!
	BeginTextureMode(target);
	ClearBackground(background);

	return UPDATE_CONTINUE;
}

// Update: debug camera
update_status ModuleRender::Update()
{
	return UPDATE_CONTINUE;
}

// PostUpdate present buffer to screen
update_status ModuleRender::PostUpdate()
{
    // draw FPS counter on top of everything
    DrawFPS(10, 10);

	// game has rendered -- end texture mode
	EndTextureMode();

	// scale viewport
	int targetW = App->window->GetWidth();
	int targetH = App->window->GetHeight();
	short scnW = GetScreenWidth();
	short scnH = GetScreenHeight();
	scale = (scnW < scnH) ? scnW / targetW : scnH / targetH;
	targetDest.width = targetW * scale;
	targetDest.height = targetH * scale;
	targetDest.x = scnW / 2 - targetDest.width / 2;
	targetDest.y = scnH / 2 - targetDest.height / 2;

	// render viewport
	BeginDrawing();
	ClearBackground(BLACK);
	DrawTexturePro(target.texture, targetSrc, targetDest, targetPos, 0.0f, WHITE);
	EndDrawing();

	return UPDATE_CONTINUE;
}

// Called before quitting
bool ModuleRender::CleanUp()
{
	UnloadRenderTexture(target); // cleanup render target
	return true;
}

void ModuleRender::SetBackgroundColor(Color color)
{
	background = color;
}

// Draw to screen
bool ModuleRender::Draw(Texture2D texture, int x, int y, const Rectangle* section, double angle, int pivot_x, int pivot_y) const
{
	bool ret = true;

	float scale = 1.0f;
    Vector2 position = { (float)x, (float)y };
    Rectangle rect = { 0.f, 0.f, (float)texture.width, (float)texture.height };

    if (section != NULL) rect = *section;

    position.x = (float)(x-pivot_x) * scale + camera.x;
    position.y = (float)(y-pivot_y) * scale + camera.y;

	rect.width *= scale;
	rect.height *= scale;

    DrawTextureRec(texture, rect, position, WHITE);

	return ret;
}

bool ModuleRender::DrawText(const char * text, int x, int y, Font font, int spacing, Color tint) const
{
    bool ret = true;

    Vector2 position = { (float)x, (float)y };

    DrawTextEx(font, text, position, (float)font.baseSize, (float)spacing, tint);

    return ret;
}
