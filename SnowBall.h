#pragma once

#define FREEGLUT_STATIC
#include <GL/freeglut.h>
#include "Piano.h"
#include "texture.h"

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

	float keyAccel = 20.f;
	float airResistance = 1.9f;
	float goundVMax = 400.f;
	float goundVFactor = expf(-1 / 8.f);
	float vJumping = 1250.f;

	texture_t snowTexture;
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

	void loadSnow(const char imgPath[]) {
		loadBmp(imgPath, &snowTexture);
		loadTexture(&snowTexture);
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

		// Space key is pressed, Jump!
		if (keyStatus[4] && mode == SNOWBALL_GROUND) {
			vy += vJumping;
			mode = SNOWBALL_FLYING;
		}
		if (vy > 0) {
			mode = SNOWBALL_FLYING;
		}

		// Gravity acceleration.
		vy -= 2500 * period / 1000;

		if (mode == SNOWBALL_GROUND || mode == SNOWBALL_FLYING) {
			float vxs = 0, vzs = 0;
			vxs += keyStatus[3] ? goundVMax : 0;
			vxs -= keyStatus[2] ? goundVMax : 0;
			vzs -= keyStatus[0] ? goundVMax : 0;
			vzs += keyStatus[1] ? goundVMax : 0;

			vx = goundVFactor * vx + (1 - goundVFactor) * vxs;
			vz = goundVFactor * vz + (1 - goundVFactor) * vzs;
		}

		else if (mode == SNOWBALL_FLYING) {
			vx += keyStatus[3] ? keyAccel : 0;
			vx -= keyStatus[2] ? keyAccel : 0;
			vz -= keyStatus[0] ? keyAccel : 0;
			vz += keyStatus[1] ? keyAccel : 0;

			if (!(vx == 0 && vz == 0 && vy == 0)) {
				float v_square = vx * vx + vy * vy + vz * vz;
				float resistance = airResistance * v_square * period / 1000;
				vx -= resistance * vx / v_square;
				vy -= resistance * vy / v_square;
				vz -= resistance * vz / v_square;
			}
		}
	}

	void draw(void) {
		glPushMatrix();
		glColor3ub(255, 255, 255);
		glTranslatef(cx, lowerHeight + radius, cz);
		glRotatef(90, 1, 0, 0);
		glRotatef(90, 0, 1, 0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, snowTexture.textureId);
		GLUquadric* quadricObj = gluNewQuadric(); // Create a quadric surface object
		gluQuadricTexture(quadricObj, GL_TRUE); // Set texture mode to true
		gluSphere(quadricObj, radius, 100, 20);
		gluDeleteQuadric(quadricObj); // object must be deleted or it will be created every call of the
		glDisable(GL_TEXTURE_2D);
		glPopMatrix();

		glBegin(GL_POLYGON);
		glColor4f(0.5, 0.5, 0.5, 0.8);
		glVertex3f(cx, 0.1f, cz);
		glColor4f(0.5, 0.5, 0.5, 0.4);
		// Draw the circle
		int samplingNumber = 50;
		for (int i = 0; i <= samplingNumber; ++i) {
			glVertex3f(cx + radius * cosf(2 * PI * i / samplingNumber), 0.1f,
				cz + radius * sinf(2 * PI * i / samplingNumber));
		}

		glEnd();

	}

	~SnowBall() {
		;
	}

};
