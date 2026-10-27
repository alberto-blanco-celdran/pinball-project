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
	return UPDATE_CONTINUE;
}

// Update: debug camera
update_status ModuleRender::Update()
{
	// NOTE: This function setups render batching system for
	// maximum performance, all consecutive Draw() calls are
	// not processed until EndDrawing() is called
	BeginTextureMode(target);

	return UPDATE_CONTINUE;
}

// PostUpdate present buffer to screen
update_status ModuleRender::PostUpdate()
{
    // Draw everything in our batch!
    DrawFPS(10, 10);

	EndTextureMode(); // game has rendered -- end texture mode

	// render scaled viewport
	short scnW = GetScreenWidth();
	short scnH = GetScreenHeight();
	short resScale = (scnW < scnH) ? scnW / targetSrc.width : scnH / targetSrc.height;
	targetDest.width = targetSrc.width * resScale;
	targetDest.height = targetSrc.height * resScale;
	targetDest.x = scnW / 2 - targetDest.width / 2;
	targetDest.y = scnH / 2 - targetDest.height / 2;

	BeginDrawing();
	ClearBackground(background);
	DrawTexturePro(target.texture, targetSrc, targetDest, targetPos, 0.0f, WHITE);
	EndDrawing();

	return UPDATE_CONTINUE;
}

// Called before quitting
bool ModuleRender::CleanUp()
{
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
