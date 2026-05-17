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
#include "GameObject.h"
#include "ltMath.h"
#include <fstream>

using namespace std;

int g_gl_width = 640;
int g_gl_height = 480;
float xi = -1.0f;
float xf = 1.0f;
float yi = -1.0f;
float yf = 1.0f;
float w = xf - xi;
float h = yf - yi;
int tileSetCols = 9, tileSetRows = 9;
float mapTileWidth, mapTileHeight, tw2, th2;
int cx = 6, cy = 6;

struct TileMapWithVAO
{
	TileMap *tileMap;
	GLuint vao;
};

TilemapView *tview = new DiamondView();
TileMap *tmap = NULL;

GLFWwindow *g_window = NULL;

TileMap *readMap(const string filename, int tileSetCols, int tileSetRows)
{
	ifstream arq(filename.c_str());
	int w, h;
	arq >> w >> h;
	TileMap *tmap = new TileMap(w, h, 0, tileSetCols, tileSetRows);
	for (int r = 0; r < h; r++)
	{
		for (int c = 0; c < w; c++)
		{
			int tileId;
			arq >> tileId;
			cout << tileId << " ";
			// é colidível se existe ! após o número do tile
			bool collidable = false;
			if (arq.peek() == '!')
			{
				collidable = true;
				arq.get(); // descarta o caractere '!'
			}
			tmap->setTile(c, h - r - 1, tileId, collidable);
		}
		cout << endl;
	}
	arq.close();
	return tmap;
}

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

	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		if (nrChannels == 4)
		{
			cout << "Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			cout << "Without Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
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
	parse_file_into_str("_geral_vs.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("_geral_fs.glsl", fragment_shader, 1024 * 256);

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
		// 		print_programme_info_log( shader_programme );
		return false;
	}

	return shader_programme;
}

TileMapWithVAO loadTileMap(const string mapFile, const string tileSetFile, int tileSetCols, int tileSetRows)
{	
	float tileW2, tileH2;

	TileMapWithVAO result;
	result.tileMap = readMap(mapFile.c_str(), tileSetCols, tileSetRows);	
	
	mapTileWidth = w / (float)result.tileMap->getWidth();
	mapTileHeight = mapTileWidth / 2.0f;
	tw2 = mapTileHeight;
	th2 = mapTileHeight / 2.0f;
	tileW2 = result.tileMap->getTileW() / 2.0f;
	tileH2 = result.tileMap->getTileH() / 2.0f;

	GLuint textureId;
	loadTexture(textureId, tileSetFile);

	result.tileMap->setTid(textureId);

	float vertices[] = {
		// positions                           // texture coords
		xi,                yi + th2,           0.0f, tileH2,  // left
		xi + tw2,          yi,                 tileW2, 0.0f,  // bottom
		xi + mapTileWidth, yi + th2,           result.tileMap->getTileW(), tileH2, // right
		xi + tw2,          yi + mapTileHeight, tileW2, result.tileMap->getTileH(), // top
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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// discard color
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(2);

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

	// float width, float height, float tileWidth, float tileHeight, unsigned int tid
	return new GameObject(
		objWidth,
		objHeight,
		tileWidth,
		tileHeight,
		textureId);
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
			int t_id = (int)tileMap->getTile(c, r);

			if (t_id == 255) continue;

			int u = t_id % tileMap->getTileSetCols();
			int v = t_id / tileMap->getTileSetCols();

			tileView->computeDrawPosition(c, r, mapTileWidth, mapTileHeight, x, y);

			glBindVertexArray(tileVAO);

			glUniform1f(glGetUniformLocation(shader, "offsetx"), u * tileMap->getTileW());
			glUniform1f(glGetUniformLocation(shader, "offsety"), v * tileMap->getTileH());
			glUniform1f(glGetUniformLocation(shader, "tx"), x);
			glUniform1f(glGetUniformLocation(shader, "ty"), y + 1.0);
			glUniform1f(glGetUniformLocation(shader, "layer_z"), tileMap->getZ());
			glUniform1f(glGetUniformLocation(shader, "weight"), (c == cx) && (r == cy) ? 0.5 : 0.0);
			glUniform1i(glGetUniformLocation(shader, "sprite"), 0);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
	}
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

	// TileMapWithVAO tileMap = loadTileMap(
	// 	"./maps/terrain1.tmap",
	// 	"./resources/world/terrain.png",
	// 	9, 9
	// );

	//TODO: Unificar texturas em um arquivo
	TileMapWithVAO forestTileMap = loadTileMap(
		"./maps/forest.tmap", 
		"./resources/world/Forest.png",
		3, 6
	);

	TileMapWithVAO roadTileMap = loadTileMap(
		"./maps/road.tmap", 
		"./resources/world/Terrain1.png",
		3, 6
	);

	GameObject *charObject = getCharObject();
	charObject->row = 6;
	charObject->column = 6;
	charObject->u = 4;
	charObject->v = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, charObject->getTid());

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
		renderTileMap(shader_programme, roadTileMap.vao, roadTileMap.tileMap, tview);

		float x, y;
		glBindVertexArray(charObject->getVAO());
		tview->computeDrawPosition(
			charObject->column,
			charObject->row,
			mapTileWidth,
			mapTileHeight,
			x,
			y);

		float offsetX = charObject->u * charObject->getTileWidth();
		float offsetY = charObject->v * charObject->getTileHeight();

		glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), offsetX);
		glUniform1f(glGetUniformLocation(shader_programme, "offsety"), offsetY);
		glUniform1f(glGetUniformLocation(shader_programme, "tx"), x + (tw2 / 3.0f));
		glUniform1f(glGetUniformLocation(shader_programme, "ty"), y + 1.0);
		glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), charObject->z);
		glUniform1f(glGetUniformLocation(shader_programme, "weight"), 0.0);
		glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 1);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwPollEvents();
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(g_window, 1);
		}

		const bool up = glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_UP) == GLFW_PRESS;
		const bool left = glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_LEFT) == GLFW_PRESS;
		const bool down = glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_DOWN) == GLFW_PRESS;
		const bool right = glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_RIGHT) == GLFW_PRESS;

		if ((current_seconds - previous) > 0.3)
		{
			previous = current_seconds;

			current_seconds = glfwGetTime();
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
					charObject->u = 0;
				else if (direction == DIRECTION_NORTHEAST)
					charObject->u = 1;
				else if (direction == DIRECTION_EAST)
					charObject->u = 2;
				else if (direction == DIRECTION_SOUTHEAST)
					charObject->u = 3;
				else if (direction == DIRECTION_SOUTH)
					charObject->u = 4;
				else if (direction == DIRECTION_SOUTHWEST)
					charObject->u = 5;
				else if (direction == DIRECTION_WEST)
					charObject->u = 6;
				else if (direction == DIRECTION_NORTHWEST)
					charObject->u = 7;

				int nextTile = nextRow * 9 + nextColumn;

				// auto teste = find(colission.begin(), colission.end(), nextTile);

				// Se valor encontrado, teve colisão
				// if (tmap->isCollidable(nextColumn, nextRow))
				// {
				// 	cout << "Colisão na tile: " << nextTile << endl;
				// 	glfwSwapBuffers(g_window);
				// 	continue;
				// }

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
		}

		// put the stuff we've been drawing onto the display
		glfwSwapBuffers(g_window);
	}

	// close GL context and any other GLFW resources
	glfwTerminate();
	delete tmap;
	return 0;
}
