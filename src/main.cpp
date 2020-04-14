#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include "glExtension.h"                          
#include "Timer.h"


void displayCB();
void reshapeCB(int w, int h);
void timerCB(int millisec);
void idleCB();
void keyboardCB(unsigned char key, int x, int y);
void mouseCB(int button, int stat, int x, int y);
void mouseMotionCB(int x, int y);

void exitCB();

void initGL();
int  initGLUT(int argc, char **argv);
bool initSharedMem();
void clearSharedMem();
void initLights();
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ);
void drawString(const char *str, int x, int y, float color[4], void *font);
void drawString3D(const char *str, float pos[3], float color[4], void *font);
void showInfo();
void showTransferRate();
void printTransferRate();
void draw();
void add(unsigned char* src, int width, int height, int shift, unsigned char* dst);
void toOrtho();
void toPerspective();


const int SCREEN_WIDTH = 512;
const int SCREEN_HEIGHT = 512;
const float CAMERA_DISTANCE = 5.0f;
const int CHANNEL_COUNT = 4;
const int DATA_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT * CHANNEL_COUNT;
const GLenum PIXEL_FORMAT = GL_BGRA;
const int PBO_COUNT = 2;

void *font = GLUT_BITMAP_8_BY_13;
GLuint pboIds[PBO_COUNT];         
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance;
bool pboSupported;
bool pboUsed;
int drawMode = 0;
Timer timer, t1;
float readTime, processTime;
GLubyte* colorBuffer = 0;


void draw()
{
    glBegin(GL_QUADS);
        glNormal3f(0,0,1);
        glColor3f(1,1,1);
        glVertex3f(1,1,1);
        glColor3f(1,1,0);
        glVertex3f(-1,1,1);
        glColor3f(1,0,0);
        glVertex3f(-1,-1,1);
        glColor3f(1,0,1);
        glVertex3f(1,-1,1);

        glNormal3f(1,0,0);
        glColor3f(1,1,1);
        glVertex3f(1,1,1);
        glColor3f(1,0,1);
        glVertex3f(1,-1,1);
        glColor3f(0,0,1);
        glVertex3f(1,-1,-1);
        glColor3f(0,1,1);
        glVertex3f(1,1,-1);

        glNormal3f(0,1,0);
        glColor3f(1,1,1);
        glVertex3f(1,1,1);
        glColor3f(0,1,1);
        glVertex3f(1,1,-1);
        glColor3f(0,1,0);
        glVertex3f(-1,1,-1);
        glColor3f(1,1,0);
        glVertex3f(-1,1,1);

        glNormal3f(-1,0,0);
        glColor3f(1,1,0);
        glVertex3f(-1,1,1);
        glColor3f(0,1,0);
        glVertex3f(-1,1,-1);
        glColor3f(0,0,0);
        glVertex3f(-1,-1,-1);
        glColor3f(1,0,0);
        glVertex3f(-1,-1,1);

        glNormal3f(0,-1,0);
        glColor3f(0,0,0);
        glVertex3f(-1,-1,-1);
        glColor3f(0,0,1);
        glVertex3f(1,-1,-1);
        glColor3f(1,0,1);
        glVertex3f(1,-1,1);
        glColor3f(1,0,0);
        glVertex3f(-1,-1,1);

        glNormal3f(0,0,-1);
        glColor3f(0,0,1);
        glVertex3f(1,-1,-1);
        glColor3f(0,0,0);
        glVertex3f(-1,-1,-1);
        glColor3f(0,1,0);
        glVertex3f(-1,1,-1);
        glColor3f(0,1,1);
        glVertex3f(1,1,-1);
    glEnd();
}



int main(int argc, char **argv)
{
    initSharedMem();

    atexit(exitCB);

    initGLUT(argc, argv);
    initGL();

    glExtension& ext = glExtension::getInstance();
    pboSupported = pboUsed = ext.isSupported("GL_ARB_pixel_buffer_object");
    if(pboSupported)
    {
        std::cout << "Video card supports GL_ARB_pixel_buffer_object." << std::endl;
    }
    else
    {
        std::cout << "[ERROR] Video card does not supports GL_ARB_pixel_buffer_object." << std::endl;
    }

#ifdef _WIN32
    if(ext.isSupported("WGL_EXT_swap_control"))
    {
        wglSwapIntervalEXT(0);
        std::cout << "Video card supports WGL_EXT_swap_control." << std::endl;
    }
#endif

    if(pboSupported)
    {
        glGenBuffers(PBO_COUNT, pboIds);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[0]);
        glBufferData(GL_PIXEL_PACK_BUFFER, DATA_SIZE, 0, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[1]);
        glBufferData(GL_PIXEL_PACK_BUFFER, DATA_SIZE, 0, GL_STREAM_READ);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    timer.start();

    glutMainLoop();

    return 0;
}


int initGLUT(int argc, char **argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_ALPHA); 

    glutInitWindowSize(SCREEN_WIDTH*2, SCREEN_HEIGHT); 

    glutInitWindowPosition(100, 100);           

    int handle = glutCreateWindow(argv[0]);    

    glutDisplayFunc(displayCB);
    //glutTimerFunc(33, timerCB, 33);             
    glutIdleFunc(idleCB);                      
    glutReshapeFunc(reshapeCB);
    glutKeyboardFunc(keyboardCB);
    glutMouseFunc(mouseCB);
    glutMotionFunc(mouseMotionCB);

    return handle;
}



void initGL()
{
    glShadeModel(GL_SMOOTH);                   
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);     

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glClearColor(0, 0, 0, 0);                   
    glClearStencil(0);                        
    glClearDepth(1.0f);                     
    glDepthFunc(GL_LEQUAL);

    initLights();
    //setCamera(0, 0, 8, 0, 0, 0);
}



void drawString(const char *str, int x, int y, float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); 
    glDisable(GL_LIGHTING);    
    glDisable(GL_TEXTURE_2D);

    glColor4fv(color);       
    glRasterPos2i(x, y);       

    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopAttrib();
}



void drawString3D(const char *str, float pos[3], float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); 
    glDisable(GL_LIGHTING);    
    glDisable(GL_TEXTURE_2D);

    glColor4fv(color);         
    glRasterPos3fv(pos);       

    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopAttrib();
}



bool initSharedMem()
{
    cameraAngleX = cameraAngleY = 0.0f;
    cameraDistance = CAMERA_DISTANCE;

    mouseX = mouseY = 0;
    mouseLeftDown = mouseRightDown = false;

    drawMode = 0; 
    colorBuffer = new GLubyte[DATA_SIZE];
    memset(colorBuffer, 255, DATA_SIZE);

    return true;
}



void clearSharedMem()
{
    // deallocate frame buffer
    delete [] colorBuffer;
    colorBuffer = 0;

    // clean up PBOs
    if(pboSupported)
    {
        glDeleteBuffers(PBO_COUNT, pboIds);
    }
}


void initLights()
{

    GLfloat lightKa[] = {.2f, .2f, .2f, 1.0f};  // ambient light
    GLfloat lightKd[] = {.7f, .7f, .7f, 1.0f};  // diffuse light
    GLfloat lightKs[] = {1, 1, 1, 1};           // specular light
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);


    float lightPos[4] = {0, 0, 20, 1}; /
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHT0);                       
}


void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(posX, posY, posZ, targetX, targetY, targetZ, 0, 1, 0);
}



void showInfo()
{
    glPushMatrix();                     
    glLoadIdentity();                   
    glMatrixMode(GL_PROJECTION);     
    glPushMatrix();                  
    glLoadIdentity();               
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT); 

    const int FONT_HEIGHT = 14;
    float color[4] = {1, 1, 1, 1};

    std::stringstream ss;
    ss << "PBO: ";
    if(pboUsed)
        ss << "on" << std::ends;
    else
        ss << "off" << std::ends;

    drawString(ss.str().c_str(), 1, SCREEN_HEIGHT-FONT_HEIGHT, color, font);
    ss.str(""); // clear buffer

    ss << "Read Time: " << readTime << " ms" << std::ends;
    drawString(ss.str().c_str(), 1, SCREEN_HEIGHT-(2*FONT_HEIGHT), color, font);
    ss.str("");

    ss << std::fixed << std::setprecision(3);
    ss << "Process Time: " << processTime << " ms" << std::ends;
    drawString(ss.str().c_str(), 1, SCREEN_HEIGHT-(3*FONT_HEIGHT), color, font);
    ss.str("");

    ss << "Press SPACE to toggle PBO." << std::ends;
    drawString(ss.str().c_str(), 1, 1, color, font);

    ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);

    glPopMatrix();                   
    glMatrixMode(GL_MODELVIEW);      
    glPopMatrix();                   
}


void showTransferRate()
{
    static Timer timer;
    static int count = 0;
    static std::stringstream ss;

    ++count;
    double elapsedTime = timer.getElapsedTime();
    if(elapsedTime > 1.0)
    {
        ss.str("");
        ss << std::fixed << std::setprecision(1);
        ss << "Transfer Rate: " << (count / elapsedTime) * (SCREEN_WIDTH * SCREEN_HEIGHT) / (1024 * 1024) << " Mp" << std::ends; // update fps string
        ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
        count = 0;                     
        timer.start();                 
    }

    glPushMatrix();                    
    glLoadIdentity();                
    glMatrixMode(GL_PROJECTION);      
    glPushMatrix();                     
    glLoadIdentity();                  
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT); 

    float color[4] = {1, 1, 0, 1};
    drawString(ss.str().c_str(), 200, 286, color, font);

    glPopMatrix();                     
    glMatrixMode(GL_MODELVIEW);  
    glPopMatrix();              
}



void printTransferRate()
{
    const double INV_MEGA = 1.0 / (1024 * 1024);
    static Timer timer;
    static int count = 0;
    static std::stringstream ss;
    double elapsedTime;


    elapsedTime = timer.getElapsedTime();
    if(elapsedTime < 1.0)
    {
        ++count;
    }
    else
    {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "Transfer Rate: " << (count / elapsedTime) * (SCREEN_WIDTH * SCREEN_HEIGHT) * INV_MEGA << " Mpixels/s. (" << count / elapsedTime << " FPS)\n";
        std::cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
        count = 0;                    
        timer.start();              
    }
}


void add(unsigned char* src, int width, int height, int shift, unsigned char* dst)
{
    if(!src || !dst)
        return;

    int value;
    for(int i = 0; i < height; ++i)
    {
        for(int j = 0; j < width; ++j)
        {
            value = *src + shift;
            if(value > 255) *dst = (unsigned char)255;
            else            *dst = (unsigned char)value;
            ++src;
            ++dst;

            value = *src + shift;
            if(value > 255) *dst = (unsigned char)255;
            else            *dst = (unsigned char)value;
            ++src;
            ++dst;

            value = *src + shift;
            if(value > 255) *dst = (unsigned char)255;
            else            *dst = (unsigned char)value;
            ++src;
            ++dst;

            ++src;    
            ++dst;
        }
    }
}


void toOrtho()
{
   
    glViewport((GLsizei)SCREEN_WIDTH, 0, (GLsizei)SCREEN_WIDTH, (GLsizei)SCREEN_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}



void toPerspective()
{
    glViewport(0, 0, (GLsizei)SCREEN_WIDTH, (GLsizei)SCREEN_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)(SCREEN_WIDTH)/SCREEN_HEIGHT, 0.1f, 1000.0f); 

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}







void displayCB()
{
    static int shift = 0;
    static int index = 0;
    int nextIndex = 0;      

    ++shift;
    shift %= 200;

    index = (index + 1) % 2;
    nextIndex = (index + 1) % 2;

    glReadBuffer(GL_FRONT);

    if(pboUsed) // with PBO
    {
        t1.start();

        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[index]);
        glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);


        t1.stop();
        readTime = t1.getElapsedTimeInMilliSec();


        t1.start();

        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[nextIndex]);
        GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if(src)
        {

            add(src, SCREEN_WIDTH, SCREEN_HEIGHT, shift, colorBuffer);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);      

        }


        t1.stop();
        processTime = t1.getElapsedTimeInMilliSec();


        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    else      

    {

        t1.start();

        glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, colorBuffer);


        t1.stop();
        readTime = t1.getElapsedTimeInMilliSec();

        t1.start();

        add(colorBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, shift, colorBuffer);

        t1.stop();
        processTime = t1.getElapsedTimeInMilliSec();
    }

    glDrawBuffer(GL_BACK);
    toPerspective(); 

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glTranslatef(0, 0, -cameraDistance);
    glRotatef(cameraAngleX, 1, 0, 0);  
    glRotatef(cameraAngleY, 0, 1, 0);   

    glPushMatrix();
    draw();
    glPopMatrix();

    toOrtho();     
    glRasterPos2i(0, 0);
    glDrawPixels(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_FORMAT, GL_UNSIGNED_BYTE, colorBuffer);

    showInfo();
    printTransferRate();

    glutSwapBuffers();
}


void reshapeCB(int w, int h)
{
    toPerspective();
}


void timerCB(int millisec)
{
    glutTimerFunc(millisec, timerCB, millisec);
    glutPostRedisplay();
}


void idleCB()
{
    glutPostRedisplay();
}


void keyboardCB(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 27: // ESCAPE
        exit(0);
        break;

    case ' ':
        if(pboSupported)
            pboUsed = !pboUsed;
        std::cout << "PBO mode: " << (pboUsed ? "on" : "off") << std::endl;
         break;

    case 'd': 
    case 'D':
        ++drawMode;
        drawMode %= 3;
        if(drawMode == 0)        
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
        }
        else if(drawMode == 1)  
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
        }
        else                    
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
        }
        break;

    default:
        ;
    }
}


void mouseCB(int button, int state, int x, int y)
{
    mouseX = x;
    mouseY = y;

    if(button == GLUT_LEFT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseLeftDown = true;
        }
        else if(state == GLUT_UP)
            mouseLeftDown = false;
    }

    else if(button == GLUT_RIGHT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseRightDown = true;
        }
        else if(state == GLUT_UP)
            mouseRightDown = false;
    }
}


void mouseMotionCB(int x, int y)
{
    if(mouseLeftDown)
    {
        cameraAngleY += (x - mouseX);
        cameraAngleX += (y - mouseY);
        mouseX = x;
        mouseY = y;
    }
    if(mouseRightDown)
    {
        cameraDistance -= (y - mouseY) * 0.2f;
        if(cameraDistance < 2.0f)
            cameraDistance = 2.0f;

        mouseY = y;
    }
}



void exitCB()
{
    clearSharedMem();
}
