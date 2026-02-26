#ifndef SPACE_HPP_SENTRY
#define SPACE_HPP_SENTRY
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader_s.h"
#include <GLFW/glfw3.h>

typedef struct character_s {
	unsigned int texture_id;  // ID handle of the glyph texture
	glm::ivec2   size;       // Size of glyph
	glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
	unsigned int advance;    // Offset to advance to next glyph
} character;

typedef struct text_render_object_s {
	unsigned int VBO, VAO;
	Shader *shader;
	character *characters;
} text_render_object;

typedef struct euclidean_space {
	GLFWwindow *window;
	unsigned int axes_VBO, axes_VAO;
	Shader *shader;
	text_render_object *text_render_obj;
} space;

void prepare_plane(space **plane);

void draw_graph(space *plane,
		void (*draw_something)(unsigned int, int, Shader*),
		unsigned int VAO, int count_vertices);

void delete_plane(space *plane);

#endif
