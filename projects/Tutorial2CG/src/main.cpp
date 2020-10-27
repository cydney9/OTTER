#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main()
{
	//Initialize GLFW
	if (glfwInit() == GLFW_FALSE)
	{
		std::cout << "Failed to initialize GLFW" << std::endl;
		return 1;

	}

	//Create a new GLFW window
	GLFWwindow* window = glfwCreateWindow(300, 300, "100749161", nullptr, nullptr);
	//We want GL commands to be executed for our window so we make our windows context the current one
	glfwMakeContextCurrent(window);


	//Let glad know what function loader we are using (will call gl commands via glfw)
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
	{
		std::cout << "Faied to initialize Glad" << std::endl;

	}

	std::cout << glGetString(GL_RENDERER) << std::endl;
	std::cout << glGetString(GL_VERSION) << std::endl;

	//Run as long as the window is open
	while (!glfwWindowShouldClose(window))
	{
		//poll for events from windows (clicks, keypressed, closing, all that)
		glClearColor(0.9f, 0.7f, 0.5f, 1.0f);//rgba
		glClear(GL_COLOR_BUFFER_BIT);


		//Present our image to windows
		glfwSwapBuffers(window);

	}



	return 0;




}
