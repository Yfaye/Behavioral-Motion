/* CS6555 Computer Animation, Fall 2014
* Lab 4£ºBehavioral Motion Control System
* Edited by Fei Yan (Email:hcifaye@gwu.edu)
* Reference: Lab 0's code ; Lab 1's code;
*			 Lab 2's code; Lab 3's code
* Basic Steps:
*		Step 1: generate the leading boid with given position and spline
*		Step 2: initialize the rest boids with given position
*		Step 3: apply 4 rules to generate the velocity of the rest of boids
*		Step 4: renew the position and volecity of the rest of boids
*/

// window
#include "stdafx.h"

// standard
#include <assert.h>
#include <math.h>

// glut
#include <GL/glut.h>

//================================
// global variables
//================================
// screen size
int g_screenWidth = 0;
int g_screenHeight = 0;

// velocity generated by each rule
GLfloat v_rule1[3] = {0};
GLfloat v_rule2[3] = {0};
GLfloat v_rule3[3] = {0};
GLfloat v_rule4[3] = {0};

// boids velocity and postion
GLfloat velocity_boids[20][3] = {};
GLfloat position_boids[20][3] = {};

// Number of Points for Spline
static int points = 0; //points index started from 0
static int number_point = 5; //The total number of points 

// time variables for generate Q(t)
static GLfloat t = 0;

// time incrase by 0.1
GLfloat timeIncrease = 0.1;

// Total number of boids
static int number_boids = 20;

//the matrix for 20 boids, each row is one M for one boids
GLfloat boidsM[20][16];

//the matrix for single boid implementing glMatrixMult
static GLfloat M[16] = { 0 };

//the matrix for the leading boid implementing glMatrixMult
static GLfloat leaderM[16] = { 0 };

// Original Boids Position
GLfloat location[20][3] = { { -3.0f, 7.0f, -5.6f },
							{ 9.0f, 8.5f, -4.0f },
							{ 4.0f, 7.2f, -5.7f },
							{ -4.5f, 6.8f, -5.8f },
							{ 3.0f, 8.6f, -5.0f },
							{ 5.0f, 9.8f, -4.9f },
							{ -4.0f, 9.0f, -4.9f },
							{ 4.0f, 12.0f, -4.5f },
							{ 0.0f, 8.2f, -5.0f },
							{ 1.0f, 7.6f, -4.5f },
							{ -2.0f, 8.0f, -4.3f },
							{ 10.0f, 9.5f, -3.9f },
							{ 5.0f, 8.2f, -4.2f },
							{ -3.5f, 7.8f, -4.1f },
							{ 4.0f, 9.6f, -4.9f },
							{ 6.0f, 10.8f, -4.8f },
							{ -3.0f, 10.0f, -4.8f },
							{ 5.0f, 13.0f, -4.4f },
							{ 1.0f, 9.2f, -4.9f },
							{ 2.0f, 8.6f, -4.4f } };

// 7 Pionts for the leading boid in Quternion£º the first 4 numbers represent w, x, y, z in quaternion, and the rest 3 numbers represent the position x,y,z in world Cartisian System
static GLfloat point_quaternion[7][7] = { { 1, 0, 0, 0, 8, 2, -20 },   //point 1
											{ 0, 1, 0, 0, -8, -2, -20 },  //point 2
											{ 0, 0, 1, 0, -5, -6, -10 },   //point 3
											{ 0, 0, 0, 1, 5, -8, -10 }, //point 4
											{ 0, 0, 1, 0, 3, -10, -5 },    //point 5
											{ 0, 1, 0, 0, -3, -14, -5 },  //point 6
											{ 1, 0, 0, 0, 1, -18, -3 } }; //point 7

// The Catmul-Rom Spline M Marix
static GLfloat CRSplineM[16] = { -0.5f, 1.0f, -0.5f, 0.0f,	  // Column 1
									1.5f, -2.5f, 0.0f, 1.0f,      // Column 2
									-1.5f, 2.0f, 0.5f, 0.0f,      // Column 3
									0.5f, -0.5f, 0.0f, 0.0f };    // Column 4

// The B Spline M Marix
static GLfloat BSplineM[16] = { -1.0f / 6.0f, 3.0f / 6.0f, -3.0f / 6.0f, 1.0f / 6.0f,  // Column 1
								3.0f / 6.0f, -6.0f / 6.0f, 0.0f / 6.0f, 4.0f / 6.0f,  // Column 2
								-3.0f / 6.0f, 3.0f / 6.0f, 3.0f / 6.0f, 1.0f / 6.0f,  // Column 3
								1.0f / 6.0f, 0.0f / 6.0f, 0.0f / 6.0f, 0.0f / 6.0f };// Column 4

// initialate function of different rules
void Rule_1(int index);
void Rule_2(int index);
void Rule_3(int index);
void Rule_4(int index);

//=========================================================================================================================
// Matrix Initialization: set up boidsM matrix and position_boids matrix by read in the original position of all the boids
//=========================================================================================================================
void init()
{
	for (int j = 0; j<number_boids; j++){
		boidsM[j][0] = 1.0f;
		boidsM[j][5] = 1.0f;
		boidsM[j][10] = 1.0f;
		for (int i = 0; i<3; i++){
			boidsM[j][12 + i] = location[j][i];
			position_boids[j][i] = boidsM[j][12 + i];
		}
		boidsM[j][15] = 1.0f;
	}
}

//==============================================================================
// Blending Function : Q(t) = T*M*G, finding out the vector position of time t
//==============================================================================
GLfloat blend(GLfloat T[4], GLfloat MS[16], GLfloat G[4])
{
	// B[4] is the result of T*M
	GLfloat B[4] = { 0 };
	B[0] = T[0] * MS[0] + T[1] * MS[1] + T[2] * MS[2] + T[3] * MS[3];	 //column 1
	B[1] = T[0] * MS[4] + T[1] * MS[5] + T[2] * MS[6] + T[3] * MS[7];	 //column 2
	B[2] = T[0] * MS[8] + T[1] * MS[9] + T[2] * MS[10] + T[3] * MS[11];  //column 3
	B[3] = T[0] * MS[12] + T[1] * MS[13] + T[2] * MS[14] + T[3] * MS[15];//column 4

	// Generate the result of T*M*G
	GLfloat Qt = B[0] * G[0] + B[1] * G[1] + B[2] * G[2] + B[3] * G[3];

	return Qt;
}

//=====================================================================
// Vector Normalization : normalize the vector 
//=====================================================================
void Normalization(GLfloat N_tempV[3])
{
	GLfloat squa_quaterion = N_tempV[0] * N_tempV[0] + N_tempV[1] * N_tempV[1] + N_tempV[2] * N_tempV[2];
	if (squa_quaterion != 0) // avoid being divided by 0
	{
		GLfloat base_quaternion = sqrt(squa_quaterion);
		N_tempV[0] = N_tempV[0] / base_quaternion;
		N_tempV[1] = N_tempV[1] / base_quaternion;
		N_tempV[2] = N_tempV[2] / base_quaternion;
	}
}

//==================================================================================================
// Quaternion to Rotation Matrix : generate Rotation Matrix with Given Quaternion and Position
//==================================================================================================
void QuaternionRoatationM(GLfloat Q_tempM[7], GLfloat R[16])
{
	GLfloat w = Q_tempM[0];
	GLfloat x = Q_tempM[1];
	GLfloat y = Q_tempM[2];
	GLfloat z = Q_tempM[3];
	R[0] = 1.0f - 2.0f*y*y - 2.0f*z*z; //column1 row1
	R[1] = 2.0f*x*y + 2.0f*w*z;        //....... row2
	R[2] = 2.0f*x*z - 2.0f*w*y;		   //....... row3
	R[3] = 0.0f;					   //....... row4
	R[4] = 2.0f*x*y - 2.0f*w*z;		   //column2 row1
	R[5] = 1.0f - 2.0f*x*x - 2.0f*z*z; //....... row2
	R[6] = 2.0f*y*z + 2.0f*w*x;		   //....... row3
	R[7] = 0.0f;					   //....... row4
	R[8] = 2.0f*x*z + 2.0f*w*y;		   //column3 row1
	R[9] = 2.0f*y*z - 2.0f*w*x;		   //....... row2
	R[10] = 1.0f - 2.0f*x*x - 2.0f*y*y;//....... row3
	R[11] = 0.0f;					   //....... row4
	R[12] = Q_tempM[4];				   //column4 row1
	R[13] = Q_tempM[5];			       //....... row2
	R[14] = Q_tempM[6];			       //....... row3
	R[15] = 1.0f;					   //....... row4
}

//=========================================================================================================
// Quaternion Interpolating Function : generate the movement of the leading boid.
//                                     generate interpolation with given quaterions, postions and spline styles 
//=========================================================================================================
void q_interpolate(GLfloat p_quaternion[7][7], GLfloat SplineM[16])
{
	// Set up T matrix T = {t*t*t,t*t,t,1}
	GLfloat TMatrix_q[4] = { t*t*t, t*t, t, 1 };

	// Set up temporate matrix to store the interpolation track (pose and position)
	GLfloat tempM[7];

	// Loop to generate the interpolation track based on 4 points every time
	// i = 0, get w's changing along the Q(t) curve to tempM[0];
	// i = 1, get x's changing along the Q(t) curve to tempM[1];
	// i = 2, get y's changing along the Q(t) curve to tempM[2];
	// i = 3, get z's changing along the Q(t) curve to tempM[3];
	// i = 4, get x's changing along the Q(t) curve to tempM[4];
	// i = 5, get y's changing along the Q(t) curve to tempM[5];
	// i = 6, get z's changing along the Q(t) curve to tempM[6];

	for (int i = 0; i < 7; i++)
	{
		// the value of points would be changed by timer function in the following
		GLfloat GMatrix_q[4] = { p_quaternion[points][i],
			p_quaternion[(points + 1) % number_point][i],
			p_quaternion[(points + 2) % number_point][i],
			p_quaternion[(points + 3) % number_point][i] };

		tempM[i] = blend(TMatrix_q, SplineM, GMatrix_q);
	}

	Normalization(tempM);
	QuaternionRoatationM(tempM, leaderM);
}

//=====================================================================
// Distance: Calculate the distances between two balls
//=====================================================================
GLfloat Distance(GLfloat TempV1[3], GLfloat TempV2[3]){

	GLfloat Distance = sqrt((TempV1[0] - TempV2[0])*(TempV1[0] - TempV2[0]) + (TempV1[1] - TempV2[1])*(TempV1[1] - TempV2[1]) + (TempV1[2] - TempV2[2])*(TempV1[2] - TempV2[2]));
	return Distance;

}
//====================================================
// BoidsMove : generate the movement of boids
//			   adding the velocity generated by rules
//             calculating the position of boids
//====================================================
void BoidsMove()
{
	// initialize temperate vector for generated velocity by rules
	GLfloat v1[3] = { 0 }; 
	GLfloat v2[3] = { 0 };
	GLfloat v3[3] = { 0 };
	GLfloat v4[3] = { 0 };

	// for each boid, generating volecity by each rule
	for (int i = 0; i < number_boids; i++)
	{
		Rule_1(i);
		for (int j = 0; j < 3; j++)
		{
			v1[j] = v_rule1[j];
		}
		Rule_2(i);
		for (int j = 0; j < 3; j++)
		{
			v2[j] = v_rule2[j];
		}
		Rule_3(i);
		for (int j = 0; j < 3; j++)
		{
			v3[j] = v_rule3[j];
		}
		Rule_4(i);
		for (int j = 0; j < 3; j++)
		{
			v4[j] = v_rule4[j];
		}

		// adding up all the velocity generated by rules and renew the position in M matrix for boids
		for (int j = 0; j < 3; j++)
		{
			velocity_boids[i][j] = velocity_boids[i][j] + v1[j] + v2[j] + v3[j] + v4[j];
			position_boids[i][j] = position_boids[i][j] + velocity_boids[i][j] * 0.15;
			boidsM[i][12 + j] = position_boids[i][j];
		}
	}
}

//=======================================================
// Rule 1 : following - boids following the leading boid
//=======================================================
void Rule_1(int index)
{
	for (int i = 0; i < 3; i++)
	{
		v_rule1[i] = (leaderM[12 + i] - boidsM[index][12 + i]) / 3000;
	}
}
//=====================================================================
// Rule 2 : together - a boid stays close with the center of rest boids
//=====================================================================
void Rule_2(int index)
{
	
	GLfloat all[3] = {};
	GLfloat center[3] = {};
	for (int i = 0; i < number_boids; i++)
	{
		// Adding up all the boids position
		all[0] += boidsM[i][12];
		all[1] += boidsM[i][13];
		all[2] += boidsM[i][14];
	
		// Calculating the average as the center position of the rest boids
		center[0] = (all[0] - boidsM[index][12]) / (number_boids - 1);
		center[1] = (all[1] - boidsM[index][13]) / (number_boids - 1);
		center[2] = (all[2] - boidsM[index][14]) / (number_boids - 1);
	}

	//generating the volecity for rule 1
	for (int i = 0; i < 3; i++)
	{
		v_rule2[i] = (center[i] - boidsM[index][12 + i]) / 2000;
	}
}

//================================================================================
// Rule 3 : No collision - a boid stays a small distance away with the other boids
//================================================================================
void Rule_3(int index)
{
	for (int i = 0; i < number_boids; i++)
	{
		if (i != index)
		{
			if (Distance(position_boids[index], position_boids[i])< 4)
			{
				for (int j = 0; j < 3; j++)
				{
					v_rule3[j] = (v_rule2[j] - (position_boids[i][j] - position_boids[index][j])) / 2000;
				}
			}
		}
	}	
}
//=====================================================================
// Rule 4 : same speed - a boid match its velocity with the rest boids
//=====================================================================
void Rule_4(int index)
{

	GLfloat all[3] = {};
	GLfloat average[3] = {};
	for (int i = 0; i < number_boids; i++)
	{
		// Adding up all the boids velocity
		all[0] += velocity_boids[i][0];
		all[1] += velocity_boids[i][1];
		all[2] += velocity_boids[i][2];

		// Calculating the average velocity of the rest boids
		average[0] = (all[0] - velocity_boids[index][0]) / (number_boids - 1);
		average[1] = (all[1] - velocity_boids[index][1]) / (number_boids - 1);
		average[2] = (all[2] - velocity_boids[index][2]) / (number_boids - 1);
	}

	//generating the volecity for rule 3
	for (int i = 0; i < 3; i++)
	{
		v_rule4[i] = (average[i] - velocity_boids[index][i]) / 2000;
	}
}


//=======================================================
// Display_single: display a single boid
//=======================================================
void Display_single(int index)
{
	glPushMatrix();
	for (int j = 0; j < 16; j++)
	{
		M[j] = boidsM[index][j];
	}
	glMultMatrixf(M);
	glutSolidSphere(0.3,20,20);
	glPopMatrix();
}

//=======================================================
// Display_leader: display the leading boid
//=======================================================
void Display_leader()
{
	glPushMatrix();
	q_interpolate(point_quaternion, BSplineM);
	glMultMatrixf(leaderM);
	//glutSolidDodecahedron();
	glutSolidIcosahedron();
	glPopMatrix();
}

//=================================================================
// BoidsAnimation: display the leading boid and the following boids
//=================================================================
void BoidsAnimation()
{
	Display_leader();
	BoidsMove();
	for (int i = 0; i < number_boids; i++)
	{
		Display_single(i);
	}
}


//============================================================
// timer : triggered every 16ms ( about 60 frames per second )
//============================================================
void timer(int value) {
	// render
	glutPostRedisplay();

	// Set time increase by 0.01
	t = t + 0.01;
	if (t >= 1)
	{
		t = 0;
		if (points < number_point - 1)
		{
			points++;
		}
		else
		{
			points = 0;
		}
	}
	// reset timer
	glutTimerFunc(16, timer, 0);
}

//================================
// render
//================================
void render(void) {
	// clear buffer
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// render state
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

	// enable lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// light source attributes
	GLfloat LightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightDiffuse[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	GLfloat LightSpecular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightPosition[] = { 5.0f, 5.0f, 5.0f, 1.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);

	// surface material attributes
	GLfloat material_Ka[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	GLfloat material_Kd[] = { 0.43f, 0.47f, 0.54f, 1.0f };
	GLfloat material_Ks[] = { 0.33f, 0.33f, 0.52f, 1.0f };
	GLfloat material_Ke[] = { 0.1f, 0.0f, 0.1f, 1.0f };
	GLfloat material_Se = 10;

	glMaterialfv(GL_FRONT, GL_AMBIENT, material_Ka);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, material_Kd);
	glMaterialfv(GL_FRONT, GL_SPECULAR, material_Ks);
	glMaterialfv(GL_FRONT, GL_EMISSION, material_Ke);
	glMaterialf(GL_FRONT, GL_SHININESS, material_Se);


	// modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 15.0, 15.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	// animation	
	BoidsAnimation();

	// disable lighting
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	// swap back and front buffers
	glutSwapBuffers();
}

//================================
// keyboard input
//================================
void keyboard(unsigned char key, int x, int y) {}

//================================
// reshape : update viewport and projection matrix when the window is resized
//================================
void reshape(int w, int h) {
	// screen size
	g_screenWidth = w;
	g_screenHeight = h;

	// viewport
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);

	// projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(65.0, (GLfloat)w / (GLfloat)h, 1.0, 50.0);
}

//================================
// main
//================================
int main(int argc, char** argv) {
	// create opengL window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Lab 4 - Computer Animation - Fei Yan");

	// user initialization
	init();

	// set callback functions
	glutDisplayFunc(render);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(16, timer, 0);

	// main loop
	glutMainLoop();

	return 0;
}