
#include "platform.hpp"

// third-party libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// standard C++ libraries
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <list>

// tdogl classes
#include "tdogl/Program.h"
#include "tdogl/Texture.h"
#include "tdogl/Camera.h"

/*
 Represents a textured geometry asset

 Contains everything necessary to draw arbitrary geometry with a single texture:

  - shaders
  - a texture
  - a VBO
  - a VAO
  - the parameters to glDrawArrays (drawType, drawStart, drawCount)
 */
struct ModelAsset {
    tdogl::Program* shaders;
    tdogl::Texture* texture;
    GLuint vbo;
    GLuint vao;
    GLenum drawType;
    GLint drawStart;
    GLint drawCount;

    ModelAsset() :
        shaders(NULL),
        texture(NULL),
        vbo(0),
        vao(0),
        drawType(GL_TRIANGLES),
        drawStart(0),
        drawCount(0)
    {}
};

/*
 Represents an instance of an `ModelAsset`

 Contains a pointer to the asset, and a model transformation matrix to be used when drawing.
 */
struct ModelInstance {
    ModelAsset* asset;
    glm::mat4 transform;

    ModelInstance() :
        asset(NULL),
        transform()
    {}
};

/*
 Represents a point light
 */
struct Light {
    glm::vec3 position;
    glm::vec3 intensities; //a.k.a. the color of the light
};

/*
 Contains vertex, UV and normal information of the grid
 */
struct GridUnit {
    glm::vec3 vertex1;
    glm::vec3 vertex2;
    glm::vec3 vertex3;
    glm::vec3 vertex4;
    glm::vec2 UV1;
    glm::vec2 UV2;
    glm::vec2 UV3;
    glm::vec2  UV4;
    glm::vec3 normal1;
    glm::vec3 normal2;
    glm::vec3 normal3;
    glm::vec3 normal4;
    
    GridUnit() :
    vertex1(0.0f,0.0f,0.0f),
    vertex2(0.0f,0.0f,0.0f),
    vertex3(0.0f,0.0f,0.0f),
    UV1(0.0f,0.0f),
    UV2(0.0f,0.0f),
    UV3(0.0f,0.0f),
    normal1(0.0f,0.0f,0.0f),
    normal2(0.0f,0.0f,0.0f),
    normal3(0.0f,0.0f,0.0f)
    {}
};

// constants
const glm::vec2 SCREEN_SIZE(800, 600);
const glm::int32 GRID_HEIGHT(100);
const glm::int32 GRID_WIDTH(100);
const glm::int32 WATER_HEIGHT (5);
const glm::float64 TARGET_FRAME_TIME (1/60);

// globals
GLFWwindow* gWindow = NULL;
GLfloat vertexData[480000];
double gScrollY = 0.0;
double deltaTime = TARGET_FRAME_TIME;
bool isFluid = false;
tdogl::Camera gCamera;
ModelAsset gFloor;
ModelAsset gGrid;

//global assets for Navier Stokes fluid equation
ModelAsset gAdvect;
ModelAsset gDiverge;
ModelAsset gJacobi;
ModelAsset gPressure;

std::list<ModelInstance> gInstances;
GLfloat gDegreesRotated = 0.0f;
Light gLight;
std::list<GLfloat> gridData;

//helper functions
static GridUnit computeUnit(GLfloat x, GLfloat y, GLfloat z);
static std::list<GLfloat> computeGridData();

// returns a new tdogl::Program created from the given vertex and fragment shader filenames
static tdogl::Program* LoadShaders(const char* vertFilename, const char* fragFilename) {
    std::vector<tdogl::Shader> shaders;
    shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(vertFilename), GL_VERTEX_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(fragFilename), GL_FRAGMENT_SHADER));
    return new tdogl::Program(shaders);
}


// returns a new tdogl::Texture created from the given filename
static tdogl::Texture* LoadTexture(const char* filename) {
    tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(filename));
    bmp.flipVertically();
    return new tdogl::Texture(bmp);
}

//initializes the gAdvect global
static void LoadAdvectAsset(){
    // set all the elements of gAdvect
    isFluid = true;
    gAdvect.shaders = LoadShaders("grid-vertex-shader.txt", "advect-fragment-shader.txt");
    gAdvect.drawType = GL_TRIANGLES;
    gAdvect.drawStart = 0;
    gAdvect.drawCount = 1*100*100*3;//6*2*3;// //100 * 100 unit grids and each grid unit call has 32 (4*(x,y,z,u,v,nx,ny,nz)) floating point values
    gAdvect.texture = LoadTexture("blue.jpg");
    glGenBuffers(1, &gAdvect.vbo);
    glGenVertexArrays(1, &gAdvect.vao);
    
    // bind the VAO
    glBindVertexArray(gAdvect.vao);
    
    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, gAdvect.vbo);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    
    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(gAdvect.shaders->attrib("vert"));
    glVertexAttribPointer(gAdvect.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), NULL);
    
    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(gAdvect.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gAdvect.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    
    // connect the normal to the "vertNormal" attribute of the vertex shader
    glEnableVertexAttribArray(gAdvect.shaders->attrib("vertNormal"));
    glVertexAttribPointer(gAdvect.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));
    
    // unbind the VAO
    glBindVertexArray(0);

    
}

// initialises the gFloor global
static void LoadFloorAsset() {
    // set all the elements of gFloor
    gFloor.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    gFloor.drawType = GL_TRIANGLES;
    gFloor.drawStart = 0;
    gFloor.drawCount = 1*2*3;
    gFloor.texture = LoadTexture("wooden-crate.jpg");
    glGenBuffers(1, &gFloor.vbo);
    glGenVertexArrays(1, &gFloor.vao);

    // bind the VAO
    glBindVertexArray(gFloor.vao);

    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, gFloor.vbo);

    // Make a cube out of triangles (two triangles per side)
    GLfloat vertexData[] = {
        //  X     Y     Z       U     V          Normal
        // front
          -10.0f,-10.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
           90.0f,-10.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
          -10.0f, 90.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
        
           90.0f,-10.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
           90.0f, 90.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
          -10.0f, 90.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(gFloor.shaders->attrib("vert"));
    glVertexAttribPointer(gFloor.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), NULL);

    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(gFloor.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gFloor.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    // connect the normal to the "vertNormal" attribute of the vertex shader
    glEnableVertexAttribArray(gFloor.shaders->attrib("vertNormal"));
    glVertexAttribPointer(gFloor.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

    // unbind the VAO
    glBindVertexArray(0);
}

//create gridData
static std::list<GLfloat> computeGridData(){
    std::list<GLfloat> gridData;
    int index = 0;
    for(int y = 0; y < GRID_HEIGHT; y++){ //y
        for(int x = 0; x < GRID_WIDTH; x++){ //x
            GridUnit temp;
           // temp = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
            temp = computeUnit(x,y,WATER_HEIGHT);
            
            //save all to unit array
            //Triangle 1
            //Vertex1
            gridData.push_back(temp.vertex1[0]);
            gridData.push_back(temp.vertex1[1]);
            gridData.push_back(temp.vertex1[2]);
            gridData.push_back(temp.UV1[0]);
            gridData.push_back(temp.UV1[1]);
            gridData.push_back(temp.normal1[0]);
            gridData.push_back(temp.normal1[1]);
            gridData.push_back(temp.normal1[2]);
            
            //Vertex2
            gridData.push_back(temp.vertex2[0]);
            gridData.push_back(temp.vertex2[1]);
            gridData.push_back(temp.vertex2[2]);
            gridData.push_back(temp.UV2[0]);
            gridData.push_back(temp.UV2[1]);
            gridData.push_back(temp.normal2[0]);
            gridData.push_back(temp.normal2[1]);
            gridData.push_back(temp.normal2[2]);
            
            //Vertex4
            gridData.push_back(temp.vertex4[0]);
            gridData.push_back(temp.vertex4[1]);
            gridData.push_back(temp.vertex4[2]);
            gridData.push_back(temp.UV4[0]);
            gridData.push_back(temp.UV4[1]);
            gridData.push_back(temp.normal4[0]);
            gridData.push_back(temp.normal4[1]);
            gridData.push_back(temp.normal4[2]);
            
            //Triangle 2
            //Vertex2
            gridData.push_back(temp.vertex2[0]);
            gridData.push_back(temp.vertex2[1]);
            gridData.push_back(temp.vertex2[2]);
            gridData.push_back(temp.UV2[0]);
            gridData.push_back(temp.UV2[1]);
            gridData.push_back(temp.normal2[0]);
            gridData.push_back(temp.normal2[1]);
            gridData.push_back(temp.normal2[2]);
            //Vertex3
            gridData.push_back(temp.vertex3[0]);
            gridData.push_back(temp.vertex3[1]);
            gridData.push_back(temp.vertex3[2]);
            gridData.push_back(temp.UV3[0]);
            gridData.push_back(temp.UV3[1]);
            gridData.push_back(temp.normal3[0]);
            gridData.push_back(temp.normal3[1]);
            gridData.push_back(temp.normal3[2]);
            
            //Vertex4
            gridData.push_back(temp.vertex4[0]);
            gridData.push_back(temp.vertex4[1]);
            gridData.push_back(temp.vertex4[2]);
            gridData.push_back(temp.UV4[0]);
            gridData.push_back(temp.UV4[1]);
            gridData.push_back(temp.normal4[0]);
            gridData.push_back(temp.normal4[1]);
            gridData.push_back(temp.normal4[2]);
        }
    }
    return gridData;
}

//get value of each unit of the grid
static GridUnit computeUnit(GLfloat x,GLfloat y, GLfloat z){ //individual grid unit or loop
    GridUnit unit;
    unit.vertex1[0] = x+0.0f;           unit.UV1[0] = unit.vertex1[0]/100.0f;           unit.normal1[0] = 0.0f;
    unit.vertex1[1] = y+0.0f;           unit.UV1[1] = unit.vertex1[1]/100.f;            unit.normal1[1] = 1.0f;
    unit.vertex1[2] = z+0.0f;                                                           unit.normal1[2] = 0.0f;
    
    unit.vertex2[0] = x+1.0f;           unit.UV2[0] = unit.vertex2[0]/100.0f;           unit.normal2[0] = 0.0f;
    unit.vertex2[1] = y+0.0f;           unit.UV2[1] = unit.vertex2[1]/100.0f;           unit.normal2[1] = 1.0f;
    unit.vertex2[2] = z+0.0f;                                                           unit.normal2[2] = 0.0f;
    
    unit.vertex3[0] = x+1.0f;           unit.UV3[0] = unit.vertex3[0]/100.0f;           unit.normal3[0] = 0.0f;
    unit.vertex3[1] = y+1.0f;           unit.UV3[1] = unit.vertex3[0]/100.0f;           unit.normal3[1] = 1.0f;
    unit.vertex3[2] = z+0.0f;                                                           unit.normal3[2] = 0.0f;
    
    unit.vertex4[0] = x+0.0f;           unit.UV4[0] = unit.vertex4[0]/100.0f;           unit.normal4[0] = 0.0f;
    unit.vertex4[1] = y+1.0f;           unit.UV4[1] = unit.vertex4[0]/100.0f;           unit.normal4[1] = 1.0f;
    unit.vertex4[2] = z+0.0f;                                                           unit.normal4[2] = 0.0f;
    
    return unit;
}

// initialises the gGrid global
static void LoadGridAsset() {
    // set all the elements of gWoodenCrate
    gGrid.shaders = LoadShaders("grid-vertex-shader.txt", "grid-fragment-shader.txt");
    gGrid.drawType = GL_TRIANGLES;
    gGrid.drawStart = 0;
    gGrid.drawCount = 1*100*100*2*3;//1 face (2D) has 100*100 grid, each grid has 2 triangles and each triangle has 3 coordinate values
    gGrid.texture = LoadTexture("blue.jpg");
    glGenBuffers(1, &gGrid.vbo);
    glGenVertexArrays(1, &gGrid.vao);
    
    // bind the VAO
    glBindVertexArray(gGrid.vao);
    
    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, gGrid.vbo);
    
    // Make a cube out of triangles (two triangles per side)
    std::list<GLfloat> gridData = computeGridData();
    int index = 1;
    for (std::list<GLfloat>::iterator it = gridData.begin(); it != gridData.end(); ++it){
        vertexData[index-1] = *it;
        std::cout<<vertexData[index-1];
        if(index %8 ==0 && index !=0){
            std::cout<<"\n";
        }else{
            std::cout<<",";
        }
        index++;
    }
    
    //for (int i = 0; i <=100*100*2*3+1; i++){
    //std::cout<<vertexData[i+i*8]<<","<<vertexData[i+1+i*8]<<","<<vertexData[i+2+i*8]<<","<<vertexData[i+3+i*8]<<","<<vertexData[i+4+i*8]<<","<<vertexData[i+5+i*8]<<","<<vertexData[i+6+i*8]<<","<<vertexData[i+7+i*8]<<"\n";
    //}
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    
    // connect the xyz to the "vert" attribute of the vertex shader
    //(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
    glEnableVertexAttribArray(gGrid.shaders->attrib("vert"));
    glVertexAttribPointer(gGrid.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), NULL);
    
    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(gGrid.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gGrid.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    
    // connect the normal to the "vertNormal" attribute of the vertex shader
    glEnableVertexAttribArray(gGrid.shaders->attrib("vertNormal"));
    glVertexAttribPointer(gGrid.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));
    
    // unbind the VAO
    glBindVertexArray(0);
}


// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
    return glm::translate(glm::mat4(), glm::vec3(x,y,z));
}


// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
    return glm::scale(glm::mat4(), glm::vec3(x,y,z));
}


//create all the `instance` structs for the 3D scene, and add them to `gInstances`
static void CreateInstances() {
    ModelInstance dot;
    dot.asset = &gFloor;
    dot.transform = translate(-10,-7,0);
    gInstances.push_back(dot);

     ModelInstance grid;
     grid.asset = &gGrid;
    grid.transform = translate(-10,-10,0);
     gInstances.push_back(grid);

     //std::cout<<"advect start \n";
    // ModelInstance advect;
    // advect.asset = &gAdvect;
    //advect.transform = translate(-10,-10,0);
    // gInstances.push_back(advect);
//
//    ModelInstance hRight;
//    hRight.asset = &gWoodenCrate;
//    hRight.transform = translate(-4,0,0) * scale(1,6,1);
//    gInstances.push_back(hRight);
//
//    ModelInstance hMid;
//    hMid.asset = &gWoodenCrate;
//    hMid.transform = translate(-6,0,0) * scale(2,1,0.8f);
//    gInstances.push_back(hMid);
}


//renders a single `ModelInstance`
static void RenderInstance(const ModelInstance& inst) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;

    //bind the shaders
    shaders->use();

    //set the shader uniforms
    if(asset != &gAdvect){
        shaders->setUniform("camera", gCamera.matrix());
        shaders->setUniform("model", inst.transform);
        shaders->setUniform("tex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
        shaders->setUniform("light.position", gLight.position);
        shaders->setUniform("light.intensities", gLight.intensities);
    }
    //Fluid Shaders
    if(asset == &gAdvect){
        shaders->setUniform("OldVelocity", 0);
        shaders->setUniform("InverseGridSize", 1.0 / GRID_WIDTH, 1.0 / GRID_HEIGHT);
        shaders->setUniform("DeltaT", deltaTime);
    }
    
    //bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asset->texture->object());

    //bind VAO and draw
    glBindVertexArray(asset->vao);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);

    //unbind everything
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    shaders->stopUsing();
}


// draws a single frame
static void Render() {
    // clear everything
    glClearColor(0, 0, 0, 1); // black
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render all the instances
    std::list<ModelInstance>::const_iterator it;
    for(it = gInstances.begin(); it != gInstances.end(); ++it){
        RenderInstance(*it);
    }

    // swap the display buffers (displays what was just drawn)
    glfwSwapBuffers(gWindow);
}


// update the scene based on the time elapsed since last update
static void Update(float secondsElapsed) {
    //rotate the first instance in `gInstances`
    const GLfloat degreesPerSecond = 0.0f;
    gDegreesRotated = degreesPerSecond;
    while(gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;
    gInstances.front().transform = glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0,1,0));

    //increase or decrease field of view based on mouse wheel
    const float zoomSensitivity = -0.2f;
    float fieldOfView = gCamera.fieldOfView() + zoomSensitivity * (float)gScrollY;
    //std::cout<<"\n Field of view: "<<fieldOfView<<"\n";
    if(fieldOfView < 5.0f) fieldOfView = 5.0f;
    if(fieldOfView > 165.0f) fieldOfView = 165.0f;
    gCamera.setFieldOfView(fieldOfView);
    gScrollY = 0;
}

// records how far the y axis has been scrolled
void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
    gScrollY += deltaY;
}

void OnError(int errorCode, const char* msg) {
    throw std::runtime_error(msg);
}

// the program starts here
void AppMain() {
    // initialise GLFW
    glfwSetErrorCallback(OnError);
    if(!glfwInit())
        throw std::runtime_error("glfwInit failed");

    // open a window with GLFW
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    gWindow = glfwCreateWindow((int)SCREEN_SIZE.x, (int)SCREEN_SIZE.y, "OpenGL Tutorial", NULL, NULL);
    if(!gWindow)
        throw std::runtime_error("glfwCreateWindow failed. Can your hardware handle OpenGL 3.2?");

    // GLFW settings
    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(gWindow, 0, 0);
    glfwSetScrollCallback(gWindow, OnScroll);
    glfwMakeContextCurrent(gWindow);

    // initialise GLEW
    glewExperimental = GL_TRUE; //stops glew crashing on OSX :-/
    if(glewInit() != GLEW_OK)
        throw std::runtime_error("glewInit failed");

    // GLEW throws some errors, so discard all the errors so far
    while(glGetError() != GL_NO_ERROR) {}

    // print out some info about the graphics drivers
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // make sure OpenGL version 3.2 API is available
    if(!GLEW_VERSION_3_2)
        throw std::runtime_error("OpenGL 3.2 API is not available.");

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // initialise the gWoodenCrate asset
    LoadFloorAsset();
    LoadGridAsset();
    //LoadAdvectAsset();

    // create all the instances in the 3D scene based on the gWoodenCrate asset
    CreateInstances();

    // setup gCamera
    gCamera.setPosition(glm::vec3(40,40,120));
    gCamera.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
    gCamera.setNearAndFarPlanes(0.5f, 1000.0f);

    // setup gLight
    gLight.position = gCamera.position();
    gLight.intensities = glm::vec3(1,1,1); //white

    // run while the window is open
    float lastTime = (float)glfwGetTime();
    while(!glfwWindowShouldClose(gWindow)){
        // process pending events
        glfwPollEvents();

        // update the scene based on the time elapsed since last update
        float thisTime = (float)glfwGetTime();
        deltaTime = thisTime - lastTime;
        //std::cout<<"deltaTime: "<<deltaTime<<"\n";
        Update(thisTime - lastTime);
        lastTime = thisTime;

        // draw one frame
        Render();

        // check for errors
        GLenum error = glGetError();
        if(error != GL_NO_ERROR)
            std::cerr << "OpenGL Error " << error << std::endl;

        //exit program if escape key is pressed
        if(glfwGetKey(gWindow, GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(gWindow, GL_TRUE);
    }

    // clean up and exit
    glfwTerminate();
}


int main(int argc, char *argv[]) {
    try {
        AppMain();
    } catch (const std::exception& e){
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
