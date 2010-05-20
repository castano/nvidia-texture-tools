// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>

#include <nvimage/Image.h>
#include <nvimage/DirectDrawSurface.h>

#include <GL/glew.h>
#include <GL/glut.h>

#include "cmdline.h"

GLuint tex0, tex1;

float scale = 1.0f;
float tx = 0, ty = 0;


void initOpengl()
{
    glewInit();

    if (!glewIsSupported(
        "GL_VERSION_2_0 "
        "GL_ARB_vertex_program "
        "GL_ARB_fragment_program "
        ))
    {
        printf("Unable to load required extension\n");
        exit(-1);
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glutReportErrors();
}

GLuint createTexture(GLenum target, GLint internalformat, GLenum format, GLenum type, int w, int h)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(target, tex);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(target, 0, internalformat, w, h, 0, format, type, 0);
    return tex;
}

void drawQuad()
{
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, -1.0);
    glTexCoord2f(1.0, 0.0); glVertex2f(1.0, -1.0);
    glTexCoord2f(1.0, 1.0); glVertex2f(1.0, 1.0);
    glTexCoord2f(0.0, 1.0); glVertex2f(-1.0, 1.0);
    glEnd();
}


void glutKeyboardCallback(unsigned char key, int x, int y)
{
    switch(key) {
    case 27:
    case 'q':
        exit(0);
        break;

    case '=':
    case '+':
        scale *= 1.5;
        break;

    case '-':
    case '_':
        scale /= 1.5;
        break;

    case 'r':
        scale = 1.0;
        tx = ty = 0.0;
        break;
    }
}

void glutKeyboardUpCallback(unsigned char key, int x, int y)
{
}

void special(int key, int x, int y)
{
    switch(key) {
    case GLUT_KEY_RIGHT:
        tx -= 0.1;
        break;
    case GLUT_KEY_LEFT:
        tx += 0.1;
        break;
    case GLUT_KEY_DOWN:
        ty += 0.1;
        break;
    case GLUT_KEY_UP:
        ty -= 0.1;
        break;
    }
    glutPostRedisplay();
}


void glutDisplayCallback(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawQuad();

	glutSwapBuffers();
}

void glutIdleCallback(void)
{
	//glutPostRedisplay();
}





int main(int argc, char *argv[])
{
	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	if (argc != 2 && argc != 3)
	{
		printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007\n\n");
		printf("usage: nvddsview file0 [file1]\n\n");
		return 1;
	}

	// Load surface.
	nv::DirectDrawSurface dds(argv[1]);
	if (!dds.isValid())
	{
		printf("The file '%s' is not a valid DDS file.\n", argv[1]);
		return 1;
	}
	
	uint w = dds.width();
	uint h = dds.height();

	// @@ Clamp window size is texture larger than desktop?


	glutInit(&argc, argv);

	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowSize( w, h );
	glutCreateWindow( "DDS View" );
	glutKeyboardFunc( glutKeyboardCallback );
	glutKeyboardUpFunc( glutKeyboardUpCallback );
	glutDisplayFunc( glutDisplayCallback );
	glutIdleFunc( glutIdleCallback );

	initOpengl();

	/*
	glGenTextures(1, &tex0);
	glBindTexture(GL_TEXTURE_2D, tex0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img_w, img_h, 0, image.getFormat(), image.getType(), image.getLevel(0));
	*/
	//tex0 = createTexture(GL_TEXTURE_2D, GL_RGBA, GL_RGBA, type, w, h,);

	// @@ Add IMGUI, fade in and out depending on mouse movement.


	glutMainLoop();

	return 0;
}

