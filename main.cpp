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
#include <mutex>          // std::mutex

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
bool paused = false;

Piano piano;
SnowBall snowball;
std::list<Firework> fireworks;

bool freeMode = false;
sample_16b_2ch_t* playerBuf;
int32_t playerBufLen;
int32_t playerBufIndex = 0;

std::mutex mtx;           // mutex for critical section

void writeWave(float freq, float during) {
	if (!freeMode)
		return;
	int duringSample = during * wav.sampleRate;
	int curIndex = playerBufIndex;
	if (duringSample <= 1000) return;
	mtx.lock();
	for (int i = 0; i < duringSample; ++i) {
		if (i == 1000)
			mtx.unlock();
		int value = playerBuf[curIndex].L + 12000 * sinf(2 * PI * freq * i / wav.sampleRate) * exp(-7.0f * i / duringSample);
		if (value >= 32768) value = 32767;
		if (value < -32768) value = -32768;
		playerBuf[curIndex].L = playerBuf[curIndex].R = (int16_t)value;
		if (++curIndex >= playerBufLen) {
			curIndex = 0;
		}
	}
}


void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	// In playback mode copy data to pOutput. In capture mode read data from pInput. In full-duplex mode, both
	// pOutput and pInput will be valid and you can move data from pInput into pOutput. Never process more than
	// frameCount frames.

	if (freeMode) {
		mtx.lock();
		int size1 = playerBufLen - playerBufIndex;
		if (size1 >= frameCount) {
			memcpy(pOutput, &playerBuf[playerBufIndex], frameCount * wav.blockAlign);
			memset(&playerBuf[playerBufIndex], 0, frameCount * wav.blockAlign);
			playerBufIndex += frameCount;
		}
		else {
			memcpy(pOutput, &playerBuf[playerBufIndex], size1 * wav.blockAlign);
			memset(&playerBuf[playerBufIndex], 0, size1 * wav.blockAlign);
			int size2 = frameCount - size1;
			memcpy((sample_16b_2ch_t*)pOutput + size1, playerBuf, size2 * wav.blockAlign);
			memset(playerBuf, 0, size2 * wav.blockAlign);
			playerBufIndex = size2;
		}
		mtx.unlock();
	}
	else {
		if (paused)
			return;
		// Copy the next piece of the music PCM data.
		if (sampleIndex + frameCount > wav.numSamples) {
			size_t frameAvailable = wav.numSamples - sampleIndex;
			memcpy(pOutput, &wav.sample[sampleIndex], frameAvailable * wav.blockAlign);
			freeMode = true;
		}
		else {
			memcpy(pOutput, &wav.sample[sampleIndex], (size_t)frameCount * wav.blockAlign);
		}
		sampleIndex += frameCount;
	}
}

// Change the view volume and viewport. This is called when the window is resized.
void reshapeCallback(int w, int h) {
	// Set viewport to window dimensions
	glViewport(0, 0, w, h);
	window_width = w;
	window_height = h;
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


void mouseCallback(int button, int state, int x, int y){
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (!freeMode)
			return;
		GLint viewport[4];
		GLdouble mvmatrix[16], projmatrix[16];
		GLfloat winx, winy, winz;
		GLdouble posx, posy, posz;

		glPushMatrix();

		//glScalef(0.1, 0.1, 0.1);
		glGetIntegerv(GL_VIEWPORT, viewport);			/* 获取三个矩阵 */
		glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
		glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);

		glPopMatrix();

		winx = x;
		winy = window_height - y;

		glReadPixels((int)winx, (int)winy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winz);			/* 获取深度 */
		gluUnProject(winx, winy, winz, mvmatrix, projmatrix, viewport, &posx, &posy, &posz);	/* 获取三维坐标 */

		piano.mouseInteractive(posx, posz);
	}
}


void keyboardCallback(unsigned char key, int x, int y) {// keyboard interaction
	if (key == ' ') {
		keyStatus[4] = true;
	}
	if (key == 'p') {
		paused = !paused;
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

void myPerspective(double fovx, double aspect, double zNear, double zFar)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	double xmin, xmax, ymin, ymax;

	xmax = zNear * tan(fovx * M_PI / 360.0);
	xmin = -xmax;
	ymin = xmin / aspect;
	ymax = xmax / aspect;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);

	glMatrixMode(GL_MODELVIEW);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glDepthMask(GL_TRUE);
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
	myPerspective(60.0, (double)window_width / window_height, 4, 2500.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 250, 1500, 0, -300.f, 0, 0, 1, 0);
	//gluPerspective()

	// Draw the blue-white background
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -500.0f);
	glBegin(GL_POLYGON);//多边形
	glColor3ub(228, 237, 241);
	glNormal3f(0, 0, 1);
	glVertex3f(-1500, -500, 0.f);
	glVertex3f(+1500, -500, 0.f);

	glColor3ub(167, 221, 239);
	glVertex3f(+1500, +500, 0.f);
	glVertex3f(-1500, +500, 0.f);
	glEnd();

	glBegin(GL_POLYGON);
	glVertex3f(-1500, +500, 0.f);
	glVertex3f(+1500, +500, 0.f);
	glVertex3f(+1500, 2500, 0.f);
	glVertex3f(-1500, 2500, 0.f);
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
	glVertex3f(-1500, 0, 2500);
	glNormal3f(-5 / 13.f, 12 / 13.f, 0);
	glVertex3f(+1500, 0, 2500);
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

	// Default parameters to play the sound
	wav.sampleRate = 48000;
	wav.blockAlign = 4;

	char musicPath[200] = "The Music Visualizer - Free Mode";

	if (argc > 1) {
		strcpy_s(musicPath, argv[1]);
	}

	/* Load a list of demo musics. Disabled for the copyright reason.
	else {
		char musicList[][30] = {
			"Free Mode - Skip the music",
			"The Winter.wav",
			"The Spring.wav",
			"The Piano.wav",
			"The Time.wav",
			"The Ocean.wav",
		};
		int musicListLen = sizeof(musicList) / sizeof(musicList[0]);
		int selectedMusic = 0;

		printf("This is a music related project.\nPlease make sure open your speaker!\n\n");
		printf("USAGE:\nArrow keys move the snow ball\nSpace key jumps the snow ball.\nMouse can play the piano after th music is finished.\n\n");
		do {
			for (int i = 0; i < musicListLen; ++i) {
				printf("[%d] ", i);
				std::cout << musicList[i] << std::endl;
			}
			printf("Please make sure open your speaker now!\n");
			printf("Please select a music to play, type [0-%d]: ", musicListLen - 1);
			scanf_s("%d", &selectedMusic);
		} while (selectedMusic < 0 || selectedMusic >= musicListLen);

		if(selectedMusic)
			strcat_s(musicPath, "music/");
		strcat_s(musicPath, musicList[selectedMusic]);
	}
	*/

	ma_decoder decoder;
	ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_s16, 2, 0);

	if (ma_decoder_init_file(musicPath, &decoderConfig, &decoder) != MA_SUCCESS) {
		printf("Can not open wav music file: %s\n", musicPath);
	}
	else {
		wav.sampleRate = decoder.outputSampleRate;
		wav.numSamples = ma_decoder_get_length_in_pcm_frames(&decoder);
		wav.sample = new sample_16b_2ch_t[wav.numSamples];
		ma_decoder_read_pcm_frames(&decoder, wav.sample, wav.numSamples);
		printMeta(wav);
	}
	ma_decoder_uninit(&decoder);

	playerBufLen = wav.sampleRate * 6;
	playerBuf = new sample_16b_2ch_t[playerBufLen];
	memset(playerBuf, 0, (size_t)playerBufLen * wav.blockAlign);

	piano.init(N_FFT, wav.sampleRate);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(window_width, window_height);
	glutCreateWindow(musicPath);

	glutDisplayFunc(RenderScene);
	glutTimerFunc(timeIncreasemet, timerCallback, 1);
	glutReshapeFunc(reshapeCallback);
	glutKeyboardFunc(keyboardCallback);
	glutKeyboardUpFunc(keyboardUpCallback);
	glutSpecialFunc(specialKeyCallback);
	glutSpecialUpFunc(specialKeyUpCallback);
	glutMouseFunc(mouseCallback);

	SetupRC();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GLUT_MULTISAMPLE);

	snowball.setPiano(&piano);
	snowball.loadSnow("texture/snow.bmp");

	for (int i = 0; i < N_FFT; ++i) {
		w_hanning[i] = 0.5 - 0.5 * cosf(2 * PI * i / (float)N_FFT);
	}

	srand(time(NULL));

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = ma_format_s16;   // Set to ma_format_unknown to use the device's native format.
	config.playback.channels = wav.numChannels;   // Set to 0 to use the device's native channel count.
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
	delete[] playerBuf;
	freeWav(wav);
	return 0;
}
