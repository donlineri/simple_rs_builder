#ifndef SPACE_HPP_SENTRY
#define SPACE_HPP_SENTRY
#include <glad/glad.h>
#include <cglm/call.h>
#include <GLFW/glfw3.h>

typedef struct character_s {
	unsigned int texture_id;  // ID handle of the glyph texture
	ivec2   size;             // Size of glyph
	ivec2   bearing;          // Offset from baseline to left/top of glyph
	unsigned int advance;     // Offset to advance to next glyph
} character;

typedef struct text_render_object_s {
	unsigned int VBO, VAO, shader_id;
	character *characters;
} text_render_object;

typedef struct coordinate_plane {
	GLFWwindow *window;
	unsigned int axes_VBO, axes_VAO, shader_id;
	text_render_object *tro;
	float clip, offset_x, offset_y;
} coordplane;

void coordplane_create(coordplane **plane);
void coordplane_process_input(coordplane *plane);
void coordplane_fill_with_color(float r, float g, float b);
void coordplane_shader_set_up(coordplane *plane);
void coordplane_draw_axes(coordplane *plane);
void coordplane_draw_numbering(coordplane *plane);
void coordplane_delete(coordplane *plane);
/*
 void coordplane_get_projmat(coordplane *plane, mat4 *projection);
 void coordplane_get_viewmat(coordplane *plane, mat4 *view);
*/

#endif
