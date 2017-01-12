#include "glplayer.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QTime>

GlPlayer::GlPlayer(int width, int height) :
    user_data(new UserData()),
    p_state(new CUBE_STATE_T()),
    m_width(width),
    m_height(height)
{
    p_state->user_data = user_data;
    initEGL(m_width, m_height);
    if(!init()) exit(1);
}

GlPlayer::~GlPlayer(){
    fprintf(stderr, "~GlPlayer()\n");
    releaseEGL();
    if(user_data != NULL) delete user_data;
    if(p_state != NULL) delete p_state;
}

// Create a simple width x height texture image with four different colors
//
GLuint GlPlayer::createSimpleTexture2D(){
   // Texture object handle
   GLuint textureId;
   // Use tightly packed data
   glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
   // Generate a texture object
   glGenTextures ( 1, &textureId );
   // Bind the texture object
   glBindTexture ( GL_TEXTURE_2D, textureId );
   // Set the filtering mode
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   return textureId;
}


// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint GlPlayer::loadShader(GLenum type, const char *shaderSrc){
    GLuint shader;
    GLint compiled;
    // Create the shader object
    shader = glCreateShader(type);
    if(shader == 0)
    return 0;
    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);
    // Compile the shader
    glCompileShader(shader);
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled){
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1){
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint GlPlayer::loadProgram ( const char *vertShaderSrc, const char *fragShaderSrc ){
   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = loadShader ( GL_VERTEX_SHADER, vertShaderSrc );
   if ( vertexShader == 0 ) return 0;

   fragmentShader = loadShader ( GL_FRAGMENT_SHADER, fragShaderSrc );
   if ( fragmentShader == 0 ){
      glDeleteShader( vertexShader );
      return 0;
   }

   // Create the program object
   programObject = glCreateProgram ();
   if ( programObject == 0 ) return 0;

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

   // Link the program
   glLinkProgram ( programObject );

   // Check the link status
   glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );
   if ( !linked ){
      GLint infoLen = 0;
      glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );
      if ( infoLen > 1 ){
         char* infoLog = (char*)malloc (sizeof(char) * infoLen );
         glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
         fprintf (stderr, "Error linking program:\n%s\n", infoLog );
         free ( infoLog );
      }

      glDeleteProgram ( programObject );
      return 0;
   }

   // Free up no longer needed shader resources
   glDeleteShader ( vertexShader );
   glDeleteShader ( fragmentShader );

   return programObject;
}

void GlPlayer::initEGL(int width, int height){
    int32_t success = 0;
    EGLBoolean result;
    EGLint num_config;

    bcm_host_init();

    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    static const EGLint attribute_list[] =
    {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    static const EGLint context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLConfig config;

    // get an EGL display connection
    p_state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    // initialize the EGL display connection
    result = eglInitialize(p_state->display, NULL, NULL);

    // get an appropriate EGL frame buffer configuration
    result = eglChooseConfig(p_state->display, attribute_list, &config, 1, &num_config);
    assert(EGL_FALSE != result);

    // get an appropriate EGL frame buffer configuration
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);

    // create an EGL rendering context
    p_state->context = eglCreateContext(p_state->display, config, EGL_NO_CONTEXT, context_attributes);
    assert(p_state->context!=EGL_NO_CONTEXT);

    // create an EGL window surface
    success = graphics_get_display_size(0 /* LCD */, &p_state->width, &p_state->height);
    assert( success >= 0 );

    p_state->width = width;
    p_state->height = height;

    QDesktopWidget *dwsktopwidget = QApplication::desktop();
    QRect deskrect = dwsktopwidget->availableGeometry();

    int scale_w = 0, scale_h = 0;
    if(p_state->height > p_state->width){
        scale_h = deskrect.height()-24;
        scale_w = (quint32)(p_state->width*scale_h/p_state->height);
    }else{
        scale_w = deskrect.width()-24;
        scale_h = (quint32)(p_state->height*scale_w/p_state->width);
    }

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = deskrect.width();
    dst_rect.height = deskrect.height();

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = p_state->width;
    src_rect.height = p_state->height;

    p_state->dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    p_state->dispman_update = vc_dispmanx_update_start( 0 );

    p_state->dispman_element =
    vc_dispmanx_element_add(p_state->dispman_update, p_state->dispman_display,
                0/*layer*/, &dst_rect, 0/*src*/,
                &src_rect, DISPMANX_PROTECTION_NONE,
                0 /*alpha*/, 0/*clamp*/, DISPMANX_NO_ROTATE/*transform*/);

    p_state->nativewindow.element = p_state->dispman_element;
    p_state->nativewindow.width = deskrect.width();
    p_state->nativewindow.height = deskrect.height();
    vc_dispmanx_update_submit_sync( p_state->dispman_update );

    p_state->surface = eglCreateWindowSurface( p_state->display, config, &(p_state->nativewindow), NULL );
    assert(p_state->surface != EGL_NO_SURFACE);

    // connect the context to the surface
    result = eglMakeCurrent(p_state->display, p_state->surface, p_state->surface, p_state->context);
    assert(EGL_FALSE != result);
}

///
// Initialize the shader and program object
//
int GlPlayer::init(){
   UserData *userData = p_state->user_data;
   memset(userData, 0, sizeof(UserData));
   char vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

   char fShaderStr[] =
      "precision mediump float;                            \n"
      "varying vec2 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
      "}                                                   \n";

   // Load the shaders and get a linked program object
   userData->programObject = loadProgram ( vShaderStr, fShaderStr );
   // Get the attribute locations
   userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );
   userData->texCoordLoc = glGetAttribLocation ( userData->programObject, "a_texCoord" );
   // Get the sampler location
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );
   // Load the texture
   userData->textureId = createSimpleTexture2D ();

   // Set the viewport
   QDesktopWidget *dwsktopwidget = QApplication::desktop();
   QRect deskrect = dwsktopwidget->availableGeometry();
   glViewport (0, 0, deskrect.width(), deskrect.height());
   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->programObject );
   // Bind the texture
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );
   // Set the sampler texture unit to 0
   glUniform1i ( userData->samplerLoc, 0 );
   return GL_TRUE;
}

// Draw triangles using the shader pair created in Init()
//
void GlPlayer::draw(char * image){
    if(image == NULL){
        printf("image error\n");
        return;
    }
    if(p_state == NULL){
        printf("p_state error\n");
        return;
    }
    UserData *userData = p_state->user_data;
    if(userData == NULL){
        printf("userData error\n");
        return;
    }
    printf("draw image, size %dx%d\n",m_width,m_height);
#if 0
    QImage img((uchar *)image,m_width,m_height,QImage::Format_ARGB32);
    QDateTime time = QDateTime::currentDateTime();
    QString filename = "/home/pi/image/"+time.toString("yyyy-MM-dd-hh-mm-ss")+".png";
    img.save(filename);
#endif


    glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,  // Position 0
                            0.0f,  0.0f,        // TexCoord 0
                           -1.0f, -1.0f, 0.0f,  // Position 1
                            0.0f,  1.0f,        // TexCoord 1
                            1.0f, -1.0f, 0.0f,  // Position 2
                            1.0f,  1.0f,        // TexCoord 2
                            1.0f,  1.0f, 0.0f,  // Position 3
                            1.0f,  0.0f         // TexCoord 3
                          };
     GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    //GLushort indices[] = {1, 0, 3, 0, 2, 0, 1 };

    // Load the vertex position
    glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vVertices );
    // Load the texture coordinate
    glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );

    glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
    eglSwapBuffers(p_state->display, p_state->surface);
}

void GlPlayer::releaseEGL(){
    eglMakeCurrent(p_state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(p_state->display, p_state->surface);
    eglDestroyContext(p_state->display, p_state->context);
    eglTerminate(p_state->display);
}
