#include "coordinate_plane.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "shader_s.h"

static const float axis_vertices[] = {
	//X-axis
	-1.0f, 0.0f, 0.0f, 0.5f, 0.41f, 0.33f,
	1.0f, 0.0f, 0.0f, 0.5f, 0.41f, 0.33f,
	1.0f, 0.0f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.9f, 0.2f, 0.0f, 0.5f, 0.41f, 0.33f,
	1.0f, 0.0f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.9f, -0.2f, 0.0f, 0.5f, 0.41f, 0.33f,
	//X-axis divisions
	0.0f, 0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.0f, -0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.5f, 0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.5f, -0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.25f, 0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.25f, -0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.75f, 0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	0.75f, -0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	-0.5f, 0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	-0.5f, -0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	-0.25f, 0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	-0.25f, -0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	-0.75f, 0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
	-0.75f, -0.1f, 0.0f, 0.5f, 0.41f, 0.33f,
};

//static Camera cam(glm::vec3(0.0f, 0.0f, 1.0f));

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void coordplane_process_input(coordplane *plane)
{
	int key_up, key_down, is_zoom_inc, is_zoom_dec;
	GLFWwindow *window;
	window = plane->window;
	key_up = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
	key_down = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
	is_zoom_inc =  key_up &&
		glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	is_zoom_dec = key_down &&
		glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	if(is_zoom_inc)
		plane->clip += 0.1f;
	else if(key_up)
		plane->offset_y += 0.1f;
	if(is_zoom_dec) {
		plane->clip -= 0.1f;
		if(plane->clip <= 0.1f)
			plane->clip = 0.1f;
	}
	else if(key_down)
		plane->offset_y -= 0.1f;
	if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		plane->offset_x += 0.1f;
	if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		plane->offset_x -= 0.1f;
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, 1);
}

static void prepare_window(GLFWwindow **window_ptr)
{
	GLFWwindow *window;
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

	window = glfwCreateWindow(700, 700, "graph", NULL, NULL);
	if(window == NULL) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		exit(4);
	}
	glfwMakeContextCurrent(window);

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize GLAD\n");
		exit(4);
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	*window_ptr = window;
}

static void render_text(text_render_object *text_render_obj, const char *text,
                 float x, float y, float scale, vec3 color)
{
	// activate corresponding render state
	shader_use(text_render_obj->shader_id);
	unsigned int textColorLoc = glGetUniformLocation(text_render_obj->shader_id,
                                                   "textColor");
	glUniform3f(textColorLoc, color[0], color[1], color[2]);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(text_render_obj->VAO);

	int c;
	// iterate through all characters
	//std::string::const_iterator c;
	//for (c = text.begin(); c != text.end(); c++)
	for(c = 0; text[c]; c++)
	{
		//Character ch = Characters[*c];
		int index = text[c];
		character ch = text_render_obj->characters[index];

		float xpos = x + ch.bearing[0] * scale;
		float ypos = y - (ch.size[1] - ch.bearing[1]) * scale;

		float w = ch.size[0] * scale;
		float h = ch.size[1] * scale;
		// update VBO for each character
		float vertices[6][4] = {
				{ xpos,     ypos + h,   0.0f, 0.0f },            
				{ xpos,     ypos,       0.0f, 1.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },

				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },
				{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.texture_id);
		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, text_render_obj->VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static int string_size(character *characters, const char *text)
{
	int c, result = 0;
	character ch;
	for(c = 0; text[c]; c++) {
		ch = characters[(int)text[c]];
		result += (ch.advance >> 6);
	}
	return result;
}

void get_aclip(coordplane *plane, float *aclip)
{
	int width, height;
	float width_height_ratio;
	glfwGetWindowSize(plane->window, &width, &height);
	width_height_ratio = (float) width / (float) height;
	if(width_height_ratio < 1.0) {
		aclip[0] = plane->clip*width_height_ratio;
		aclip[1] = plane->clip;
	}
	else {
		aclip[0] = plane->clip;
		aclip[1] = plane->clip/width_height_ratio;
	}
}

void coordplane_draw_numbering(coordplane *plane)
{
	int i, count_vertices = sizeof(axis_vertices)/sizeof(float);
	float magic, pos, aclip[2];
	char buf[50];
	mat4 model;
	vec3 a;
	text_render_object *tro;
	tro = plane->tro;

	get_aclip(plane, aclip);
	glmc_mat4_identity(model);
	a[0] = 0.1f*aclip[0];
	a[1] = 0.1f*aclip[1];
	a[2] = 1.0f;
	glmc_scale(model, a);
	a[0] = plane->offset_x/(0.1f*aclip[0]);
	a[1] = 0.0f;
	a[2] = 0.0f;
	glmc_translate(model, a);
	shader_use(tro->shader_id);
	shader_set_matrix(tro->shader_id, "model", model);

	magic = -0.5f*0.01f;
	a[0] = 0.5f;
	a[1] = 0.8f;
	a[2] = 0.2f;
	for(i = 6*6; i < count_vertices; i += 2*6) {
		pos = axis_vertices[i];
		snprintf(buf, 50, "%.2f", plane->offset_x+pos*aclip[0]);
		render_text(tro, buf, 10.0f*pos+magic*string_size(tro->characters, buf),
				-0.55, 0.01f, a);
	}

	glmc_mat4_identity(model);
	a[0] = 0.1f*aclip[0];
	a[1] = 0.1f*aclip[1];
	a[2] = 1.0f;
	glmc_scale(model, a);
	a[0] = 0.0f;
	a[1] = plane->offset_y/(0.1f*aclip[1]);
	a[2] = 0.0f;
	glmc_translate(model, a);
	shader_set_matrix(tro->shader_id, "model", model);
	magic = -0.5f*tro->characters[48].size[1]*0.01f;
	a[0] = 0.5f;
	a[1] = 0.8f;
	a[2] = 0.2f;
	for(i = 6*6; i < count_vertices; i += 2*6) {
		pos = axis_vertices[i];
		snprintf(buf, 50, "%.2f", plane->offset_y+pos*aclip[1]);
		render_text(tro, buf, 0.35, 10.0f*pos+magic, 0.01f, a);
	}
}

void gen_characters(character **characters)
{
	FT_Library ft;
	if(FT_Init_FreeType(&ft))
	{
		printf("ERROR::FREETYPE: Could not init FreeType Library\n");
		exit(1);
	}

	FT_Face face;
	if(FT_New_Face(ft, "font/DejaVuSans.ttf", 0, &face))
	{
		printf("ERROR::FREETYPE: Failed to load font\n");
		exit(1);
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
																				 //
	character *chars = (character *) malloc(sizeof(character)*128);
	unsigned char c;
	for(c = 0; c < 128; c++)
	{
		// load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			printf("ERROR::FREETYTPE: Failed to load Glyph %d\n", c);
			continue;
		}
		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// now store character for later use
		chars[c].texture_id = texture;
		chars[c].size[0] = face->glyph->bitmap.width;
		chars[c].size[1] = face->glyph->bitmap.rows;
		chars[c].bearing[0] = face->glyph->bitmap_left;
		chars[c].bearing[1] = face->glyph->bitmap_top;
		chars[c].advance = (unsigned int) face->glyph->advance.x;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	*characters = chars;
}

void prepare_text(text_render_object *text_render_obj)
{
	character *characters;
	gen_characters(&characters);
	text_render_obj->characters = characters;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	shader_create(&text_render_obj->shader_id, "text.vs", "text.fs");

	unsigned int text_VBO, text_VAO;
	glGenVertexArrays(1, &text_VAO);
	glGenBuffers(1, &text_VBO);
	glBindVertexArray(text_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	text_render_obj->VBO = text_VBO;
	text_render_obj->VAO = text_VAO;
}

void prepare_axes(unsigned int *axes_VBO, unsigned int *axes_VAO)
{
	glGenVertexArrays(1, axes_VAO);
	glGenBuffers(1, axes_VBO);
	glBindVertexArray(*axes_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, *axes_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axis_vertices), axis_vertices,
			GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float),
			(void *)0);
	glEnableVertexAttribArray(0); //0 = Location in Vertex shader
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float),
			(void *)(3*sizeof(float)));
	glEnableVertexAttribArray(1); //1 = layout (Location) in Vertex shader
	//no needed VBO bind to GL_ARRAY_BUFFER now
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//unbind VAO
	glBindVertexArray(0);
}

void coordplane_create(coordplane **plane)
{
	*plane = malloc(sizeof(coordplane));
	prepare_window(&(*plane)->window);
	prepare_axes(&(*plane)->axes_VBO, &(*plane)->axes_VAO);
	shader_create(&(*plane)->shader_id, "shader.vs", "shader.fs");
	(*plane)->tro = malloc(sizeof(text_render_object));
	prepare_text((*plane)->tro);
	(*plane)->offset_x = 0.0f;
	(*plane)->offset_y = 0.0f;
	(*plane)->clip = 10.0f;
}

void coordplane_fill_with_color(float r, float g, float b)
{
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void coordplane_shader_set_up(coordplane *plane)
{
	float aclip[2];
	mat4 projection, view, model;
	vec3 a;

	get_aclip(plane, aclip);
	glmc_ortho(-aclip[0], aclip[0], -aclip[1], aclip[1], 0.0f, 1.0f, projection);
	glmc_mat4_identity(view);
	a[0] = -plane->offset_x;
	a[1] = -plane->offset_y;
	a[2] = -1.0f;
	glmc_translate(view, a);
	glmc_mat4_identity(model);

	shader_use(plane->shader_id);
	shader_set_matrix(plane->shader_id, "projection", projection);
	shader_set_matrix(plane->shader_id, "view", view);
	shader_set_matrix(plane->shader_id, "model", model);
	shader_use(plane->tro->shader_id);
	shader_set_matrix(plane->tro->shader_id, "projection", projection);
	shader_set_matrix(plane->tro->shader_id, "view", view);
}

void coordplane_draw_axes(coordplane *plane)
{
	float save_line_width, aclip[2];
	mat4 model1, model2;
	vec3 a;
	glGetFloatv(GL_LINE_WIDTH, &save_line_width);
	glLineWidth(2.0f);
	glBindVertexArray(plane->axes_VAO);
	get_aclip(plane, aclip);
	glmc_mat4_identity(model1);
	glmc_mat4_identity(model2);
	a[0] = 1.0f*aclip[0];
	a[1] = 0.1f*aclip[1];
	a[2] = 1.0f;
	glmc_scale(model1, a);
	a[0] = plane->offset_x/aclip[0];
	a[1] = 0.0f;
	a[2] = 0.0f;
	glmc_translate(model1, a);
	a[0] = 0.1f*aclip[0];
	a[1] = 1.0f*aclip[1];
	a[2] = 1.0f;
	glmc_scale(model2, a);
	a[0] = 0.0f;
	a[1] = plane->offset_y/aclip[1];
	a[2] = 0.0f;
	glmc_translate(model2, a);
	a[0] = 0.0f;
	a[1] = 0.0f;
	a[2] = 1.0f;
	glmc_rotate(model2, glm_rad(90.0f), a);
	shader_use(plane->shader_id);
	shader_set_matrix(plane->shader_id, "model", model1);
	glDrawArrays(GL_LINES, 0, 20);
	shader_set_matrix(plane->shader_id, "model", model2);
	glDrawArrays(GL_LINES, 0, 20);
	glmc_mat4_identity(model1);
	shader_set_matrix(plane->shader_id, "model", model1);
	glLineWidth(save_line_width);
}

void coordplane_delete(coordplane *plane)
{
	int i;
	glDeleteProgram(plane->shader_id);
	glDeleteProgram(plane->tro->shader_id);
	for(i = 0; i < 128; i++)
		glDeleteTextures(1, &plane->tro->characters[i].texture_id);
	free(plane->tro->characters);
	glDeleteVertexArrays(1, &plane->axes_VAO);
	glDeleteBuffers(1, &plane->axes_VBO);
	glDeleteVertexArrays(1, &plane->tro->VAO);
	glDeleteBuffers(1, &plane->tro->VBO);
	free(plane->tro);
	glfwTerminate();
	free(plane);
}
