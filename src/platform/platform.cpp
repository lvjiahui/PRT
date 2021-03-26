#include "platform.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

Timer timer;


//whenever the window size changed (by OS or user resize) this callback function executes
void Platform::framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void Platform::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;
	auto *app = (App *)glfwGetWindowUserPointer(window);
	auto &camera = app->camera;

	float xoffset = xpos - camera.lastX;
	float yoffset = camera.lastY - ypos; // reversed since y-coordinates go from bottom to top

	camera.lastX = xpos;
	camera.lastY = ypos;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
	{
		return;
	}
	camera.ProcessMouseMovement(xoffset, yoffset);
}

//whenever the mouse scroll wheel scrolls, this callback is called
void Platform::scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;
	App *app = (App *)glfwGetWindowUserPointer(window);
	auto &camera = app->camera;
	camera.ProcessMouseScroll(yoffset);
}

void processInput(GLFWwindow* window)
{
	auto* app = (App*)glfwGetWindowUserPointer(window);
	auto& camera = app->camera;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	auto deltaTime = ImGui::GetIO().DeltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

Platform::Platform() { 
	if (!glfwInit())
	{
		fmt::print("glfw init failure\n");
	}
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PRT", NULL, NULL);
	glfwMakeContextCurrent(window);

	glfwSwapInterval(1); // Enable vsync
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

	if (!gladLoadGL())
	{
		fmt::print("glad init failure\n");
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	glEnable(GL_DEPTH_TEST);

}

Platform::~Platform() { platform_shutdown(); }


void Platform::platform_shutdown()
{
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Platform::begin_frame()
{
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Platform::complete_frame()
{

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(window);
}

void Platform::loop(App &app)
{
	glfwSetWindowUserPointer(window, &app); //for call back function

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
        processInput(window);
		glfwPollEvents();

		begin_frame();

		app.render();

		complete_frame();
	}
}
