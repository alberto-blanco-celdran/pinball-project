#include "PhysicsBody.h"

void PhysicsBody::GetPosition(int& x, int& y) const
{
	b2Vec2 pos = body->GetPosition();
	x = METERS_TO_PIXELS(pos.x);
	y = METERS_TO_PIXELS(pos.y);
}

float PhysicsBody::GetRotation() const
{
	return body->GetAngle();
}

bool PhysicsBody::Contains(int x, int y) const
{
	b2Vec2 p(PIXEL_TO_METERS(x), PIXEL_TO_METERS(y));

	// True if the point is inside ANY of the shapes of this body
	for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
	{
		if (fixture->GetShape()->TestPoint(body->GetTransform(), p))
			return true;
	}

	return false;
}

int PhysicsBody::RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const
{
	// No hit: return -1
	// Hit: fill normal_x and normal_y and return the distance between x1,y1 and the hit point
	int ret = -1;

	b2RayCastInput input;
	input.p1.Set(PIXEL_TO_METERS(x1), PIXEL_TO_METERS(y1));
	input.p2.Set(PIXEL_TO_METERS(x2), PIXEL_TO_METERS(y2));
	input.maxFraction = 1.0f;

	float closest = 1.0f;

	for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
	{
		// A chain has one child per edge
		for (int32 i = 0; i < fixture->GetShape()->GetChildCount(); ++i)
		{
			b2RayCastOutput output;
			if (fixture->GetShape()->RayCast(&output, input, body->GetTransform(), i) && output.fraction <= closest)
			{
				closest = output.fraction;

				float dx = (float)(x2 - x1);
				float dy = (float)(y2 - y1);
				ret = (int)(output.fraction * sqrtf(dx * dx + dy * dy));

				normal_x = output.normal.x;
				normal_y = output.normal.y;
			}
		}
	}

	return ret;
}

PhysicsBody* ModulePhysics::CreateCircle(int x, int y, int radius)
{
	PhysicsBody* pbody = new PhysicsBody();

	b2BodyDef body;
	body.type = b2_dynamicBody;
	body.position.Set(PIXEL_TO_METERS(x), PIXEL_TO_METERS(y));
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2CircleShape shape;
	shape.m_radius = PIXEL_TO_METERS(radius);

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.density = 1.0f;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}

PhysicsBody* ModulePhysics::CreateStaticRectangle(int x, int y, int half_width, int half_height) {

	PhysicsBody* pbody = new PhysicsBody();

	b2BodyDef body;
	body.type = b2_staticBody;
	body.position.Set(PIXEL_TO_METERS(x), PIXEL_TO_METERS(y));
	body.userData.pointer = reinterpret_cast<uintptr_t>(pbody);

	b2Body* b = world->CreateBody(&body);

	b2PolygonShape shape;
	shape.SetAsBox(PIXEL_TO_METERS(half_width), PIXEL_TO_METERS(half_height));

	b2FixtureDef fixture;
	fixture.shape = &shape;
	b->CreateFixture(&fixture);

	pbody->body = b;

	return pbody;
}
