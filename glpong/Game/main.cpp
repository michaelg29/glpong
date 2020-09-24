#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

// settings
unsigned int scrWidth = 800;
unsigned int scrHeight = 600;
const char* title = "Pong";
GLuint shaderProgram;

// graphics paramters
const float paddleSpeed = 75.0f;
const float paddleHeight = 100.0f;
const float halfPaddleHeight = paddleHeight / 2.0f;
const float paddleWidth = 10.0f;
const float halfPaddleWidth = paddleWidth / 2.0f;
const float ballDiameter = 16.0f;
const float ballRadius = ballDiameter / 2.0f;
const float offset = ballRadius;
const float paddleBoundary = halfPaddleHeight + offset;

/*
    initialization methods
*/

// initialize GLFW
void initGLFW(unsigned int versionMajor, unsigned int versionMinor) {
    glfwInit();

    // pass in window params
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versionMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versionMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // macos specific parameter
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

// create window
void createWindow(GLFWwindow*& window, 
    const char* title, unsigned int width, unsigned int height, 
    GLFWframebuffersizefun framebufferSizeCallback) {
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
}

// load GLAD library
bool loadGlad() {
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

/*
    shader methods
*/

// read file
std::string readFile(const char* filename) {
    std::ifstream file;
    std::stringstream buf;

    std::string ret = "";

    // open file
    file.open(filename);

    if (file.is_open()) {
        // read buffer
        buf << file.rdbuf();
        ret = buf.str();
    }
    else {
        std::cout << "Could not open " << filename << std::endl;
    }

    // close file
    file.close();

    return ret;
}

// generate shader
int genShader(const char* filepath, GLenum type) {
    std::string shaderSrc = readFile(filepath);
    const GLchar* shader = shaderSrc.c_str();

    // build and compile shader
    int shaderObj = glCreateShader(type);
    glShaderSource(shaderObj, 1, &shader, NULL);
    glCompileShader(shaderObj);

    // check for errors
    int success;
    char infoLog[512];
    glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderObj, 512, NULL, infoLog);
        std::cout << "Error in shader compilation: " << infoLog << std::endl;
        return -1;
    }

    return shaderObj;
}

// generate shader program
int genShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath) {
    int shaderProgram = glCreateProgram();

    // compile shaders
    int vertexShader = genShader(vertexShaderPath, GL_VERTEX_SHADER);
    int fragmentShader = genShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

    if (vertexShader == -1 || fragmentShader == -1) {
        return -1;
    }

    // link shaders
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // check for errors
    int success;
    char infoLog[512];
    glGetShaderiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Error in shader linking: " << infoLog << std::endl;
        return -1;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// bind shader
void bindShader(int shaderProgram) {
    glUseProgram(shaderProgram);
}

// set projection
void setOrthographicProjection(int shaderProgram,
    float left, float right,
    float bottom, float top,
    float near, float far) {
    float mat[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, -2.0f / (far - near), 0.0f },
        { -(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1.0f }
    };

    bindShader(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &mat[0][0]);
}

// delete shader
void deleteShader(int shaderProgram) {
    glDeleteProgram(shaderProgram);
}

/*
    Vertex Array Object/Buffer Object Methods
*/

// structure for VAO storing Array Object and its Buffer Objects
struct VAO {
    GLuint val;
    GLuint posVBO;
    GLuint offsetVBO;
    GLuint sizeVBO;
    GLuint EBO;
};

// generate VAO
void genVAO(VAO* vao) {
    glGenVertexArrays(1, &vao->val);
    glBindVertexArray(vao->val);
}

// generate buffer of certain type and set data
template<typename T>
void genBufferObject(GLuint& bo, GLenum type, GLuint noElements, T* data, GLenum usage) {
    glGenBuffers(1, &bo);
    glBindBuffer(type, bo);
    glBufferData(type, noElements * sizeof(T), data, usage);
}

// update data in a buffer object
template<typename T>
void updateData(GLuint& bo, GLintptr offset, GLuint noElements, T* data) {
    glBindBuffer(GL_ARRAY_BUFFER, bo);
    glBufferSubData(GL_ARRAY_BUFFER, offset, noElements * sizeof(T), data);
}

// set attribute pointers
template<typename T>
void setAttPointer(GLuint& bo, GLuint idx, GLint size, GLenum type, GLuint stride, GLuint offset, GLuint divisor = 0) {
    glBindBuffer(GL_ARRAY_BUFFER, bo);
    glVertexAttribPointer(idx, size, type, GL_FALSE, stride * sizeof(T), (void*)(offset * sizeof(T)));
    glEnableVertexAttribArray(idx);
    if (divisor > 0) {
        // reset _idx_ attribute every _divisor_ iteration through instances
        glVertexAttribDivisor(idx, divisor);
    }
}

// draw VAO
void draw(VAO vao, GLenum mode, GLuint count, GLenum type, GLint indices, GLuint instanceCount = 1) {
    glBindVertexArray(vao.val);
    glDrawElementsInstanced(mode, count, type, (void*)indices, instanceCount);
}

// unbind buffer
void unbindBuffer(GLenum type) {
    glBindBuffer(type, 0);
}

// unbind VAO
void unbindVAO() {
    glBindVertexArray(0);
}

// deallocate VAO/VBO memory
void cleanup(VAO vao) {
    glDeleteBuffers(1, &vao.posVBO);
    glDeleteBuffers(1, &vao.offsetVBO);
    glDeleteBuffers(1, &vao.sizeVBO);
    glDeleteBuffers(1, &vao.EBO);
    glDeleteVertexArrays(1, &vao.val);
}

// method to generate arrays for circle model
void gen2DCircleArray(float*& vertices, unsigned int*& indices, unsigned int noTriangles, float radius = 0.5f) {
    vertices = new float[(noTriangles + 1) * 2];
    /*
        vertices

        x   y   index
        0.0 0.0 0
        x1  y1  1
        x2  y2  2
    */
    
    // set origin
    vertices[0] = 0.0f;
    vertices[1] = 0.0f;

    indices = new unsigned int[noTriangles * 3];

    float pi = 4 * atanf(1.0f);
    float noTrianglesF = (float)noTriangles;
    float theta = 0.0f;

    for (unsigned int i = 0; i < noTriangles; i++) {
        /*
            radius
            theta = i * (2 * pi / noTriangles)
            x = rcos(theta) = vertices[(i + 1) * 2]
            y = rsin(theta) = vertices[(i + 1) * 2 + 1]
        */

        // set vertices
        vertices[(i + 1) * 2 + 0] = radius * cosf(theta);
        vertices[(i + 1) * 2 + 1] = radius * sinf(theta);

        // set indices
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;

        // step up theta
        theta += 2 * pi / noTriangles;
    }

    // set last index to wrap around to beginning
    indices[(noTriangles - 1) * 3 + 2] = 1;
}

/*
    main loop methods
*/

// callback for window size change
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    scrWidth = width;
    scrHeight = height;

    // update projection matrix
    setOrthographicProjection(shaderProgram, 0, width, 0, height, 0.0f, 1.0f);
}

// process input
void processInput(GLFWwindow* window, double dt, float *paddleOffsets) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // left paddle
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // boundary condition
        if (paddleOffsets[1] < scrHeight - paddleBoundary) {
            paddleOffsets[1] += dt * paddleSpeed;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (paddleOffsets[1] > paddleBoundary) {
            paddleOffsets[1] -= dt * paddleSpeed;
        }
    }

    // right paddle
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        if (paddleOffsets[3] < scrHeight - paddleBoundary) {
            paddleOffsets[3] += dt * paddleSpeed;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        if (paddleOffsets[3] > paddleBoundary) {
            paddleOffsets[3] -= dt * paddleSpeed;
        }
    }
}

// clear screen
void clearScreen() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

// new frame
void newFrame(GLFWwindow* window) {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

/*
    cleanup methods
*/

// terminate glfw
void cleanup() {
    glfwTerminate();
}

int main() {
    std::cout << "Hello, Atari!" << std::endl;

    // timing
    double dt = 0.0;
    double lastFrame = 0.0;

    // initialization
    initGLFW(3, 3);

    // create window
    GLFWwindow* window = nullptr;
    createWindow(window, title, scrWidth, scrHeight, framebufferSizeCallback);
    if (!window) {
        std::cout << "Could not create window" << std::endl;
        cleanup();
        return -1;
    }

    // load glad
    if (!loadGlad()) {
        std::cout << "Could not init GLAD" << std::endl;
        cleanup();
        return -1;
    }

    glViewport(0, 0, scrWidth, scrHeight);

    // shaders
    shaderProgram = genShaderProgram("main.vs", "main.fs");
    setOrthographicProjection(shaderProgram, 0, scrWidth, 0, scrHeight, 0.0f, 1.0f);

    /*
        Paddle VAO/BOs
    */

    // setup vertex data
    float paddleVertices[] = {
        //	x		y
         0.5f,  0.5f,
        -0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f, -0.5f
    };

    // setup index data
    unsigned int paddleIndices[] = {
        0, 1, 2, // top left triangle
        2, 3, 0  // bottom right triangle
    };

    // offsets array
    float paddleOffsets[] = {
        35.0f, scrHeight / 2.0f,
        scrWidth - 35.0f, scrHeight / 2.0f
    };

    // size array
    float paddleSizes[] = {
        paddleWidth, paddleHeight
    };

    // setup VAO
    VAO paddleVAO;
    genVAO(&paddleVAO);

    // pos VBO
    genBufferObject<float>(paddleVAO.posVBO, GL_ARRAY_BUFFER, 2 * 4, paddleVertices, GL_STATIC_DRAW);
    setAttPointer<float>(paddleVAO.posVBO, 0, 2, GL_FLOAT, 2, 0);

    // offset VBO
    genBufferObject<float>(paddleVAO.offsetVBO, GL_ARRAY_BUFFER, 2 * 2, paddleOffsets, GL_DYNAMIC_DRAW);
    setAttPointer<float>(paddleVAO.offsetVBO, 1, 2, GL_FLOAT, 2, 0, 1);

    // size VBO
    genBufferObject<float>(paddleVAO.sizeVBO, GL_ARRAY_BUFFER, 2 * 1, paddleSizes, GL_STATIC_DRAW);
    setAttPointer<float>(paddleVAO.sizeVBO, 2, 2, GL_FLOAT, 2, 0, 2);

    // EBO
    genBufferObject<GLuint>(paddleVAO.EBO, GL_ELEMENT_ARRAY_BUFFER, 2 * 4, paddleIndices, GL_STATIC_DRAW);

    // unbind VBO and VAO
    unbindBuffer(GL_ARRAY_BUFFER);
    unbindVAO();

    /*
        Ball VAO/BOs
    */

    // setup vertex and index data
    float* ballVertices;
    unsigned int* ballIndices;
    unsigned int noTriangles = 50;
    gen2DCircleArray(ballVertices, ballIndices, noTriangles, 0.5f);

    // offsets array
    float ballOffsets[] = {
        scrWidth / 2.0f, scrHeight / 2.0f
    };

    // size array
    float ballSizes[] = {
        ballDiameter, ballDiameter
    };

    // setup VAO
    VAO ballVAO;
    genVAO(&ballVAO);

    // pos VBO
    genBufferObject<float>(ballVAO.posVBO, GL_ARRAY_BUFFER, 2 * (noTriangles + 1), ballVertices, GL_STATIC_DRAW);
    setAttPointer<float>(ballVAO.posVBO, 0, 2, GL_FLOAT, 2, 0);

    // offset VBO
    genBufferObject<float>(ballVAO.offsetVBO, GL_ARRAY_BUFFER, 1 * 2, ballOffsets, GL_DYNAMIC_DRAW);
    setAttPointer<float>(ballVAO.offsetVBO, 1, 2, GL_FLOAT, 2, 0, 1);

    // size VBO
    genBufferObject<float>(ballVAO.sizeVBO, GL_ARRAY_BUFFER, 1 * 2, ballSizes, GL_STATIC_DRAW);
    setAttPointer<float>(ballVAO.sizeVBO, 2, 2, GL_FLOAT, 2, 0, 1);

    // EBO
    genBufferObject<unsigned int>(ballVAO.EBO, GL_ELEMENT_ARRAY_BUFFER, 3 * noTriangles, ballIndices, GL_STATIC_DRAW);

    // unbind VBO and VAO
    unbindBuffer(GL_ARRAY_BUFFER);
    unbindVAO();

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // update time
        dt = glfwGetTime() - lastFrame;
        lastFrame += dt;

        // input
        processInput(window, dt, paddleOffsets);

        // clear screen for new frame
        clearScreen();

        // update
        updateData<float>(paddleVAO.offsetVBO, 0, 2 * 2, paddleOffsets);
        updateData<float>(ballVAO.offsetVBO, 0, 1 * 2, ballOffsets);

        // render object
        bindShader(shaderProgram);
        draw(paddleVAO, GL_TRIANGLES, 3 * 2, GL_UNSIGNED_INT, 0, 2);
        draw(ballVAO, GL_TRIANGLES, 3 * noTriangles, GL_UNSIGNED_INT, 0);

        // swap frames
        newFrame(window);
    }

    // cleanup memory
    cleanup(paddleVAO);
    cleanup(ballVAO);
    deleteShader(shaderProgram);
    cleanup();

    return 0;
}