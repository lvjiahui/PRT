#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "app.h"

class Timer
{
public:
	bool enable = true;
	template <typename Callable, typename... Args>
	decltype(auto) operator()(const std::string& func_name, Callable&& func, Args &&... args) {
		if (!enable)
			return std::invoke(func, std::forward<Args>(args)...);
		struct time {
			double start_time;
			const std::string& _name;
			time(const std::string& func_name) : _name(func_name), start_time(glfwGetTime()) {}
			~time() { fmt::print("time for {} : {}s\n", _name, glfwGetTime() - start_time); }
		} t(func_name);
		return std::invoke(func, std::forward<Args>(args)...);
	}
};
extern Timer timer;

class Platform {
public:
	Platform();
	~Platform();

	void loop(App& app);
	const unsigned int SCR_WIDTH = 1280;
	const unsigned int SCR_HEIGHT = 720;

private:
	void platform_shutdown();
	void begin_frame();
	void complete_frame();
	GLFWwindow* window = nullptr;

	// call back
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};