#pragma once

#define FREEGLUT_STATIC
#include <GL/freeglut.h>
#include "Piano.h"

typedef enum {
	SNOWBALL_FLYING = 0,
	SNOWBALL_GROUND,
} snowball_mode_t;

class SnowBall {
public:
	float cx = 0, lowerHeight = 500.f, cz = 0;	// Coordinate of center
	float vx = 0, vy = 0, vz = 0;	// Velocity, per second
	float radius = 25.f;			// Radius
	float period = 16.f;
	snowball_mode_t mode = SNOWBALL_FLYING;

	float keyAccel = 30.f;
	float airResistance = 1.0f;
	float groundResistance = 4.f;

	Piano* piano;

	void setPiano(Piano* piano) {
		this->piano = piano;
	}

	void setPosition(float cx, float height, float cz) {
		this->cx = cx;
		lowerHeight = height;
		this->cz = cz;
	}

	void setRadius(float radius) {
		this->radius = radius;
	}

	void update(bool keyStatus[]) {

		//vx += -2.5f + getRandf(5.f);
		//vy += -2.5f + getRandf(5.f);

		// Get current position
		cx += vx * period / 1000;
		lowerHeight += vy * period / 1000;
		cz += vz * period / 1000;

		// Interative with the piano

		piano_interactive_t result = piano->interactive(cx, cz, lowerHeight);
		if (result.crashed) {
			lowerHeight = result.height;
			vy = result.vHeight;
			mode = SNOWBALL_GROUND;
		}
		else if (lowerHeight < 0) {
			lowerHeight = 0;
			vy = 0;
			mode = SNOWBALL_GROUND;
		}

		// Gravity acceleration.
		vy -= 1500 * period / 1000;

		vx += keyStatus[3] ? keyAccel : 0;
		vx -= keyStatus[2] ? keyAccel : 0;
		vz -= keyStatus[0] ? keyAccel : 0;
		vz += keyStatus[1] ? keyAccel : 0;

		if (mode == SNOWBALL_GROUND && !(vx == 0 && vz == 0)) {
			float v_square = vx * vx + vz * vz;
			float resistance = groundResistance * v_square * period / 1000;
			vx -= resistance * vx / v_square;
			vz -= resistance * vz / v_square;
		}

		if (mode == SNOWBALL_FLYING && !(vx == 0 && vz == 0 && vy == 0)) {
			float v_square = vx * vx + vy * vy + vz * vz;
			float resistance = airResistance * v_square * period / 1000;
			vx -= resistance * vx / v_square;
			vy -= resistance * vy / v_square;
			vz -= resistance * vz / v_square;
		}
	}

	void draw(void) {
		glPushMatrix();
		glColor3ub(240, 240, 240);
		glTranslatef(cx, lowerHeight + radius, cz);
		GLUquadric* quad = gluNewQuadric();
		gluSphere(quad, radius, 100, 20);
		glPopMatrix();
	}
};
