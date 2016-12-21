#ifndef GLPLAYER_H
#define GLPLAYER_H

#include <QObject>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <cassert>
#include "bcm_host.h"

typedef struct
{
    // Handle to a program object
    GLuint programObject;
   // Attribute locations
   GLint  positionLoc;
   GLint  texCoordLoc;
   // Sampler location
   GLint samplerLoc;
   // Texture handle
   GLuint textureId;
} UserData;

typedef struct CUBE_STATE_T
{
    uint32_t width;
    uint32_t height;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

    EGL_DISPMANX_WINDOW_T nativewindow;
    UserData *user_data;

    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
} CUBE_STATE_T;

class GlPlayer{
private:

    GLuint createSimpleTexture2D();
    GLuint loadShader(GLenum type, const char *shaderSrc);
    GLuint loadProgram (const char *vertShaderSrc, const char *fragShaderSrc);
    void initEGL( int width, int height);
    int init();
    void releaseEGL();
public:
    GlPlayer(int width, int height);
    ~GlPlayer();
    void draw(char * image);

public:
    int m_width;
    int m_height;
    CUBE_STATE_T *p_state;
    UserData *user_data;
};

#endif // GLPLAYER_H
