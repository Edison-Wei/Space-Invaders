#include "Include.h"
#include "Alien.h"
#include "alienSprites.h"
#include "Buffer.h"
#include "Player.h"
#include "Projectile.h"
#include "Sprite.h"
#include "SpriteAnimation.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>


#define GAME_MAX_PROJECTILES 128

bool gameRunning = false; // Check to see if the ESC keys is pressed or the lives is at 0
int moveDirection = 0; // Direction of player movement (-1 for left, 1 for right);
bool firePressed = false;

void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

void validateShader(GLuint shader, const char* file = 0) {
    static const unsigned int BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    GLsizei length = 0;

    glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

    if(length > 0) {
        printf("Shader %d(%s) compile error: %s \n", shader, (file ? file: ""), buffer);
    }
}

bool validateProgram(GLuint program) {
    static const GLsizei BUFFER_SIZE = 512;
    GLchar buffer[BUFFER_SIZE];
    GLsizei length = 0;

    glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);

    if(length > 0) {
        printf("Program %d link error: %s \n", program, buffer);
        return false;
    }
    return true;
}

void keyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
        if(action == GLFW_PRESS)
            gameRunning = false;
        break;
    case GLFW_KEY_RIGHT:
        if(action == GLFW_PRESS)
            moveDirection += 1;
        else if(action == GLFW_RELEASE)
            moveDirection -= 1;
        break;
    case GLFW_KEY_LEFT:
        if(action == GLFW_PRESS)
            moveDirection -= 1;
        else if(action == GLFW_RELEASE)
            moveDirection += 1;
        break;
    case GLFW_KEY_SPACE:
        if(action == GLFW_RELEASE)
            firePressed = true;
        break;
    default:
        break;
    }
}

enum AlienType: uint8_t {
    ALIEN_DEAD   = 0,
    ALIEN_TYPE_A = 1,
    ALIEN_TYPE_B = 2,
    ALIEN_TYPE_C = 3
};

struct Game {
    size_t width, height;
    size_t numAliens;
    size_t numProjectiles;
    Alien* aliens;
    Player player;
    Projectile projectiles[GAME_MAX_PROJECTILES];
};

uint32_t rgbToUint32(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 24) | (g << 16) | (b << 8) | 255;
}

void bufferClear(Buffer* buffer, uint32_t colour) {
    for(size_t i = 0; i < buffer->width * buffer->height; i++) {
        buffer->data[i] = colour;
    }
}

void bufferDrawSprite(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t colour) {
    for(size_t xi = 0; xi < sprite.width; xi++) {
        for(size_t yi = 0; yi < sprite.height; yi++) {
            size_t sy = sprite.height - 1 + y - yi;
            size_t sx = x + xi;
            if (sprite.data[yi * sprite.width + xi] && sy < buffer->height && sx < buffer->width) {
                buffer->data[sy * buffer->width + sx] = colour;
            }
        }
    }
}

bool spriteOverLapCheck(const Sprite& spriteA, const Sprite& spriteB, size_t xA, size_t yA, size_t xB, size_t yB) {
    if(xA < xB + spriteB.width && xA + spriteA.width > xB && yA < yB + spriteB.height && yA + spriteA.height > yB)
        return true;
    return false;
}

void bufferDrawText(Buffer* buffer, const Sprite& textSpriteSheet, const char* text, size_t x, size_t y, uint32_t colour) {
    // size_t xp = x;
    size_t stride = textSpriteSheet.width * textSpriteSheet.height;
    Sprite sprite = textSpriteSheet;
    for (const char*  charbit = text; *charbit != '\0'; charbit++) {
        char character = *charbit - 32;
        if(character < 0 || character >= 65)
            continue;
        
        sprite.data = textSpriteSheet.data + character * stride;
        bufferDrawSprite(buffer, sprite, x, y, colour);
        x += sprite.width + 1;
    }
}

void bufferDrawNumber(Buffer* buffer, const Sprite& numberSpriteSheet, size_t number, size_t x, size_t y, uint32_t colour) {
    uint8_t digits[64];
    size_t numDigits = 0;
    do {
        digits[numDigits++] = number % 10;
        number = number / 10;
    } while (number > 0);

    size_t stride = numberSpriteSheet.width * numberSpriteSheet.height;
    Sprite sprite = numberSpriteSheet;
    for (size_t i = 0; i < numDigits; i++)
    {
        uint8_t digit = digits[numDigits - i - 1];
        sprite.data = numberSpriteSheet.data + digit * stride;
        bufferDrawSprite(buffer, sprite, x, y, colour);
        x += sprite.width + 1;
    }
}

int main() {
    const size_t buffer_width = 224;
    const size_t buffer_height = 256;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Space Invaders", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, keyCallBack);

    glfwMakeContextCurrent(window);

    GLenum err = glewInit();
    if(err != GLEW_OK) {
        fprintf(stderr, "Error initializing GLEW.\n");
        glfwTerminate();
        return -1;
    }

    int glVersion[2] = {-1, 1};
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

    printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);
    printf("Renderer used: %s\n", glGetString(GL_RENDERER));

    glfwSwapInterval(1);

    glClearColor(1.0, 0.0, 0.0, 1.0);

    // Graphics buffer
    Buffer buffer;
    buffer.width = buffer_width;
    buffer.height = buffer_height;
    buffer.data = new uint32_t[buffer.width * buffer.height];
    bufferClear(&buffer, 0);

    GLuint bufferTexture;
    glGenTextures(1, &bufferTexture);

    glBindTexture(GL_TEXTURE_2D, bufferTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB8,
        buffer.width, buffer.height, 0,
        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data
    );
    // To tell the GPU not the apply any filtering when rendering 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Use edge values when going beyond texture bounds
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    static const char* vertexShader =
        "\n"
        "#version 330 \n"
        "noperspective out vec2 TexCoord; \n"
        "\n"
        "void main(void) { \n"
        "\n"
        "   TexCoord.x = (gl_VertexID == 2) ? 2.0 : 0.0; \n"
        "   TexCoord.y = (gl_VertexID == 1) ? 2.0 : 0.0; \n"
        "\n"
        "   gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0); \n"
        "} \n";
    
    static const char* fragmentShader = 
        "\n"
        "#version 330 \n"
        "\n"
        "uniform sampler2D buffer; \n"
        "noperspective in vec2 TexCoord; \n"
        "\n"
        "out vec3 outColor; \n"
        "\n"
        "void main(void) { \n"
        "    outColor = texture(buffer, TexCoord).rgb; \n"
        "} \n";
    
    GLuint fullscreenTriangleVao;
    glGenVertexArrays(1, &fullscreenTriangleVao);
    glBindVertexArray(fullscreenTriangleVao);

    // To create into the program forGPU useage
    GLuint shaderID = glCreateProgram();
    
    // For creation of vertex shader
    {
        GLuint shaderVP = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(shaderVP, 1, &vertexShader, 0);
        glCompileShader(shaderVP);
        validateShader(shaderVP, vertexShader);
        glAttachShader(shaderID, shaderVP);

        glDeleteShader(shaderVP);
    }

    // For creation of fragment shader
    {
        GLuint shaderFP = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(shaderFP, 1, &fragmentShader, 0);
        glCompileShader(shaderFP);
        validateShader(shaderFP, fragmentShader);
        glAttachShader(shaderID, shaderFP);

        glDeleteShader(shaderFP);
    }

    glLinkProgram(shaderID);

    if(!validateProgram(shaderID)) {
        fprintf(stderr, "Error while validating shader. \n");
        glfwTerminate();
        glDeleteVertexArrays(1, &fullscreenTriangleVao);
        delete[] buffer.data;
        return -1;
    }

    glUseProgram(shaderID);

    // Just to attach the shader
    GLint location = glGetUniformLocation(shaderID, "buffer");
    glUniform1i(location, 0);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(fullscreenTriangleVao);

    // Create Alien and Player sprites encoded as a bitmap
    Sprite alienSprites[6];
    alienSprites[0].width = 8;
    alienSprites[0].height = 8;
    alienSprites[0].data = new uint8_t[64]
    {
        0,0,0,1,1,0,0,0, // ...@@...
        0,0,1,1,1,1,0,0, // ..@@@@..
        0,1,1,1,1,1,1,0, // .@@@@@@.
        1,1,0,1,1,0,1,1, // @@.@@.@@
        1,1,1,1,1,1,1,1, // @@@@@@@@
        0,1,0,1,1,0,1,0, // .@.@@.@.
        1,0,0,0,0,0,0,1, // @......@
        0,1,0,0,0,0,1,0  // .@....@.
    };

    alienSprites[1].width = 8;
    alienSprites[1].height = 8;
    alienSprites[1].data = new uint8_t[64]
    {
        0,0,0,1,1,0,0,0, // ...@@...
        0,0,1,1,1,1,0,0, // ..@@@@..
        0,1,1,1,1,1,1,0, // .@@@@@@.
        1,1,0,1,1,0,1,1, // @@.@@.@@
        1,1,1,1,1,1,1,1, // @@@@@@@@
        0,0,1,0,0,1,0,0, // ..@..@..
        0,1,0,1,1,0,1,0, // .@.@@.@.
        1,0,1,0,0,1,0,1  // @.@..@.@
    };

    alienSprites[2].width = 11;
    alienSprites[2].height = 8;
    alienSprites[2].data = new uint8_t[88]
    {
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
        0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
        0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
        0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
    };

    alienSprites[3].width = 11;
    alienSprites[3].height = 8;
    alienSprites[3].data = new uint8_t[88]
    {
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
    };

    alienSprites[4].width = 12;
    alienSprites[4].height = 8;
    alienSprites[4].data = new uint8_t[96]
    {
        0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
        0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        0,0,0,1,1,0,0,1,1,0,0,0, // ...@@..@@...
        0,0,1,1,0,1,1,0,1,1,0,0, // ..@@.@@.@@..
        1,1,0,0,0,0,0,0,0,0,1,1  // @@........@@
    };


    alienSprites[5].width = 12;
    alienSprites[5].height = 8;
    alienSprites[5].data = new uint8_t[96]
    {
        0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
        0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        0,0,1,1,1,0,0,1,1,1,0,0, // ..@@@..@@@..
        0,1,1,0,0,1,1,0,0,1,1,0, // .@@..@@..@@.
        0,0,1,1,0,0,0,0,1,1,0,0  // ..@@....@@..
    };

    Sprite alienDeathSprite;
    alienDeathSprite.width = 13;
    alienDeathSprite.height = 7;
    alienDeathSprite.data = new uint8_t[91]
    {
        0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
    };

    Sprite playerSprite;
    playerSprite.width = 11;
    playerSprite.height = 7;
    playerSprite.data = new uint8_t[77]
    {
        0,0,0,0,0,1,0,0,0,0,0, // .....@.....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
    };

    Sprite textSpriteSheet;
    textSpriteSheet.width = 5;
    textSpriteSheet.height = 7;
    textSpriteSheet.data = new uint8_t[65 * 35]
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,
        0,1,0,1,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,0,1,0,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,0,1,0,1,0,
        0,0,1,0,0,0,1,1,1,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,1,1,1,0,0,0,1,0,0,
        1,1,0,1,0,1,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,1,0,1,0,1,1,
        0,1,1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,1,0,0,0,1,0,1,1,1,1,
        0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,
        1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
        0,0,1,0,0,1,0,1,0,1,0,1,1,1,0,0,0,1,0,0,0,1,1,1,0,1,0,1,0,1,0,0,1,0,0,
        0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
        0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,

        0,1,1,1,0,1,0,0,0,1,1,0,0,1,1,1,0,1,0,1,1,1,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,0,1,1,0,0,1,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,0,0,1,0,0,0,1,1,0,0,1,0,1,0,1,0,0,1,0,1,1,1,1,1,0,0,0,1,0,0,0,0,1,0,
        1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,

        0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,
        0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,
        0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
        1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,0,1,0,0,0,1,0,1,1,1,0,

        0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,
        1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,0,1,1,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,
        0,1,1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,
        0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,1,0,1,0,1,0,0,1,1,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0,0,0,1,
        1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,0,0,0,1,1,1,0,1,1,1,0,1,0,1,1,0,1,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,1,0,0,1,1,1,0,0,0,1,1,0,0,0,1,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1,0,1,1,0,0,1,1,0,1,1,1,1,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,1,0,0,1,0,0,1,0,1,0,0,0,1,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,0,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,1,1,1,0,
        1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1,0,1,1,0,1,0,1,1,1,0,1,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,1,1,1,1,

        0,0,0,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,1,
        0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,
        1,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,1,1,0,0,0,
        0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
        0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };

    Sprite numberSpriteSheet = textSpriteSheet;
    numberSpriteSheet.data += 16 * 35;

    Sprite projectileSprite;
    projectileSprite.width = 1;
    projectileSprite.height = 3;
    projectileSprite.data = new uint8_t[3]
    {
        1, // @
        1, // @
        1  // @
    };


    SpriteAnimation alienAnimation[3];

    for(size_t i = 0; i < 3; ++i)
    {
        alienAnimation[i].loop = true;
        alienAnimation[i].numFrames = 2;
        alienAnimation[i].frameDuration = 10;
        alienAnimation[i].time = 0;

        alienAnimation[i].frames = new Sprite*[2];
        alienAnimation[i].frames[0] = &alienSprites[2 * i];
        alienAnimation[i].frames[1] = &alienSprites[2 * i + 1];
    }

    Game game;
    game.width = buffer_width;
    game.height = buffer_height;
    game.numAliens = 55;
    game.aliens = new Alien[game.numAliens];
    game.numProjectiles = 0;

    game.player.x = 112 - 5;
    game.player.y = 32;
    game.player.life = 3;

    for(size_t yi = 0; yi < 5; yi++) {
        for(size_t xi = 0; xi < 11; xi++) {
            Alien& alien = game.aliens[yi * 11 + xi];
            alien.type = (5 - yi) / 2 + 1;
            const Sprite& sprite = alienSprites[2 * (alien.type - 1)];

            alien.x = 16 * xi + 20 + (alienDeathSprite.width - sprite.width)/2;
            alien.y = 17 * yi + 128;
        }
    }

    uint8_t* deathCounters = new uint8_t[game.numAliens];
    for(size_t i = 0; i < game.numAliens; i++) {
        deathCounters[i] = 10;
    }
    
    size_t score = 0;

    uint32_t clearColour = rgbToUint32(0, 128, 0);
    int playerMoveDirection = 0;
    gameRunning = true;

    while(!glfwWindowShouldClose(window) && gameRunning) {
        bufferClear(&buffer, clearColour);

        // Draw Score board
        bufferDrawText(&buffer, textSpriteSheet, "SCORE", 4, game.height - textSpriteSheet.height - 7, rgbToUint32(128, 0, 0));

        bufferDrawNumber(&buffer, numberSpriteSheet, score, 4 + 2 * numberSpriteSheet.width, game.height - 2 * numberSpriteSheet.height - 12, rgbToUint32(128, 0, 0));

        // Draw Alien Sprites
        for(size_t ai = 0; ai < game.numAliens; ai++) {
            if(!deathCounters[ai])
                continue;
            const Alien& alien = game.aliens[ai];
            if(alien.type == ALIEN_DEAD){
                bufferDrawSprite(&buffer, alienDeathSprite, alien.x, alien.y, rgbToUint32(128, 0, 0));
            }
            else {
                const SpriteAnimation& animation = alienAnimation[alien.type - 1];
                size_t currentFrame = animation.time / animation.frameDuration;
                const Sprite& sprite = *animation.frames[currentFrame];
                bufferDrawSprite(&buffer, sprite, alien.x, alien.y, rgbToUint32(128, 0, 0));
            }
        }

        // Draw Projectile Sprite
        for (size_t bi = 0; bi < game.numProjectiles; bi++) {
            const Projectile& projectile = game.projectiles[bi];
            const Sprite& sprite = projectileSprite;
            bufferDrawSprite(&buffer, sprite, projectile.x, projectile.y, rgbToUint32(128, 0, 0));
        }
        
        // Draw Player Sprite
        bufferDrawSprite(&buffer, playerSprite, game.player.x, game.player.y, rgbToUint32(128, 0, 0));

        // Update Animations
        for (size_t i = 0; i < 3; i++) {
            alienAnimation[i].time++;
            if (alienAnimation[i].time == alienAnimation[i].numFrames * alienAnimation[i].frameDuration) {
                alienAnimation[i].time = 0;
            }
        }
        

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        buffer.width, buffer.height,
                        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
                        buffer.data);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);

        // Alien Movement
        for (size_t ai = 0; ai < game.numAliens; ai++) {
            const Alien& alien = game.aliens[ai];
            if(alien.type == ALIEN_DEAD && deathCounters[ai]) {
                deathCounters[ai]--;
            }
        }
        

        // Projectile Movement
        for (size_t bi = 0; bi < game.numProjectiles;) {
            game.projectiles[bi].y += game.projectiles[bi].direction;
            if(game.projectiles[bi].y >= game.height || game.projectiles[bi].y < projectileSprite.height) {
                game.projectiles[bi] = game.projectiles[game.numProjectiles - 1];
                game.numProjectiles--;
                continue;
            }
            for (size_t ai = 0; ai < game.numAliens; ai++) {
                const Alien& alien = game.aliens[ai];
                if(alien.type == ALIEN_DEAD)
                    continue;
                
                const SpriteAnimation& animation = alienAnimation[alien.type - 1];
                size_t currentFrame = animation.time / animation.frameDuration;
                const Sprite& alienSprite = *animation.frames[currentFrame];
                bool overlap = spriteOverLapCheck(projectileSprite, alienSprite, game.projectiles[bi].x, game.projectiles[bi].y, alien.x, alien.y);
                if(overlap) {
                    score += 5 * (4 - game.aliens[ai].type);
                    game.aliens[ai].type = ALIEN_DEAD;

                    game.aliens[ai].x -= (alienDeathSprite.width - alienSprite.width)/2;
                    game.projectiles[bi] = game.projectiles[game.numProjectiles - 1];
                    game.numProjectiles--;
                    continue;
                }
            }
            bi++;
        }
        

        // Player Movement
        playerMoveDirection = 2 * moveDirection;
        if(playerMoveDirection != 0) {
            if(game.player.x + playerSprite.width + playerMoveDirection >= game.width) {
                game.player.x = game.width - playerSprite.width;
            }
            else if ((int)game.player.x + playerMoveDirection <= 0) {
                game.player.x = 0;
            }
            else
                game.player.x += playerMoveDirection;
        }

        if(firePressed && game.numProjectiles < GAME_MAX_PROJECTILES) {
            game.projectiles[game.numProjectiles].x = game.player.x + playerSprite.width / 2;
            game.projectiles[game.numProjectiles].y = game.player.y + playerSprite.height;
            game.projectiles[game.numProjectiles].direction = 2;
            game.numProjectiles++;
        }
        firePressed = false;

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    glDeleteVertexArrays(1, &fullscreenTriangleVao);

    for(size_t i = 0; i < 6; ++i)
    {
        delete[] alienSprites[i].data;
    }

    delete[] alienDeathSprite.data;

    for(size_t i = 0; i < 3; ++i)
    {
        delete[] alienAnimation[i].frames;
    }

    delete[] deathCounters;
    delete[] buffer.data;
    delete[] game.aliens;
    
    return 0;
}