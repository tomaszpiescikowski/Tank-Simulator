#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"

#include <iostream>

#include <assimp/Importer.hpp>	//obiekt ktory wczytuje obiket z modelami 3d
#include <assimp/scene.h>		//interpretuje cala zawartosc pliku
#include <assimp/postprocess.h>


float speed_x = 0; //angular speed in radians
float speed_y = 0; //angular speed in radians
float aspectRatio = 1;


GLuint tex;

class Model3D
{
public:
	Model3D();
	~Model3D();
	std::vector<glm::vec4> verts;
	std::vector<glm::vec4> norms;
	std::vector<glm::vec2> texCoords;	//tutaj powinna byc klasa, model3D
	std::vector<unsigned int> indices;
};
Model3D::Model3D()
{}
Model3D::~Model3D()
{}


int numberofmeshes = 0;
Model3D* models;


//Error processing callback procedure
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_LEFT) speed_y = -PI / 2;
		if (key == GLFW_KEY_RIGHT) speed_y = PI / 2;
		if (key == GLFW_KEY_UP) speed_x = PI / 2;
		if (key == GLFW_KEY_DOWN) speed_x = -PI / 2;
	}
	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT) speed_y = 0;
		if (key == GLFW_KEY_RIGHT) speed_y = 0;
		if (key == GLFW_KEY_UP) speed_x = 0;
		if (key == GLFW_KEY_DOWN) speed_x = 0;
	}
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}

GLuint readTexture(const char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);

	//Read into computers memory
	std::vector<unsigned char> image;   //Allocate memory 
	unsigned width, height;   //Variables for image size
	//Read the image
	unsigned error = lodepng::decode(image, width, height, filename);

	//Import to graphics card memory
	glGenTextures(1, &tex); //Initialize one handle
	glBindTexture(GL_TEXTURE_2D, tex); //Activate handle
	//Copy image to graphics cards memory reprezented by the active handle
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tex;
}

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
				/*
				aiTextureMapping mapping; //jak wygenerowano wsp. texturowania (opcj.)
				unsigned int uvMapping; //numer zestawu wsp. reksturowania (opcjonalne
				ai_real blend; //wspolczynnik polaczenia kolorow z kolejna tekstura (ocpjonalne)
				aiTextureOp op; //sposob laczenia kolorow z kolejna tekstura(opcjonalne)
				aiTextureMapMode mapMode; //sposob adresowania tekstury (opcjonalne)
				*/
				material->GetTexture(aiTextureType_DIFFUSE, k, &str/*, &mapping, &uvMapping, &blend, &op, &mapMode */);
				cout << str.C_Str() << endl;
			}
		}
	}
		//podobnie można uzyskać dostęp do: listy zrodel swiatla, listy kamer, listy materialow, listy wbudowanych tekstur, listy animacji		
}


//Initialization code procedure
void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	//************Place any code here that needs to be executed once, at the program start************
	glClearColor(0, 0, 0, 1); //Set color buffer clear color
	glEnable(GL_DEPTH_TEST); //Turn on pixel depth test based on depth buffer
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	tex = readTexture("brick.png");
	loadModel(std::string("./assets/uploads_files_2792345_Koenigsegg.obj"));
}

//Release resources allocated by the program
void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();
	glDeleteTextures(1, &tex);
	//************Place any code here that needs to be executed once, after the main loop ends************
	delete models;
}


void drawModels(glm::mat4 P, glm::mat4 V, glm::mat4 M) {

	spLambertTextured->use();
	glUniformMatrix4fv(spLambertTextured->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(spLambertTextured->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(M));

	for (int i = 0; i < numberofmeshes; i++)
	{
		glEnableVertexAttribArray(spLambert->a("vertex"));
		glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, models[i].verts.data());

		glEnableVertexAttribArray(spLambertTextured->a("texCoord"));
		glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, models[i].texCoords.data());

		glEnableVertexAttribArray(spLambertTextured->a("normal"));
		glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, models[i].norms.data());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glUniform1i(spLambertTextured->u("tex"), 0);

		//glDrawArrays(GL_TRIANGLES, 0, myCubeVertexCount);
		glDrawElements(GL_TRIANGLES, models[i].indices.size(), GL_UNSIGNED_INT, models[i].indices.data());

		glDisableVertexAttribArray(spLambertTextured->a("vertex"));
		glDisableVertexAttribArray(spLambertTextured->a("color"));
		glDisableVertexAttribArray(spLambertTextured->a("normal"));
	}
}


//Drawing procedure
void drawScene(GLFWwindow* window, float angle_x, float angle_y) {
	//************Place any code here that draws something inside the window******************l
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear color and depth buffers

	glm::mat4 M = glm::mat4(1.0f); //Initialize model matrix with abn identity matrix
	M = glm::rotate(M, angle_y, glm::vec3(0.0f, 1.0f, 0.0f)); //Multiply model matrix by the rotation matrix around Y axis by angle_y degrees
	M = glm::rotate(M, angle_x, glm::vec3(1.0f, 0.0f, 0.0f)); //Multiply model matrix by the rotation matrix around X axis by angle_x degrees
	glm::mat4 V = glm::lookAt(glm::vec3(0.0f, 0.0f, -40.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); //Compute view matrix
	glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 1.0f, 50.0f); //Compute projection matrix


	drawModels(P, V, M);

	glfwSwapBuffers(window); //Copy back buffer to the front buffer
}

int main(void)
{
	GLFWwindow* window; //Pointer to object that represents the application window

	glfwSetErrorCallback(error_callback);//Register error processing callback procedure

	if (!glfwInit()) { //Initialize GLFW library
		fprintf(stderr, "Can't initialize GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(1500, 1500, "OpenGL", NULL, NULL);  //Create a window 500pxx500px titled "OpenGL" and an OpenGL context associated with it. 

	if (!window) //If no window is opened then close the program
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Since this moment OpenGL context corresponding to the window is active and all OpenGL calls will refer to this context.
	glfwSwapInterval(1); //During vsync wait for the first refresh

	GLenum err;
	if ((err = glewInit()) != GLEW_OK) { //Initialize GLEW library
		fprintf(stderr, "Can't initialize GLEW: %s\n", glewGetErrorString(err));
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Call initialization procedure

	//Main application loop
	float angle_x = 0; //declare variable for storing current rotation angle
	float angle_y = 0; //declare variable for storing current rotation angle
	glfwSetTime(0); //clear internal timer
	while (!glfwWindowShouldClose(window)) //As long as the window shouldnt be closed yet...
	{
		angle_x += speed_x * glfwGetTime(); //Compute an angle by which the object was rotated during the previous frame
		angle_y += speed_y * glfwGetTime(); //Compute an angle by which the object was rotated during the previous frame
		glfwSetTime(0); //clear internal timer
		drawScene(window, angle_x, angle_y); //Execute drawing procedure
		glfwPollEvents(); //Process callback procedures corresponding to the events that took place up to now
	}
	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Delete OpenGL context and the window.
	glfwTerminate(); //Free GLFW resources
	exit(EXIT_SUCCESS);
}
