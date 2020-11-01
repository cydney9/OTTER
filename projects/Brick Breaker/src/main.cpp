#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <windows.h>
#include <entt.hpp>
#include "ObjLoader.h"
#include "Shader.h"
#include "Camera.h"
#include "Transform.h"
#include "Texture2D.h"
#include "Texture2DData.h"

//Credit: Used starter from gdw project to build up from

void GetDesktopResolution(int& horizontal, int& vertical)
{
	RECT desktop;
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);
	// The top left corner will have coordinates (0,0)
	// and the bottom right corner will have coordinates
	// (horizontal, vertical)
	horizontal = desktop.right;
	vertical = desktop.bottom;
}

void RenderVAO
(
	const Shader::sptr& shader,
	const VertexArrayObject::sptr& vao,
	const Camera::sptr& camera,
	const Transform::sptr& transform
)
{
	shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transform->LocalTransform());
	shader->SetUniformMatrix("u_Model", transform->LocalTransform());
	vao->Render();
}

struct Material
{
	Texture2D::sptr Albedo;
	Texture2D::sptr Specular;
	float           Shininess;
};

int main() {
	// Initialize GLFW
	if (glfwInit() == GLFW_FALSE) {
		std::cout << "Failed to initialize Glad" << std::endl;
		return 1;
	}

	// Create a new GLFW window
	int windowWidth, windowHeight;
	GetDesktopResolution(windowWidth, windowHeight);
	GLFWwindow* window = glfwCreateWindow(800, 800, "CG = Brick Breaker", nullptr/*glfwGetPrimaryMonitor()*/, nullptr);
	// We want GL commands to be executed for our window, so we make our window's context the current one
	glfwMakeContextCurrent(window);

	// Let glad know what function loader we are using (will call gl commands via glfw)
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		std::cout << "Failed to initialize Glad" << std::endl;
		return 2;
	}

	// Display our GPU and OpenGL version
	std::cout << glGetString(GL_RENDERER) << std::endl;
	std::cout << glGetString(GL_VERSION) << std::endl;

	//VAOS
	VertexArrayObject::sptr vao0 = ObjLoader::LoadFile("Player.obj");
	VertexArrayObject::sptr vao1 = ObjLoader::LoadFile("ball.obj");
	VertexArrayObject::sptr vao2 = ObjLoader::LoadFile("wall.obj");

	VertexArrayObject::sptr vao[3];
	vao[0] = vao0;
	vao[1] = vao1;
	vao[2] = vao2;

	//Textures


	// Load our shaders
	Shader::sptr shader = Shader::Create();
	shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
	shader->LoadShaderPartFromFile("shaders/frag_blinn_phong.glsl", GL_FRAGMENT_SHADER);
	shader->Link();

	glm::vec3 lightPos = glm::vec3(4.0f, 6.0f, 2.0f);
	glm::vec3 lightCol = glm::vec3(0.3f, 0.2f, 0.5f);
	float     lightAmbientPow = 0.05f;
	float     lightSpecularPow = 1.0f;
	glm::vec3 ambientCol = glm::vec3(1.0f);
	float     ambientPow = 0.1f;
	float     shininess = 4.0f;

	shader->SetUniform("u_LightPos", lightPos);
	shader->SetUniform("u_LightCol", lightCol);
	shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
	shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
	shader->SetUniform("u_AmbientCol", ambientCol);
	shader->SetUniform("u_AmbientStrength", ambientPow);
	shader->SetUniform("u_Shininess", shininess);

	// GL states
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	Transform::sptr transform[3];
	transform[0] = Transform::Create();
	transform[1] = Transform::Create();
	transform[2] = Transform::Create();

	transform[0]->SetLocalPosition(0.0f, 2.5f, 0.0f);
	transform[0]->SetLocalScale(0.8f, 0.2f, 0.5f);

	transform[1]->SetLocalScale(0.125f, 0.125f, 0.125f);
	float ballPosX = 0.25f;
	
	transform[2]->SetLocalScale(100.f, 100.f, 0.01f);

	Camera::sptr camera = Camera::Create();
	camera->SetPosition(glm::vec3(0, 3, 3)); // Set initial position
	camera->SetUp(glm::vec3(0, 0, 1)); // Use a z-up coordinate system
	camera->LookAt(glm::vec3(0.0f)); // Look at center of the screen
	camera->SetFovDegrees(90.0f); // Set an initial FOV

	double lastFrame = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		// Poll for events from windows (clicks, keypressed, closing, all that)
		glfwPollEvents();

		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			//transform2 = glm::rotate(transform2, 0.001f, glm::vec3(0, 0, 1));
			transform[0]->MoveLocal(0.001, 0, 0);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			//transform2 = glm::rotate(transform2, -0.001f, glm::vec3(0, 0, 1));
			transform[0]->MoveLocal(-0.001, 0, 0);
		}

		// Clear our screen every frame
		glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->Bind();

		shader->SetUniform("u_CamPos", camera->GetPosition());

		//Ball
		transform[1]->MoveLocal(0.0f, 0.0005f, 0.f);


		//RenderVAO(shader, vao[1], camera, transform[1]);
		for (int ix = 0; ix < 2; ix++) {
			// TODO: Apply materials
			RenderVAO(shader, vao[ix], camera, transform[ix]);
		}
		
		// Present our image to windows
		glfwSwapBuffers(window);
		lastFrame = thisFrame;

	}

	return 0;
}


bool checkCollision()
{
	return 0;
}