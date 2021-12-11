#pragma once

#define FREEGLUT_STATIC
#define PI 3.14159265358979323846

#include <GL/freeglut.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <list>
#include <ctime>

#include "wav.h"

typedef enum {
	NORMAL_SHAPE = 0,
	L_SHAPE,
	INV_L_SHAPE,
	TU_SHAPE,
	BLACK_SHAPE,
} pianoKeyShape_t;

typedef struct {
	float x, z;
} point_t;

typedef struct {
	float height, vHeight;
	bool crashed;
} piano_interactive_t;

class PianoKey {
public:

	pianoKeyShape_t shape = NORMAL_SHAPE;
	float fL, f0, fH;	// Lower corner, center higher corner freq
	float cx, cz;
	float w = 60, l = 150;
	float ws = 20, ls = 60;
	uint8_t r = 127, g = 127, b = 127;
	float height = 0;
	float vh = 0;
	std::vector<point_t> pts;
	float smoothUp = 0.9, smoothDown = 0.04;

	void initBpf(float fl, float fp, float fh, uint32_t n_fft, uint32_t sampleFreq) {
		fL = fl * n_fft / sampleFreq;
		f0 = fp * n_fft / sampleFreq;
		fH = fh * n_fft / sampleFreq;
	}

	void initPosition(float x, float z, pianoKeyShape_t sh) {
		cx = x;
		cz = z;
		shape = sh;
		switch (shape) {
		case NORMAL_SHAPE:
			pts.push_back({ 0.f, 0.f });
			pts.push_back({ 0.f, l });
			pts.push_back({ w, l });
			pts.push_back({ w, 0.f });
			break;
		case L_SHAPE:
			pts.push_back({ 0.f, 0.f });
			pts.push_back({ 0.f, l });
			pts.push_back({ w, l });
			pts.push_back({ w, ls });
			pts.push_back({ w - ws, ls });
			pts.push_back({ w - ws, 0.f });
			break;
		case INV_L_SHAPE:
			pts.push_back({ ws, 0.f });
			pts.push_back({ ws, ls });
			pts.push_back({ 0.f, ls });
			pts.push_back({ 0.f, l });
			pts.push_back({ w, l });
			pts.push_back({ w, 0 });
			break;
		case TU_SHAPE:
			pts.push_back({ ws, 0.f });
			pts.push_back({ ws, ls });
			pts.push_back({ 0.f, ls });
			pts.push_back({ 0.f, l });

			pts.push_back({ w, l });
			pts.push_back({ w, ls });
			pts.push_back({ w - ws, ls });
			pts.push_back({ w - ws, 0.f });
			break;
		case BLACK_SHAPE:
			pts.push_back({ -ws, 0.f });
			pts.push_back({ -ws, ls });
			pts.push_back({ ws, ls });
			pts.push_back({ ws, 0.f });
			break;
		}

	}

	void setColor3b(uint8_t r, uint8_t g, uint8_t b) {
		this->r = r;
		this->g = g;
		this->b = b;
	}

	void initSize(float w, float l, float ws, float ls) {
		this->w = w;
		this->l = l;
		this->ws = ws;
		this->ls = ls;
	}

	float update(float mag[]) {
		float h_observed = 0.f;
		float last_height = height;
		uint32_t fld = ceilf(fL);
		uint32_t fhd = ceilf(fH);
		for (uint32_t i = fld; i < fhd; ++i) {
			if (i < f0) {
				h_observed += mag[i] * (i - fL) / (f0 - fL);
			}
			else {
				h_observed += mag[i] * (i - fH) / (f0 - fH);
			}
		}

		h_observed *= 3;

		if (h_observed > height)
			height = smoothUp * h_observed + (1 - smoothUp) * height;
		else
			height = smoothDown * h_observed + (1 - smoothDown) * height;
		vh = height - last_height;

		return height;
	}

	void draw() {
		glPushMatrix();
		glTranslatef(cx, 0, cz);
		glColor3ub(r, g, b);

		float w1, w2;
		switch (shape) {
		case L_SHAPE:
			w1 = 0;
			w2 = w - ws;
			break;
		case INV_L_SHAPE:
			w1 = ws;
			w2 = w;
			break;
		case TU_SHAPE:
			w1 = ws;
			w2 = w - ws;
			break;
		}

		switch (shape) {
		case NORMAL_SHAPE:
		case BLACK_SHAPE:
			glBegin(GL_QUADS);
			for (int i = 0; i < pts.size(); ++i) {
				glNormal3f(0, -1, 0);
				glVertex3f(pts[i].x, 0, pts[i].z);
			}
			for (int i = 0; i < pts.size(); ++i) {
				glNormal3f(0, 1, 0);
				glVertex3f(pts[i].x, height, pts[i].z);
			}
			glEnd();
			break;
		case L_SHAPE:
		case INV_L_SHAPE:
		case TU_SHAPE:
			glBegin(GL_QUADS);
			glNormal3f(0, -1, 0);
			glVertex3f(0, 0, ls);
			glVertex3f(0, 0, l);
			glVertex3f(w, 0, l);
			glVertex3f(w, 0, ls);

			glNormal3f(0, -1, 0);
			glVertex3f(w1, 0, 0);
			glVertex3f(w1, 0, ls);
			glVertex3f(w2, 0, ls);
			glVertex3f(w2, 0, 0);

			glNormal3f(0, 1, 0);
			glVertex3f(0, height, ls);
			glVertex3f(0, height, l);
			glVertex3f(w, height, l);
			glVertex3f(w, height, ls);

			glNormal3f(0, 1, 0);
			glVertex3f(w1, height, 0);
			glVertex3f(w1, height, ls);
			glVertex3f(w2, height, ls);
			glVertex3f(w2, height, 0);

			glEnd();
			break;
		}



		for (int i = 0; i < pts.size(); ++i) {
			int nextIndex = (i == pts.size() - 1) ? 0 : i + 1;
			glBegin(GL_POLYGON);
			float nx = pts[i].z - pts[nextIndex].z;
			float nz = pts[nextIndex].x - pts[i].x;
			float norm = sqrtf(nx * nx + nz * nz);
			nx /= norm; nz /= norm;
			glNormal3f(nx, 0, nz);
			glVertex3f(pts[i].x, 0, pts[i].z);
			glVertex3f(pts[nextIndex].x, 0, pts[nextIndex].z);
			glVertex3f(pts[nextIndex].x, height, pts[nextIndex].z);
			glVertex3f(pts[i].x, height, pts[i].z);
			glEnd();
		}
		glPopMatrix();
	}

};

class Piano {
public:
	PianoKey pianoKeys[36];

	float cx = -750, cz = -120;
	float w = 60, l = 180, ws = 20, ls = 60;
	float gapGrid = 5, gapGroup = 30;
	float wGrid = 70, wGroup = 520;

	int positions[12] = {0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6};
	int whiteIndexToKeyIndex[7] = { 0, 2, 4, 5, 7, 9, 11 };
	int blackIndexToKeyIndex[8] = { 0, 1, 3, 0, 6, 8, 10, 0 };

	pianoKeyShape_t shapes[12] = {
		L_SHAPE,		// C
		BLACK_SHAPE,	// C#
		TU_SHAPE,		// D
		BLACK_SHAPE,	// D#
		INV_L_SHAPE,	// E

		L_SHAPE,		// F
		BLACK_SHAPE,	// F#
		TU_SHAPE,		// G
		BLACK_SHAPE,	// G#
		TU_SHAPE,		// A
		BLACK_SHAPE,	// A#
		INV_L_SHAPE,	// B
	};


	void init(int n_fft, int sampleRate) {

		wGrid = w + 2 * gapGrid;
		wGroup = 7 * wGrid + gapGroup;

		for (int32_t i = 0; i < 36; ++i) {

			int keyIndex = i % 12, groupIndex = i / 12;
			float cx =
				positions[keyIndex] * wGrid +
				(shapes[keyIndex] == BLACK_SHAPE ? 0 : 5) +
				groupIndex * wGroup;

			// Set the location and shape for the piano key
			pianoKeys[i].initSize(w, l, ws, ls);
			pianoKeys[i].initPosition(cx, 0, shapes[keyIndex]);

			// Init the center and corner frequency for the band-pass filter
			pianoKeys[i].initBpf(
				440 * powf(2.f, (i - 10) / 12.f),
				440 * powf(2.f, (i - 9) / 12.f),
				440 * powf(2.f, (i - 8) / 12.f),
				n_fft, sampleRate);

			if (shapes[keyIndex] == BLACK_SHAPE)
				pianoKeys[i].setColor3b(50, 65, 125);
			else {
				pianoKeys[i].setColor3b(240, 240, 240);
			}
		}
	}

	void update(float mag[]) {
		for (uint32_t i = 0; i < 36; ++i) {
			pianoKeys[i].update(mag);
		}
	}

	void draw() {
		glPushMatrix();
		glTranslatef(cx, 0, cz);
		for (uint32_t i = 0; i < 36; ++i) {
			pianoKeys[i].draw();
		}
		glPopMatrix();
	}

	piano_interactive_t interactive(float px, float pz, float height) {
		piano_interactive_t ret;
		ret.crashed = false;

		// Out of the piano area, return false
		px -= cx;
		pz -= cz;
		if (px < 0 || px >= wGroup * 3 || pz < 0 || pz > l) {
			return ret;
		}

		// In the group gap, return false
		int groupIndex = floorf(px / wGroup);
		px -= wGroup * groupIndex;
		if (px >= wGrid * 7) {
			return ret;
		}

		int whiteIndex = floorf(px / wGrid);
		bool isBlockKey = false;
		int blackIndex;
		// May be on the black keys
		if (pz < ls) {
			blackIndex = roundf(px / wGrid);
			if (blackIndexToKeyIndex[blackIndex]) {
				if (fabsf(px - blackIndex * wGrid) < ws) {
					isBlockKey = true;
				}
			}
		}
		int keyIndex = isBlockKey ? blackIndexToKeyIndex[blackIndex] : whiteIndexToKeyIndex[whiteIndex];
		keyIndex += 12 * groupIndex;
		if (height > pianoKeys[keyIndex].height) {
			return ret;
		}
		else {
			ret.crashed = true;
			ret.height = pianoKeys[keyIndex].height;
			ret.vHeight = pianoKeys[keyIndex].vh;
			return ret;
		}
	}
};
