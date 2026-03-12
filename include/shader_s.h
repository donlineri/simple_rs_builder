#ifndef SHADER_S_H_SENTRY
#define SHADER_S_H_SENTRY
#include <cglm/call.h>

#ifdef __cplusplus
extern "C" {
#endif

void shader_create(unsigned int *shader_id, const char *vertex_path,
		const char *fragment_path);

void shader_use(unsigned int shader_id);

void shader_set_int(unsigned int shader_id, const char *name, int value);

void shader_set_float(unsigned int shader_id, const char *name, float value);

void shader_set_matrix(unsigned int shader_id, const char *locname, mat4 mat);

#ifdef __cplusplus
}
#endif

#endif
