/*****************************************************************************
 * Wave Simulation in OpenGL
 * sthapa5@lsu.edu
 *****************************************************************************/

#if defined(_MSC_VER)
 // Make MS math.h define M_PI
#define _USE_MATH_DEFINES
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GLFW/glfw3.h>

#include <linmath.h>

// Maximum delta T to allow for differential calculations
#define MAX_DELTA_T 0.01

// Animation speed (10.0 looks good)
#define ANIMATION_SPEED 10.0

GLfloat alpha = 210.f, beta = -70.f;
GLfloat zoom = 2.f;

double cursorX;
double cursorY;

struct Vertex
{
	GLfloat x, y, z;
	GLfloat r, g, b;
};

#define GRIDW 50
#define GRIDH 50
#define VERTEXNUM (GRIDW*GRIDH)

#define QUADW (GRIDW - 1)
#define QUADH (GRIDH - 1)
#define QUADNUM (QUADW*QUADH)

GLuint quad[4 * QUADNUM];
struct Vertex vertex[VERTEXNUM];

/* The grid will look like this:
 *
 *      3   4   5
 *      *---*---*
 *      |   |   |
 *      | 0 | 1 |
 *      |   |   |
 *      *---*---*
 *      0   1   2
 */

 //========================================================================
 // Initialize grid geometry
 //========================================================================

void init_vertices(void)
{
	int x, y, p;

	// Place the vertices in a grid
	for (y = 0; y < GRIDH; y++)
	{
		for (x = 0; x < GRIDW; x++)
		{
			p = y * GRIDW + x;

			vertex[p].x = (GLfloat)(x - GRIDW / 2) / (GLfloat)(GRIDW / 2);
			vertex[p].y = (GLfloat)(y - GRIDH / 2) / (GLfloat)(GRIDH / 2);
			vertex[p].z = 0;

			if ((x % 4 < 2) ^ (y % 4 < 2))
				vertex[p].r = 0.0;
			else
				vertex[p].r = 1.0;

			vertex[p].g = (GLfloat)y / (GLfloat)GRIDH;
			vertex[p].b = 1.f - ((GLfloat)x / (GLfloat)GRIDW + (GLfloat)y / (GLfloat)GRIDH) / 2.f;
		}
	}

	for (y = 0; y < QUADH; y++)
	{
		for (x = 0; x < QUADW; x++)
		{
			p = 4 * (y * QUADW + x);

			quad[p + 0] = y * GRIDW + x;     // Some point
			quad[p + 1] = y * GRIDW + x + 1; // Neighbor at the right side
			quad[p + 2] = (y + 1) * GRIDW + x + 1; // Upper right neighbor
			quad[p + 3] = (y + 1) * GRIDW + x;     // Upper neighbor
		}
	}
}

double dt;
double p[GRIDW][GRIDH];		//pressure
double vx[GRIDW][GRIDH], vy[GRIDW][GRIDH];	//velocity
double ax[GRIDW][GRIDH], ay[GRIDW][GRIDH];	//accleration
double normx[GRIDW][GRIDH], normy[GRIDW][GRIDH], normz[GRIDW][GRIDH];	//normals
double avgHeight[GRIDW][GRIDH];		//average height

//========================================================================
// Initialize grid
//========================================================================

void init_grid(void)
{
	int x, y;
	double dx, dy, d;

	for (y = 0; y < GRIDH; y++)
	{
		for (x = 0; x < GRIDW; x++)
		{
			dx = (double)(x - GRIDW / 2);
			dy = (double)(y - GRIDH / 2);
			d = sqrt(dx * dx + dy * dy);
			if (d < 0.1 * (double)(GRIDW / 2))
			{
				d = d * 10.0;
				p[x][y] = -cos(d * (M_PI / (double)(GRIDW * 8))) * 50.0;
			}
			else
				p[x][y] = 0.0;

			vx[x][y] = 0.0;
			vy[x][y] = 0.0;
		}
	}
}

//========================================================================
// Compute Normal
//========================================================================

struct Vertex compute_normal(struct Vertex v1, struct Vertex v2, struct Vertex v3) {
	struct Vertex temp1, temp2, temp3, n1, n2, n3, n;
	float l1, l2, l3;

	//v1-v2 && v1-v3
	temp1.x = v1.x - v2.x;
	temp1.y = v1.y - v2.y;
	temp1.z = v1.z - v2.z;

	temp2.x = v1.x - v3.x;
	temp2.y = v1.y - v3.y;
	temp2.z = v1.z - v3.z;

	temp3.x = temp1.y *temp2.z - temp1.z * temp2.y;
	temp3.y = temp1.z *temp2.x - temp1.x * temp2.z;
	temp3.z = temp1.x *temp2.y - temp1.y * temp2.x;

	l1 = sqrt(temp3.x*temp3.x + temp3.y*temp3.y + temp3.z*temp3.z);
	n1.x = temp3.x / l1; n1.y = temp3.y / l1; n1.z = temp3.z / l1;

	//v2-v1 && v2-v3
	temp1.x = v2.x - v1.x;
	temp1.y = v2.y - v1.y;
	temp1.z = v2.z - v1.z;

	temp2.x = v2.x - v3.x;
	temp2.y = v2.y - v3.y;
	temp2.z = v2.z - v3.z;

	temp3.x = temp1.y *temp2.z - temp1.z * temp2.y;
	temp3.y = temp1.z *temp2.x - temp1.x * temp2.z;
	temp3.z = temp1.x *temp2.y - temp1.y * temp2.x;

	l2 = sqrt(temp3.x*temp3.x + temp3.y*temp3.y + temp3.z*temp3.z);
	n2.x = temp3.x / l2; n2.y = temp3.y / l2; n2.z = temp3.z / l2;

	//v3-v1 && v3-v2
	temp1.x = v3.x - v1.x;
	temp1.y = v3.y - v1.y;
	temp1.z = v3.z - v1.z;

	temp2.x = v3.x - v2.x;
	temp2.y = v3.y - v2.y;
	temp2.z = v3.z - v2.z;

	temp3.x = temp1.y *temp2.z - temp1.z * temp2.y;
	temp3.y = temp1.z *temp2.x - temp1.x * temp2.z;
	temp3.z = temp1.x *temp2.y - temp1.y * temp2.x;

	l3 = sqrt(temp3.x*temp3.x + temp3.y*temp3.y + temp3.z*temp3.z);
	n3.x = temp3.x / l3; n3.y = temp3.y / l3; n3.z = temp3.z / l3;

	//average
	n.x = (n1.x + n2.x + n3.x) / 3;
	n.y = (n1.y + n2.y + n3.y) / 3;
	n.z = (n1.z + n2.z + n3.z) / 3;
	return n;
}

//========================================================================
// Draw scene
//========================================================================

void draw_scene(GLFWwindow* window)
{
	// Clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// We don't want to modify the projection matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Move back
	glTranslatef(0.0, 0.0, -zoom);
	// Rotate the view
	glRotatef(beta, 1.0, 0.0, 0.0);
	glRotatef(alpha, 0.0, 0.0, 1.0);

	glDrawElements(GL_QUADS, 4 * QUADNUM, GL_UNSIGNED_INT, quad);

	glfwSwapBuffers(window);
}


//========================================================================
// Initialize Miscellaneous OpenGL state
//========================================================================

void init_opengl(void)
{
	// Use Gouraud (smooth) shading
	glShadeModel(GL_SMOOTH);

	// Switch on the z-buffer
	glEnable(GL_DEPTH_TEST);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(struct Vertex), vertex);
	glColorPointer(3, GL_FLOAT, sizeof(struct Vertex), &vertex[0].r); // Pointer to the first color

	glPointSize(2.0);

	// Background color is black
	glClearColor(0, 0, 0, 0);
}


//========================================================================
// Modify the height of each vertex according to the pressure
//========================================================================

void adjust_grid(void)
{
	int pos;
	int x, y;

	for (y = 0; y < GRIDH; y++)
	{
		for (x = 0; x < GRIDW; x++)
		{
			pos = y * GRIDW + x;

			vertex[pos].z = (float)(p[x][y] * (1.0 / 50.0));

		}
	}
}


//========================================================================
// Calculate wave propagation
//========================================================================

void calc_grid(void)
{
	int x, y, x2, y2;
	double time_step = dt * ANIMATION_SPEED;

	// Compute accelerations
	for (x = 0; x < GRIDW; x++)
	{
		x2 = (x + 1) % GRIDW;
		for (y = 0; y < GRIDH; y++)
			ax[x][y] = p[x][y] - p[x2][y];
	}

	for (y = 0; y < GRIDH; y++)
	{
		y2 = (y + 1) % GRIDH;
		for (x = 0; x < GRIDW; x++)
			ay[x][y] = p[x][y] - p[x][y2];
	}

	// Compute speeds
	for (x = 0; x < GRIDW; x++)
	{
		for (y = 0; y < GRIDH; y++)
		{
			vx[x][y] = vx[x][y] + ax[x][y] * time_step;
			vy[x][y] = vy[x][y] + ay[x][y] * time_step;
		}
	}

	// Compute pressure
	for (x = 1; x < GRIDW; x++)
	{
		x2 = x - 1;
		for (y = 1; y < GRIDH; y++)
		{
			y2 = y - 1;
			p[x][y] = p[x][y] + (vx[x2][y] - vx[x][y] + vy[x][y2] - vy[x][y]) * time_step;
		}
	}

	// Compute normal and average height
	int pos;
	struct Vertex v1, v2, v3, v4, n1, n2, n3, n4, n;
	for (x = 0; x < QUADW; x++)
	{
		for (y = 0; y < QUADH; y++)
		{
			v1 = vertex[y * GRIDW + x];
			v2 = vertex[y * GRIDW + x + 1];
			v3 = vertex[(y + 1) * GRIDW + x + 1];
			v4 = vertex[(y + 1) * GRIDW + x];

			//cal normal of v1 v2 v3
			n1 = compute_normal(v1, v2, v3);

			//cal normal of v3 v4 v1
			n2 = compute_normal(v3, v4, v1);

			//cal normal of v1 v2 v4
			n3 = compute_normal(v1, v2, v4);

			//cal normal of v4 v3 v2
			n4 = compute_normal(v4, v3, v2);

			//cal average
			n.x = (n1.x + n2.x + n3.x + n4.x) / 4;
			n.y = (n1.y + n2.y + n3.y + n4.y) / 4;
			n.z = (n1.z + n2.z + n3.z + n4.z) / 4;

			//save to the array
			normx[x][y] = n.x;
			normy[x][y] = n.y;
			normz[x][y] = n.z;

			avgHeight[x][y] = (v1.z + v2.z + v3.z + v4.z) / 4;
		}
	}

}

//========================================================================
// Print errors
//========================================================================

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}


//========================================================================
// Handle key strokes
//========================================================================

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
		return;

	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, GL_TRUE);
		break;
	case GLFW_KEY_SPACE:
		init_grid();
		break;
	case GLFW_KEY_LEFT:
		alpha += 5;
		break;
	case GLFW_KEY_RIGHT:
		alpha -= 5;
		break;
	case GLFW_KEY_UP:
		beta -= 5;
		break;
	case GLFW_KEY_DOWN:
		beta += 5;
		break;
	case GLFW_KEY_PAGE_UP:
		zoom -= 0.25f;
		if (zoom < 0.f)
			zoom = 0.f;
		break;
	case GLFW_KEY_PAGE_DOWN:
		zoom += 0.25f;
		break;
	default:
		break;
	}
}


//========================================================================
// Callback function for mouse button events
//========================================================================

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	if (action == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwGetCursorPos(window, &cursorX, &cursorY);
	}
	else
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


//========================================================================
// Callback function for cursor motion events
//========================================================================

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
	{
		alpha += (GLfloat)(x - cursorX) / 10.f;
		beta += (GLfloat)(y - cursorY) / 10.f;

		cursorX = x;
		cursorY = y;
	}
}


//========================================================================
// Callback function for scroll events
//========================================================================

void scroll_callback(GLFWwindow* window, double x, double y)
{
	zoom += (float)y / 4.f;
	if (zoom < 0)
		zoom = 0;
}


//========================================================================
// Callback function for framebuffer resize events
//========================================================================

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	float ratio = 1.f;
	mat4x4 projection;

	if (height > 0)
		ratio = (float)width / (float)height;

	// Setup viewport
	glViewport(0, 0, width, height);

	// Change to the projection matrix and set our viewing volume
	glMatrixMode(GL_PROJECTION);
	mat4x4_perspective(projection,
		60.f * (float)M_PI / 180.f,
		ratio,
		1.f, 1024.f);
	glLoadMatrixf((const GLfloat*)projection);
}


//========================================================================
// main
//========================================================================

int main(int argc, char* argv[])
{
	GLFWwindow* window;
	double t, dt_total, t_old;
	int width, height;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	window = glfwCreateWindow(640, 480, "Wave Simulation", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwGetFramebufferSize(window, &width, &height);
	framebuffer_size_callback(window, width, height);

	// Initialize OpenGL
	init_opengl();

	// Initialize simulation
	init_vertices();
	init_grid();
	adjust_grid();


	// Initialize timer
	t_old = glfwGetTime() - 0.01;

	while (!glfwWindowShouldClose(window))
	{
		t = glfwGetTime();
		dt_total = t - t_old;
		t_old = t;

		// Safety - iterate if dt_total is too large
		while (dt_total > 0.f)
		{
			// Select iteration time step
			dt = dt_total > MAX_DELTA_T ? MAX_DELTA_T : dt_total;
			dt_total -= dt;

			// Calculate wave propagation
			calc_grid();
		}

		// Compute height of each vertex
		adjust_grid();

		// Draw wave grid to OpenGL display
		draw_scene(window);

		glfwPollEvents();
	}

	exit(EXIT_SUCCESS);
}

