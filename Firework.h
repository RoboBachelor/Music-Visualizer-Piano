#pragma once

#include <GL/freeglut.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <list>
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif
#define TIME_INC 16

int getRand(int maxN) {
	return rand() % (maxN + 1);
}

float getRandf(float max) {
	return max * getRand(1000) / 1000;
}

typedef struct {
	float r;       // a fraction between 0 and 1
	float g;       // a fraction between 0 and 1
	float b;       // a fraction between 0 and 1
} RGB_t;

typedef struct {
	float h;       // angle in degrees
	float s;       // a fraction between 0 and 1
	float v;       // a fraction between 0 and 1
} HSV_t;

RGB_t hsv2rgb(HSV_t in)
{
	float hh, p, q, t, ff;
	int i;
	RGB_t out;

	if (in.s <= 0.0) {       // < is bogus, just shuts up warnings
		out.r = in.v;
		out.g = in.v;
		out.b = in.v;
		return out;
	}
	hh = in.h;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0 - in.s);
	q = in.v * (1.0 - (in.s * ff));
	t = in.v * (1.0 - (in.s * (1.0 - ff)));

	switch (i) {
	case 0:
		out.r = in.v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.v;
		break;
	case 5:
	default:
		out.r = in.v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}


class Ball {
public:
	float cx = 0, cy = 0;	// Coordinate of center
	float radius = 4.5;		// Radius
	float vx = 0, vy = 0;	// Velocity, pixels per second.
	HSV_t hsv = { 0 };		// Color in HSV color space
	float luminanceFactor = 1.f;	// Luminance exponential decay factor
	float alpha = 1.f;				// Luminance
	bool deleted = false;	// Lazy delete flag
	bool gradientColor = false;

	// Color for matrix of balls. v[ORANGE]v						v[PURPLE]v
	HSV_t positiveColor = { 30.f, 1.f, 1.f }, negativeColor = { 285.f, 0.5f, 1.f };
	// Weight value for matrix of balls.
	float weight = 0.f;
	int samplingNumber = 16;

	void setCenter(float cx, float cy);
	void setRadius(float radius);
	void setColor(float h, float s, float v);
	void setVelocity(float vx, float vy);
	void setLuminanceFactor(float factor);
	void nextState(void);
	void draw(void);
	void drawForMatrix(void);

};

void Ball::setLuminanceFactor(float factor) {
	luminanceFactor = powf(factor, TIME_INC / 1000.f);
}

void Ball::setCenter(float cx, float cy) {
	this->cx = cx;
	this->cy = cy;
}

void Ball::setRadius(float r) {
	this->radius = r;
}

void Ball::setColor(float h, float s, float v) {
	while (h > 360.f) {
		h -= 360.f;
	}
	while (h < 0.f) {
		h += 360.f;
	}

	this->hsv.h = h;
	this->hsv.s = s;
	this->hsv.v = v;
}

void Ball::setVelocity(float vx, float vy) {
	this->vx = vx;
	this->vy = vy;
}

void Ball::nextState() {
	if (deleted)
		return;

	vx += -2.5f + getRandf(5.f);
	vy += -2.5f + getRandf(5.f);

	cx += vx * TIME_INC / 1000;
	cy += vy * TIME_INC / 1000;

	// Gravity acceleration is 500 pixels / s^2.
	vy -= 500 * TIME_INC / 1000;

	float v_square = vx * vx + vy * vy;
	float resistance = 3.f * v_square * TIME_INC / 1000;
	vx -= resistance * vx / v_square;
	vy -= resistance * vy / v_square;

	// luminance(t) = 0.6 ^ t, where t in seconds
	// Luminance exponential decay function
	alpha *= luminanceFactor;

	if (alpha < 0.01)
		deleted = true;

	return;
}

void Ball::draw() {
	if (deleted)
		return;

	// Convert HSV colorspace to RGB values
	RGB_t rgb = hsv2rgb(this->hsv);

	glBegin(GL_POLYGON);
	glColor4f(rgb.r, rgb.g, rgb.b, alpha);
	// Draw the circle
	for (int i = 0; i <= samplingNumber; ++i) {
		glVertex2f(cx + radius * cosf(2 * M_PI * i / samplingNumber),
			cy + radius * sinf(2 * M_PI * i / samplingNumber));
	}

	glEnd();
}

void Ball::drawForMatrix() {

	// Convert HSV colorspace to RGB values
	RGB_t rgb;
	if (weight > 0) {
		rgb = hsv2rgb(positiveColor);
	}
	else {
		rgb = hsv2rgb(negativeColor);
	}

	glBegin(GL_POLYGON);
	glColor4f(rgb.r, rgb.g, rgb.b, weight * (weight > 0 ? 1 : -1));
	glVertex2f(cx, cy);
	glColor4f(rgb.r, rgb.g, rgb.b, 0.4f * weight * (weight > 0 ? 1 : -1));
	// Draw the circle
	for (int i = 0; i <= samplingNumber; ++i) {
		glVertex2f(cx + radius * cosf(2 * M_PI * i / samplingNumber),
			cy + radius * sinf(2 * M_PI * i / samplingNumber));
	}

	glEnd();
	return;

	glColor4f(rgb.r, rgb.g, rgb.b, weight * (weight > 0 ? 1 : -1));
	glPushMatrix();
	glTranslatef(cx, cy, 0);
	gluSphere(gluNewQuadric(), radius, 100, 20);
	glPopMatrix();
	return;


}


class Firework {
public:
	int numBalls = 360;
	float cx, cy;
	float maxSpeed = 700;
	float centerHue = 30;
	float timeToLive = 6000;
	std::vector<Ball> balls;

	Firework(float cx, float cy, float maxSpeed, float centerHue);
	bool nextState();
	void draw();
};

Firework::Firework(float cx, float cy, float maxSpeed, float centerHue) {
	this->cx = cx;
	this->cy = cy;
	this->maxSpeed = maxSpeed;
	this->centerHue = centerHue;

	Ball ball;
	for (int i = 0; i < numBalls; ++i) {
		float speed = maxSpeed * getRandf(1.f);
		ball.setCenter(cx, cy);
		ball.setRadius(0.5f + getRandf(3.f));
		ball.setColor(centerHue - 10 + getRandf(20), 1, 1);
		ball.setVelocity(speed * cos(i * M_PI / 180), speed * sin(i * M_PI / 180));
		ball.setLuminanceFactor(0.2 + getRand(600) / 1000.f);
		balls.push_back(ball);
	}

}

bool Firework::nextState() {
	timeToLive -= TIME_INC;
	if (timeToLive < 0) {
		return false;
	}
	for (int i = 0; i < numBalls; ++i) {
		balls[i].nextState();
	}
	return true;
}

void Firework::draw() {
	for (int i = 0; i < numBalls; ++i) {
		balls[i].draw();
	}
}

