/**
 * @file main.cpp
 *
 * @brief Main function of CPT205 assignment.
 *
 * @author Jingyi Wang 1929591
 * 
 * @email Jingyi.Wang1903@student.xjtlu.edu.cn
 */


#define MINIAUDIO_IMPLEMENTATION
#define FREEGLUT_STATIC
#define PI 3.14159265358979323846

#include <GL/freeglut.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <list>
#include <ctime>

#include "fft.h"
#include "wav.h"
#include "miniaudio.h"
#include "Piano.h"
#include "SnowBall.h"
#include "Firework.h"

static int window_width = 1280, window_height = 720;
int timeIncreasemet = 16;	// Refresh period in ms

wav_t wav;
uint32_t sampleIndex = 0;
fft_complex_t values[N_FFT] = { 0.f, 0.f };
float mag[N_FFT] = { 0.f }, mag_smoothed[N_FFT] = { 0.f };
float height_smoothed[36];
float w_hanning[N_FFT];
float smoothUp = 0.9, smoothDown = 0.04;

// Up, Down, Left, Right, Space
bool keyStatus[5] = { false };

Piano piano;
SnowBall snowball;
std::list<Firework> fireworks;

// Change the view volume and viewport. This is called when the window is resized.
void reshapeCallback(int w, int h) {
	// Set viewport to window dimensions
	glViewport(0, 0, w, h);
}

// Setting up lighting and material parameters for enhanced rendering effect.
// This topic will be covered later on in the module so please skip this for now.
void SetupRC() {
	// Light parameters and coordinates
	GLfloat whiteLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat diffuseLight[] = { 0.45f, 0.45f, 0.45f, 1.0f };
	GLfloat specularLight[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat lightPos[] = { -200.f, 500.0f, 350.0f, 0.0f };
	glEnable(GL_DEPTH_TEST); // Hidden surface removal
	glEnable(GL_LIGHTING); // Enable lighting
	// Setup and enable light 0
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL); // Enable colour tracking
	// Set material properties to follow glColor values
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
}


void keyboardCallback(unsigned char key, int x, int y) {// keyboard interaction
	if (key == ' ') {
		keyStatus[4] = true;
	}
}

void keyboardUpCallback(unsigned char key, int x, int y) {// keyboard interaction
	if (key == ' ') {
		keyStatus[4] = false;
	}
}

// Respond to arrow keys
void specialKeyCallback(int key, int x, int y){
	switch (key) {
	case GLUT_KEY_UP:
		keyStatus[0] = true;
		break;
	case GLUT_KEY_DOWN:
		keyStatus[1] = true;
		break;
	case GLUT_KEY_LEFT:
		keyStatus[2] = true;
		break;
	case GLUT_KEY_RIGHT:
		keyStatus[3] = true;
		break;
	}
}

void specialKeyUpCallback(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_UP:
		keyStatus[0] = false;
		break;
	case GLUT_KEY_DOWN:
		keyStatus[1] = false;
		break;
	case GLUT_KEY_LEFT:
		keyStatus[2] = false;
		break;
	case GLUT_KEY_RIGHT:
		keyStatus[3] = false;
		break;
	}
}


void timerCallback(int value) {

	// A timer which has a random value of period [0, 8000]s.
	static int cnt = 0;
	static int threshold = 0;
	if (cnt++ * timeIncreasemet >= threshold) {
		cnt = 0;
		threshold = getRand(8000);
		fireworks.push_back(Firework(getRand(window_width) - window_width / 2,  60 + getRand(300), 500 + getRand(800), getRand(360)));
	}

	for (auto it = fireworks.begin(); it != fireworks.end();) {
		if (!(*it).nextState()) {
			// Delete current firework and assign the next firework to it.
			it = fireworks.erase(it);
		} else {
			++it;
		}
	}


	for (int32_t i = 0; i < N_FFT; ++i) {
		if (i + sampleIndex >= 0 && i + sampleIndex < wav.numSamples)
			values[i] = wav.sample[i + sampleIndex].L / 32768.f * w_hanning[i];
		else
			values[i] = 0;
	}

	fft(values, N_FFT);

	for (int32_t i = 0; i < N_FFT; ++i) {
		mag[i] = abs(values[i]);
		if (mag[i] > mag_smoothed[i])
			mag_smoothed[i] = smoothUp * mag[i] + (1 - smoothUp) * mag_smoothed[i];
		else
			mag_smoothed[i] = smoothDown * mag[i] + (1 - smoothDown) * mag_smoothed[i];
	}

	piano.update(mag);
	snowball.update(keyStatus);

	glutPostRedisplay(); // force OpenGL to redraw the current window
	glutTimerFunc(timeIncreasemet, timerCallback, value + 1);
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	// In playback mode copy data to pOutput. In capture mode read data from pInput. In full-duplex mode, both
	// pOutput and pInput will be valid and you can move data from pInput into pOutput. Never process more than
	// frameCount frames.

	if (sampleIndex >= wav.numSamples)
		return;

	if (sampleIndex + frameCount > wav.numSamples) {
		size_t frameAvailable = wav.numSamples - sampleIndex;
		memcpy(pOutput, &wav.sample[sampleIndex], frameAvailable * wav.blockAlign);
	}
	else {
		memcpy(pOutput, &wav.sample[sampleIndex], (size_t)frameCount * wav.blockAlign);
	}
	sampleIndex += frameCount;
}

// Draw scene
void RenderScene(void) {

	// Clear the window with current clearing color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Perform the depth test to render multiple objects in the correct order of Z-axis value
	glEnable(GL_DEPTH_TEST); // Hidden surface removal


	//glMatrixMode(GL_MODELVIEW);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 16/9., 4, 2000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 100, 800, 0, -200.f, 0, 0, 1, 0);
	//gluPerspective()

	// Draw the blue-white background
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -500.0f);
	glBegin(GL_POLYGON);//¶à±ßÐÎ
	glColor3ub(228, 237, 241);
	glNormal3f(0, 0, 1);
	glVertex3f(-1500, -500, 0.f);
	glVertex3f(+1500, -500, 0.f);

	glColor3ub(167, 221, 239);
	glVertex3f(+1500, +500, 0.f);
	glVertex3f(-1500, +500, 0.f);
	glEnd();
	glPopMatrix();

	// The Ground Coordinate
	glPushMatrix();
	glTranslatef(0.0f, -500.0f, 0.0f);

	// Draw the gray ground
	glPushMatrix();
	glBegin(GL_POLYGON);
	glColor3ub(160, 170, 175);
	glNormal3f(5/13.f, 12/13.f, 0);
	glVertex3f(-1500, 0, -500);
	glVertex3f(-1500, 0, 500);
	glNormal3f(-5 / 13.f, 12 / 13.f, 0);
	glVertex3f(+1500, 0, 500);
	glVertex3f(+1500, 0, -500);
	glEnd();
	glPopMatrix();

	piano.draw();
	snowball.draw();

	glPushMatrix();
	glTranslatef(-1024, 0.0f, -480.0f);
	glBegin(GL_LINES);
	glColor3ub(45, 89, 172);

	for (uint32_t i = 0; i < N_FFT; ++i) {
		if (i == N_FFT >> 1) {
			glColor3ub(100, 52, 158);
		}
		glVertex3f(i, 0.f, 0.f);
		glVertex3f(i, 4 * mag_smoothed[i], 0.f);
	}

	glEnd();
	glPopMatrix();
	
	glPopMatrix();

	for (auto it = fireworks.begin(); it != fireworks.end(); ++it) {
		(*it).draw();
	}

	glutSwapBuffers();
}


int main(int argc, char* argv[]) {

	//test_fft();

	loadWav("music/The Winter.wav", wav);
	printMeta(wav);

	piano.init(N_FFT, wav.sampleRate);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(window_width, window_height);
	glutCreateWindow("Music visualizer - The Winter");

	glutDisplayFunc(RenderScene);
	glutTimerFunc(timeIncreasemet, timerCallback, 1);
	glutReshapeFunc(reshapeCallback);
	glutKeyboardFunc(keyboardCallback);
	glutKeyboardUpFunc(keyboardUpCallback);
	glutSpecialFunc(specialKeyCallback);
	glutSpecialUpFunc(specialKeyUpCallback);

	SetupRC();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GLUT_MULTISAMPLE);

	//glEnable(GL_TEXTURE_2D);

	snowball.setPiano(&piano);
	snowball.loadSnow("texture/snow.bmp");

	for (uint32_t i = 0; i < N_FFT; ++i) {
		w_hanning[i] = 0.5 - 0.5 * cosf(2 * PI * i / N_FFT);
	}

	srand(time(NULL));

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = ma_format_s16;   // Set to ma_format_unknown to use the device's native format.
	config.playback.channels = 2;               // Set to 0 to use the device's native channel count.
	config.sampleRate = wav.sampleRate;           // Set to 0 to use the device's native sample rate.
	config.dataCallback = data_callback;   // This function will be called when miniaudio needs more data.
	config.pUserData = &wav;   // Can be accessed from the device object (device.pUserData).

	ma_device device;
	if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
		printf("Error: Can not initialize the sound player device!\n");
		return -1;  // Failed to initialize the device.
	}
	ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.

	glutMainLoop();

	ma_device_uninit(&device);    // This will stop the device so no need to do that manually.
	return 0;
}

