/*
 * chair.cpp
 *
 *  Hanah Deering (hanah.deering@snhu.edu)
 *
 *  
 */

/* Header Inclusions */
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

/* GLM Math Header inclusions */
#include <GL/glm/glm.hpp>
#include <GL/glm/gtc/matrix_transform.hpp>
#include <GL/glm/gtc/type_ptr.hpp>

/* Soil Image Loader inclusion */
#include "SOIL2/SOIL2.h"

using namespace std; // standard namespace

#define WINDOW_TITLE "Hanah Deering | Zig Zag Chair - Designed by Gerrit Rietveld (1934)" // window title macro

/* Shader Program Macro */
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

/* Variable declarations for shader, window size initialization, buffer and array objects */
GLint objShaderProgram, lampShaderProgram, WindowWidth = 1064, WindowHeight = 800;
GLuint ObjVAO, LightVAO, VBO, EBO, texture;

GLfloat cameraSpeed = 0.005f; // movement speed per frame
GLfloat zoomSpeed = 0.005f;	  // movement speed per frame when using mouse

// keyboard global variables
GLchar currentKey;				// will store key pressed
GLchar currentProjection = 'p'; // initial projection as perspective
GLchar userSelection;			// will store user projection selection
int mouseButton, mouseState;	// will store mouse click pressed

// mouse global variables
GLfloat lastMouseX = 532, lastMouseY = 400;					  // locks mouse cursor at the center of the screen
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f; // mouse offset, yaw, and pitch variables
GLfloat sensitivity = 0.005f;								  // Used for mouse / camera rotation sensitivity

// light0 position, color, and scale
glm::vec3 light0Position(-4.0f, 2.0f, -5.0f);
glm::vec3 light0Scale(0.3f);
glm::vec3 light0Color(1.0f, 1.0f, 0.0f);

// light1 position, color, and scale
glm::vec3 light1Position(4.0f, -2.0f, 5.0f);
glm::vec3 light1Scale(0.3f);
glm::vec3 light1Color(0.8f, 0.4f, 0.6f);

// global vector declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);	 // Initial camera position.
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f);		 // Temporary Y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f); // Temporary Z unit vector
glm::vec3 front;										 // Temporary z unit vector for mouse

/* Function Prototypes */
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UGenerateTexture(void);
void UKeyboard(unsigned char key, int x, int y);
void UKeyReleased(unsigned char key, int x, int y);
void UMouseClick(int button, int state, int x, int y);
void UMousePers(int x, int y);
void UMouseOrtho(int x, int y);
void WireframeModeOn();
void WireframeModeOff();
void UControls();

/* Object Vertex Shader Source Code */
const GLchar *objVertexShaderSource = GLSL(
	330,
	layout(location = 0) in vec3 position;			 // VAP position 0 for vector position data
	layout(location = 1) in vec3 normal;			 // VAP position 1 for normals
	layout(location = 2) in vec2 textureCoordinates; // VAP position 2 for texture

	out vec3 Normal;			   // for outgoing normals to fragment shader
	out vec3 FragmentPos;		   // for outgoing color / pixels to fragment shader
	out vec2 objTextureCoordinate; // texture coordinates

	// global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(position, 1.0f);					// transforms vertices to clip coordinates
		FragmentPos = vec3(model * vec4(position, 1.0f));								// gets fragment / pixel position in world space only (exclude view and projection)
		Normal = mat3(transpose(inverse(model))) * normal;								// get normal vectors in world space only and exclude normal translation properties
		objTextureCoordinate = vec2(textureCoordinates.x, 1.0f - textureCoordinates.y); // flips of texture horizontally
	});

/* Object Fragment Shader Source Code */
const GLchar *objFragmentShaderSource = GLSL(
	330,
	in vec3 Normal;		 // for incoming normal
	in vec3 FragmentPos; // for incoming fragment position
	in vec2 objTextureCoordinate;

	out vec4 objColor; // Variable to pass phong data to the GPU

	uniform sampler2D uTexture; // useful when working with multiple textures

	// uniform global variables for key and fill light color, key and fill light position, and camera/view position
	uniform vec3 light0Color;
	uniform vec3 light1Color;
	uniform vec3 lightPos;
	uniform vec3 viewPosition;

	/*--- phong light model calculations to generate ambient, diffuse, and specular components ---*/
	vec3 LightCalc(vec3 fragPos, vec3 objTex, vec3 norm, vec3 viewDir, vec3 lightColor, vec3 lightPos) {
		// calculate ambient lighting
		float ambientStrength = 0.1f;				 // set ambient or global lighting strength
		vec3 ambient = ambientStrength * lightColor; // generate ambient light color

		// calculate diffuse lighting
		vec3 lightDirection = normalize(lightPos - fragPos); // calculate distance (light direction) between light source and fragment/pixels on
		float impact = max(dot(norm, lightDirection), 0.1);	 // calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse = impact * lightColor;					 // generate diffuse light color

		// calculate specular lighting
		float highlightSize = 16.0f;					  // set specular highlight size
		vec3 reflectDir = reflect(-lightDirection, norm); // calculate reflection vector

		// calculate specular component
		float specularIntensity = 0.5f; // set specular light strength
		float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;

		// return calculated value
		return (ambient + diffuse + specular) * objTex;
	}

	void main() {
		// properties
		vec3 objTexture = texture(uTexture, objTextureCoordinate).xyz; // sends texture to the GPU for rendering
		vec3 norm = normalize(Normal);								   // normalize vectors to 1 unit
		vec3 viewDir = normalize(viewPosition - FragmentPos);		   // calculate view direction

		/*--- Calculate Phong Value---*/
		// light0
		vec3 phong = LightCalc(FragmentPos, objTexture, norm, viewDir, light0Color, lightPos);
		phong += LightCalc(FragmentPos, objTexture, norm, viewDir, light1Color, lightPos);

		objColor = vec4(phong, 1.0f); // send lighting results to GPU
	});

/* Lamp Shader Source Code */
const GLchar *lampVertexShaderSource = GLSL(
	330,
	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

	// uniform global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices into clip coordinates
	});

/* Lamp Fragment Shader Source Code */
const GLchar *lampFragmentShaderSource = GLSL(
	330,
	out vec4 color; // for outgoing lamp color (smaller pyramid) to the GPU

	void main() {
		color = vec4(1.0f); // set color to white (1.0f, 1.0f, 1.0f) with alpha 1.0
	});

/* Main Program */
int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);

	glutReshapeFunc(UResizeWindow);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	UControls();		// display user controls on console
	UCreateShader();	// create shader
	UCreateBuffers();	// create buffer
	UGenerateTexture(); // create texture

	glClearColor(0.9f, 0.9f, 0.9f, 0.5f); // set background color
	glutDisplayFunc(URenderGraphics);	  // render object

	// keyboard controls
	glutKeyboardFunc(UKeyboard);	  // detects key press
	glutKeyboardUpFunc(UKeyReleased); // detects key release

	// mouse operation
	glutMouseFunc(UMouseClick);		   // detects mouse click
	glutPassiveMotionFunc(UMousePers); // detects mouse movement in perspective
	glutMotionFunc(UMouseOrtho);	   // detects mouse movement in orthographic

	glutMainLoop();

	// destroy buffer objects once used
	glDeleteVertexArrays(1, &ObjVAO);
	glDeleteVertexArrays(1, &LightVAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	return 0;
}

/* Resizes The Window */
void UResizeWindow(int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

/* Renders Graphics */
void URenderGraphics(void)
{
	glEnable(GL_DEPTH_TEST);							// enable z-depth
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clears the screen
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// really nice perspective calculations

	glBindVertexArray(ObjVAO); // activate the vertex Array Object before rendering and transforming them

	// object panning left
	if (currentKey == 'l')
	{
		cameraPosition -= glm::normalize(glm::cross(CameraForwardZ, CameraUpY)) * cameraSpeed;
	}
	// object panning right
	if (currentKey == 'r')
	{
		cameraPosition += glm::normalize(glm::cross(CameraForwardZ, CameraUpY)) * cameraSpeed;
	}
	// object panning up
	if (currentKey == 'u')
	{
		cameraPosition -= CameraUpY * cameraSpeed;
	}
	// object panning down
	if (currentKey == 'd')
	{
		cameraPosition += CameraUpY * cameraSpeed;
	}

	// camera reset
	if (currentKey == 'q')
	{
		cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	// wireframe mode of the object
	if (currentKey == 'w')
		WireframeModeOn(); // enable wireframe

	if (currentKey == 'f')
		WireframeModeOff();
	; // disable wireframe

	CameraForwardZ = front; // replaces camera forward vector with Radians normalized as a unit vector

	GLint modelLoc, viewLoc, projLoc, uTextureLoc, viewPositionLoc, light0ColorLoc, light0PositionLoc, light1ColorLoc, light1PositionLoc;

	glm::mat4 model = glm::mat4(1.0);
	glm::mat4 view;
	glm::mat4 projection;

	/*** Use the object shader and activate the object VAO for rendering and transforming ***/
	glUseProgram(objShaderProgram);
	glBindVertexArray(ObjVAO);

	// transforms the object
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));		 // place the object at the center of the viewport
	model = glm::rotate(model, 180.0f, glm::vec3(0.0f, 1.0f, 0.0f)); // rotate the object 180 degrees on Y axis
	model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));			 // increase the object size by a scale of 2

	// transforms the camera
	view = glm::lookAt(cameraPosition - CameraForwardZ, cameraPosition, CameraUpY);

	// creates an orthographic and perspective projection
	if (currentProjection == 'o' || userSelection == 'o')
	{
		// orthographic projection
		projection = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, 0.1f, 100.0f);
		userSelection = 'o';
	}

	if (currentProjection == 'p' || userSelection == 'p')
	{
		// perspective projection
		projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);
		userSelection = 'p';
	}

	// retrieves and passes transform matrices to the shader program
	modelLoc = glGetUniformLocation(objShaderProgram, "model");
	viewLoc = glGetUniformLocation(objShaderProgram, "view");
	projLoc = glGetUniformLocation(objShaderProgram, "projection");

	// pass matrix data to the shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// reference matrix uniforms from the pyramid shader program for the pyramid color, light color, light position, and camera position
	uTextureLoc = glGetUniformLocation(objShaderProgram, "uTexture");
	light0ColorLoc = glGetUniformLocation(objShaderProgram, "light0Color");
	light0PositionLoc = glGetUniformLocation(objShaderProgram, "light0Pos");
	light1ColorLoc = glGetUniformLocation(objShaderProgram, "light1Color");
	light1PositionLoc = glGetUniformLocation(objShaderProgram, "light1Pos");
	viewPositionLoc = glGetUniformLocation(objShaderProgram, "viewPosition");

	// pass color, light, and camera data to the pyramid shader program's corresponding uniforms
	glUniform1i(uTextureLoc, 0);
	glUniform3f(light0ColorLoc, light0Color.r, light0Color.g, light0Color.b);
	glUniform3f(light0PositionLoc, light0Position.x, light0Position.y, light0Position.z);
	glUniform3f(light1ColorLoc, light1Color.r, light1Color.g, light1Color.b);
	glUniform3f(light1PositionLoc, light1Position.x, light1Position.y, light1Position.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	glBindTexture(GL_TEXTURE_2D, texture); // activate object texture

	glDrawElements(GL_TRIANGLES, 126, GL_UNSIGNED_INT, 0); // draws object triangles

	/*** Use the lamps shader and activate the lamp VAO for rendering and transforming ***/
	glUseProgram(lampShaderProgram);
	glBindVertexArray(LightVAO);

	// transform the smaller object used as a visual que for the key light source
	model = glm::translate(model, light0Position);
	model = glm::scale(model, light0Scale);
	model = glm::translate(model, light1Position);
	model = glm::scale(model, light1Scale);

	// reference matrix uniforms from the lamp shader program
	modelLoc = glGetUniformLocation(lampShaderProgram, "model");
	viewLoc = glGetUniformLocation(lampShaderProgram, "view");
	projLoc = glGetUniformLocation(lampShaderProgram, "projection");

	// pass matrix data to the lamp shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glDrawElements(GL_TRIANGLES, 126, GL_UNSIGNED_INT, 0); // draws small object triangles

	glBindVertexArray(0); // deactivate the VAO

	glutPostRedisplay();
	glutSwapBuffers(); // flips the back buffer with the front buffer every frame
}

/* Creates the Shader Program */
void UCreateShader(void)
{
	// object vertex shader
	GLint objVertexShader = glCreateShader(GL_VERTEX_SHADER);		  // creates the vertex shader
	glShaderSource(objVertexShader, 1, &objVertexShaderSource, NULL); // attaches the vertex shader to the source code
	glCompileShader(objVertexShader);								  // compiles the vertex shader

	// object fragment shader
	GLint objFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);		  // creates the fragment shader
	glShaderSource(objFragmentShader, 1, &objFragmentShaderSource, NULL); // attaches the fragment shader to the source code
	glCompileShader(objFragmentShader);									  // compiles the fragment shader

	// object shader program
	objShaderProgram = glCreateProgram();				 // creates the shader program
	glAttachShader(objShaderProgram, objVertexShader);	 // attach vertex shader to the shader program
	glAttachShader(objShaderProgram, objFragmentShader); // attach fragment shader to shader program
	glLinkProgram(objShaderProgram);					 // link vertex and fragment shaders to shader program

	// delete the vertex and fragment shaders once linked
	glDeleteShader(objVertexShader);
	glDeleteShader(objFragmentShader);

	// lamp vertex shader
	GLint lampVertexShader = glCreateShader(GL_VERTEX_SHADER);			// creates the vertex shader
	glShaderSource(lampVertexShader, 1, &lampVertexShaderSource, NULL); // attaches the vertex shader to the source code
	glCompileShader(lampVertexShader);									// compiles the vertex shader

	// lamp fragment shader
	GLint lampFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);			// creates the fragment shader
	glShaderSource(lampFragmentShader, 1, &lampFragmentShaderSource, NULL); // attaches the fragment shader to the source code
	glCompileShader(lampFragmentShader);									// compiles the fragment shader

	// lamp shader program
	lampShaderProgram = glCreateProgram();				   // creates the lamp shader program
	glAttachShader(lampShaderProgram, lampVertexShader);   // attach vertex shader to the shader program
	glAttachShader(lampShaderProgram, lampFragmentShader); // attach fragment shader to shader program
	glLinkProgram(lampShaderProgram);					   // link vertex and fragment shaders to shader program

	// delete the vertex and fragment shaders once linked
	glDeleteShader(lampVertexShader);
	glDeleteShader(lampFragmentShader);
}

void UCreateBuffers(void)
{
	// position and color data
	GLfloat vertices[] = {
		// vertex positions   // normals		  // texture coord
		0.45f, -0.80f, 0.35f, -1.0f, 1.0f, -1.0f, 1.0f, 0.0f,  // vertex 0
		0.45f, -0.75f, 0.35f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f,  // vertex 1
		-0.30f, -0.75f, 0.35f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f,  // vertex 2
		-0.35f, -0.70f, 0.35f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f,  // vertex 3
		0.50f, 0.20f, 0.45f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,  // vertex 4
		-0.35f, 0.20f, 0.35f, 1.0f, -1.0f, -1.0f, 0.0f, 1.0f,  // vertex 5
		-0.45f, 0.90f, 0.35f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,  // vertex 6
		-0.50f, 0.90f, 0.35f, 1.0f, -1.0f, -1.0f, 0.0f, 1.0f,  // vertex 7
		-0.40f, 0.15f, 0.35f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f,  // vertex 8
		0.30f, 0.15f, 0.45f, -1.0f, -1.0f, -1.0f, 1.0f, 0.0f,  // vertex 9
		0.35f, 0.10f, 0.45f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,  // vertex 10
		-0.50f, -0.80f, 0.35f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f,  // vertex 11
		0.45f, -0.80f, -0.35f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, // vertex 12
		0.45f, -0.75f, -0.35f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f,  // vertex 13
		-0.30f, -0.75f, -0.35f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // vertex 14
		-0.35f, -0.70f, -0.35f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  // vertex 15
		0.50f, 0.20f, -0.45f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f,   // vertex 16
		-0.35f, 0.20f, -0.35f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f,  // vertex 17
		-0.45f, 0.90f, -0.35f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f,  // vertex 18
		-0.50f, 0.90f, -0.35f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f,  // vertex 19
		-0.40f, 0.15f, -0.35f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  // vertex 20
		0.30f, 0.15f, -0.45f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f,   // vertex 21
		0.35f, 0.10f, -0.45f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f,   // vertex 22
		-0.50f, -0.80f, -0.35f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f   // vertex 23
	};

	// index data to share position data
	GLuint indices[] = {

		// base
		0, 1, 13,
		0, 12, 13,
		0, 11, 2,
		0, 1, 2,
		1, 2, 14,
		1, 13, 14,
		12, 23, 14,
		12, 13, 14,
		0, 11, 23,
		0, 12, 23,

		// leg
		2, 3, 15,
		2, 14, 15,
		2, 11, 3,
		3, 4, 16,
		3, 15, 16,
		3, 11, 10,
		3, 4, 10,
		4, 9, 10,
		14, 23, 15,
		15, 23, 22,
		15, 16, 22,
		16, 21, 22,
		11, 10, 22,
		11, 23, 22,
		9, 10, 22,
		9, 21, 22,

		// seat
		4, 5, 17,
		4, 16, 17,
		4, 5, 8,
		4, 9, 8,
		16, 17, 20,
		16, 21, 20,
		9, 8, 20,
		9, 21, 20,

		// chair back
		5, 6, 18,
		5, 17, 18,
		5, 8, 7,
		5, 6, 7,
		17, 20, 19,
		17, 18, 19,
		8, 20, 19,
		8, 7, 19,
		6, 7, 19,
		6, 18, 19};

	// generate buffer ids
	glGenVertexArrays(1, &ObjVAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// activate the vertex array object before binding and setting any VBOs and vertex attribute pointers
	glBindVertexArray(ObjVAO);

	// activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // copy vertices to VBO

	// activate the element buffer object (EOB) / indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // copy indices to EBO

	// set attribute pointer 0 to hold position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0); // Enables vertex attribute

	// set attribute pointer 1 to hold Normal data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1); // Enables vertex attribute

	// set attribute pointer 2 to hold texture coordinate data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// generate buffer for lamp (smaller pyramid)
	glGenVertexArrays(1, &LightVAO); // vertex array object for pyramid vertex copies to serve as light source

	// activate the vertex array object before binding and setting any VBOs and VAPs
	glBindVertexArray(LightVAO);

	// referencing the same VBO for its vertices
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// set attribute pointer 0 to hold position data (used for the lamp)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Deactivates the VAO which is good practice
	glBindVertexArray(0);
}

/* Generate and Load The Texture */
void UGenerateTexture()
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int width, height;

	unsigned char *image = SOIL_load_image("wood-texture1.jpg", &width, &height, 0, SOIL_LOAD_RGB); // loads texture file

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); // unbind the texture
}

/* Implements The UKeyboard Function */
void UKeyboard(unsigned char key, GLint x, GLint y)
{
	switch (key)
	{
	case 'o':
		currentProjection = key;
		cout << "Orthographic Projection Active!" << endl;
		cout << "===================================" << endl;
		cout << "Press Mouse Left Button for Orbit" << endl;
		cout << "-----------------------------------" << endl;
		break;
	case 'p':
		currentProjection = key;
		cout << "Perspective Projection Active!" << endl;
		cout << "===================================" << endl;
		cout << "Click Mouse Left Button and Press ALT + Mouse Move for Orbit" << endl;
		cout << "Click Mouse Right Button and Press ALT + Mouse Move for Zoom" << endl;
		cout << "-------------------------------------------------------------" << endl;
		break;
	case 'w':
		currentKey = key;
		cout << "Wireframe Mode Enable!" << endl;
		cout << "-----------------------------------" << endl;
		break;
	case 'f':
		currentKey = key;
		cout << "Wireframe Mode Disable!" << endl;
		cout << "-----------------------------------" << endl;
		break;
	case 'q':
		currentKey = key;
		cout << "Camera Position Reset!" << endl;
		cout << "-----------------------------------" << endl;
		break;
	case 'l':
		currentKey = key;
		break;
	case 'r':
		currentKey = key;
		break;
	case 'u':
		currentKey = key;
		break;
	case 'd':
		currentKey = key;
		break;
	default:
		cout << "xxxxxxx Not a valid key! xxxxxxx" << endl;
	}
}

/* Implements the UKeyReleased Function */
void UKeyReleased(unsigned char key, GLint x, GLint y)
{
	currentKey = '0';
	currentProjection = userSelection;
}

/* Implements the UMouseMove Function */
void UMousePers(int x, int y)
{
	if (userSelection == 'p')
	{
		if (glutGetModifiers() == GLUT_ACTIVE_ALT)
		{
			// rotate the object when ALT + Mouse Move right/left
			if (mouseButton == GLUT_LEFT_BUTTON && mouseState == GLUT_DOWN)
			{
				// gets the direction the mouse was moved in x and y
				mouseXOffset = x - lastMouseX;
				mouseYOffset = lastMouseY - y; // Inverted Y

				// applies sensitivity to mouse direction
				mouseXOffset *= sensitivity;
				mouseYOffset *= sensitivity;

				// accumulates the yaw and pitch variables
				yaw += mouseXOffset;
				pitch += mouseYOffset;

				// Maintain a 90 degree pitch for gimbal lock
				if (pitch > 89.0f)
					pitch = 89.0f;

				if (pitch < -89.0f)
					pitch = -89.0f;
			}

			// zoom in/out the object when ALT + Mouse Move up/down
			if (mouseButton == GLUT_RIGHT_BUTTON && mouseState == GLUT_DOWN)
			{
				if ((lastMouseY - y) > 0)
					cameraPosition += zoomSpeed * CameraForwardZ;
				if ((lastMouseY - y) < 0)
					cameraPosition -= zoomSpeed * CameraForwardZ;
			}
		}
	}

	// updates with new mouse coordinates
	lastMouseX = x;
	lastMouseY = y;

	// converts mouse coordinates / degrees into Radians, then to vectors
	front.x = 5.0f * cos(yaw);
	front.y = 5.0f * sin(pitch);
	front.z = sin(yaw) * cos(pitch) * 5.0f;
}

void UMouseOrtho(int x, int y)
{
	if (userSelection == 'o')
	{
		// rotate object when mouse left button is pressed
		if ((mouseButton == GLUT_LEFT_BUTTON) && (mouseState == GLUT_DOWN))
		{
			// gets the direction the mouse was moved in x and y
			mouseXOffset = x - lastMouseX;
			mouseYOffset = lastMouseY - y; // Inverted Y

			// applies sensitivity to mouse direction
			mouseXOffset *= sensitivity;
			mouseYOffset *= sensitivity;

			// accumulates the yaw and pitch variables
			yaw += mouseXOffset;
			pitch += mouseYOffset;

			// maintain a 90 degree pitch for gimbal lock
			if (pitch > 89.0f)
				pitch = 89.0f;

			if (pitch < -89.0f)
				pitch = -89.0f;
		}
	}

	// Updates with new mouse coordinates
	lastMouseX = x;
	lastMouseY = y;

	// Converts mouse coordinates / degrees into Radians, then to vectors
	front.x = 5.0 * cos(yaw);
	front.y = 5.0 * sin(pitch);
	front.z = sin(yaw) * cos(pitch) * 5.0;
}

/* Implements The UMouseClick Function */
void UMouseClick(int button, int state, int x, int y)
{
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
	{
		mouseButton = button;
		mouseState = state;
	}

	if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN))
	{
		mouseButton = button;
		mouseState = state;
	}
}

/* Draw the object in Wireframe mode */
void WireframeModeOn()
{
	glLineWidth(1.0f);						   // Add thick to the lines
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // draw wireframe lines
}

void WireframeModeOff()
{
	glLineWidth(1.0f);						   // Add thick to the lines
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // draw fills
}

/* Show in console key controls */
void UControls()
{
	cout << "[Controls]" << endl;
	cout << "----------" << endl;
	cout << "Press o for Orthographic Projection" << endl;
	cout << "Press p for Perspective Projection" << endl;
	cout << "Press w for Wireframe Mode Enable" << endl;
	cout << "Press f for Wireframe Mode Disable" << endl;
	cout << "Press q for Camera Position Reset" << endl;
	cout << "Press l for Pan Camera Left" << endl;
	cout << "Press r for Pan Camera Right" << endl;
	cout << "Press u for Pan Camera Up" << endl;
	cout << "Press d for Pan Camera Down" << endl;
	cout << "-----------------------------------" << endl;
	cout << "Perspective Projection Active!" << endl;
	cout << "===================================" << endl;
	cout << "Click Mouse Left Button and Press ALT key for Orbit" << endl;
	cout << "Click Mouse Right Button and Press ALT key for Zoom" << endl;
	cout << "----------------------------------------------------" << endl;
	cout << "Wireframe Mode Disable!" << endl;
	cout << "------------------------------------" << endl;
}
