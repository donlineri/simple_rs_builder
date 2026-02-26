#include "space.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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

static float offset_x = 0.0f, offset_y = 0.0f, clip_x = 10.0f, clip_y = 10.0f;
static float width_height_ratio = 1.0f;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	float width_d = width, height_d = height;
	width_height_ratio = width_d / height_d;
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
	int key_up = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
	int key_down = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
	int is_zoom_inc =  key_up &&
		glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	int is_zoom_dec = key_down &&
		glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	if(is_zoom_inc) {
		clip_y += 0.1f;
		clip_x += 0.1f;
	}
	else if(key_up)
		offset_y += 0.1f;
	if(is_zoom_dec) {
		clip_y -= 0.1f;
		clip_x -= 0.1f;
		if(clip_y <= 0.1f) {
			clip_y = 0.1f;
			clip_x = 0.1f;
		}
	}
	else if(key_down)
		offset_y -= 0.1f;
	if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		offset_x += 0.1f;
	if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		offset_x -= 0.1f;
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, 1);
}

static void prepare_window(GLFWwindow **window_ptr)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(700, 700, "graph", NULL, NULL);
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
                 float x, float y, float scale, glm::vec3 color)
{
	// activate corresponding render state
	text_render_obj->shader->use();
	unsigned int textColorLoc = glGetUniformLocation(text_render_obj->shader->ID,
                                                   "textColor");
	glUniform3f(textColorLoc, color.x, color.y, color.z);
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

		float xpos = x + ch.bearing.x * scale;
		float ypos = y - (ch.size.y - ch.bearing.y) * scale;

		float w = ch.size.x * scale;
		float h = ch.size.y * scale;
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

static void set_matrix(Shader *shader, const char *locname, glm::mat4 *mat)
{
	unsigned int loc;
	loc = glGetUniformLocation(shader->ID, locname);
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(*mat));
}

void draw_axis(unsigned int axes_VAO, Shader *shader, float aclip_x,
		float aclip_y, int is_x)
{
	glm::mat4 e_mat = glm::mat4(1.0f);
	glm::mat4 model;

	if(is_x) {
		model = glm::scale(e_mat, glm::vec3(1.0f*aclip_x, 0.1f*aclip_y, 1.0f));
		model = glm::translate(model, glm::vec3(offset_x/aclip_x, 0.0f, 0.0f));
	}
	else {
		model = glm::scale(e_mat, glm::vec3(0.1f*aclip_x, 1.0f*aclip_y, 1.0f));
		model = glm::translate(model, glm::vec3(0.0f, offset_y/aclip_y, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	shader->use();
	set_matrix(shader, "model", &model);

	glDrawArrays(GL_LINES, 0, 20);
}

void draw_numbering(text_render_object *text_render_obj, float aclip_x,
		float aclip_y)
{
	int i, count_vertices = sizeof(axis_vertices)/sizeof(float);
	float magic, pos;
	char buf[50];
	glm::mat4 model;
	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.1f*aclip_x, 0.1f*aclip_y, 1.0f));
	model = glm::translate(model, glm::vec3(offset_x/(0.1f*aclip_x),
				0.0f, 0.0f));
	text_render_obj->shader->use();
	set_matrix(text_render_obj->shader, "model", &model);

	magic = -0.5f*0.01f;
	for(i = 6*6; i < count_vertices; i += 2*6) {
		pos = axis_vertices[i];
		snprintf(buf, 50, "%.2f", offset_x+pos*aclip_x);
		render_text(text_render_obj, buf,
				10.0f*pos+magic*string_size(text_render_obj->characters, buf), -0.55,
				0.01f, glm::vec3(0.5f, 0.8f, 0.2f));
	}

	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.1f*aclip_x, 0.1f*aclip_y, 1.0f));
	model = glm::translate(model, glm::vec3(0.0f, offset_y/(0.1f*aclip_y),
				0.0f));
	set_matrix(text_render_obj->shader, "model", &model);
	magic = -0.5f*text_render_obj->characters[48].size.y*0.01f;
	for(i = 6*6; i < count_vertices; i += 2*6) {
		pos = axis_vertices[i];
		snprintf(buf, 50, "%.2f", offset_y+pos*aclip_y);
		render_text(text_render_obj, buf, 0.35,
				10.0f*pos+magic,
				0.01f, glm::vec3(0.5f, 0.8f, 0.2f));
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
		//Characters[c] = malloc(sizeof(struct Character));
		//Character *character = malloc(sizeof(Character));
		chars[c] = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			(unsigned int) face->glyph->advance.x
		};
		//Characters[c] = character;
		//Characters.insert(std::pair<char, Character>(c, character));
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

	Shader *text_shader = new Shader("text.vs", "text.fs");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	text_render_obj->shader = text_shader;

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

void prepare_plane(space **plane)
{
	*plane = (space *) malloc(sizeof(space));
	prepare_window(&(*plane)->window);
	prepare_axes(&(*plane)->axes_VBO, &(*plane)->axes_VAO);
	(*plane)->shader = new Shader("shader.vs", "shader.fs");
	(*plane)->text_render_obj = (text_render_object *) malloc(
			sizeof(text_render_object));
	prepare_text((*plane)->text_render_obj);
}

void draw_graph(space *plane,
		void (*draw_something)(unsigned int, int, Shader*),
		unsigned int VAO, int count_vertices)
{
	glm::mat4 projection, view;
	float aclip_x, aclip_y;
	printf("cp_count:%d\n", count_vertices);
	while(!glfwWindowShouldClose(plane->window))
	{
		processInput(plane->window);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		if(width_height_ratio < 1.0) {
			aclip_x = clip_x*width_height_ratio;
			aclip_y = clip_y;
		}
		else {
			aclip_x = clip_x;
			aclip_y = clip_y/width_height_ratio;
		}

		/*
		projection = glm::ortho(-aclip_x+offset_x, aclip_x+offset_x,
				-aclip_y+offset_y, aclip_y+offset_y, 0.0f, 1.0f);
		*/
		projection = glm::ortho(-aclip_x, aclip_x, -aclip_y, aclip_y, 0.0f, 1.0f);
		plane->shader->use();
		set_matrix(plane->shader, "projection", &projection);
		view = glm::mat4(1.0f);
		view = glm::translate(view, glm::vec3(-offset_x, -offset_y, -1.0f));
		set_matrix(plane->shader, "view", &view);
		glBindVertexArray(plane->axes_VAO);
		glLineWidth(2.0f);
		draw_axis(plane->axes_VAO, plane->shader, aclip_x, aclip_y, 1);
		draw_axis(plane->axes_VAO, plane->shader, aclip_x, aclip_y, 0);
		glLineWidth(0.0f);
		draw_something(VAO, count_vertices, plane->shader);
		plane->text_render_obj->shader->use();
		set_matrix(plane->text_render_obj->shader, "projection", &projection);
		set_matrix(plane->text_render_obj->shader, "view", &view);
		draw_numbering(plane->text_render_obj, aclip_x, aclip_y);

		glfwSwapBuffers(plane->window);
		glfwPollEvents();
	}
}

void delete_plane(space *plane)
{
	delete plane->shader;
	delete plane->text_render_obj->shader;
	free(plane->text_render_obj->characters);
	glDeleteVertexArrays(1, &plane->axes_VAO);
	glDeleteBuffers(1, &plane->axes_VBO);
	glDeleteVertexArrays(1, &plane->text_render_obj->VAO);
	glDeleteBuffers(1, &plane->text_render_obj->VBO);
	free(plane->text_render_obj);
	glfwTerminate();
	free(plane);
}
