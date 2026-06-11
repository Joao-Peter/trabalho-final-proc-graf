// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gl_utils.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <time.h>
#define GL_LOG_FILE "gl.log"
#include <iostream>
#include <vector>
#include <algorithm>
#include "TileMap.h"
#include "DiamondView.h"
#include "MovingGameObject.h"
#include "ltMath.h"
#include <fstream>
#include "DesafioFinal.h"

using namespace std;

enum Objects
{
	CHARACTER = 0,
	HAMBURGER = 1,
	ORB = 2,
	LODGE = 3,
	BREAKABLE = 4,
	GAMEOVER = 5,
	VICTORY = 6
};;

struct TileMapWithVAO
{
	TileMap *tileMap;
	GLuint vao;
};

int g_gl_width = 980;
int g_gl_height = 780;
float xi = -1.0f;
float xf = 1.0f;
float yi = -1.0f;
float yf = 1.0f;
float w = xf - xi;
float h = yf - yi;
int tileSetCols = 9, tileSetRows = 9;
float mapTileWidth, mapTileHeight, tw2, th2;
int cx = 6, cy = 6;
bool gotHamburger = false;
TilemapView *tview = new DiamondView();
GLFWwindow *g_window = NULL;
GameObject *gameOverObject = nullptr;
GameObject *victoryObject = nullptr;

void loadTexture(unsigned int &texture, const string filename, GLint param = GL_LINEAR)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	GLint mipmap = param == GL_LINEAR
		? GL_LINEAR_MIPMAP_LINEAR
		: GL_NEAREST_MIPMAP_LINEAR;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap);

	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;

	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}

GameObject *getHamburgerObject()
{
	GLuint textureId;
	loadTexture(textureId, "./resources/hamburger.png", GL_NEAREST);

	auto object = new GameObject(
		Objects::HAMBURGER,
		(mapTileWidth / 2.0f),
		(mapTileHeight / 1.0f),
		1.0f,
		1.0f,
		textureId,
		OBJECTIVE
	);

	object->z = -0.1f;

	return object;
}

GameObject *getOrbObject(unsigned int direction)
{
	GLuint textureId;
	loadTexture(textureId, "./resources/orb.png", GL_NEAREST);

	auto object = new MovingGameObject(
		Objects::ORB,
		(mapTileWidth / 2.0f),
		(mapTileHeight / 1.0f),
		1.0f,
		1.0f,
		textureId,
		AVOID,
		direction
	);

	object->z = -0.1f;

	return object;		
}

GameObject *getLodgeObject()
{
	GLuint textureId;
	loadTexture(textureId, "./resources/lodge2.png", GL_NEAREST);

	auto object = new GameObject(
		Objects::LODGE,
		mapTileWidth * 2.0f,
		mapTileHeight * 4.0f,
		1.0f,
		1.0f,
		textureId,
		OBJECTIVE
	);

	object->z = -0.1f;
	object->setBlocked(true);

	return object;		
}

TileMap *readMap(const string filename, int tileSetCols, int tileSetRows)
{
	ifstream arq(filename.c_str());
	int fileW, fileH;
	arq >> fileW >> fileH;
	TileMap *tmap = new TileMap(fileW, fileH, 0, tileSetCols, tileSetRows);

	mapTileWidth = w / (float)tmap->getWidth();
	mapTileHeight = mapTileWidth / 2.0f;

	for (int r = fileH - 1; r >= 0; r--)
	{
		for (int c = 0; c < fileW; c++)
		{
			int tileId;
			arq >> tileId;
			cout << tileId << " ";
			// é colidível se existe ! após o número do tile
			bool collidable = false;
			bool breakable = false;
			if (arq.peek() == '!')
			{
				collidable = true;
				arq.get(); // descarta o caractere '!'
			}
			if (arq.peek() == 'H')
			{
				tmap->addObject(getHamburgerObject(), c, fileH - r - 1);
				arq.get(); // descarta o caractere 'H'
			}
			if (arq.peek() == 'A')
			{
				tmap->addObject(getOrbObject(c == 0 ? DIRECTION_WEST : DIRECTION_EAST), c, fileH - r - 1);
				arq.get(); // descarta o caractere 'A'
			}
			if (arq.peek() == 'L')
			{
				tmap->addObject(getLodgeObject(), c, fileH - r - 1);
				arq.get(); // descarta o caractere 'L'
			}
			if (arq.peek() == 'B')
			{				
				breakable = true;
				arq.get(); // descarta o caractere 'B'
			}
			tmap->setTile(c, fileH - r - 1, tileId, collidable, breakable);
		}
		cout << endl;
	}
	arq.close();
	return tmap;
}

int getDirectionFromKeys(bool up, bool left, bool down, bool right)
{
	if (up)
	{
		if (left)
			return DIRECTION_NORTHWEST;
		if (right)
			return DIRECTION_NORTHEAST;

		return DIRECTION_NORTH;
	}

	if (down)
	{
		if (left)
			return DIRECTION_SOUTHWEST;
		if (right)
			return DIRECTION_SOUTHEAST;

		return DIRECTION_SOUTH;
	}

	if (left)
		return DIRECTION_WEST;
	if (right)
		return DIRECTION_EAST;

	return 0;
}

void SRD2SRU(double &mx, double &my, float &x, float &y)
{
	x = xi + (mx / g_gl_width) * w;
	y = yi + (1 - (my / g_gl_height)) * h;
}

GLuint createShaderProgramme()
{
	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];
	parse_file_into_str("vertex_shader.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("fragment_shader.glsl", fragment_shader, 1024 * 256);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar *p = (const GLchar *)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	// check for compile errors
	int params = -1;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		print_shader_info_log(vs);
		return 1; // or exit or something
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar *)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);

	// check for compile errors
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		print_shader_info_log(fs);
		return 1; // or exit or something
	}

	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: could not link shader programme GL index %i\n",
				shader_programme);		
		return false;
	}

	return shader_programme;
}

TileMapWithVAO loadTileMap(const string mapFile, const string tileSetFile, int tileSetCols, int tileSetRows)
{
	float tileW2, tileH2;

	TileMapWithVAO result;
	result.tileMap = readMap(mapFile.c_str(), tileSetCols, tileSetRows);

	tw2 = mapTileHeight;
	th2 = mapTileHeight / 2.0f;
	tileW2 = result.tileMap->getTileW() / 2.0f;
	tileH2 = result.tileMap->getTileH() / 2.0f;

	GLuint textureId;
	loadTexture(textureId, tileSetFile);

	result.tileMap->setTid(textureId);

	float vertices[] = {
		// positions                           // texture coords
		xi,                yi + th2,           0.0f,                       tileH2,                     // left
		xi + tw2,          yi,                 tileW2,                     0.0f,                       // bottom
		xi + mapTileWidth, yi + th2,           result.tileMap->getTileW(), tileH2,                     // right
		xi + tw2,          yi + mapTileHeight, tileW2,                     result.tileMap->getTileH(), // top
	};

	unsigned int indices[] = {
		0, 1, 3, // first triangle
		3, 1, 2	 // second triangle
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

	result.vao = VAO;
	return result;
}

GameObject *getCharObject()
{
	float tileHeight = (1.0f / 3.0f); //  8 colunas no tileset de personagens
	float tileWidth = (1.0f / 8.0f);  // 12 linhas no tileset de personagens

	float objHeight = (16.0f * mapTileHeight) / 15.0f;
	float objWidth = (11.0f * mapTileWidth) / 15.0f;

	GLuint textureId;
	loadTexture(textureId, "./resources/character/char.png", GL_NEAREST);

	auto object = new GameObject(
		Objects::CHARACTER,
		objWidth,
		objHeight,
		tileWidth,
		tileHeight,
		textureId,
		CHAR
	);

	object->z = -0.2f;
	
	return object;
}

void renderTileMap(GLuint shader, unsigned int tileVAO, TileMap *tileMap, TilemapView *tileView)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tileMap->getTileSet());

	glBindVertexArray(tileVAO);
	float x, y;
	int r = 0, c = 0;
	for (int r = 0; r < tileMap->getHeight(); r++)
	{
		for (int c = 0; c < tileMap->getWidth(); c++)
		{
			int t_id = (int)tileMap->getTile(c, r).id;

			if (t_id == 255)
				continue;

			int u = t_id % tileMap->getTileSetCols();
			int v = t_id / tileMap->getTileSetCols();

			tileView->computeDrawPosition(c, r, mapTileWidth, mapTileHeight, x, y);

			glBindVertexArray(tileVAO);

			glUniform1f(glGetUniformLocation(shader, "offsetx"), u * tileMap->getTileW());
			glUniform1f(glGetUniformLocation(shader, "offsety"), v * tileMap->getTileH());
			glUniform1f(glGetUniformLocation(shader, "tx"), x);
			glUniform1f(glGetUniformLocation(shader, "ty"), y + 1.0);
			glUniform1f(glGetUniformLocation(shader, "layer_z"), tileMap->getZ());
			// glUniform1f(glGetUniformLocation(shader, "weight"), (c == cx) && (r == cy) ? 0.5 : 0.0);
			glUniform1f(glGetUniformLocation(shader, "weight"), 0);
			glUniform1i(glGetUniformLocation(shader, "sprite"), 0);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
	}
}

void drawObject(GLuint shader, GameObject *object)
{
	float x, y;
	glBindVertexArray(object->getVAO());
	if (object->row < 0 && object->column < 0)
	{
		x = 0.0f;
		y = 0.0f;
	}
	else
	{
		tview->computeDrawPosition(
			object->column,
			object->row,
			mapTileWidth,
			mapTileHeight,
			x,
			y);
				
		CenterObjectInTile(object, x, y);
	}

	float offsetX = object->u * object->getTileWidth();
	float offsetY = object->v * object->getTileHeight();

	glBindTexture(GL_TEXTURE_2D, object->getTid());
	glUniform1f(glGetUniformLocation(shader, "offsetx"), offsetX);
	glUniform1f(glGetUniformLocation(shader, "offsety"), offsetY);
	glUniform1f(glGetUniformLocation(shader, "tx"), x);
	glUniform1f(glGetUniformLocation(shader, "ty"), y);
	glUniform1f(glGetUniformLocation(shader, "layer_z"), object->z);
	glUniform1f(glGetUniformLocation(shader, "weight"), 0.0);
	glUniform1i(glGetUniformLocation(shader, "sprite"), 0);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void CenterObjectInTile(GameObject * object, float &x, float &y)
{
		if (object->getWidth() > mapTileWidth) 
		{
			x -= (object->getWidth()/2.0f) - tw2;
		}
		else 
		{
			x += (tw2 / 4.0f);
		}

		if (object->getHeight() > mapTileHeight) 
		{			
			y -= (object->getHeight()/4.0f) - th2;
		}
		else 
		{
			y += (th2 / 4.0f);
		}

		y += 1.0f;
}

GameObject *getFullscreenObject(const int id, const string filename)
{
	GLuint textureId;
	loadTexture(textureId, filename, GL_NEAREST);

	auto object = new GameObject(
		id,
		2.0f,
		2.0f,
		1.0f,
		1.0f,
		textureId,
		OBJECTIVE
	);

	object->z = -1.0f;

	return object;
}

GameObject *getGameOverObject()
{
	return getFullscreenObject(Objects::GAMEOVER, "./resources/gameover.png");
}

GameObject *getVictoryObject()
{
	return getFullscreenObject(Objects::VICTORY, "./resources/victory.png");
}

int main()
{
	restart_gl_log();
	// all the GLFW and GLEW start-up code is moved to here in gl_utils.cpp
	start_gl();
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS);

	GLuint shader_programme = createShaderProgramme();
	float previous = glfwGetTime();

	TileMapWithVAO forestTileMap = loadTileMap(
		"./maps/forest.tmap",
		"./resources/world/WorldTileset.png",
		6, 12);

	GameObject *charObject = getCharObject();
	charObject->row = 7;
	charObject->column = 0;
	charObject->u = 4;
	charObject->v = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	while (!glfwWindowShouldClose(g_window))
	{
		_update_fps_counter(g_window);
		double current_seconds = glfwGetTime();

		// wipe the drawing surface clear
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, g_gl_width, g_gl_height);

		glUseProgram(shader_programme);

		renderTileMap(shader_programme, forestTileMap.vao, forestTileMap.tileMap, tview);

		drawObject(shader_programme, charObject);
		for (GameObject *object : forestTileMap.tileMap->getObjects())
		{
			drawObject(shader_programme, object);
		}		

		glfwPollEvents();
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(g_window, 1);
		}

		if (gameOverObject)
		{
			drawObject(shader_programme, gameOverObject);
			glfwSwapBuffers(g_window);
			continue;
		}

		if (victoryObject)
		{
			drawObject(shader_programme, victoryObject);
			glfwSwapBuffers(g_window);
			continue;
		}

		const bool up = glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_UP) == GLFW_PRESS;
		const bool left = glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_LEFT) == GLFW_PRESS;
		const bool down = glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_DOWN) == GLFW_PRESS;
		const bool right = glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_RIGHT) == GLFW_PRESS;

		if ((current_seconds - previous) > 0.3)
		{
			previous = current_seconds;

			current_seconds = glfwGetTime();

			auto objects = forestTileMap.tileMap->getObjects();

			for (GameObject *object : objects)
			{
				auto movingObject = dynamic_cast<MovingGameObject *>(object);
				if (movingObject != nullptr)
				{
					int nextColumn = object->column;
					int nextRow = object->row;
					
					if (nextColumn < 0 || nextRow < 0 || nextColumn >= forestTileMap.tileMap->getWidth() || nextRow >= forestTileMap.tileMap->getHeight())
					{
						nextRow = 0;
					}
					else {
						tview->computeTileWalking(
							nextColumn,
							nextRow,
							movingObject->getDirection()
						);
					}

					object->column = nextColumn;
					object->row = nextRow;
				}
			}

			auto collidingObject = forestTileMap.tileMap->checkIsObjectColliding(charObject->column, charObject->row);

			if (collidingObject && collidingObject->getType() == AVOID)
			{
				cout << "Colidiu com a seta! Foi de spawn!" << endl;
				gameOverObject = getGameOverObject();
				glfwSwapBuffers(g_window);
				continue;
			}

			int direction = getDirectionFromKeys(up, left, down, right);

			if (direction > 0)
			{
				int nextColumn = charObject->column;
				int nextRow = charObject->row;

				tview->computeTileWalking(
					nextColumn,
					nextRow,
					direction);

				// Define a direção para a qual o personagem olha
				if (direction == DIRECTION_NORTH)
					charObject->u = 1;
				else if (direction == DIRECTION_NORTHEAST)
					charObject->u = 2;
				else if (direction == DIRECTION_EAST)
					charObject->u = 3;
				else if (direction == DIRECTION_SOUTHEAST)
					charObject->u = 4;
				else if (direction == DIRECTION_SOUTH)
					charObject->u = 5;
				else if (direction == DIRECTION_SOUTHWEST)
					charObject->u = 6;
				else if (direction == DIRECTION_WEST)
					charObject->u = 7;
				else if (direction == DIRECTION_NORTHWEST)
					charObject->u = 0;

				int nextTile = nextRow * 9 + nextColumn;

				auto nextTile2 = forestTileMap.tileMap->getTile(nextColumn, nextRow);

				auto outsideMapLimits = nextColumn < 0 || nextRow < 0 || nextColumn >= forestTileMap.tileMap->getWidth() || nextRow >= forestTileMap.tileMap->getHeight();				
				if (outsideMapLimits || forestTileMap.tileMap->isCollidable(nextColumn, nextRow))				
				{
					cout << "Colisão na tile: " << nextTile << endl;
					glfwSwapBuffers(g_window);
					continue;										
				}

				if (forestTileMap.tileMap->isBreakableObjectAt(charObject->column, charObject->row))
				{
					cout << "Colidiu com objeto quebrável na tile: " << nextTile << endl;
					auto currentTile = forestTileMap.tileMap->getTile(charObject->column, charObject->row);
					forestTileMap.tileMap->setTile(
						charObject->column, 
						charObject->row, 
						currentTile.id + 1, 
						false, //Colisão
						true, //Quebrável
						false   //Gameover
					);
				}

				if (forestTileMap.tileMap->isGameOverTile(nextColumn, nextRow))
				{
					cout << "Colidiu com tile de game over: " << nextTile << endl;
					gameOverObject = getGameOverObject();					
				}

				auto object = forestTileMap.tileMap->checkIsObjectColliding(nextColumn, nextRow);

				if (object)
				{
					cout << "Colisão com objeto na tile: " << nextTile << endl;

					if (object->getType() == OBJECTIVE)
					{
						if (object->isBlocked()) 
						{
							glfwSwapBuffers(g_window);
							continue;
						}


						if (object->getId() == Objects::HAMBURGER) 
						{
							gotHamburger = true;
							forestTileMap.tileMap->getObjectById(Objects::LODGE)->setBlocked(false);
							cout << "Pegou o hambúrguer! Cabana desbloqueada" << endl;
							forestTileMap.tileMap->removeObjectAt(nextColumn, nextRow);
						}
						else if (object->getId() == Objects::LODGE && gotHamburger)
						{
							cout << "Chegou na cabana com o hambúrguer! Você venceu!" << endl;
							victoryObject = getVictoryObject();
						}
					}
					else if (object->getType() == AVOID)
					{
						cout << "Colidiu com a seta! Foi de spawn!" << endl;
						gameOverObject = getGameOverObject();				
					}
				}

				// Se não teve colisão, alterna entre os frames de caminhada (v = 0 e v = 2) para animar o personagem
				if (charObject->v < 2)
					charObject->v = 2;
				else
					charObject->v = 0;

				charObject->column = nextColumn;
				charObject->row = nextRow;
			}
			else
			{
				charObject->v = 1;
			}

			collidingObject = forestTileMap.tileMap->checkIsObjectColliding(charObject->column, charObject->row);

			if (collidingObject && collidingObject->getType() == AVOID)
			{
				cout << "Colidiu com a seta! Foi de spawn!" << endl;
				gameOverObject = getGameOverObject();
				glfwSwapBuffers(g_window);		
			}
		}
		
		glfwSwapBuffers(g_window);
	}
	
	glfwTerminate();
	delete forestTileMap.tileMap;
	delete charObject;
	return 0;
}