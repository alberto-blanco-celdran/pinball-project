#include "Ball.h"

void Ball::Update()
{
	int x, y;
	body->GetPosition(x, y);

	// Red when the mouse is inside the body
	Color tint = body->Contains(GetMouseX(), GetMouseY()) ? RED : WHITE;

	// Box2D positions are the center of the body: draw the texture around its center
	Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
	Rectangle dest = { (float)x, (float)y, (float)texture.width, (float)texture.height };
	Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
	DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, tint);
}