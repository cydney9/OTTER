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
#include "Utilities/NotObjLoader.h"
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
	window = glfwCreateWindow(800, 800, "Brick Breaker", nullptr, nullptr);
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

void checkCollision(Transform::sptr ball, Transform::sptr paddle)
{
	if (ball->GetLocalPosition().y >= (paddle->GetLocalPosition().y + (paddle->GetLocalScale().y / 2)))
	{
		ball->SetLocalPosition(0.0f, -0.5f, 0.0f);
	}
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

void ManipulateTransformWithInput(const Transform::sptr& transform, float dt) {
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		transform->MoveLocal(0.0f, -1.0f * dt, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { 
		transform->MoveLocal(0.0f,  1.0f * dt, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		transform->MoveLocal(-1.0f * dt, 0.0f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		transform->MoveLocal( 1.0f * dt, 0.0f,  0.0f); 
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		transform->MoveLocal(0.0f, 0.0f,  1.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		transform->MoveLocal(0.0f, 0.0f, -1.0f * dt);
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { 
		transform->RotateLocal(0.0f, -45.0f * dt, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		transform->RotateLocal(0.0f,  45.0f * dt,0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		transform->RotateLocal( 45.0f * dt, 0.0f,0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		transform->RotateLocal(-45.0f * dt, 0.0f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		transform->RotateLocal(0.0f, 0.0f, 45.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		transform->RotateLocal(0.0f, 0.0f, -45.0f * dt);
	}
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

	glm::vec3 lightPos = glm::vec3(4.0f, 6.0f, 2.0f);
	glm::vec3 lightCol = glm::vec3(0.3f, 0.2f, 0.5f);
	float     lightAmbientPow = 1.0f;
	float     lightSpecularPow = 1.0f;
	glm::vec3 ambientCol = glm::vec3(1.0f);
	float     ambientPow = 0.7f;
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

	// Create some transforms and initialize them
	Transform::sptr transform[3];
	transform[0] = Transform::Create();
	transform[1] = Transform::Create();
	transform[2] = Transform::Create();

	transform[0]->SetLocalPosition(0.0f, 2.5f, 0.0f);
	transform[0]->SetLocalScale(0.8f, 0.2f, 0.5f);

	transform[1]->SetLocalScale(0.125f, 0.125f, 0.125f);
	float ballPosX = 0.25f;

	transform[2]->SetLocalScale(100.f, 100.f, 0.01f);

	// We'll store all our VAOs into a nice array for easy access
		//VAOS
	VertexArrayObject::sptr vao0 = ObjLoader::LoadFromFile("Player.obj");
	VertexArrayObject::sptr vao1 = ObjLoader::LoadFromFile("ball.obj");
	VertexArrayObject::sptr vao2 = ObjLoader::LoadFromFile("wall.obj");

	VertexArrayObject::sptr vao[3];
	vao[0] = vao0;
	vao[1] = vao1;
	vao[2] = vao2;

	// TODO: load textures
	// Load our texture data from a file
	Texture2DData::sptr diffuseMap = Texture2DData::LoadFromFile("images/Stone_001_Diffuse.png");
	Texture2DData::sptr specularMap = Texture2DData::LoadFromFile("images/Stone_001_Specular.png");
	// Create a texture from the data
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
	Material materials[3];
	materials[0].Albedo = diffuse;
	materials[0].Specular = specular;
	materials[0].Shininess = 40.0f;
	materials[1].Albedo = diffuse;
	materials[1].Specular = specular;
	materials[1].Shininess = 16.0f;
	materials[2].Albedo = diffuse;
	materials[2].Specular = specular;
	materials[2].Shininess = 5.0f;

	
	camera = Camera::Create();
	camera->SetPosition(glm::vec3(0, 3, 3)); // Set initial position
	camera->SetUp(glm::vec3(0, 0, 1)); // Use a z-up coordinate system
	camera->LookAt(glm::vec3(0.0f)); // Look at center of the screen
	camera->SetFovDegrees(90.0f); // Set an initial FOV
	camera->SetOrthoHeight(3.0f);

	// We'll use a vector to store all our key press events for now
	std::vector<KeyPressWatcher> keyToggles;
	// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
	// how this is implemented. Note that the ampersand here is capturing the variables within
	// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
	// use std::bind
	keyToggles.emplace_back(GLFW_KEY_T, [&](){ camera->ToggleOrtho(); });

	int selectedVao = 2; // select cube by default
	keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
		selectedVao++;
		if (selectedVao >= 4)
			selectedVao = 1;
	});
	keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
		selectedVao--;
		if (selectedVao <= 0)
			selectedVao = 3;
	});

		
	// Our high-precision timer
	double lastFrame = glfwGetTime();
	
	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Calculate the time since our last frame (dt)
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

		glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->Bind();
		// These are the uniforms that update only once per frame
		shader->SetUniformMatrix("u_View", camera->GetView());
		shader->SetUniform("u_CamPos", camera->GetPosition());

		// Tell OpenGL that slot 0 will hold the diffuse, and slot 1 will hold the specular
		shader->SetUniform("s_Diffuse",  0);
		shader->SetUniform("s_Specular", 1); 

		//Ball
		transform[1]->MoveLocal(0.0f, 0.0005f, 0.f);

		checkCollision(transform[1], transform[0]);

		// Render all VAOs in our scene
		for(int ix = 0; ix < 3; ix++) {
			// TODO: Apply materials
			materials[ix].Albedo->Bind(0);
			materials[ix].Specular->Bind(1);
			shader->SetUniform("u_Shininess", materials[ix].Shininess);
			RenderVAO(shader, vao[ix], camera, transform[ix]);		
		}

		glfwSwapBuffers(window);
		lastFrame = thisFrame;
	}

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}