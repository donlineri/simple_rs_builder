#include "shader_s.h"
#include <glad/glad.h>
#include <stdio.h>
#include <string.h>

enum {
	max_file_size = 1024,
	max_info_log_size = 1024,
};

void get_str(char *str, FILE *file)
{
	int c, pos;
	pos = 0;
	while(((c = fgetc(file)) != EOF) && (pos < max_file_size - 1)) {
		str[pos] = c;
		pos++;
	}
	str[pos] = '\0';
	//printf("DEBUG: %s", str);
}

void check_compile_errors(unsigned int id, const char *type)
{
	int success;
	char info_log[max_info_log_size];
	if(strcmp("PROGRAM", type) != 0) {
		glGetShaderiv(id, GL_COMPILE_STATUS, &success);
		//printf("success: %d\n", success);
		if(!success) {
			glGetShaderInfoLog(id, max_info_log_size, NULL, info_log);
			fprintf(stderr, "ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n"
					" -- --------------------------------------------------- -- \n",
					type, info_log);
		}
	}
	else {
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if(!success) {
			glGetProgramInfoLog(id, max_info_log_size, NULL, info_log);
			fprintf(stderr, "ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n"
					" -- --------------------------------------------------- -- \n",
					type, info_log);
		}
	}
}

void shader_create(unsigned int *shader_id, const char *vertex_path,
		const char *fragment_path)
{
	char vertex_code[max_file_size], fragment_code[max_file_size];
	const char *vertex_ptr = vertex_code, *fragment_ptr = fragment_code;
	FILE *v_shader_file, *f_shader_file;
	unsigned int vertex, fragment;

	v_shader_file = fopen(vertex_path, "r");
	if(!v_shader_file)
		perror(vertex_path);

	f_shader_file = fopen(fragment_path, "r");
	if(!f_shader_file)
		perror(fragment_path);

	get_str(vertex_code, v_shader_file);
	get_str(fragment_code, f_shader_file);

	fclose(v_shader_file);
	fclose(f_shader_file);

	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertex_ptr, NULL);
	glCompileShader(vertex);
	check_compile_errors(vertex, "VERTEX");
	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragment_ptr, NULL);
	glCompileShader(fragment);
	check_compile_errors(fragment, "FRAGMENT");
	// shader Program
	*shader_id = glCreateProgram();
	glAttachShader(*shader_id, vertex);
	glAttachShader(*shader_id, fragment);
	glLinkProgram(*shader_id);
	check_compile_errors(*shader_id, "PROGRAM");
	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

// activate the shader
// ------------------------------------------------------------------------
void shader_use(unsigned int shader_id)
{ 
	glUseProgram(shader_id);
}
// utility uniform functions
// ------------------------------------------------------------------------
void shader_set_int(unsigned int shader_id, const char *name, int value)
{ 
	glUniform1i(glGetUniformLocation(shader_id, name), value);
}
// ------------------------------------------------------------------------
void shader_set_float(unsigned int shader_id, const char *name, float value)
{
	glUniform1f(glGetUniformLocation(shader_id, name), value);
}
