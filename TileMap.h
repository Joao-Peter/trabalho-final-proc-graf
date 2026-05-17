class TileMap {
    float z;               // caso de eventual de vários tilemaps sobrepostos
    unsigned int tid;      // indicação do tileset utilizado
    int width, height;     // dimensões da matriz
    int tileSetCols, tileSetRows; // número de colunas e linhas do tileset utilizado
    unsigned char *map; // mapa com ids dos tiles que formam o cenário
    bool *collidable; // indica se o tilemap é colidível ou não    
    float tileH, tileW; // dimensões de um tile, calculadas a partir do número de colunas e linhas do tileset
    
public:
    TileMap(int w, int h, unsigned char initWith, int tileSetCols, int tileSetRows) {
        this->map = new unsigned char [w*h];
        this->collidable = new bool [w*h];
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
    
    void setTile(int col, int row, unsigned char tile, bool collidable = false) {
        this->map[col + row * this->width] = tile;
        this->collidable[col + row * this->width] = collidable;
    }
    
    bool isCollidable(int col, int row) {
        return this->collidable[col + row * this->width];
    }
    
    void setCollidable(int col, int row, bool collidable) {
        this->collidable[col + row * this->width] = collidable;
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

