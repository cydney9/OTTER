#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Gameplay/Camera.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Gameplay/Transform.h"
#include "Graphics/Texture2D.h"
#include "Graphics/Texture2DData.h"
#include "Utilities/InputHelpers.h"
#include "Utilities/MeshBuilder.h"
#include "Utilities/MeshFactory.h"
#include "Utilities/ObjLoader.h"
#include "Utilities/VertexTypes.h"

#define LOG_GL_NOTIFICATIONS


/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
	case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
	case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
		#ifdef LOG_GL_NOTIFICATIONS
	case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
		#endif
	default: break;
	}
}

GLFWwindow* window;
Camera::sptr camera = nullptr;

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	camera->ResizeWindow(width, height);
}

bool initGLFW() {
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
	
	//Create a new GLFW window
	window = glfwCreateWindow(1200, 900, "Brick Breaker", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	return true;
}

bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

float checkCollisionBallYSpeed(Transform::sptr ball, Transform::sptr paddle, float ballYSpeed)
{
	float max, min;
	max = paddle->GetLocalPosition().x + (paddle->GetLocalScale().x);
	min = paddle->GetLocalPosition().x - (paddle->GetLocalScale().x);
	
	if (ball->GetLocalPosition().y >= (paddle->GetLocalPosition().y - (paddle->GetLocalScale().y)) && ball->GetLocalPosition().x > min && ball->GetLocalPosition().x < max)
	{
		
		ballYSpeed = -ballYSpeed;
	}
	else if (ball->GetLocalPosition().y < -12.0)
	{
		ballYSpeed = -ballYSpeed;
		
	}
	
	return ballYSpeed;
}
float checkCollisionBrickY(Transform::sptr ball, Transform::sptr brick, float ballYSpeed) 
{
	float max, min;
	max = brick->GetLocalPosition().x + (brick->GetLocalScale().x);
	min = brick->GetLocalPosition().x + (brick->GetLocalScale().x);
	if (ball->GetLocalPosition().y >= (brick->GetLocalPosition().y - (brick->GetLocalScale().y)) && ball->GetLocalPosition().x > min && ball->GetLocalPosition().x < max)
	{

		ballYSpeed = -ballYSpeed;
	}


	return ballYSpeed;
	
}
float checkCollisionBrickX(Transform::sptr ball, Transform::sptr brick, float ballXSpeed)
{
	float max, min, middle;
	max = brick->GetLocalPosition().x + (brick->GetLocalScale().x);
	min = brick->GetLocalPosition().x - (brick->GetLocalScale().x);
	middle = max + min / 2;

	if (ball->GetLocalPosition().y >= (brick->GetLocalPosition().y - (brick->GetLocalScale().y)) && ball->GetLocalPosition().x > min && ball->GetLocalPosition().x < max)
	{
		if (ball->GetLocalPosition().x > middle)
			ballXSpeed = 0.0025;
		if (ball->GetLocalPosition().x < middle)
			ballXSpeed = -0.0025;
	}

	return ballXSpeed;
}

float checkCollisionBallXSpeed(Transform::sptr ball, Transform::sptr paddle, float ballXSpeed)
{
	float max, min, middle;
	max = paddle->GetLocalPosition().x + (paddle->GetLocalScale().x);
	min = paddle->GetLocalPosition().x - (paddle->GetLocalScale().x);
	middle = max + min / 2;

	if (ball->GetLocalPosition().y >= (paddle->GetLocalPosition().y - (paddle->GetLocalScale().y)) && ball->GetLocalPosition().x > min && ball->GetLocalPosition().x < max)
	{
		if (ball->GetLocalPosition().x > middle)
		ballXSpeed = 0.0025;
		if (ball->GetLocalPosition().x < middle)
			ballXSpeed = -0.0025;
	}

	if ((ball->GetLocalPosition().x < -3))
	{
		ballXSpeed = ballXSpeed*-1;
	}
	if ((ball->GetLocalPosition().x > 3))
	{
		ballXSpeed = ballXSpeed * -1;
	}
	else if (ball->GetLocalPosition().y >= (paddle->GetLocalPosition().y + (paddle->GetLocalScale().y * 2)))
	{
		ball->SetLocalPosition(0.0f, -0.01f, 0.0f);
		ballXSpeed = 0.0f;
	}

	return ballXSpeed;
}
void RenderVAO(
	const Shader::sptr& shader,
	const VertexArrayObject::sptr& vao,
	const Camera::sptr& camera,
	const Transform::sptr& transform)
{
	shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transform->LocalTransform());
	shader->SetUniformMatrix("u_Model", transform->LocalTransform());
	shader->SetUniformMatrix("u_NormalMatrix", transform->NormalMatrix());
	vao->Render();
}

struct Material
{
	Texture2D::sptr Albedo;
	Texture2D::sptr Specular;
	float           Shininess;
};

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);
		
	// Load our shaders
	Shader::sptr shader = Shader::Create();
	shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
	shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);  
	shader->Link();  

	glm::vec3 lightPos = glm::vec3(0.0f, 2.0f, 2.0f);
	glm::vec3 lightCol = glm::vec3(0.3f, 0.2f, 0.5f);
	float     lightAmbientPow = 0.5f;
	float     lightSpecularPow = 1.0f;
	glm::vec3 ambientCol = glm::vec3(1.0f);
	float     ambientPow = 0.5f;
	float     shininess = 4.0f;
	float     lightLinearFalloff = 0.09f;
	float     lightQuadraticFalloff = 0.032f;
	
	// These are our application / scene level uniforms that don't necessarily update
	// every frame
	shader->SetUniform("u_LightPos", lightPos);
	shader->SetUniform("u_LightCol", lightCol);
	shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
	shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
	shader->SetUniform("u_AmbientCol", ambientCol);
	shader->SetUniform("u_AmbientStrength", ambientPow);
	shader->SetUniform("u_Shininess", shininess);
	shader->SetUniform("u_LightAttenuationConstant", 1.0f);
	shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
	shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

	// GL states
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// NEW STUFF
	//
	static const int numB = 15;
	Transform::sptr transformB[numB];


	for (int b = 0; b < numB; b++)
	{
		transformB[b] = Transform::Create();

		if (b >= 0 && b < 5) 
		{
			transformB[b]->SetLocalPosition(-2.0f, -10.5f + b, 0.0f);
			transformB[b]->SetLocalScale(0.5f, 0.2f, 0.2f);
		}
		else if (b >= 5 && b < 10) 
		{
			transformB[b]->SetLocalPosition(0.0f, -10.5f + b - 5, 0.0f);
			transformB[b]->SetLocalScale(0.5f, 0.2f, 0.2f);
		}
		else if (b >= 10 && b < numB)
		{
			transformB[b]->SetLocalPosition(2.0f, -10.5f + b - 10, 0.0f);
			transformB[b]->SetLocalScale(0.5f, 0.2f, 0.2f);
		}
	
			
	}







	// Create some transforms and initialize them
	Transform::sptr transform[7];
	transform[0] = Transform::Create();
	transform[1] = Transform::Create();
	transform[2] = Transform::Create();
	transform[3] = Transform::Create();
	transform[4] = Transform::Create();
	transform[5] = Transform::Create();
	transform[6] = Transform::Create();

	transform[0]->SetLocalPosition(0.0f, 2.5f, 0.0f);
	transform[0]->SetLocalScale(0.8f, 0.2f, 0.5f);

	transform[1]->SetLocalScale(0.125f, 0.125f, 0.125f);
	float ballYSpeed = 0.0025f;
	float ballXSpeed = 0.0f;

	transform[2]->SetLocalScale(5.f, 25.f, 0.01f);

	transform[3]->SetLocalPosition(-4.f, 0.f, 0.10f);
	transform[3]->SetLocalScale(1.f, 15.f, 1.0f);

	transform[4]->SetLocalPosition(4.f, 0.f, 0.10f);
	transform[4]->SetLocalScale(1.f, 15.f, 1.0f);

	transform[5]->SetLocalPosition(0.f, -13.f, 0.10f);
	transform[5]->SetLocalScale(5.f, 1.f, 1.0f);

	transform[6]->SetLocalPosition(0.f, 0.f, -0.5f);
	transform[6]->SetLocalScale(25.f, 25.f, 0.0f);

	// We'll store all our VAOs into a nice array for easy access
		//VAOS
	VertexArrayObject::sptr vao0 = ObjLoader::LoadFromFile("Player.obj");
	VertexArrayObject::sptr vao1 = ObjLoader::LoadFromFile("ball.obj");
	VertexArrayObject::sptr vao2 = ObjLoader::LoadFromFile("wall.obj");

	VertexArrayObject::sptr vao[7];
	vao[0] = vao0;
	vao[1] = vao1;
	vao[2] = vao2;
	vao[3] = vao2;
	vao[4] = vao2;
	vao[5] = vao2;
	vao[6] = vao2;


	
	//For bricks
	VertexArrayObject::sptr vaoB0 = ObjLoader::LoadFromFile("Player.obj");


	VertexArrayObject::sptr vaoB[numB];

	for (int num = 0; num < numB; num++) 
	{
		vaoB[num] = vaoB0;

	
	}

	

	// TODO: load textures
	// Load our texture data from a file
	Texture2DData::sptr blueMap = Texture2DData::LoadFromFile("images/blue.png", true);
	Texture2DData::sptr diffuseMap = Texture2DData::LoadFromFile("images/sample.png");
	Texture2DData::sptr specularMap = Texture2DData::LoadFromFile("images/Stone_001_Specular.png");
	Texture2DData::sptr woodwallMap = Texture2DData::LoadFromFile("images/woodwall.png", true);
	Texture2DData::sptr yellowMap = Texture2DData::LoadFromFile("images/yellow.png", true);
	Texture2DData::sptr blackMap = Texture2DData::LoadFromFile("images/black.png", true);

	// Create a texture from the data
	Texture2D::sptr blue = Texture2D::Create();
	blue->LoadData(blueMap);
	Texture2D::sptr woodwall = Texture2D::Create();
	woodwall->LoadData(woodwallMap);
	Texture2D::sptr yellow = Texture2D::Create();
	yellow->LoadData(yellowMap);
	Texture2D::sptr black = Texture2D::Create();
	black->LoadData(blackMap);

	Texture2D::sptr diffuse = Texture2D::Create();
	diffuse->LoadData(diffuseMap);

	Texture2D::sptr specular = Texture2D::Create();
	specular->LoadData(specularMap);


	// Creating an empty texture
	Texture2DDescription desc = Texture2DDescription();
	desc.Width = 1;
	desc.Height = 1;
	desc.Format = InternalFormat::RGB8;
	Texture2D::sptr texture2 = Texture2D::Create(desc);
	texture2->Clear();

	// TODO: store some info about our materials for each object
	Material materials[7];
	materials[0].Albedo = woodwall;
	materials[0].Specular = specular;
	materials[0].Shininess = 40.0f;
	materials[1].Albedo = yellow;
	materials[1].Specular = specular;
	materials[1].Shininess = 16.0f;
	materials[2].Albedo = blue;
	materials[2].Specular = specular;
	materials[2].Shininess = 5.0f;
	materials[3].Albedo = woodwall;
	materials[3].Specular = specular;
	materials[3].Shininess = 16.0f;
	materials[4].Albedo = woodwall;
	materials[4].Specular = specular;
	materials[4].Shininess = 16.0f;
	materials[5].Albedo = woodwall;
	materials[5].Specular = specular;
	materials[5].Shininess = 16.0f;
	materials[6].Albedo = black;
	materials[6].Specular = specular;
	materials[6].Shininess = 16.0f;

	//Brick Materials
	Texture2DData::sptr brickMap = Texture2DData::LoadFromFile("images/brick.png");

	Texture2D::sptr brick = Texture2D::Create();
	brick->LoadData(brickMap);


	Material materialsBrick[1];
		materialsBrick[0].Albedo = brick;
		materialsBrick[0].Specular = specular;
		materialsBrick[0].Shininess = 16.0f;

	camera = Camera::Create();
	camera->SetPosition(glm::vec3(0, 2, 3)); // Set initial position
	camera->SetUp(glm::vec3(0, 0, 1)); // Use a z-up coordinate system
	camera->LookAt(glm::vec3(0.0f)); // Look at center of the screen
	camera->SetFovDegrees(90.0f); // Set an initial FOV
	camera->SetOrthoHeight(3.0f);
	
	// Our high-precision timer
	double lastFrame = glfwGetTime();
	
	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			if (transform[0]->GetLocalPosition().x <= 2)
				transform[0]->MoveLocal(0.001, 0, 0);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			//transform2 = glm::rotate(transform2, -0.001f, glm::vec3(0, 0, 1));
			if (transform[0]->GetLocalPosition().x >= -2)
				transform[0]->MoveLocal(-0.001, 0, 0);

		}

		

		glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->Bind();
		// These are the uniforms that update only once per frame
		shader->SetUniformMatrix("u_View", camera->GetView());
		shader->SetUniform("u_CamPos", camera->GetPosition());

		// Tell OpenGL that slot 0 will hold the diffuse, and slot 1 will hold the specular
		shader->SetUniform("s_Diffuse",  0);
		shader->SetUniform("s_Specular", 1); 


		ballYSpeed = checkCollisionBallYSpeed(transform[1], transform[0], ballYSpeed);
		ballXSpeed = checkCollisionBallXSpeed(transform[1], transform[0], ballXSpeed);
		for (int i = 0; i < numB; i++)
		{
			if (transform[1]->GetLocalPosition().x - transformB[i]->GetLocalPosition().x <=1 &&
				transform[1]->GetLocalPosition().y - transformB[i]->GetLocalPosition().y <= 1) 
			{
				ballYSpeed = checkCollisionBrickY(transform[1], transformB[i], ballYSpeed);
				ballXSpeed = checkCollisionBrickX(transform[1], transformB[i], ballXSpeed);
			}
		
		}
		//Ball
		transform[1]->MoveLocal(ballXSpeed, ballYSpeed, 0.f);

		// Render all VAOs in our scene
		for(int ix = 0; ix <= 6; ix++) {
			// TODO: Apply materials
			materials[ix].Albedo->Bind(0);
			materials[ix].Specular->Bind(1);
			shader->SetUniform("u_Shininess", materials[ix].Shininess);
			RenderVAO(shader, vao[ix], camera, transform[ix]);		
		}

		//Render all VAO for bricks in our scene
		for (int ixB = 0; ixB < numB; ixB++) 
		{
			// TODO: Apply materials
			materials[0].Albedo->Bind(0);
			materials[0].Specular->Bind(1);
			shader->SetUniform("u_Shininess", materials[0].Shininess);
			RenderVAO(shader, vaoB[ixB], camera, transformB[ixB]);
		
		}



		glfwSwapBuffers(window);
		lastFrame = thisFrame;
	}

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}