/* Program cieniujacy dziala na karcie graficznej */
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

//pliki naglowkowe zalaczone do projektu - dodatki
#include "constants.h"	//zawiera zdefiniowane stale, np PI
#include "allmodels.h" //kod rysujacy przykladowe modele
#include "lodepng.h"	//sluzy do wczytywania tekstur
#include "shaderprogram.h"	//modul, ktory realizuje programy cieniujace

#include <assimp/Importer.hpp>	//obiekt ktory wczytuje obiket z modelami 3d
#include <assimp/scene.h>		//interpretuje cala zawartosc pliku
#include <assimp/postprocess.h>

/*
	PREDEFINICJE FUNKCJI
*/

//Procedura obslugi bledow
void error_callback(int error, const char* description);

//Procedura obslugi klawiszy
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

//Procedura obslugi rozszerzalnosci okna
void windowResizeCallback(GLFWwindow* window, int width, int height);

//Wczytywanie tekstury
GLuint readTexture(const char* filename);


//Wczytywanie obiektow
void drawTank(glm::mat4 P, glm::mat4 V, float angle_x, float angle_y, float angle_x_turret, float angle_y_turret);
void drawBase(glm::mat4 P, glm::mat4 V, float angle_x, float angle_y, float angle_x_turret, float angle_y_turret);
void drawWheelsRight(glm::mat4 Base, float angle_x, float angle_y);
void drawWheelsLeft(glm::mat4 Base, float angle_x, float angle_y);
void drawPads(glm::mat4 Base);
void drawTurret(glm::mat4 Base, float angle_x_turret, float angle_y_turret);
void drawGun(glm::mat4 Turret, float angle_y_turret);
void cleanTank();

//-----------------------------------------------------------------------------------------


class Model3D
{
public:
	Model3D() {};
	~Model3D() {};
	std::vector<glm::vec4> verts;
	std::vector<glm::vec4> norms;
	std::vector<glm::vec2> texCoords;	//tutaj powinna byc klasa, model3D
	std::vector<unsigned int> indices;
};


float speed_x_turret = 0;
float speed_y_turret = 0;

float speed_x = 0;	//[radiany/s]
float speed_y = 0;	//[radiany/s]
float aspectRatio = 1;	//okno
int numberofmeshes = 0;
GLuint tex;
Model3D* models;


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

				//aiVector3D texCoord = mesh->mTextureCoords[0][vertIndex];	//0 to numer zestawu wspolrzednych teksturowania, zakladamy ze mamy tylko 1 teksture
				//models[meshIndex].texCoords.push_back(glm::vec2(texCoord.x, texCoord.y));

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


//Initialization code procedure
void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	//Tutaj umieszczamy kod, ktory nalezy wykonac raz, na poczatku programu
	glClearColor(0.8, 0.8, 0.8, 1); //Ustawianie koloru jakim bedzie czyszczone okno
	glEnable(GL_DEPTH_TEST); //Turn on pixel depth test based on depth buffer
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	tex = readTexture("bricks.png");
	loadModel(std::string("./assets/m1.obj"));
}




//Zwolnienie zasobow zajetych przez program 
void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();
	glDeleteTextures(1, &tex);
	//Tutaj umieszaczaj kod, ktory nalezy wykonac po zakonczeniu petli glownej
	delete models;
}

void drawTank(glm::mat4 P, glm::mat4 V, float angle_x, float angle_y, float angle_x_turret, float angle_y_turret) {
	spLambertTextured->use();	//Aktywacja programu cieniujacego
	glUniformMatrix4fv(spLambertTextured->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(spLambertTextured->u("V"), 1, false, glm::value_ptr(V));
	glEnableVertexAttribArray(spLambert->a("vertex"));	//Wlaczenie uzywania danego atrybutu
	glEnableVertexAttribArray(spLambertTextured->a("texCoord"));	//Wlaczenie uzywania danego atrybutu
	glEnableVertexAttribArray(spLambertTextured->a("normal"));	//Wlaczenie uzywania danego atrybutu
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(spLambertTextured->u("tex"), 0);
	drawBase(P, V, angle_x, angle_y, angle_x_turret, angle_y_turret);
}

void drawBase(glm::mat4 P, glm::mat4 V, float angle_x, float angle_y, float angle_x_turret, float angle_y_turret) {

	glm::mat4 Base = glm::mat4(1.0f); //Zainicjuj macierz modelu macierzą jednostkową
	Base = glm::rotate(Base, angle_y, glm::vec3(0.0f, 1.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi Y o angle_Y stopni
	Base = glm::rotate(Base, angle_x, glm::vec3(1.0f, 0.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi X o angle_X stopni

	glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(Base));
	glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[15].verts.data());
	glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[15].texCoords.data());
	glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[15].norms.data());
	glDrawElements(GL_TRIANGLES, models[15].indices.size(), GL_UNSIGNED_INT, models[15].indices.data());
	
	drawWheelsRight(Base, angle_x, angle_y);
	drawWheelsLeft(Base, angle_x, angle_y);

	drawPads(Base);

	drawTurret(Base, angle_x_turret, angle_y_turret);
}
void drawWheelsRight(glm::mat4 Base, float angle_x, float angle_y) {
	glm::mat4 WheelsRight = Base; //Zainicjuj macierz modelu macierzą jednostkową
	//WheelsRight = glm::rotate(WheelsRight, angle_y, glm::vec3(1.0f, 0.0f, 0.0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi Y o angle_Y stopni
	//WheelsRight = glm::rotate(WheelsRight, angle_x, glm::vec3(0.0f, 1.0f, .0f)); //Pomnóż macierz modelu razy macierz obrotu o kąt angle wokół osi X o angle_X stopni

	for (int i = 0; i < 2; i++)
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
	
	for (int i = 11; i < 14; i++)
	{
		if (i == 12) continue;	//pominalem chwilowo dziwny obiekt o indexie 12
		glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(Gun));
		glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[i].verts.data());
		glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[i].texCoords.data());
		glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[i].norms.data());
		glDrawElements(GL_TRIANGLES, models[i].indices.size(), GL_UNSIGNED_INT, models[i].indices.data());
	}
	cleanTank();
}

void cleanTank() {
	glDisableVertexAttribArray(spLambertTextured->a("vertex"));	//Sprzatamy
	glDisableVertexAttribArray(spLambertTextured->a("color"));
	glDisableVertexAttribArray(spLambertTextured->a("normal"));
}

//Drawing procedure
void drawScene(GLFWwindow* window, float angle_x, float angle_y, float angle_x_turret, float angle_y_turret) {
	//Tutaj umieszamy kod rysujacy obraz
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Czyszczenie bufora kolorow i glebi
	//macierz widoku - obserwator, cel, wysokosc
	glm::mat4 V = glm::lookAt(glm::vec3(0.0f, 0.0f, -40.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); 
	//macierz rzutowania
	glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 1.0f, 50.0f); //Compute projection matrix

	drawTank(P, V, angle_x, angle_y, angle_x_turret, angle_y_turret);

	glfwSwapBuffers(window); //Przerzucenie bufora tylniego na przedni
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

	initOpenGLProgram(window); //Operacje inicjujace - jestesmy gotowi do dzialania

	//Zmienne pomocnicze
	float angle_y_turret = 0;
	float angle_x_turret = 0;

	float angle_x = 0; //zadeklaruj zmienna przechowującą aktualny kat obrotu
	float angle_y = 0; //zadeklaruj zmienna przechowującą aktualny kat obrotu
	glfwSetTime(0); //Wyzeruj licznik czasu
	//Glowna petla
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamkniete
	{
		angle_x += speed_x * glfwGetTime(); //Oblicz kat o jaki obiekt obrócił się podczas poprzedniej klatki
		angle_y += speed_y * glfwGetTime(); //Oblicz kat o jaki obiekt obrócił się podczas poprzedniej klatki

		if (angle_x_turret <= PI/8 && angle_x_turret >= -PI / 8) 
			angle_x_turret += speed_x_turret * glfwGetTime(); //Oblicz kat o jaki obiekt obrócił się podczas poprzedniej klatki
		else if (angle_x_turret >= PI / 8 && speed_x_turret < 0) angle_x_turret += speed_x_turret * glfwGetTime();
		else if (angle_x_turret <= -PI / 8 && speed_x_turret > 0) angle_x_turret += speed_x_turret * glfwGetTime();

		if (angle_y_turret <= PI / 8 && angle_y_turret >= -PI / 8)
			angle_y_turret += speed_y_turret * glfwGetTime(); //Oblicz kat o jaki obiekt obrócił się podczas poprzedniej klatki
		else if (angle_y_turret >= PI / 8 && speed_y_turret < 0) angle_y_turret += speed_y_turret * glfwGetTime();
		else if (angle_y_turret <= -PI / 8 && speed_y_turret > 0) angle_y_turret += speed_y_turret * glfwGetTime();
		
		glfwSetTime(0); //Wyzeruj licznik czasu
		drawScene(window, angle_x, angle_y, angle_x_turret, angle_y_turret); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);	//Zwalniamy zasoby

	glfwDestroyWindow(window);	//Usun kontekst OpenGL i okno
	glfwTerminate();	//Zwolnij zasoby zajete przez GLFW
	exit(EXIT_SUCCESS);
}





void error_callback(int error, const char* description) {
	fputs(description, stderr);
}


void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_LEFT) speed_y = -PI / 2;
		if (key == GLFW_KEY_RIGHT) speed_y = PI / 2;
		if (key == GLFW_KEY_UP) speed_x = PI / 2;
		if (key == GLFW_KEY_DOWN) speed_x = -PI / 2;

		if (key == GLFW_KEY_A) speed_y_turret = -PI / 10;
		if (key == GLFW_KEY_D) speed_y_turret = PI / 10;
		if (key == GLFW_KEY_W) speed_x_turret = -PI / 20;
		if (key == GLFW_KEY_S) speed_x_turret = PI / 20;
	}
	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT) speed_y = 0;
		if (key == GLFW_KEY_RIGHT) speed_y = 0;
		if (key == GLFW_KEY_UP) speed_x = 0;
		if (key == GLFW_KEY_DOWN) speed_x = 0;

		if (key == GLFW_KEY_A) speed_y_turret = 0;
		if (key == GLFW_KEY_D) speed_y_turret = 0;
		if (key == GLFW_KEY_W) speed_x_turret = 0;
		if (key == GLFW_KEY_S) speed_x_turret = 0;
	}
}

GLuint readTexture(const char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);

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