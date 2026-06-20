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

// Identificadores usados para diferenciar os objetos criados no jogo.
enum Objects
{
	CHARACTER = 0,
	HAMBURGER = 1,
	ORB = 2,
	LODGE = 3,
	BREAKABLE = 4,
	GAMEOVER = 5,
	VICTORY = 6
};

// Agrupa o mapa carregado com o VAO usado para desenhar cada tile no OpenGL.
struct TileMapWithVAO
{
	TileMap *tileMap;
	GLuint vao;
};

// Dimensões da janela OpenGL.
int g_gl_width = 980;
int g_gl_height = 780;

// Limites do sistema de coordenadas usado para renderizar a cena.
float xi = -1.0f;
float xf = 1.0f;
float yi = -1.0f;
float yf = 1.0f;
float w = xf - xi;
float h = yf - yi;

// Informações globais do tileset e do tamanho calculado dos tiles no mapa.
int tileSetCols = 9, tileSetRows = 9;
float mapTileWidth, mapTileHeight, tw2, th2;
int cx = 6, cy = 6;

// Estado geral da partida.
bool gotHamburger = false;
TilemapView *tview = new DiamondView();
GLFWwindow *g_window = NULL;
GameObject *gameOverObject = nullptr;
GameObject *victoryObject = nullptr;

// Carrega uma imagem do disco e cria uma textura OpenGL configurada para uso nos sprites/tiles.
// O parâmetro "param" define se a textura será filtrada suavemente ou no estilo pixel art.
void loadTexture(unsigned int &texture, const string filename, GLint param = GL_LINEAR)
{
	// Cria e seleciona uma textura 2D para receber os dados da imagem.
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Escolhe o tipo de mipmap conforme o filtro principal solicitado.
	GLint mipmap = param == GL_LINEAR
		? GL_LINEAR_MIPMAP_LINEAR
		: GL_NEAREST_MIPMAP_LINEAR;

	// Configura repetição e filtros de amostragem da textura.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap);

	// Usa o maior nível disponível de anisotropia para melhorar a qualidade da textura.
	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;

	// Força a imagem a ser carregada com canal alfa para padronizar o formato RGBA.
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
	if (data)
	{
		// Envia os pixels para a GPU e gera os mipmaps usados à distância.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}

// Cria o objeto coletável do hambúrguer, que desbloqueia a cabana quando é pego.
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

// Cria um orbe inimigo móvel. A direção recebida define para onde ele anda no mapa.
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

// Cria a cabana, que começa bloqueada e só pode ser acessada depois do hambúrguer.
GameObject *getLodgeObject()
{
	GLuint textureId;
	loadTexture(textureId, "./resources/lodge.png", GL_NEAREST);

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

// Lê o arquivo .tmap e monta a estrutura TileMap com tiles, colisões e objetos especiais.
// Marcadores após o número do tile:
// ! = colisão, H = hambúrguer, A = orbe, L = cabana, V = vitória, B = quebrável.
TileMap *readMap(const string filename, int tileSetCols, int tileSetRows)
{
	ifstream arq(filename.c_str());
	int fileW, fileH;
	// As duas primeiras informações do arquivo representam largura e altura do mapa.
	arq >> fileW >> fileH;
	TileMap *tmap = new TileMap(fileW, fileH, 0, tileSetCols, tileSetRows);

	// Calcula o tamanho visual de cada tile no sistema de coordenadas normalizado.
	mapTileWidth = w / (float)tmap->getWidth();
	mapTileHeight = mapTileWidth / 2.0f;

	// O arquivo é lido de baixo para cima para combinar a origem do mapa com a renderização isométrica.
	for (int r = fileH - 1; r >= 0; r--)
	{
		for (int c = 0; c < fileW; c++)
		{
			int tileId;
			arq >> tileId;
			// Cada marcador opcional altera o tile ou adiciona um objeto naquela posição.
			bool collidable = false;
			bool breakable = false;
			bool victory = false;
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
				// Orbes nas extremidades recebem direção inicial baseada na coluna.
				tmap->addObject(getOrbObject(c == 0 ? DIRECTION_WEST : DIRECTION_EAST), c, fileH - r - 1);
				arq.get(); // descarta o caractere 'A'
			}
			if (arq.peek() == 'L')
			{
				tmap->addObject(getLodgeObject(), c, fileH - r - 1);
				arq.get(); // descarta o caractere 'L'
			}
			if (arq.peek() == 'V')
			{
				victory = true;
				arq.get(); // descarta o caractere 'V'
			}
			if (arq.peek() == 'B')
			{
				breakable = true;
				arq.get(); // descarta o caractere 'B'
			}
			tmap->setTile(c, fileH - r - 1, tileId, collidable, breakable, false, victory);
		}
	}
	arq.close();
	return tmap;
}

// Converte o estado das teclas pressionadas em uma das direções de movimento do jogo.
// Combinações diagonais têm prioridade quando uma tecla vertical e uma horizontal estão pressionadas.
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

// Converte coordenadas do sistema de referência de dispositivo (mouse/janela) para o sistema usado na cena.
void SRD2SRU(double &mx, double &my, float &x, float &y)
{
	x = xi + (mx / g_gl_width) * w;
	y = yi + (1 - (my / g_gl_height)) * h;
}

// Lê, compila e linka os shaders GLSL, retornando o programa OpenGL pronto para renderização.
GLuint createShaderProgramme()
{
	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];
	// Carrega o código-fonte dos shaders de vértice e fragmento.
	parse_file_into_str("vertex_shader.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("fragment_shader.glsl", fragment_shader, 1024 * 256);

	// Compila o vertex shader, responsável por posicionar os vértices na tela.
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar *p = (const GLchar *)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	// Verifica erros de compilação do vertex shader.
	int params = -1;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		print_shader_info_log(vs);
		return 1;
	}

	// Compila o fragment shader, responsável pela cor/textura de cada fragmento desenhado.
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar *)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);
	
	// Verifica erros de compilação do fragment shader.
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		print_shader_info_log(fs);
		return 1;
	}

	// Linka os dois shaders em um único programa executado pela GPU.
	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: could not link shader programme GL index %i\n",
				shader_programme);
		return 1;
	}

	return shader_programme;
}

// Carrega o mapa e seu tileset, além de configurar os buffers OpenGL para desenhar um tile isométrico.
TileMapWithVAO loadTileMap(const string mapFile, const string tileSetFile, int tileSetCols, int tileSetRows)
{
	float tileW2, tileH2;

	TileMapWithVAO result;
	result.tileMap = readMap(mapFile.c_str(), tileSetCols, tileSetRows);

	// Metades usadas no cálculo do losango isométrico e no alinhamento de objetos.
	tw2 = mapTileHeight;
	th2 = mapTileHeight / 2.0f;
	tileW2 = result.tileMap->getTileW() / 2.0f;
	tileH2 = result.tileMap->getTileH() / 2.0f;

	GLuint textureId;
	loadTexture(textureId, tileSetFile);

	result.tileMap->setTid(textureId);

	// Define um losango formado por quatro vértices, com coordenadas de posição e textura.
	float vertices[] = {
		// positions                           // texture coords
		xi,                yi + th2,           0.0f,                       tileH2,                     // left
		xi + tw2,          yi,                 tileW2,                     0.0f,                       // bottom
		xi + mapTileWidth, yi + th2,           result.tileMap->getTileW(), tileH2,                     // right
		xi + tw2,          yi + mapTileHeight, tileW2,                     result.tileMap->getTileH(), // top
	};

	// Dois triângulos formam o losango completo do tile.
	unsigned int indices[] = {
		0, 1, 3, // primeiro triângulo
		3, 1, 2	 // segundo triângulo
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

	// Atributo 0: posição 2D do vértice.
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	// Atributo 1: coordenada 2D da textura.
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

	result.vao = VAO;
	return result;
}

// Cria o personagem principal com dimensões proporcionais ao tile e textura de spritesheet.
GameObject *getCharObject()
{
	float tileHeight = (1.0f / 3.0f); // Altura de cada frame no spritesheet do personagem.
	float tileWidth = (1.0f / 8.0f);  // Largura de cada frame no spritesheet do personagem.

	// Ajusta o tamanho do personagem para caber visualmente sobre o tile isométrico.
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

// Desenha todos os tiles visíveis do mapa usando o VAO compartilhado e deslocando a textura pelo tile id.
void renderTileMap(GLuint shader, unsigned int tileVAO, TileMap *tileMap, TilemapView *tileView)
{
	// O tileset fica na unidade de textura 0 para ser amostrado pelo shader.
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

			// O id 255 representa ausência de tile desenhável.
			if (t_id == 255)
				continue;

			// Calcula a coluna (u) e a linha (v) do tile dentro do tileset.
			int u = t_id % tileMap->getTileSetCols();
			int v = t_id / tileMap->getTileSetCols();

			// Converte a posição lógica do mapa para posição de desenho isométrica.
			tileView->computeDrawPosition(c, r, mapTileWidth, mapTileHeight, x, y);

			glBindVertexArray(tileVAO);

			// Envia ao shader o recorte da textura e a posição onde o tile será desenhado.
			glUniform1f(glGetUniformLocation(shader, "offsetx"), u * tileMap->getTileW());
			glUniform1f(glGetUniformLocation(shader, "offsety"), v * tileMap->getTileH());
			glUniform1f(glGetUniformLocation(shader, "tx"), x);
			glUniform1f(glGetUniformLocation(shader, "ty"), y + 1.0);
			glUniform1f(glGetUniformLocation(shader, "layer_z"), tileMap->getZ());
			glUniform1f(glGetUniformLocation(shader, "weight"), 0);
			glUniform1i(glGetUniformLocation(shader, "sprite"), 0);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
	}
}

// Desenha um GameObject na tela, calculando sua posição no mapa e o frame correto da textura.
void drawObject(GLuint shader, GameObject *object)
{
	float x, y;
	glBindVertexArray(object->getVAO());
	// Objetos com linha/coluna negativas são desenhados como tela cheia ou overlay fixo.
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

		// Ajusta o sprite para ficar centralizado sobre o losango do tile.
		CenterObjectInTile(object, x, y);
	}

	// u e v escolhem o frame dentro do spritesheet do objeto.
	float offsetX = object->u * object->getTileWidth();
	float offsetY = object->v * object->getTileHeight();

	// Envia textura, deslocamento e posição ao shader antes de desenhar o quad do objeto.
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

// Corrige a posição de desenho para centralizar objetos de tamanhos diferentes sobre um tile.
void CenterObjectInTile(GameObject * object, float &x, float &y)
{
		// Objetos mais largos que o tile precisam ser deslocados para a esquerda.
		if (object->getWidth() > mapTileWidth)
		{
			x -= (object->getWidth()/2.0f) - tw2;
		}
		else 
		{
			x += (tw2 / 4.0f);
		}

		// Objetos mais altos que o tile precisam subir para manter a base alinhada.
		if (object->getHeight() > mapTileHeight) 
		{			
			y -= (object->getHeight()/4.0f) - th2;
		}
		else 
		{
			y += (th2 / 4.0f);
		}

		// Compensa a translação vertical usada no mapa isométrico.
		y += 1.0f;
}

// Cria um objeto que cobre a tela inteira, usado nas telas de game over e vitória.
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

// Retorna o overlay exibido quando o jogador perde.
GameObject *getGameOverObject()
{
	return getFullscreenObject(Objects::GAMEOVER, "./resources/gameover.png");
}

// Retorna o overlay exibido quando o jogador vence.
GameObject *getVictoryObject()
{
	return getFullscreenObject(Objects::VICTORY, "./resources/victory.png");
}

int main()
{
	// Inicializa log, janela e contexto OpenGL.
	restart_gl_log();	
	start_gl();	
	// Ativa teste de profundidade para controlar a sobreposição entre tiles e objetos.
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	GLuint shader_programme = createShaderProgramme();
	// Marca o último instante em que a lógica do jogo foi atualizada.
	float previous = glfwGetTime();

	// Carrega o mapa principal e o tileset do mundo.
	TileMapWithVAO tileMap = loadTileMap(
		"./maps/tileMapConfig.tmap",
		"./resources/world/WorldTileset.png",
		6, 12);

	// Cria o personagem e posiciona seu spawn inicial no mapa.
	GameObject *charObject = getCharObject();
	charObject->row = 7;
	charObject->column = 0;
	charObject->u = 4;
	charObject->v = 0;

	// Ativa transparência para sprites PNG com canal alfa.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Loop principal: renderiza a cena, lê entrada e atualiza a lógica até a janela fechar.
	while (!glfwWindowShouldClose(g_window))
	{
		_update_fps_counter(g_window);
		double current_seconds = glfwGetTime();

		// Limpa o frame anterior antes de desenhar o novo estado do jogo.
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, g_gl_width, g_gl_height);

		glUseProgram(shader_programme);

		// Desenha primeiro o mapa e depois os objetos, respeitando a profundidade definida em z.
		renderTileMap(shader_programme, tileMap.vao, tileMap.tileMap, tview);

		drawObject(shader_programme, charObject);
		for (GameObject *object : tileMap.tileMap->getObjects())
		{
			drawObject(shader_programme, object);
		}		

		glfwPollEvents();
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(g_window, 1);
		}

		// Se a partida terminou, apenas desenha o overlay correspondente e congela a lógica.
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

		// Captura entradas equivalentes tanto em WASD quanto nas setas do teclado.
		const bool up = glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_UP) == GLFW_PRESS;
		const bool left = glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_LEFT) == GLFW_PRESS;
		const bool down = glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_DOWN) == GLFW_PRESS;
		const bool right = glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(g_window, GLFW_KEY_RIGHT) == GLFW_PRESS;

		// A lógica do jogo roda a cada 0.3 segundo para controlar velocidade e animação.
		if ((current_seconds - previous) > 0.3)
		{
			previous = current_seconds;

			current_seconds = glfwGetTime();

			auto objects = tileMap.tileMap->getObjects();

			// Atualiza todos os objetos móveis, como os orbes inimigos.
			for (GameObject *object : objects)
			{
				auto movingObject = dynamic_cast<MovingGameObject *>(object);
				if (movingObject != nullptr)
				{
					int nextColumn = object->column;
					int nextRow = object->row;
					
					// Caso o objeto saia do mapa, reposiciona sua linha para evitar acesso inválido.
					if (nextRow < 0 || nextRow >= (tileMap.tileMap->getHeight() - 1))
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

			auto collidingObject = tileMap.tileMap->checkIsObjectColliding(charObject->column, charObject->row);

			// Verifica se um inimigo móvel entrou na posição atual do jogador.
			if (collidingObject && collidingObject->getType() == AVOID)
			{
				cout << "Colidiu com a seta! Foi de spawn!" << endl;
				gameOverObject = getGameOverObject();
				glfwSwapBuffers(g_window);
				continue;
			}

			int direction = getDirectionFromKeys(up, left, down, right);

			// Só tenta mover o personagem quando alguma direção foi pressionada.
			if (direction > 0)
			{
				int nextColumn = charObject->column;
				int nextRow = charObject->row;

				tview->computeTileWalking(
					nextColumn,
					nextRow,
					direction);

				// Define a direção para a qual o personagem olha, escolhendo a coluna do spritesheet.
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

				auto nextTile2 = tileMap.tileMap->getTile(nextColumn, nextRow);

				// Bloqueia movimento para fora do mapa ou para tiles marcados como colidíveis.
				auto outsideMapLimits = nextColumn < 0 || nextRow < 0 || nextColumn >= tileMap.tileMap->getWidth() || nextRow >= tileMap.tileMap->getHeight();				
				if (outsideMapLimits || tileMap.tileMap->isCollidable(nextColumn, nextRow))				
				{
					cout << "Colisão na tile: " << nextTile << endl;
					glfwSwapBuffers(g_window);
					continue;										
				}

				// Tiles quebráveis mudam de id depois que o personagem passa por eles.
				if (tileMap.tileMap->isBreakableObjectAt(charObject->column, charObject->row))
				{
					cout << "Colidiu com objeto quebrável na tile: " << nextTile << endl;
					auto currentTile = tileMap.tileMap->getTile(charObject->column, charObject->row);
					tileMap.tileMap->setTile(
						charObject->column, 
						charObject->row, 
						currentTile.id + 1, 
						false, //Colisão
						true,  //Quebrável
						true, //Gameover
						false  //Vitória
					);
				}

				// Alguns tiles encerram a partida imediatamente com derrota.
				if (tileMap.tileMap->isGameOverTile(nextColumn, nextRow))
				{
					cout << "Colidiu com tile de game over: " << nextTile << endl;
					gameOverObject = getGameOverObject();					
				}

				// A vitória por tile só vale depois que o jogador coletou o hambúrguer.
				if (tileMap.tileMap->isVictoryTile(nextColumn, nextRow))
				{
					cout << "Colidiu com tile de vitória: " << nextTile << endl;
					
					if (gotHamburger)
					{
						victoryObject = getVictoryObject();						
					}
					
					glfwSwapBuffers(g_window);
					continue;
				}

				auto object = tileMap.tileMap->checkIsObjectColliding(nextColumn, nextRow);

				// Trata colisões com objetos colocados sobre tiles: objetivos, bloqueios e inimigos.
				if (object)
				{
					cout << "Colisão com objeto na tile: " << nextTile << endl;

					if (object->getType() == OBJECTIVE)
					{
						if (object->isBlocked()) 
						{
							// Objetos bloqueados, como a cabana antes do hambúrguer, impedem o avanço.
							glfwSwapBuffers(g_window);
							continue;
						}

						if (object->getId() == Objects::HAMBURGER) 
						{
							// Ao coletar o hambúrguer, a cabana deixa de bloquear a entrada.
							gotHamburger = true;
							tileMap.tileMap->getObjectById(Objects::LODGE)->setBlocked(false);
							cout << "Pegou o hambúrguer! Cabana desbloqueada" << endl;
							tileMap.tileMap->removeObjectAt(nextColumn, nextRow);
						}
						else if (object->getId() == Objects::LODGE && gotHamburger)
						{
							// Chegar à cabana com o hambúrguer completa o objetivo principal.
							cout << "Chegou na cabana com o hambúrguer! Você venceu!" << endl;
							victoryObject = getVictoryObject();
						}
					}
					else if (object->getType() == AVOID)
					{
						cout << "Colidiu com o orbe!" << endl;
						gameOverObject = getGameOverObject();
					}
				}

				// Se não teve colisão, alterna entre os frames de caminhada para animar o personagem.
				if (charObject->v < 2)
					charObject->v = 2;
				else
					charObject->v = 0;

				// Confirma o movimento calculado.
				charObject->column = nextColumn;
				charObject->row = nextRow;
			}
			else
			{
				// Quando parado, usa o frame central da animação.
				charObject->v = 1;
			}

			collidingObject = tileMap.tileMap->checkIsObjectColliding(charObject->column, charObject->row);

			// Segunda checagem de colisão cobre o caso em que um objeto móvel alcança o jogador após a atualização.
			if (collidingObject && collidingObject->getType() == AVOID)
			{
				cout << "Colidiu com a seta! Foi de spawn!" << endl;
				gameOverObject = getGameOverObject();
				glfwSwapBuffers(g_window);		
			}
		}
		
		// Exibe o frame renderizado na janela.
		glfwSwapBuffers(g_window);
	}
	
	// Libera recursos principais antes de encerrar o processo.
	glfwTerminate();
	delete tileMap.tileMap;
	delete charObject;
	return 0;
}