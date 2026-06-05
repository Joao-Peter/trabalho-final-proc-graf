#include <vector>
#include "GameObject.h"

using namespace std;

class TileMap {
    float z;               // caso de eventual de vários tilemaps sobrepostos
    unsigned int tid;      // indicação do tileset utilizado
    int width, height;     // dimensões da matriz
    int tileSetCols, tileSetRows; // número de colunas e linhas do tileset utilizado
    unsigned char *map; // mapa com ids dos tiles que formam o cenário
    bool *collidable; // indica se o tilemap é colidível ou não    
    bool *gameOverTiles;
    vector<GameObject*> gameObjects; // objeto de jogo associado a este tilemap, se houver
    float tileH, tileW; // dimensões de um tile, calculadas a partir do número de colunas e linhas do tileset
    vector<pair<int, int>> breakableObjectsPositions; // posições dos objetos quebráveis no mapa
    
public:
    TileMap(int w, int h, unsigned char initWith, int tileSetCols, int tileSetRows) {
        this->map = new unsigned char [w*h];
        this->collidable = new bool [w*h];
        this->gameOverTiles = new bool [w*h];
        this->gameObjects = vector<GameObject*>();
        this->breakableObjectsPositions = vector<pair<int, int>>();
        this->width = w;
        this->height = h;
        this->z = 0.0f;
        this->tileSetCols = tileSetCols;
        this->tileSetRows = tileSetRows;
        this->tid = 0;
        this->tileW = 1.0f / (float)tileSetCols;  
        this->tileH = 1.0f / (float)tileSetRows;
    }
    
    unsigned char* getMap() {
        return this->map;
    }
    
    int getWidth() {
        return this->width;
    }
    
    int getHeight() {
        return this->height;
    }
    
    int getTile(int col, int row) {
        return this->map[col + row * this->width];
    }
    
    void setTile(int col, int row, unsigned char tile, bool collidable = false, bool gameOver = false) {
        this->map[col + row * this->width] = tile;
        this->collidable[col + row * this->width] = collidable;
        this->gameOverTiles[col + row * this->width] = gameOver;
    }
    
    bool isCollidable(int col, int row) {        
        return this->collidable[col + row * this->width];
    }
    
    void setCollidable(int col, int row, bool collidable) {
        this->collidable[col + row * this->width] = collidable;
    }

    bool isGameOverTile(int col, int row) {
        return this->gameOverTiles[col + row * this->width];
    }

    void setGameOverTile(int col, int row, bool gameOver) {
        this->gameOverTiles[col + row * this->width] = gameOver;
    }

    void addObject(GameObject *object, int col, int row) {
        object->column = col;
        object->row = row;
        object->u = 0;
	    object->v = 0;        
        this->gameObjects.push_back(object);
    }

    vector<GameObject*> getObjects() {
        return this->gameObjects;
    }

    GameObject* getObjectById(int id) {
        for (GameObject *object : this->gameObjects) {
            if (object->getId() == id) {
                return object;
            }
        }
        return nullptr;
    }

    GameObject* checkIsObjectColliding(int col, int row) {
        for (GameObject *object : this->gameObjects) {
            if (object->Collides(col, row)) {
                return object;
            }
        }
        return nullptr;
    }

    bool removeObjectAt(int col, int row) {
        for (auto it = this->gameObjects.begin(); it != this->gameObjects.end(); it++) {
            if ((*it)->Collides(col, row)) {
                this->gameObjects.erase(it);
                return true;
            }
        }
        return false;
    }

    void addBreakableObjectPosition(int col, int row) {
        this->breakableObjectsPositions.push_back(make_pair(col, row));
    }

    void removeBreakableObjectPosition(int col, int row) {
        this->breakableObjectsPositions.erase(
            remove_if(
                this->breakableObjectsPositions.begin(), 
                this->breakableObjectsPositions.end(),
                [col, row](const pair<int, int>& pos) { return pos.first == col && pos.second == row; }
            ),
            this->breakableObjectsPositions.end()
        );
    }

    bool isBreakableObjectAt(int col, int row) {
        for (auto &pos : this->breakableObjectsPositions) {
            if (pos.first == col && pos.second == row) {
                return true;
            }
        }
        return false;
    }

    int getTileSet() {
        return this->tid;
    }
    
    float getZ() {
        return this->z;
    }
    
    void setZ(float z){
        this->z = z;
    }
    
    void setTid(int tid) {
        this->tid = tid;
    }

    int getTileSetCols() {
        return this->tileSetCols;
    }    

    int getTileSetRows() {
        return this->tileSetRows;
    }

    float getTileW() {
        return this->tileW;
    }

    float getTileH() {
        return this->tileH;
    }
};

