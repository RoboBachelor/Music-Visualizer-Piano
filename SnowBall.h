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

	float keyAccel = 30.f;
	float airResistance = 1.0f;
	float goundPMax = 500000.f;
	float goundFFactor = 1000.f;
	float maxAccel = 2000.f;

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

		// Gravity acceleration.
		vy -= 1500 * period / 1000;

		if (mode == SNOWBALL_GROUND) {
			float pwrx = 0, pwrz = 0;
			pwrx += keyStatus[3] ? goundPMax : 0;
			pwrx -= keyStatus[2] ? goundPMax : 0;
			pwrz -= keyStatus[0] ? goundPMax : 0;
			pwrz += keyStatus[1] ? goundPMax : 0;

			float v_mag = sqrtf(vx * vx + vz * vz);
			float fx = 0, fz = 0;
			if (v_mag != 0) {
				fx = -vx / v_mag;
				fz = -vz / v_mag;
			}
			float ax = (vx == 0 ? pwrx : pwrx / abs(vx));
			if (goundFFactor * fx > 0 && goundFFactor * fx - vx > 0) {
				ax -= vx;
			}
			else if (goundFFactor * fx < 0 && goundFFactor * fx - vx < 0) {
				ax -= vx;
			}
			else {
				ax += goundFFactor * fx;
			}
			float az = (vz == 0 ? pwrz : pwrz / abs(vz));
			if (goundFFactor * fz > 0 && goundFFactor * fz - vz > 0) {
				az -= vz;
			}
			else if (goundFFactor * fz < 0 && goundFFactor * fz - vz < 0) {
				az -= vz;
			}
			else {
				az += goundFFactor * fz;
			}
			if (ax > maxAccel) ax = maxAccel;
			if (ax < -maxAccel) ax = -maxAccel;
			if (az > maxAccel) az = maxAccel;
			if (az < -maxAccel) az = -maxAccel;
			vx += ax * period / 1000;
			vz += az * period / 1000;
		}

		if (mode == SNOWBALL_FLYING) {
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
	}

	~SnowBall() {
		;
	}

};
