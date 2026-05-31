#pragma once
#include "GameObject.h"

class MovingGameObject : public GameObject
{
private:
    unsigned int direction;

public:    
    MovingGameObject(
        float width, 
        float height, 
        float tileWidth, 
        float tileHeight, 
        unsigned int tid, 
        ObjectType type,
        unsigned int direction) : GameObject(width, height, tileWidth, tileHeight, tid, type)
    {
        this->direction = direction;
    }

    unsigned int getDirection() { return this->direction; }
};