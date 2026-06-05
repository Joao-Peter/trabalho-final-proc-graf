#include <vector>
#include "GameObject.h"

using namespace std;

struct Tile {
    unsigned char id; // identificação do tile, que corresponde à posição do tile no tileset
    bool collidable; // indica se o tile é colidível ou não
    bool breakable; // indica se o tile é quebrável ou não
    bool gameOver; // indica se o tile é de game over ou não
};

class TileMap {
    float z;               // caso de eventual de vários tilemaps sobrepostos
    unsigned int tid;      // indicação do tileset utilizado
    int width, height;     // dimensões da matriz
    int tileSetCols, tileSetRows; // número de colunas e linhas do tileset utilizado
    Tile *map; // mapa com ids dos tiles que formam o cenário
    vector<GameObject*> gameObjects; // objeto de jogo associado a este tilemap, se houver
    float tileH, tileW; // dimensões de um tile, calculadas a partir do número de colunas e linhas do tileset    
    
public:
    TileMap(int w, int h, unsigned char initWith, int tileSetCols, int tileSetRows) {
        this->map = new Tile [w*h];
        this->gameObjects = vector<GameObject*>();        
        this->width = w;
        this->height = h;
        this->z = 0.0f;
        this->tileSetCols = tileSetCols;
        this->tileSetRows = tileSetRows;
        this->tid = 0;
        this->tileW = 1.0f / (float)tileSetCols;  
        this->tileH = 1.0f / (float)tileSetRows;
    }
    
    Tile* getMap() {
        return this->map;
    }
    
    int getWidth() {
        return this->width;
    }
    
    int getHeight() {
        return this->height;
    }
    
    Tile getTile(int col, int row) {
        return this->map[col + row * this->width];
    }
    
    void setTile(int col, int row, unsigned char tile, bool collidable = false, bool breakable = false, bool gameOver = false) {
        this->map[col + row * this->width].id = tile;
        this->map[col + row * this->width].collidable = collidable;
        this->map[col + row * this->width].breakable = breakable;
        this->map[col + row * this->width].gameOver = gameOver;
    }
    
    bool isCollidable(int col, int row) {        
        return this->map[col + row * this->width].collidable;
    }
    
    void setCollidable(int col, int row, bool collidable) {
        this->map[col + row * this->width].collidable = collidable;
    }

    bool isGameOverTile(int col, int row) {
        return this->map[col + row * this->width].gameOver;
    }

    void setGameOverTile(int col, int row, bool gameOver) {
        this->map[col + row * this->width].gameOver = gameOver;
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

    void setBreakable(int col, int row, bool breakable) {
        this->map[col + row * this->width].breakable = breakable;
    }

    bool isBreakableObjectAt(int col, int row) {
        return this->map[col + row * this->width].breakable;
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

