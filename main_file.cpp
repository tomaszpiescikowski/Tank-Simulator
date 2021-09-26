#define GLM_FORCE_RADIANS

#include <GL/glew.h>	//glew - biblioteka do OpenGL'a
#include <GLFW/glfw3.h> //glfw - pomocna biblioteka, wyswietlanie okienka GUI
#include <glm/glm.hpp>  //glm - matematyka do naszych programów
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

//standardowe biblioteki do C++
#include <stdio.h>	
#include <stdlib.h>
#include <iostream>
#include <random>

//pliki naglowkowe zalaczone do projektu - dodatki
#include "constants.h"	//zawiera zdefiniowane stale, np PI
#include "allmodels.h" //kod rysujacy przykladowe modele
#include "lodepng.h"	//sluzy do wczytywania tekstur
#include "shaderprogram.h"	//modul, ktory realizuje programy cieniujace
#include "myCube.h"	//kostka uzywana do czasteczek

#include <assimp/Importer.hpp>	//obiekt ktory wczytuje obiket z modelami 3d
#include <assimp/scene.h>		//interpretuje cala zawartosc pliku
#include <assimp/postprocess.h>;




struct Particle {
	glm::vec3 position; //Położenie cząstki
	glm::vec3 speed; //Prędkość cząstki
	float ttl; //Czas życia
};

//-----------------------------PREDEFINICJE FUNKCJI--------------------------------------------------------------

//Procedura obslugi bledow
void error_callback(int error, const char* description);

//Procedura obslugi klawiszy
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

//Obsługa myszy
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

//Naciskanie klawiszy myszy.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

//Obsługa kółka myszy
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

//Procedura obslugi rozszerzalnosci okna
void windowResizeCallback(GLFWwindow* window, int width, int height);

//Wczytywanie tekstury
GLuint readTexture(const char* filename);

//Cubemap
unsigned int loadCubemap(std::vector<std::string> faces);

//Leny funkcja
glm::vec3 calcDir(float kat_x, float kat_y);

//Ladowanie modelu
void loadModel(std::string plik);

//Funkcje do czasteczek
void createParticle(Particle& p);
void initializeSystem(Particle* system, int n);
void processSystem(Particle* system, glm::vec3 gravity, int n, float timestep);

//Oteksturowany kwadrat
void texCube(glm::mat4 P, glm::mat4 V, glm::mat4 M, GLuint tekstura);


//Wczytywanie obiektów
void drawTank(glm::mat4 P, glm::mat4 V, float angle_x, float angle_y, float angle_x_turret, float angle_y_turret, float Langle_obrot);
void drawBase(glm::mat4 P, glm::mat4 V, float angle_x, float angle_y, float angle_x_turret, float angle_y_turret, float Langle_obrot);
void drawWheelsRight(glm::mat4 Base, float angle_x, float angle_y);
void drawWheelsLeft(glm::mat4 Base, float angle_x, float angle_y);
void drawPads(glm::mat4 Base);
void drawTurret(glm::mat4 Base, float angle_x_turret, float angle_y_turret);
void drawGun(glm::mat4 Turret, float angle_y_turret);
void cleanTank();



class Model3D
{
public:
	Model3D() {};
	~Model3D() {};
	std::vector<glm::vec4> verts;
	std::vector<glm::vec4> norms;
	std::vector<glm::vec2> texCoords;
	std::vector<unsigned int> indices;
};


//---------------ZMIENNE GLOBALNE------------------------------------------------------------------------------


std::default_random_engine generator;
std::normal_distribution<double> distribution(0, 200);	//Rozkład normalny cząsteczek - średnia i odchylenie standardowe

bool firstLoop = true;
bool stopParticles = false; //Naprawia problem przy zatrzymywaniu czasteczek
bool shot = false;	//Czy wykonano strzał
const int n = 1000; //Liczba cząstek
Particle particles[n]; //Tablica cząstek
glm::vec3 gravity = glm::vec3(0, -1, 0); //Wektor grawitacji


//Obroty górą czołgu i lufą.
float speed_x_turret = 0;	//[radiany/s]
float speed_y_turret = 0;	//[radiany/s]

//Obroty ogóln Tomka, potem do kamery.
float speed_x = 0;	
float speed_y = 0;	

//Poruszanie się czołgu i jego obroty wokół osi Y.
float Lena_speed = 0, Lena_speed2 = 0;
float Lspeed_obrot = 0, Lspeed_obrot2 = 0;

//Okno.
float aspectRatio = 1;	
int numberofmeshes = 0;

//Jednostka teksturująca.
GLuint tex0;
GLuint tex1;
GLuint tex2;
GLuint tex3;
GLuint tex4;
GLuint tex5;
GLuint tex6;
GLuint tex7;

GLuint buda;
GLuint caterpillars;
GLuint czarny;
GLuint kola;
Model3D* models;

//Kamera
glm::vec3 cameraPos = glm::vec3(0.0f, 6.0f, 30.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

//Kamera myszą
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 800.0f / 2.0; //Środek ekranu
float lastY = 600.0 / 2.0;  //Środek ekranu
float fov = 45.0f;			//Do zooma, do macierzy P

//Kamera od typa
float speed_x_typa = 0;
float speed_y_typa = 0;	
float walk_speed = 0;
glm::vec3 pos = glm::vec3(0, 2, -11);

bool leftRotate = false;
bool rightRotate = false;




//Procedura inicjująca. Tutaj umieszczamy kod, ktory nalezy wykonac raz, na poczatku programu.
void initOpenGLProgram(GLFWwindow* window) 
{
	initShaders();
	glClearColor(0.8, 0.8, 0.8, 1); //Ustawianie koloru jakim bedzie czyszczone okno
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetKeyCallback(window, keyCallback);					 //Funkcja zwrotna do klawiatury.
	glfwSetMouseButtonCallback(window, mouseButtonCallback);	 //Funkcja zwrotna do przycisków myszy.
	glfwSetCursorPosCallback(window, mouse_callback);			 //Funkcja zwrotna do myszy.
	glfwSetScrollCallback(window, scroll_callback);				 //Funkcja zwrotna do kółka myszy.
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //Znika kursor myszy. (HIDDEN/DISABLED/NORMAL)

	initializeSystem(particles, n);

	glActiveTexture(GL_TEXTURE0);
	tex0 = readTexture("bottom.png");
	glActiveTexture(GL_TEXTURE1);
	tex1 = readTexture("1.png");
	glActiveTexture(GL_TEXTURE2);
	tex2 = readTexture("2.png");
	glActiveTexture(GL_TEXTURE3);
	tex3 = readTexture("3.png");
	glActiveTexture(GL_TEXTURE4);
	tex4 = readTexture("4.png");
	glActiveTexture(GL_TEXTURE5);
	tex5 = readTexture("top.png");
	glActiveTexture(GL_TEXTURE5);
	tex6 = readTexture("stone-wall.png");
	glActiveTexture(GL_TEXTURE5);
	tex7 = readTexture("bricks.png");

	buda = readTexture("buda.png");
	caterpillars = readTexture("caterpillars.png");
	czarny = readTexture("czarny.png");
	kola = readTexture("kola.png");


	loadModel(std::string("m2.obj"));
}


//Zwolnienie zasobow zajetych przez program 
void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();
	glDeleteTextures(1, &tex0);
	glDeleteTextures(1, &tex1);
	glDeleteTextures(1, &tex2);
	glDeleteTextures(1, &tex3);
	glDeleteTextures(1, &tex4);
	glDeleteTextures(1, &tex5);
	glDeleteTextures(1, &tex6);
	glDeleteTextures(1, &tex7);
	//Tutaj umieszaczaj kod, ktory nalezy wykonac po zakonczeniu petli glownej
	delete models;
}





//--------Części modelu------------------------------------------------------------------------------------------------------------------------------------------





void drawTank(glm::mat4 P, glm::mat4 V, /*float angle_x, float angle_y, */  float Lena_angle, float Lena_angle2, float angle_x_turret, float angle_y_turret, float Langle_obrot) {

	spLambertTextured->use();	//Aktywacja programu cieniujacego
	glUniformMatrix4fv(spLambertTextured->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(spLambertTextured->u("V"), 1, false, glm::value_ptr(V));
	glEnableVertexAttribArray(spLambert->a("vertex"));	//Wlaczenie uzywania danego atrybutu
	glEnableVertexAttribArray(spLambertTextured->a("texCoord"));	//Wlaczenie uzywania danego atrybutu
	glEnableVertexAttribArray(spLambertTextured->a("normal"));	//Wlaczenie uzywania danego atrybutu

	drawBase(P, V, /*angle_x, angle_y, */ Lena_angle, Lena_angle2, angle_x_turret, angle_y_turret, Langle_obrot);
}





void drawBase(glm::mat4 P, glm::mat4 V, /*float angle_x, float angle_y, */ float Lena_angle, float Lena_angle2, float angle_x_turret, float angle_y_turret, float Langle_obrot) {

	glm::mat4 Base = glm::mat4(1.0f); //Zainicjuj macierz modelu macierzą jednostkową
	//Base = glm::rotate(Base, angle_y, glm::vec3(0.0f, 1.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi Y o angle_Y stopni
	///Base = glm::rotate(Base, angle_x, glm::vec3(1.0f, 0.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi X o angle_X stopni

	Base = glm::rotate(Base, Langle_obrot, glm::vec3(0.0f, 1.0f, 0.0f));		//Obrót obiektu
	Base = glm::translate(Base, glm::vec3(Lena_angle, 0.0f, Lena_angle2));		//Przesunięcie obiektu.
	Base = glm::scale(Base, glm::vec3(0.2f, 0.2f, 0.2f));		//Obrót obiektu

	/*if (leftRotate == true || rightRotate == true) {
		Base = glm::translate(Base, glm::vec3(Lena_angle, -1.3f, Lena_angle2));		//Przesunięcie obiektu.
		Base = glm::rotate(Base, Langle_obrot, glm::vec3(0.0f, 1.0f, 0.0f));		//Obrót obiektu

	}

	else
	{
		Base = glm::rotate(Base, Langle_obrot, glm::vec3(0.0f, 1.0f, 0.0f));		//Obrót obiektu
		Base = glm::translate(Base, glm::vec3(Lena_angle, -1.3f, Lena_angle2));		//Przesunięcie obiektu.
	}*/

	//Base = glm::rotate(Base, Lena_angle, glm::vec3(1.0f, 0.0f, 0.0f));

			glActiveTexture(GL_TEXTURE0); //Assign texture tex0 to the 0-th texturing unit
			glBindTexture(GL_TEXTURE_2D, buda);


	//Cieniowanie obiektu.
	glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(Base));
	glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[15].verts.data());
	glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[15].texCoords.data());
	glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[15].norms.data());

	glDrawElements(GL_TRIANGLES, models[15].indices.size(), GL_UNSIGNED_INT, models[15].indices.data());


			glActiveTexture(GL_TEXTURE0); //Assign texture tex0 to the 0-th texturing unit
			glBindTexture(GL_TEXTURE_2D, kola);
	drawWheelsRight(Base, Lena_angle, Lena_angle2);
	drawWheelsLeft(Base, Lena_angle, Lena_angle2);


			glActiveTexture(GL_TEXTURE0); //Assign texture tex0 to the 0-th texturing unit
			glBindTexture(GL_TEXTURE_2D, caterpillars);
	drawPads(Base);

			glActiveTexture(GL_TEXTURE0); //Assign texture tex0 to the 0-th texturing unit
			glBindTexture(GL_TEXTURE_2D, buda);
	drawTurret(Base, angle_x_turret, angle_y_turret);
}




void drawWheelsRight(glm::mat4 Base, float angle_x, float angle_y) {
	glm::mat4 WheelsRight = Base; //Zainicjuj macierz modelu macierzą jednostkową
	//WheelsRight = glm::rotate(WheelsRight, angle_y, glm::vec3(1.0f, 0.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi Y o angle_Y stopni
	//WheelsRight = glm::rotate(WheelsRight, angle_x, glm::vec3(0.0f, 1.0f, .0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi X o angle_X stopni


	for (int i = 0; i < 23; i++)
	{
		if (i == 0 || i == 16 || i == 17 || i == 18 || i == 19 || i == 20 || i == 21 || i == 22)	//indeksy meshow z kolami
		{
			glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(WheelsRight));
			glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[i].verts.data());
			glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[i].texCoords.data());
			glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[i].norms.data());
			glDrawElements(GL_TRIANGLES, models[i].indices.size(), GL_UNSIGNED_INT, models[i].indices.data());
		}
	}
}




void drawWheelsLeft(glm::mat4 Base, float angle_x, float angle_y) {
	glm::mat4 WheelsLeft = Base; //Zainicjuj macierz modelu macierzą jednostkową
	//WheelsLeft = glm::rotate(WheelsLeft, angle_y, glm::vec3(1.0f, 0.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi Y o angle_Y stopni
	//WheelsLeft = glm::rotate(WheelsLeft, angle_x, glm::vec3(1.0f, 0.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi X o angle_X stopni

	for (int i = 0; i < 23; i++)
	{
		if (i == 3 || i == 4 || i == 5 || i == 6 || i == 7 || i == 8 || i == 9 || i == 10)	//indeksy meshow z kolami
		{

			glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(WheelsLeft));
			glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[i].verts.data());
			glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[i].texCoords.data());
			glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[i].norms.data());
			glDrawElements(GL_TRIANGLES, models[i].indices.size(), GL_UNSIGNED_INT, models[i].indices.data());
		}
	}
}





void drawPads(glm::mat4 Base) {
	glm::mat4 Pads = Base; //Zainicjuj macierz modelu macierzą jednostkową

	for (int i = 1; i < 3; i++)
	{
		glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(Pads));
		glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[i].verts.data());
		glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[i].texCoords.data());
		glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[i].norms.data());
		glDrawElements(GL_TRIANGLES, models[i].indices.size(), GL_UNSIGNED_INT, models[i].indices.data());
	}
}






void drawTurret(glm::mat4 Base, float angle_x_turret, float angle_y_turret) {
	glm::mat4 Turret = Base; //Zainicjuj macierz modelu macierzą jednostkową
	Turret = glm::rotate(Turret, angle_y_turret, glm::vec3(0.0f, 1.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi Y o angle_Y stopni


	glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(Turret));
	glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[14].verts.data());
	glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[14].texCoords.data());
	glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[14].norms.data());
	glDrawElements(GL_TRIANGLES, models[14].indices.size(), GL_UNSIGNED_INT, models[14].indices.data());

	drawGun(Turret, angle_x_turret);
}






void drawGun(glm::mat4 Turret, float angle_x_turret) {
	float angle_x_turret_gun = angle_x_turret / 3;
	glm::mat4 Gun = Turret; //Zainicjuj macierz modelu macierzą jednostkową
	Gun = glm::rotate(Gun, angle_x_turret_gun, glm::vec3(1.0f, 0.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi X o angle_X stopni

	glActiveTexture(GL_TEXTURE0); //Assign texture tex0 to the 0-th texturing unit
	glBindTexture(GL_TEXTURE_2D, czarny);

	for (int i = 11; i < 14; i++)
	{
		if (i == 12) continue;	//pominalem chwilowo dziwny obiekt o indexie 12
		glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(Gun));
		glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[i].verts.data());
		glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[i].texCoords.data());
		glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[i].norms.data());
		glDrawElements(GL_TRIANGLES, models[i].indices.size(), GL_UNSIGNED_INT, models[i].indices.data());
	}

	glm::mat4 Part = Gun;
	Part = glm::translate(Part, glm::vec3(0.0f, 7.0f, 12.0f));
	Part = glm::scale(Part, glm::vec3(0.1f, 0.1f, 0.1f));
	Part = glm::rotate(Part, PI / 20, glm::vec3(-1.0f, 0.0f, 0.0f));
	Part = glm::rotate(Part, PI / 2, glm::vec3(1.0f, 0.0f, 0.0f));

	if (shot) {
		processSystem(particles, gravity, n, glfwGetTime());
		if (!stopParticles) {
			for (int i = 0; i < n; i++)
			{
				glm::vec3 temp = glm::vec3(0.0f, 0.0f, 0.0f);
				//std::cout << abs(distribution(generator)) << std::endl;
				//std::cout << systemxd[i].position.x << " " << systemxd[i].position.y << " " << systemxd[i].position.z << std::endl;
				Part = glm::translate(Part, particles[i].position); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi X o angle_X stopni
				//Part = glm::rotate(Part, PI/2, glm::vec3(0.0f, 0.0f, 1.0f));
				//Part = glm::translate(Part, systemxd[i].position); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi X o angle_X stopni
				glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(Part));
				glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}
		else stopParticles = false;
	}

	cleanTank();
}



void cleanTank() {
	glDisableVertexAttribArray(spLambertTextured->a("vertex"));	//Sprzatamy
	glDisableVertexAttribArray(spLambertTextured->a("color"));
	glDisableVertexAttribArray(spLambertTextured->a("normal"));
}









//``````````````````KOD RYSUJĄCY`````````````````````````````````````````````````````````````````````````````````
// 
// 
// 
// 
//Drawing procedure
void drawScene(GLFWwindow* window, glm::mat4 P, float Lena_angle, float Lena_angle2, float angle_x_turret, float angle_y_turret, float Langle_obrot, float kat_x, float kat_y) {
	//Czyszczenie bufora kolorow i glebi
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	//-----------------MACIERZ WIDOKU - KAMERA-------------------------------------------------------------------
	//Macierz widoku - obserwator, cel, wysokosc. Wybierz jedną z opcji i zakomentuj pozostałe.
	glm::mat4 V = glm::mat4(1.0f);
	//Ruch myszka, niepłynne poruszanie kmaery na płaszczyźnie. 
	V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	//Brak reakcji na ruch myszy, płynne poruszanie się kamerą. 
	//V = glm::lookAt(pos, pos + calcDir(kat_x, kat_y), glm::vec3(0.0f, 1.0f, 0.0f)); //Wylicz macierz widoku
	//Macierz rzutowania wyliczamy w pętli while.

	//To trzeba zakomentować jeżeli nie chcemy żeby kamera poruszała się razem z czołgiem tylko niezależnie od niego.
	//cameraPos = glm::vec3(Lena_angle-3.0f, 18.0f, Lena_angle2 - 28.0f);
	//------------------------------------------------------------------------------------------------------------

	drawTank(P, V, Lena_angle, Lena_angle2, angle_x_turret, angle_y_turret, Langle_obrot);


	//Aktywuj program cieniujący.
	spLambert->use(); //Aktyeuj program cieniujący

	glUniformMatrix4fv(spLambert->u("P"), 1, false, glm::value_ptr(P)); //Załaduj do programu cieniującego macierz rzutowania
	glUniformMatrix4fv(spLambert->u("V"), 1, false, glm::value_ptr(V)); //Załaduj do programu cieniującego macierz widoku

	glm::mat4 M = glm::mat4(1.0f);
	M = glm::translate(M, glm::vec3(-11, 0, -10));
	M = glm::scale(M, glm::vec3(0.8, 0.8, 0.8));
	//-------------------------Ten kawałek kodu powtarzamy dla każdego obiektu.----------------------------------------------
	glm::mat4 M1 = M;
	M1 = glm::translate(M1, glm::vec3(4, 3, 0));
	M1 = glm::scale(M1, glm::vec3(1, 3, 1));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M1)); //Załaduj do programu cieniującego macierz modelu


	glm::mat4 M2 = M;
	M2 = glm::translate(M2, glm::vec3(-4, 2, 0));
	M2 = glm::scale(M2, glm::vec3(1.5, 2, 1));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M2)); //Załaduj do programu cieniującego macierz modelu

	glm::mat4 M3 = M;
	M3 = glm::translate(M3, glm::vec3(-1, 1, 5));
	M3 = glm::scale(M3, glm::vec3(0.5, 1, 1.5));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M3)); //Załaduj do programu cieniującego macierz modelu

	glm::mat4 M4 = M;
	M4 = glm::translate(M4, glm::vec3(0, 0.25f, 0));
	M4 = glm::scale(M4, glm::vec3(0.5, 0.25, 0.5));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M4)); //Załaduj do programu cieniującego macierz modelu

	glm::mat4 Kostka = glm::mat4(1.0f);
	Kostka = glm::translate(Kostka, glm::vec3(10, 2, -7));
	Kostka = glm::scale(Kostka, glm::vec3(2.0f, 2.0f, 3.0f));
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(Kostka)); //Załaduj do programu cieniującego macierz modelu


	glm::mat4 Kostka2 = glm::mat4(1.0f);
	Kostka2 = glm::translate(Kostka2, glm::vec3(7.0f, 1, -10.5f));
	Kostka2 = glm::scale(Kostka2, glm::vec3(3.0f, 1.0f, 1.2f));

	int wymiar = 150;
	//Podłoga
	glm::mat4 M_floor = glm::mat4(1.0f);
	M_floor = glm::scale(M_floor, glm::vec3(wymiar, 0.01f, wymiar));
	//Ściana z lewej
	glm::mat4 M_left_wall = glm::mat4(1.0f);
	M_left_wall = glm::translate(M_left_wall, glm::vec3(-wymiar, wymiar, 0));
	M_left_wall = glm::scale(M_left_wall, glm::vec3(0.01, wymiar, wymiar));
	//Ściana z prawej
	glm::mat4 M_right_wall = glm::mat4(1.0f);
	M_right_wall = glm::translate(M_right_wall, glm::vec3(wymiar, wymiar, 0));
	M_right_wall = glm::scale(M_right_wall, glm::vec3(0.01, wymiar, wymiar));
	//Ściana z tyłu
	glm::mat4 M_back_wall = glm::mat4(1.0f);
	M_back_wall = glm::translate(M_back_wall, glm::vec3(0, wymiar, -wymiar));
	M_back_wall = glm::scale(M_back_wall, glm::vec3(wymiar, wymiar, 0.01));
	//Sufit
	glm::mat4 Sufit = glm::mat4(1.0f);
	Sufit = glm::translate(Sufit, glm::vec3(0, 2 * wymiar, 0));
	Sufit = glm::scale(Sufit, glm::vec3(wymiar, 0.01f, wymiar));


	texCube(P, V, M_floor, tex0);
	texCube(P, V, M_left_wall, tex1);
	texCube(P, V, M_right_wall, tex4);
	texCube(P, V, M_back_wall, tex3);
	texCube(P, V, Sufit, tex5);
	texCube(P, V, Kostka, tex6);
	texCube(P, V, Kostka2, tex6);
	texCube(P, V, M1, tex6);
	texCube(P, V, M2, tex7);
	texCube(P, V, M3, tex7);
	texCube(P, V, M4, tex7);


	//Przerzucenie tylnego bufora na przedni
	glfwSwapBuffers(window);
}




int main(void)
{
	GLFWwindow* window; //Wskaznik na obiekt reprezentujacy okno

	glfwSetErrorCallback(error_callback);	//Zarejestruj procedure obslugi bledow

	if (!glfwInit()) { //Zainicjuj biblioteke GLFW, jesli nie mozna - stderr
		fprintf(stderr, "Can't initialize GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(1500, 1500, "Tank Simulator", NULL, NULL);  //Create a window 500pxx500px titled "OpenGL" and an OpenGL context associated with it. 

	if (!window) //Jezeli okna nie udalo sie utworzyc, to zamknij program
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje sie aktywny i polecenia rysujace dzialaja w tym oknie
	glfwSwapInterval(1); //During vsync wait for the first refresh - //Czekaj na 1 powrot plamki przez pokazaniem ukrytego bufora

	GLenum err;
	if ((err = glewInit()) != GLEW_OK) { //Initialize GLEW library
		fprintf(stderr, "Can't initialize GLEW: %s\n", glewGetErrorString(err));
		exit(EXIT_FAILURE);
	}

	//--------------START-------------------------------------------------------------------------------------------------------
	initOpenGLProgram(window);

	//Zmienne pomocnicze
	float Lena_angle = 0.0f, Lena_angle2 = 0.0f;	//Poruszanie się.
	float Langle_obrot = 0, Langle_obrot2 = 0;		//Obrót wokół osi Y.
	float angle_y_turret = 0, angle_x_turret = 0;	//Obrót góry czołgu i lufy.
	//float angle_x = 0, angle_y = 0;				//Obrót wokół osi X.
	
	glfwSetTime(0); //Wyzeruj licznik czasu

	//Od typa.
	float angle = 0; 
	float kat_x = 0;	
	float kat_y = 0;
	//Glowna petla
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamkniete
	{
		//angle_x += speed_x * glfwGetTime();
		//angle_y += speed_y * glfwGetTime();

		//Poruszanie się czołgu.
		Lena_angle -= Lena_speed * glfwGetTime(); 
		Lena_angle2 += Lena_speed2 * glfwGetTime(); 
		//Obrót czołgu wokół osi Y.
		Langle_obrot += Lspeed_obrot * glfwGetTime(); 
		Langle_obrot += Lspeed_obrot2 * glfwGetTime(); 
		//Obroty górą i lufą z ograniczeniami.
		if (angle_x_turret <= PI/8 && angle_x_turret >= -PI / 8) 
			angle_x_turret += speed_x_turret * glfwGetTime(); //Oblicz kat o jaki obiekt obrócił się podczas poprzedniej klatki
		else if (angle_x_turret >= PI / 8 && speed_x_turret < 0) angle_x_turret += speed_x_turret * glfwGetTime();
		else if (angle_x_turret <= -PI / 8 && speed_x_turret > 0) angle_x_turret += speed_x_turret * glfwGetTime();

		if (angle_y_turret <= PI / 8 && angle_y_turret >= -PI / 8)
			angle_y_turret += speed_y_turret * glfwGetTime(); //Oblicz kat o jaki obiekt obrócił się podczas poprzedniej klatki
		else if (angle_y_turret >= PI / 8 && speed_y_turret < 0) angle_y_turret += speed_y_turret * glfwGetTime();
		else if (angle_y_turret <= -PI / 8 && speed_y_turret > 0) angle_y_turret += speed_y_turret * glfwGetTime();
		//Kamera typa.
		kat_x += speed_x_typa * glfwGetTime();
		kat_y += speed_y_typa * glfwGetTime();
		pos += (float)(walk_speed * glfwGetTime()) * calcDir(kat_x, kat_y);

		//Poruszanie się kamerą za pomocą myszy.
		glm::mat4 P = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);

		glfwSetTime(0); //Wyzeruj licznik czasu
		drawScene(window, /*angle_x, angle_y, */P, Lena_angle, Lena_angle2, angle_x_turret, angle_y_turret, Langle_obrot, kat_x, kat_y); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);	//Zwalniamy zasoby

	glfwDestroyWindow(window);	//Usun kontekst OpenGL i okno
	glfwTerminate();	//Zwolnij zasoby zajete przez GLFW
	exit(EXIT_SUCCESS);
}









//------implementacje funkcji


void loadModel(std::string plik) {
	using namespace std;
	//aiProcess_Triangulate - wyszystkie wielokaty przerob na trojkaty - dobra opcja, gwarancja ze wszystujemy model z trojkatow
	//aiProcess_FlipUVs - odwrocenie wspolrzedniej Y na teksturach, zamiana punktu zaczepienia tekstury z lewego dolnedo na lewy gorny - zeby dobrze sie wczytaly tekstury 
	//aiProcess_GenSmoothNormals - automatycznie wygenerowanie wektorow normalnych jesli obiekt ich nie ma, jesli ma to nic sie nie dzieje
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(plik, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);	//importer ma metode readfile
	cout << importer.GetErrorString() << endl;	//jezeli wczytanie pliku sie nie uda, wowczas mamy null i wtedy nam wywali blad
	//obiekt jeest sam usuwany z pamieci wiec nie usuwamy go pozniej recznie

//czy w pliku są siatki wielokątów (meshes)
	if (scene->HasMeshes()) {

		models = new Model3D[(int)scene->mNumMeshes];
		numberofmeshes = (int)scene->mNumMeshes;
		cout << "Liczba meshy: " << numberofmeshes << endl;

		for (int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)	//liczba siatek wielokątów
		{
			aiMesh* mesh = scene->mMeshes[meshIndex];	//dostęp do konkretnej siatki	//pobranie wskażnika na mesh ktory bedziemy przegladac

			for (int vertIndex = 0; vertIndex < mesh->mNumVertices; vertIndex++) {

				aiVector3D vertex = mesh->mVertices[vertIndex];	//aiVector3D podobny do glm::vec3, ten obiekt wspolrzednych posiada pola x,y,z typu float
				//tablica mVertices posiada mNumVertices wierzcholkow

				models[meshIndex].verts.push_back(glm::vec4(vertex.x, vertex.y, vertex.z, 1));	//przenosimy to do zdefiniowanego przez nas wektora z wiercholkami
				//cout << vertex.x << " " << vertex.y << " " << vertex.z << endl;

				//z kazdym wierzcholkiem skojarzony jest wektor normalny, robimy tak samo jak powyzej
				aiVector3D normal = mesh->mNormals[vertIndex];	//aiVector3D podobny do glm::vec3

				models[meshIndex].norms.push_back(glm::vec4(normal.x, normal.y, normal.z, 0));	//0 bo jest to kierunek, a nie pozycja
				//cout << normal.x << " " << normal.y << " " << normal.z << endl;

				aiVector3D texCoord = mesh->mTextureCoords[0][vertIndex];	//0 to numer zestawu wspolrzednych teksturowania, zakladamy ze mamy tylko 1 teksture
				models[meshIndex].texCoords.push_back(glm::vec2(texCoord.x, texCoord.y));

			}

			//obiekt mesh zawiera roznie tablice mFaces, ktora ma mNumFaces pozycji
			//dla kazdego wielokata skladowego
			for (int faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
				aiFace& face = mesh->mFaces[faceIndex]; //face to jeden z wielokatow modelu/siatki (mesha)

				//i teraz kazdy face, zawiera tablice mIndices z mNumIndices pozycjami(3), wskazujacymi na wierzcholki tworzoce ten face(trojkat)
				//dla kazdego indeksu->wierzcholka tworzacego wielokat
				//dla aiProcess_Triangulate to zawsze bedzie 3
				for (int j = 0; j < face.mNumIndices; j++) {
					models[meshIndex].indices.push_back(face.mIndices[j]);
				}

			}
			//kazdy mesh ma 1 material
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			/*for (int i = 0; i < 19; i++) {
				cout << i << " " << material->GetTextureCount((aiTextureType)i) << endl;
			}*/

			for (int k = 0; k < material->GetTextureCount(aiTextureType_DIFFUSE); k++) {
				aiString str; //nazwa pliku

				aiTextureMapping mapping; //jak wygenerowano wsp. texturowania (opcj.)
				unsigned int uvMapping; //numer zestawu wsp. reksturowania (opcjonalne
				ai_real blend; //wspolczynnik polaczenia kolorow z kolejna tekstura (ocpjonalne)
				aiTextureOp op; //sposob laczenia kolorow z kolejna tekstura(opcjonalne)
				aiTextureMapMode mapMode; //sposob adresowania tekstury (opcjonalne)

				material->GetTexture(aiTextureType_DIFFUSE, k, &str, &mapping, &uvMapping, &blend, &op, &mapMode);
				cout << str.C_Str() << endl;
			}
		}
	}
	//podobnie można uzyskać dostęp do: listy zrodel swiatla, listy kamer, listy materialow, listy wbudowanych tekstur, listy animacji		
}





glm::vec3 calcDir(float kat_x, float kat_y) {
	glm::vec4 dir = glm::vec4(0, 0, 1, 0);
	glm::mat4 M = glm::rotate(glm::mat4(1.0f), kat_y, glm::vec3(0, 1, 0));
	M = glm::rotate(M, kat_x, glm::vec3(1, 0, 0));
	dir = M * dir;
	return glm::vec3(dir);
}





GLuint readTexture(const char* filename) {
	GLuint tex;

	//Wczytywanie do pamieci operacyjnej
	std::vector<unsigned char> image;   //Alokowanie pamieci 
	unsigned width, height;   //Zmienne do rozmiaru tekstury
	//Czytanie tekstury
	unsigned error = lodepng::decode(image, width, height, filename);

	//Importowanie do karty graficznej
	glGenTextures(1, &tex); //Inicjacja uchwytu
	glBindTexture(GL_TEXTURE_2D, tex); //Aktywacja uchwytu
	//Copy image to graphics cards memory reprezented by the active handle
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tex;
}







unsigned int loadCubemap(std::vector<std::string> faces)
{
	//Uchwyty do tekstury
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);


	//Wczytywanie do pamieci operacyjnej 6 zdjęć tła
	std::vector<unsigned char> image;   //Alokowanie pamieci 
	unsigned width, height;   //Zmienne do rozmiaru tekstury


	for (unsigned int i = 0; i < faces.size(); i++)
	{
		//Czytanie tekstury
		unsigned data = lodepng::decode(image, width, height, faces[i]);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, (unsigned char*)image.data());
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
		}
	}

	//Metody zawijania tekstury.
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}


//Błędy.
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}


//Dopasowuje rozmiar okna.
void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}


//Reakcja na input.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	float cameraSpeed = 5.5f;
	if (action == GLFW_PRESS) {
		//Zamknij okno przez escape.
		if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);

		//Strzał czasteczkami
		if (key == GLFW_KEY_Q) shot = true;

		//Poruszanie się.
		if (key == GLFW_KEY_LEFT) Lena_speed = -PI/2;
		if (key == GLFW_KEY_RIGHT) Lena_speed = PI/2;
		if (key == GLFW_KEY_UP) Lena_speed2 = PI;
		if (key == GLFW_KEY_DOWN) Lena_speed2 = -PI;
		//Obrót górą i lufą.
		if (key == GLFW_KEY_A) speed_y_turret = -PI / 20;
		if (key == GLFW_KEY_D) speed_y_turret = PI / 20;
		if (key == GLFW_KEY_W) speed_x_turret = -PI / 20;
		if (key == GLFW_KEY_S) speed_x_turret = PI / 20;
		//Obrót całośći wokół osi Y.
		if (key == GLFW_KEY_1) {
			Lspeed_obrot = -PI / 10;
			leftRotate = true;
		}
		if (key == GLFW_KEY_2) {
			Lspeed_obrot2 = PI / 10;
			rightRotate = true;
		}
		//Kamera.
		if (key == GLFW_KEY_3) cameraPos += cameraSpeed * cameraFront;
		if (key == GLFW_KEY_4) cameraPos -= cameraSpeed * cameraFront;
		if (key == GLFW_KEY_5) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (key == GLFW_KEY_6) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		//Kamera od typa
		/*if (key == GLFW_KEY_LEFT) speed_y_typa = 1;
		if (key == GLFW_KEY_RIGHT) speed_y_typa = -1;
		if (key == GLFW_KEY_PAGE_UP) speed_x_typa = 1;
		if (key == GLFW_KEY_PAGE_DOWN) speed_x_typa = -1;
		if (key == GLFW_KEY_UP) walk_speed = 2;
		if (key == GLFW_KEY_DOWN) walk_speed = -2;*/

		/*	"Dachowanie".
		if (key == GLFW_KEY_LEFT) speed_y = -PI / 2;
		if (key == GLFW_KEY_RIGHT) speed_y = PI / 2;
		if (key == GLFW_KEY_UP) speed_x = PI / 2;
		if (key == GLFW_KEY_DOWN) speed_x = -PI / 2;*/
	}

	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT) Lena_speed = 0;
		if (key == GLFW_KEY_RIGHT) Lena_speed = 0;
		if (key == GLFW_KEY_UP) Lena_speed2 = 0;
		if (key == GLFW_KEY_DOWN) Lena_speed2 = 0;

		if (key == GLFW_KEY_1) {
			Lspeed_obrot = 0; leftRotate = false;
		}
		if (key == GLFW_KEY_2) {
			Lspeed_obrot2 = 0; rightRotate = false;
		}


		if (key == GLFW_KEY_A) speed_y_turret = 0;
		if (key == GLFW_KEY_D) speed_y_turret = 0;
		if (key == GLFW_KEY_W) speed_x_turret = 0;
		if (key == GLFW_KEY_S) speed_x_turret = 0;

		//Kamera.
		if (key == GLFW_KEY_3) cameraPos += cameraSpeed * cameraFront;
		if (key == GLFW_KEY_4) cameraPos -= cameraSpeed * cameraFront;
		if (key == GLFW_KEY_5) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (key == GLFW_KEY_6) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

		//Kamera od typa
		/*if (key == GLFW_KEY_LEFT) speed_y_typa = 0;
		if (key == GLFW_KEY_RIGHT) speed_y_typa = 0;
		if (key == GLFW_KEY_PAGE_UP) speed_x_typa = 0;
		if (key == GLFW_KEY_PAGE_DOWN) speed_x_typa = 0;
		if (key == GLFW_KEY_UP) walk_speed = 0;
		if (key == GLFW_KEY_DOWN) walk_speed = 0;*/

	}
}




//Naciskanie klawiszy myszy.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		std::cout << "Nacisniety lewy przycisk myszy. Tutaj moze nastapić strzal." << std::endl;
		shot = true;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		std::cout << "Lewy przycisk myszy puszczony." << std::endl;
	}
}




//Obsługa kółka myszki.
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	fov -= (float)yoffset;
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 45.0f)
		fov = 45.0f;
}





//Obsługa myszy.
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	//Ograniczenia kamery, coby sobie głowy nie wykręcić
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}




void createParticle(Particle& p) {		//Zainicjowanie cząstki
	//std::cout << "createParticle()" << std::endl;
	p.position = glm::vec3(0, 0, 0);
	p.speed = glm::vec3(distribution(generator), 5 * abs(distribution(generator)), distribution(generator));
	p.ttl = 0.1;
}


void initializeSystem(Particle* particles, int n) {		//Zainicjowanie każdej cząstki
	//std::cout << "initializeSystem()" << std::endl;
	for (int i = 0; i < n; i++) createParticle(particles[i]);
}


void processSystem(Particle* particles, glm::vec3 gravity, int n, float timestep) {
	//std::cout << "processSystem()" << std::endl;
	for (int i = 0; i < n; i++) {

		particles[i].position += particles[i].speed * timestep;	 //przesunięcie
		particles[i].speed += gravity * timestep;	 //uwzględnienie grawitacji
		particles[i].ttl -= timestep;	//Skrócenie czasu życia cząstki

		//if (system[i].ttl <= 0) createParticle(system[i]);//Reinkarnacja cząstki - zakomentowana bo nie chce ich odnawiac automatycznie
		if (particles[i].ttl <= 0) {
			shot = false;	//Jak czas czastki sie skonczy to ma zniknac 
			stopParticles = true;
			initializeSystem(particles, n); 
		}	
	}
}

void texCube(glm::mat4 P, glm::mat4 V, glm::mat4 M, GLuint tekstura) {
	//This array should rather be placed in the myCube.h file, but I placed it here so that the full solution of the exercise is placed in a single procedure
	float myCubeTexCoords[] = {
		1.0f, 0.0f,	  0.0f, 1.0f,    0.0f, 0.0f,
		1.0f, 0.0f,   1.0f, 1.0f,    0.0f, 1.0f,

		1.0f, 0.0f,	  0.0f, 1.0f,    0.0f, 0.0f,
		1.0f, 0.0f,   1.0f, 1.0f,    0.0f, 1.0f,

		1.0f, 0.0f,	  0.0f, 1.0f,    0.0f, 0.0f,
		1.0f, 0.0f,   1.0f, 1.0f,    0.0f, 1.0f,

		1.0f, 0.0f,	  0.0f, 1.0f,    0.0f, 0.0f,
		1.0f, 0.0f,   1.0f, 1.0f,    0.0f, 1.0f,

		1.0f, 0.0f,	  0.0f, 1.0f,    0.0f, 0.0f,
		1.0f, 0.0f,   1.0f, 1.0f,    0.0f, 1.0f,

		1.0f, 0.0f,	  0.0f, 1.0f,    0.0f, 0.0f,
		1.0f, 0.0f,   1.0f, 1.0f,    0.0f, 1.0f,
	};

	spTextured->use();

	glUniformMatrix4fv(spTextured->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(spTextured->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(spTextured->u("M"), 1, false, glm::value_ptr(M));


	glEnableVertexAttribArray(spTextured->a("vertex"));
	glVertexAttribPointer(spTextured->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices);

	glEnableVertexAttribArray(spTextured->a("texCoord"));
	glVertexAttribPointer(spTextured->a("texCoord"), 2, GL_FLOAT, false, 0, myCubeTexCoords);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tekstura);
	glUniform1i(spTextured->u("tex"), 0);

	glDrawArrays(GL_TRIANGLES, 0, myCubeVertexCount);

	glDisableVertexAttribArray(spTextured->a("vertex"));
	glDisableVertexAttribArray(spTextured->a("color"));
}