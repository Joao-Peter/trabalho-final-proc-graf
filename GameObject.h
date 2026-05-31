#pragma once
#include <GL/glew.h>

enum ObjectType
{
    AVOID,
    GRAB,
    CHAR
};

class GameObject
{
    const float xi = -1.0f;
    const float yi = -1.0f;

    unsigned int tid;            // indicação do tileset utilizado
    float width, height;         // dimensões do objeto
    float tileWidth, tileHeight; // tamanho de um tile        
    GLuint VAO;
    ObjectType type;

    GLuint genVAO()
    {
        float vertices[] = {
            // positions             // texture coords
            xi,         yi + height, 0.0f,      0.0f, // left
            xi + width, yi,          tileWidth, tileHeight, // bottom
            xi,         yi,          0.0f,      tileHeight, // right
            xi + width, yi + height, tileWidth, 0.0f, // top
        };

        unsigned int indices[] = {
            0, 1, 3, // first triangle
            0, 1, 2	 // second triangle
        };

        unsigned int VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
        // texture coord attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));        

        return VAO;
    }

public:
    virtual ~GameObject() = default;
    int row, column, u, v;
    float z;

    GameObject(float width, float height, float tileWidth, float tileHeight, unsigned int tid, ObjectType type)
    {
        this->width = width;
        this->height = height;
        this->tileWidth = tileWidth;
        this->tileHeight = tileHeight;
        this->type = type;
        this->tid = tid;
        this->row = -1;
        this->column = -1;
        this->u = 0;
        this->v = 0;
        this->z = -1;

        VAO = genVAO();
    }

    bool Collides(GameObject *otherObject)
    {
        return this->column == otherObject->column && this->row == otherObject->row;
    }

    bool Collides (int column, int row)
    {
        return this->column == column && this->row == row;
    }
        
    float getWidth() { return this->width; }
    float getHeight() { return this->height; }
    float getTileHeight() { return this->tileHeight; }
    float getTileWidth() { return this->tileWidth; }
    GLuint getVAO() { return this->VAO; }
    GLuint getTid() { return this->tid; }
    ObjectType getType() { return this->type; }
};