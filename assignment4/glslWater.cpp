/*
 *  glslWater.cpp
 *  glslWater
 *
 *  Created by Michael Doggett on 15/09/10.
 *  Copyright 2010 Michael Doggett. All rights reserved.
 *
 *  Based on simple GLSL demo from www.lighthouse3d.com
 *        and gpuSPPM by Toshiya Hachisuka from http://graphics.ucsd.edu/~toshiya/
 */

#include <iostream>
#include <GL/glew.h>
#include "glut.h"
#include "lodepng.h"
#include "matrix.h"

// Macro for handling offsets into buffer objects
#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))

// constants
static const int QUAD_GRID_SIZE = 40;
static const int NR_VERTICES = (QUAD_GRID_SIZE+1)*(QUAD_GRID_SIZE+1);
static const int NR_TRIANGLES = 2*QUAD_GRID_SIZE*QUAD_GRID_SIZE;
static const int NR_INDICES = 3*NR_TRIANGLES;


// Variables
vec3 gEyePos(0, 100.0f, 140.0f); // The camera position
vec3 gViewRotate(0,0,0); // Amount of rotation
vec2 gMouseMove, gMouseCurrentPos; // Mouse input
bool gMouseLeftButton;
mat4 gProjectionMatrix;

bool gWireframe = false;
GLuint gDrawNumber = NR_INDICES; // How many indices to draw 

GLuint gShaderProgramID; // ID for shader program	
GLuint gVaoID;			 // ID for vertex array object
GLuint gVboID;			 // ID for vertex array object
GLuint gIndexID;		 // ID for vertex array object

// Function forward declarations
GLuint initShaders();
void initTextures();
void init(); 
void renderScene(const float time);
void loadImage( char* filename, unsigned char* &imgData, int &width, int &height);
const char* textFileRead(char *fn);
void printInfoLogs(GLuint obj, GLuint shader);
void processNormalKeys(unsigned char key, int x, int y);
void processMouse(int button, int state, int x, int y);
void processMouseActiveMotion(int x, int y);
void resize(int w, int h);
void cleanup();

// Function definitions

GLuint initShaders() {
		
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint pixelShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	
	const char* vs = textFileRead("../glslWater.vert");
	const char* fs = textFileRead("../glslWater.frag");
	
	glShaderSource(vertexShaderID, 1, &vs,NULL);
	glShaderSource(pixelShaderID, 1, &fs,NULL);
	
	delete [] vs;
	delete [] fs;
	
	glCompileShader(vertexShaderID);
	glCompileShader(pixelShaderID);
	
	GLuint program = glCreateProgram();
	glAttachShader(program,pixelShaderID);
	printInfoLogs(program,pixelShaderID);
	glAttachShader(program,vertexShaderID);
	printInfoLogs(program,vertexShaderID);
	
	glLinkProgram(program);
	glUseProgram(program);

	return program;
}

void init() {
	//Generate grid positions
	const float scale = 50.0f;
	const float delta = 2.0f/QUAD_GRID_SIZE;

	vec3* vertices = new vec3[NR_VERTICES];
	vec2* texcoords = new vec2[NR_VERTICES];
	GLuint* indices = new GLuint[3*NR_TRIANGLES];

	for (int y=0; y<=QUAD_GRID_SIZE; y++) {
		for (int x=0; x<=QUAD_GRID_SIZE; x++) {
			int vertexPosition = y*(QUAD_GRID_SIZE+1) + x;
			vertices[vertexPosition].x = ( x*delta - 1.0 ) * scale;
			vertices[vertexPosition].y = 0;
			vertices[vertexPosition].z = ( y*delta - 1.0 ) * scale ;
			texcoords[vertexPosition].x = x*delta;
			texcoords[vertexPosition].y = y*delta;
		}
	}

	// Generate indices into vertex list
	for (int y=0; y<QUAD_GRID_SIZE; y++) {
		for (int x=0; x<QUAD_GRID_SIZE; x++) {
			int indexPosition = y*QUAD_GRID_SIZE + x;
			// tri 0
			indices[6*indexPosition  ] = y    *(QUAD_GRID_SIZE+1) + x;    //bl  
			indices[6*indexPosition+1] = (y+1)*(QUAD_GRID_SIZE+1) + x + 1;//tr
			indices[6*indexPosition+2] = y    *(QUAD_GRID_SIZE+1) + x + 1;//br
			// tri 1
			indices[6*indexPosition+3] = y    *(QUAD_GRID_SIZE+1) + x;    //bl
			indices[6*indexPosition+4] = (y+1)*(QUAD_GRID_SIZE+1) + x;    //tl
			indices[6*indexPosition+5] = (y+1)*(QUAD_GRID_SIZE+1) + x + 1;//tr
		}
	}

    // Create a vertex array object
    glGenVertexArrays( 1, &gVaoID );
    glBindVertexArray( gVaoID );

    // Create and initialize a buffer object
    glGenBuffers( 1, &gVboID );
    glBindBuffer( GL_ARRAY_BUFFER, gVboID );
    glBufferData( GL_ARRAY_BUFFER, NR_VERTICES*sizeof(vertices[0])+NR_VERTICES*sizeof(texcoords[0]), 
		NULL, GL_STATIC_DRAW );

	// Set the buffer pointers
	glBufferSubData( GL_ARRAY_BUFFER, 0, NR_VERTICES*sizeof(vertices[0]), vertices);
	glBufferSubData( GL_ARRAY_BUFFER, NR_VERTICES*sizeof(vertices[0]),
					 NR_VERTICES*sizeof(texcoords[0]), texcoords);

	// Bind the index buffer
	glGenBuffers( 1, &gIndexID );
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*NR_TRIANGLES*sizeof(indices[0]), indices, GL_STATIC_DRAW);

	// Initalize shaders
	gShaderProgramID  = initShaders();

	// Initialize the vertex position attribute from the vertex shader
    GLuint pos = glGetAttribLocation( gShaderProgramID, "vPosition" );
    glEnableVertexAttribArray( pos );
    glVertexAttribPointer( pos, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(0) );

    GLuint tex = glGetAttribLocation( gShaderProgramID, "vTexCoord" );
    glEnableVertexAttribArray( tex );
    glVertexAttribPointer( tex, 2, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(NR_VERTICES*sizeof(vertices[0])) );

	delete [] vertices;
	delete [] texcoords;
	delete [] indices;

	initTextures();

	glUniform1i(glGetUniformLocation(gShaderProgramID, "SkyboxTexture"), 0);
	glUniform1i(glGetUniformLocation(gShaderProgramID, "BumpMapTexture"), 1);
}

// Error checking for GLSL
// (from http://www.mathematik.uni-dortmund.de/~goeddeke/gpgpu/tutorial.html)
void printInfoLogs(GLuint obj, GLuint shader)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;
	
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1)
	{
		infoLog = (char *)malloc(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		std::cerr << infoLog << std::endl;
		free(infoLog);
	}
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1)
	{
		infoLog = (char *)malloc(infologLength);
		glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
		std::cerr << infoLog << std::endl;
		free(infoLog);
	}
}

void loadImage( char* filename, unsigned char* &imgData, int &width, int &height)
{
	int x,y,c;
	std::vector<unsigned char> buffer, image;
	LodePNG::loadFile(buffer, filename); //load the image file with given filename
	LodePNG::Decoder decoder;
	decoder.decode(image, buffer.size() ? &buffer[0] : 0, (unsigned)buffer.size()); //decode the png

	printf("loading %s, ",filename);
	//if there's an error, display it
	if(decoder.hasError()) std::cout << "error: " << decoder.getError() << std::endl;

	width = decoder.getWidth();
	height = decoder.getHeight();
	int numChannels = decoder.getChannels();
	imgData = new unsigned char[4*width * height];

	printf("tex %d,%d, channels %d\n",width, height, numChannels);
	for(y = 0; y < height; y++)
		for(x = 0; x < width; x++) {
			for(c = 0; c < numChannels; c++) {
				imgData[4*(y*width + x) + c] =  image[4 * (width * y + x) + c];
			}
			imgData[4*(y*width + x) + 3] = 0xff;
		}
}

void initTextures()
{
	int width, height;
	unsigned char* imgData;

	GLuint SkyboxTextureID;
	GLuint BumpMapTextureID;

	loadImage((char*) "../waves.png", imgData, width, height);

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &BumpMapTextureID);
	glBindTexture(GL_TEXTURE_2D, BumpMapTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, imgData);
	
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &SkyboxTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, SkyboxTextureID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	loadImage((char*)"../cloudyhills_posx.png", imgData, width, height);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, imgData);
	loadImage((char*)"../cloudyhills_negx.png", imgData, width, height);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, imgData);
	loadImage((char*)"../cloudyhills_posy.png", imgData, width, height);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, imgData);
	loadImage((char*)"../cloudyhills_negy.png", imgData, width, height);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, imgData);
	loadImage((char*)"../cloudyhills_posz.png", imgData, width, height);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, imgData);
	loadImage((char*)"../cloudyhills_negz.png", imgData, width, height);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, imgData);

	delete [] imgData;
}

const char* textFileRead(char *fn) {
	
	FILE *fp;
	char *content = NULL;
	
	int count=0;
	
	if (fn != NULL) {
		fopen_s(&fp, fn,"rt");
		
		if (fp != NULL) {
			
			fseek(fp, 0, SEEK_END);
			count = ftell(fp);
			rewind(fp);
			
			if (count > 0) {
				content = (char *)malloc(sizeof(char) * (count+1));
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		} else {
			printf("EXITING : couldn't load shader %s\n",fn);
			exit(77);
		}
	}
	return content;
}

// If the size of the window changed, call this to update the GL matrices
void resize(int w, int h) {
	
	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if(h == 0)
		h = 1;
	
	float aspect = ((float)w) / h;

	// Calculate the projection matrix    
    float fovy = 45.0f;
    float near = 0.01f;
    float far = 1000.0f;
    gProjectionMatrix = Perspective(fovy, aspect, near, far);

	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);
}

void renderScene() {
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Calculate the modelview matrix to render our character
	//  at the proper position and rotation    
    vec3 at(0.0,0.0,0.0);
    vec3 up(0.0,1.0,0.0);
    mat4 View = LookAt(gEyePos, at, up ); // Defines View Matrix
    
	gViewRotate.x += 1000.0 * (gMouseCurrentPos.x - gMouseMove.x) / ((float)glutGet(GLUT_SCREEN_WIDTH));
	gViewRotate.y += 1000.0 * (gMouseCurrentPos.y - gMouseMove.y) / ((float)glutGet(GLUT_SCREEN_HEIGHT));
	gMouseCurrentPos = gMouseMove;
    
    mat4 RX, RY;
    RX.rotX(-gViewRotate.y);
    RY.rotY(-gViewRotate.x);
    mat4 MV = View*RX*RY; // ModelView Matrix
    
    mat4 MVP = gProjectionMatrix*MV;
           	
	// Pass the modelview projection matrix to the shader
    GLuint mvpID = glGetUniformLocation(gShaderProgramID,"ModelViewProjectionMatrix");
    glUniformMatrix4fv(mvpID, 1, GL_TRUE, (GLfloat*)MVP.getFloatArray());
    
    // Set camera pos in object space    
    mat4 MVI = MV.inverse(); // ModelViewInverse
	// Camera Object Pos = MVI*(0,0,0,1)T

	GLuint camPosID = glGetUniformLocation(gShaderProgramID,"vCameraPos");
    glUniform3f(camPosID, MVI(0,3), MVI(1,3), MVI(2,3));

	// draw a tessellated quad
	if (gWireframe) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
	} else {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	    glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}

	float time = glutGet(GLUT_ELAPSED_TIME)/1000.0f;
	glUniform1f(glGetUniformLocation(gShaderProgramID,"time"), time);		
	glDrawElements(GL_TRIANGLES,  gDrawNumber, GL_UNSIGNED_INT, 0);

	glutSwapBuffers();
}

void processNormalKeys(unsigned char key, int x, int y) 
{
	switch (key) {
		case 27:
			exit(0);
			break;
		case 'd': case 'D':
			gWireframe = !gWireframe;
			break;
 		case '+' :
			gDrawNumber += 3; 
			printf("drawNumber %d\n",gDrawNumber);
			break;
		case '-':
			gDrawNumber -= 3; 
			printf("drawNumber %d\n",gDrawNumber);
			break;
		case 'w': case 'W':
			gEyePos.y -= 5.0;
			gEyePos.z = gEyePos.y;
			break;
		case 's': case 'S':
			gEyePos.y += 5.0;
			gEyePos.z = gEyePos.y;
			break;
		default:
			break;
	}
}

void processMouse(int button, int state, int x, int y) {
	if (button==GLUT_LEFT_BUTTON) {
		gMouseLeftButton = (state==GLUT_DOWN)?true:false;
		printf("gMouseLeftButton %d\n",(gMouseLeftButton)?1:0);
		gMouseCurrentPos = vec2(x,y);
		gMouseMove = vec2(x,y);
	}
}

void processMouseActiveMotion(int x, int y) {
	if (gMouseLeftButton) {
		printf("motion %f,%f\n",gMouseMove.x, gMouseMove.y);
		gMouseMove = vec2(x,y);
	}
}

void cleanup() {
	glDeleteBuffers(1, &gVboID);
	glDeleteBuffers(1, &gIndexID);
	glDeleteVertexArrays(1, &gVaoID);
	glDeleteProgram(gShaderProgramID);
}

int main(int argc, char **argv) {
    
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(512,512);
	glutCreateWindow("GLSL Water");
	
    GLenum err = glewInit();

	// Test for OpenGL 3
	if (GLEW_VERSION_3_0)
	{
		printf("GL version 3 supported \n");
	}
	
    glutDisplayFunc(renderScene);
	glutIdleFunc(renderScene);
	glutReshapeFunc(resize);
	glutKeyboardFunc(processNormalKeys);
	glutMouseFunc(processMouse);
	glutMotionFunc(processMouseActiveMotion);

	glClearColor(0.3,0.4,0.6,1.0);
			
	init();
	
	glutMainLoop();
	
	// delete GL objects 
	cleanup();

	return 0;
}









