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

//#include "mmsystem.h"
#include "fft.h"
#include "wav.h"
#include "miniaudio.h"

//#pragma comment(lib, "winmm.lib")

// Angles of rotation
static GLfloat xRot = 0.0f;
static GLfloat yRot = 0.0f;

static int window_width = 1280, window_height = 720;
int timeIncreasemet = 16;	// Refresh period in ms
clock_t beginClock;

wav_t wav;

std::complex<double> values[N_FFT] = { 0.f, 0.f };
float mag[N_FFT] = { 0.f }, mag_smoothed[N_FFT] = { 0.f };
float w_hanning[N_FFT];

float smoothUp = 0.8, smoothDown = 0.08;



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
	HSV_t hsv = {0};		// Color in HSV color space
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
	luminanceFactor = powf(factor, timeIncreasemet / 1000.f);
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

	cx += vx * timeIncreasemet / 1000;
	cy += vy * timeIncreasemet / 1000;

	// Gravity acceleration is 500 pixels / s^2.
	vy -= 500 * timeIncreasemet / 1000;

	float v_square = vx * vx + vy * vy;
	float resistance = 3.f * v_square * timeIncreasemet / 1000;
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
		glVertex2f(cx + radius * cosf(2 * PI * i / samplingNumber),
			cy + radius * sinf(2 * PI * i / samplingNumber));
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
		glVertex2f(cx + radius * cosf(2 * PI * i / samplingNumber),
			cy + radius * sinf(2 * PI * i / samplingNumber));
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
		ball.setColor(centerHue -10 + getRandf(20), 1, 1);
		ball.setVelocity(speed * cos(i * PI / 180), speed * sin(i * PI / 180));
		ball.setLuminanceFactor(0.2 + getRand(600) / 1000.f);
		balls.push_back(ball);
	}

}

bool Firework::nextState() {
	timeToLive -= timeIncreasemet;
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

std::list<Firework> fireworks;
Ball ballMatrix[100][36];

void drawCircle(float r, float cx, float cy) {

	//glColor3f(0, 0.5, 1);
	glBegin(GL_POLYGON);
	for (float theta = 0; theta < 2 * PI; theta += 0.1) {
		glVertex2f(cx + r * cos(theta), cy + r * sin(theta));
	}
	glEnd();
}

// Change the view volume and viewport. This is called when the window is resized.
void ChangeSize(int w, int h) {
	// Set viewport to window dimensions
	glViewport(0, 0, w, h);
}

// Setting up lighting and material parameters for enhanced rendering effect.
// This topic will be covered later on in the module so please skip this for now.
void SetupRC() {
	// Light parameters and coordinates
	GLfloat whiteLight[] = { 0.45f, 0.45f, 0.45f, 1.0f };
	GLfloat sourceLight[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	GLfloat lightPos[] = { -50.f, 25.0f, 250.0f, 0.0f };
	glEnable(GL_DEPTH_TEST); // Hidden surface removal
	glEnable(GL_LIGHTING); // Enable lighting
	// Setup and enable light 0
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, sourceLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, sourceLight);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL); // Enable colour tracking
	// Set material properties to follow glColor values
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
}

float test = 0.0f;
float test2 = 0.0f;
void keyboardCallback(unsigned char key, int x, int y) {// keyboard interaction
	if (key == 'm') {
		test += 0.01;
	}
	if (key == 'M') {
		test -= 0.01;
	}
	if (key == 'a') {
		test2 += 0.01;
	} if (key == 'A') {
		test2 -= 0.01;
	}
	// Refresh the window
	glutPostRedisplay();
}

// Respond to arrow keys
void specialKeyCallback(int key, int x, int y){
	if (key == GLUT_KEY_UP)
		xRot -= 5.0f;
	if (key == GLUT_KEY_DOWN)
		xRot += 5.0f;
	if (key == GLUT_KEY_LEFT)
		yRot -= 5.0f;
	if (key == GLUT_KEY_RIGHT)
		yRot += 5.0f;
	xRot = (GLfloat)((const int)xRot % 360);
	yRot = (GLfloat)((const int)yRot % 360);
	// Refresh the window
	glutPostRedisplay();
}


void timerCallback(int value) {

	// A timer which has a random value of period [0, 3000]s.
	static int cnt = 0;
	static int threshold = 0;
	if (cnt++ * timeIncreasemet >= threshold) {
		cnt = 0;
		threshold = getRand(3000);
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

	glutPostRedisplay(); // force OpenGL to redraw the current window
	glutTimerFunc(timeIncreasemet, timerCallback, value + 1);
}


uint32_t sampleIndex = 0;
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	// In playback mode copy data to pOutput. In capture mode read data from pInput. In full-duplex mode, both
	// pOutput and pInput will be valid and you can move data from pInput into pOutput. Never process more than
	// frameCount frames.
	memcpy(pOutput, &wav.sample[sampleIndex], frameCount * wav.blockAlign);
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
	gluPerspective(90.0, 16/9., 4, 1200.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 500, 0, 0.f, 0, test, 1, 0);
	//gluPerspective()

	glPushMatrix();
	glBegin(GL_LINES);
	glColor3f(1, 0.5, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(100, 100, 0);

	glColor3f(0, 1, 0.5);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 100, 0);

	glColor3f(0, 0.5, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 100);
	glEnd();

	// Draw the blue-white background
	glPushMatrix();
		glTranslatef(0.0f, 0.0f, -500.0f);
		glBegin(GL_POLYGON);//多边形
		glColor3f(228 / 255.f, 237 / 255.f, 241 / 255.f);
		glVertex3f(-1920, -1080, 0.f);
		glVertex3f(+1920, -1080, 0.f);

		glColor3f(167 / 255.f, 221 / 255.f, 239 / 255.f);
		glVertex3f(+1920, +1080, 0.f);
		glVertex3f(-1920, +1080, 0.f);
		glEnd();
	glPopMatrix();

	// Draw the grey floor
	glPushMatrix();
	glTranslatef(0.0f, -500.0f, 0.0f);

	glBegin(GL_POLYGON);
	glColor3f(203 / 255.f, 214 / 255.f, 216 / 255.f);
	glVertex3f(-1000, 0, 500);
	glVertex3f(+1000, 0, 500);
	glVertex3f(+1000, 0, -500);
	glVertex3f(-1000, 0, -500);
	glEnd();


	clock_t cur_time = clock() - beginClock;
	int32_t cur_index = sampleIndex; // wav.sampleRate / CLOCKS_PER_SEC * cur_time - (N_FFT >> 1);


	for (int32_t i = 0; i < N_FFT; ++i) {
		if (i + cur_index >= 0 && i + cur_index < wav.numSamples) {
			values[i] = wav.sample[i + cur_index].L / 32768.f * w_hanning[i];
		}
	}

	fft(values, N_FFT);

	//if (cur_index > 200000) {
	//	for (int32_t i = 0; i < N_FFT; ++i) {
	//		printf("%lf\t", values[i].real());
	//	}
	//}

	for (int32_t i = 0; i < N_FFT; ++i) {
		mag[i] = abs(values[i]);
		if (mag[i] > mag_smoothed[i]) {
			mag_smoothed[i] = smoothUp * mag[i] + (1 - smoothUp) * mag_smoothed[i];
		}
		else {
			mag_smoothed[i] = smoothDown * mag[i] + (1 - smoothDown) * mag_smoothed[i];
		}
	}

	glTranslatef(-512, 200.0f, 0.0f);
	glBegin(GL_LINES);
	glColor3f(45 / 255.f, 89 / 255.f, 172 / 255.f);


	for (uint32_t i = 0; i < N_FFT; ++i) {
		if (i == N_FFT >> 1) {
			glColor3f(100 / 255.f, 52 / 255.f, 158 / 255.f);
		}
		glVertex3f(i, 0.f, 0.f);
		glVertex3f(i, 4 * mag_smoothed[i], 0.f);
	}

	glEnd();

	glPopMatrix();

	for (auto it = fireworks.begin(); it != fireworks.end(); ++it) {
		(*it).draw();
	}

	glPushMatrix(); // Save the matrix state and perform rotations
		glTranslatef(0.0f, 0.f, 0.0f);
		glRotatef(xRot, 1.0f, 0.0f, 0.0f);
		glRotatef(yRot, 0.0f, 1.0f, 0.0f);

		for (int i = 0; i < 100; ++i) {
			for (int j = 0; j < 36; ++j) {
				ballMatrix[i][j].drawForMatrix();
			}
		}

	glPopMatrix(); // Restore the matrix state

	glPushMatrix(); // Save the matrix state and perform rotations
		glTranslatef(-300.0f, 250.0f, 0.f);
		glRotatef(xRot, 1.0f, 0.0f, 0.0f);
		glRotatef(yRot, 0.0f, 1.0f, 0.0f);
		glColor3f(0.0f, 0.5f, 1.0f);
		glutSolidTeacup(90); // draw a wireframe teapot
	glPopMatrix(); // Restore the matrix state
	glutSwapBuffers();
}


int main(int argc, char* argv[]) {

	//test_fft();

	loadWav("D:\\CloudMusic\\Cheetah Mobile Games - The Piano.wav", wav);
	printMeta(wav);


	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(window_width, window_height);
	glutCreateWindow("Orthogonal Projection");

	glutDisplayFunc(RenderScene);
	glutTimerFunc(timeIncreasemet, timerCallback, 1);
	glutReshapeFunc(ChangeSize);
	glutSpecialFunc(specialKeyCallback);
	glutKeyboardFunc(keyboardCallback);

	//打开抗锯齿功能
	//1.开启混合功能
	glEnable(GL_BLEND);

	//2.指定混合因子
	//注意:如果你修改了混合方程式,当你使用混合抗锯齿功能时,请一定要改为默认混合方程式
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//3.开启对点\线\多边形的抗锯齿功能
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);

	/*
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glColor3f(0.0, 0.0, 0.0);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glFrustum(-640, 640, -360, 360, 400, 2500);*/

	SetupRC();

	for (int i = 0; i < 100; ++i) {
		for (int j = 0; j < 36; ++j) {
			ballMatrix[i][j].setCenter(-495 + i * 10, -175 + j * 10);
			ballMatrix[i][j].setRadius(5.f);
			ballMatrix[i][j].weight = getRandf(2) - 1;
		}
	}

	for (uint32_t i = 0; i < N_FFT; ++i) {
		w_hanning[i] = 0.5 - 0.5 * cosf(2 * PI * i / N_FFT);
	}

	srand(time(NULL));//设置随机数种子，使每次产生的随机序列不同
	//PlaySound(L"D:\\CloudMusic\\Cheetah Mobile Games - The Piano.wav", NULL, SND_FILENAME | SND_ASYNC);
	beginClock = clock() + 200;

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = ma_format_s16;   // Set to ma_format_unknown to use the device's native format.
	config.playback.channels = 2;               // Set to 0 to use the device's native channel count.
	config.sampleRate = 48000;           // Set to 0 to use the device's native sample rate.
	config.dataCallback = data_callback;   // This function will be called when miniaudio needs more data.
	config.pUserData = &wav;   // Can be accessed from the device object (device.pUserData).

	ma_device device;
	if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
		return -1;  // Failed to initialize the device.
	}

	ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.


	glutMainLoop();
	ma_device_uninit(&device);    // This will stop the device so no need to do that manually.
	return 0;
}

