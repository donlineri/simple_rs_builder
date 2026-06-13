#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <cglm/call.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "setoper.h"
#include "cdd.h"
#include "tinyexpr.h"
#include "coordinate_plane.h"
#include "shader_s.h"

#define ALMOSTZERO 0.00001

enum {
	max_length_line = 255,
};

typedef struct func2_s {
	te_expr *expr;
	double x, y;
} func2;

typedef struct func1_s {
	te_expr *expr;
	double t;
} func1;

typedef struct problem_s {
	func1 **a;
	double *x0, t, **phi;
	int n;
	func2 c_u, f;
} problem;

void draw_cp(unsigned int VAO, int count_vertices, unsigned int shader_id)
{
	shader_use(shader_id);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, count_vertices);
}

void draw_dots(coordplane *plane, unsigned int VAO, vec3 *pos, int pos_count,
		unsigned int shader_id)
{
	int i, width, height;
	float aclip[2];
	mat4 model;
	vec3 a;
	shader_use(shader_id);
	glBindVertexArray(VAO);
	glfwGetWindowSize(plane->window, &width, &height);
	coordplane_get_aclip(plane, aclip);
	a[0] = 5*aclip[0]/width;
	a[1] = 5*aclip[1]/height;
	a[2] = 1.0f;
	for(i = 0; i < pos_count; i++) {
		glmc_mat4_identity(model);
		glmc_translate(model, pos[i]);
		glmc_scale(model, a);
		shader_set_matrix(shader_id, "model", model);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

void swap_double(double *a, double *b)
{
	double t;
	t = *a;
	*a = *b;
	*b = t;
}

void swap_int(int *a, int *b)
{
	int t;
	t = *a;
	*a = *b;
	*b = t;
}

void sort(int *index, double *value, int size)
{
	int i, j;
	for(j = size; j > 1; j--)
		for(i = 1; i < j; i++)
			if(value[i-1] > value[i]) {
				swap_double(&value[i-1], &value[i]);
				swap_int(&index[i-1], &index[i]);
			}
}

void fill_cp_vertices_in_correct_order(dd_MatrixPtr G, double *cp_vertices)
{
  int i, rowsize;
	double centroidX = 0.0, centroidY = 0.0, *cp_angle_vertices;
	int *cp_number_vertices;
	rowsize = G->rowsize;
	cp_angle_vertices = (double *) malloc(sizeof(double)*rowsize);
	cp_number_vertices = (int *) malloc(sizeof(int)*rowsize);
	for(i = 0; i < rowsize; i++) {
		centroidX += dd_get_d(G->matrix[i][1]);
		centroidY += dd_get_d(G->matrix[i][2]);
	}
	centroidX /= rowsize;
	centroidY /= rowsize;
	for(i = 0; i < rowsize; i++) {
		cp_number_vertices[i] = i;
		cp_angle_vertices[i] = atan2(dd_get_d(G->matrix[i][2]) - centroidY,
				dd_get_d(G->matrix[i][1]) - centroidX);
	}
	sort(cp_number_vertices, cp_angle_vertices, rowsize);
  for(i = 0; i < rowsize; i++) {
		cp_vertices[i*6] = dd_get_d(G->matrix[cp_number_vertices[i]][1]);
		cp_vertices[i*6+1] = dd_get_d(G->matrix[cp_number_vertices[i]][2]);
		cp_vertices[i*6+2] = 0.0;
		cp_vertices[i*6+3] = 1.0;
		cp_vertices[i*6+4] = 0.5;
		cp_vertices[i*6+5] = 0.2;
  }
	free(cp_angle_vertices);
	free(cp_number_vertices);
}

void get_cp_vertices(double **omega, int omega_size, double **cp_vertices,
		int *cp_count_vertices)
{
	int i, j;
  dd_ErrorType error=dd_NoError;
  dd_MatrixPtr M, G;

  dd_PolyhedraPtr poly;
	M = dd_CreateMatrix(omega_size, 3);
  M->representation = dd_Inequality;
	for(i = 0; i < omega_size; i++)
		for(j = 0; j < 3; j++)
			dd_set_d(M->matrix[i][j], omega[i][j]);

	poly = dd_DDMatrix2Poly(M, &error);
	if(error != dd_NoError) {
  	dd_WriteErrorMessages(stderr, error);
		exit(1);
	}
	G = dd_CopyGenerators(poly);
	*cp_vertices = malloc(sizeof(double)*(G->rowsize * 6));
	*cp_count_vertices = G->rowsize;
	fill_cp_vertices_in_correct_order(G, *cp_vertices);

  dd_FreePolyhedra(poly);
  dd_FreeMatrix(M);
  dd_FreeMatrix(G);
}

void prepare_object(unsigned int *VBO, unsigned int *VAO, int count_vertices,
		const double *vertices)
{
	glGenVertexArrays(1, VAO);
	glGenBuffers(1, VBO);
	glBindVertexArray(*VAO);
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(double)*(count_vertices)*6,
			vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 6*sizeof(double),
			(void *)0);
	glEnableVertexAttribArray(0); //0 = Location in Vertex shader
	glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, 6*sizeof(double),
			(void *)(3*sizeof(double)));
	glEnableVertexAttribArray(1); //1 = layout (Location) in Vertex shader
	//no needed VBO bind to GL_ARRAY_BUFFER now
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//unbind VAO
	glBindVertexArray(0);
}

void delete_object(unsigned int VBO, unsigned int VAO)
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void printmat(double **a, int size)
{
  int x, y;
  for(x = 0; x < size; x++) {
    for(y = 0; y < size; y++)
      printf("%10lf ", a[x][y]);
    printf("\n");
  }
}

void printvec(double *b, int size)
{
  int x;
  for(x = 0; x < size; x++)
    printf("%10lf\n", b[x]);
}

double max(double x, double y)
{
	if(x > y)
		return x;
	else
		return y;
}

char *get_stroka(char *buf)
{
	char *s;
	s = fgets(buf, max_length_line, stdin);
	if(!s) {
    fprintf(stderr, "Error: empty\n");
    exit(1);
	}
	return s;
}

te_expr *get_func(char *buf, te_variable *vars, int te_vars_count)
{
	int err;
	char *s;
	te_expr *expr = NULL;
	while(!expr) {
		s = get_stroka(buf);
		while(isspace(*s))
			s++;
		if(strlen(s) == 0)
			continue;
		if(*s == '#')
			continue;
		expr = te_compile(s, vars, te_vars_count, &err);
		if(!expr) {
			fprintf(stderr, "Parse error at %d\n", err);
		}
	}
	return expr;
}

void parse_input(func1 ***a, double **x0, double *t, int *n, func2 *c_u,
		func2 *f)
{
	const int dim = 2;
	const int num_count = 4;
	int i, j, pos, te_vars_count_c_u, te_vars_count_aij, te_vars_count_f;
	char input[max_length_line], *s, *endptr;
	double data[num_count];
	te_variable vars_c_u[] = {{"max", (void*) max,
		TE_FUNCTION2}, {"phi1", &c_u->x}, {"phi2", &c_u->y}};
	(*a) = malloc(sizeof(func1 *) * dim);
	(*a)[0] = malloc(sizeof(func1) * dim);
	(*a)[1] = malloc(sizeof(func1) * dim);
	te_variable vars_aij[2][2][2] = {{{{"t", &(*a)[0][0].t}},
		{{"t", &(*a)[1][0].t}}}, {{{"t", &(*a)[0][1].t}}, {{"t", &(*a)[1][1].t}}}};
	te_variable vars_f[] = {{"x", &f->x}, {"y", &f->y}};
	te_vars_count_c_u = 3;
	te_vars_count_aij = 1;
	te_vars_count_f = 2;
	for(i = 0; i < dim; i++)
		for(j = 0; j < dim; j++) {
			(*a)[j][i].expr = get_func(input, vars_aij[i][j], te_vars_count_aij);
		}
	*x0 = malloc(sizeof(double)*dim);
	pos = 0;
	if(isatty(0))
		printf("Enter A (4 function(t)), x0 (2), T (1), N (1), c_u"
				"(function(phi1,phi2))\n");
	s = get_stroka(input);
	while(pos < num_count) {
		data[pos] = strtod(s, &endptr);
		if(endptr == s) {
			s = get_stroka(input);
		} else {
      pos++;
      s = endptr;
		}
	}

	c_u->expr = get_func(input, vars_c_u, te_vars_count_c_u);
	f->expr = get_func(input, vars_f, te_vars_count_f);
	
	/*
	for(i = 0; i < dim; i++)
		for(j = 0; j < dim; j++)
			(*a)[i][j] = data[i*dim + j];
			*/
	(*x0)[0] = data[0];
	(*x0)[1] = data[1];
	*t = data[2];
	*n = (int) data[3];
}

void freemat(double **m, int size)
{
  int i;
  for(i = 0; i < size; i++)
    free(m[i]);
  free(m);
}

void print_input(func1 ***a, double **x0, double *t, int *n, func2 *c_u,
		func2 *f)
{
	int i, j;
	printf("MATRIX A\n");
	for(i = 0; i < 2; i++)
		for(j = 0; j < 2; j++) {
			(*a)[i][j].t = 1.0;
			printf("a[%d][%d](%lf) = %lf\n", i, j, (*a)[i][j].t, te_eval((*a)[i][j].expr));
		}
	(*c_u).x = 1;
	(*c_u).y = 1;
	printf("c_u(%lf,%lf) = %lf\n", (*c_u).x, (*c_u).y, te_eval((*c_u).expr));
	(*c_u).x = 3;
	(*c_u).y = -2;
	printf("c_u(%lf,%lf) = %lf\n", (*c_u).x, (*c_u).y, te_eval((*c_u).expr));
	(*f).x = 1;
	(*f).y = 1;
	printf("f(%lf,%lf) = %lf\n", (*f).x, (*f).y, te_eval((*f).expr));
	printf("VECTOR x0\n");
	printvec(*x0, 2);
	printf("T = %lf\n", *t);
	printf("N = %d\n", *n);
}

double dot_product(double *x1, double *x2, int size)
{
  int i;
  double s = 0.0;
  for(i = 0; i < size; i++)
    s += x1[i] * x2[i];
  return s;
}

double dp_t(func1 *a_t_i, double t, double psi1, double psi2)
{
	a_t_i[0].t = t;
	a_t_i[1].t = t;
	return te_eval(a_t_i[0].expr)*psi1 + te_eval(a_t_i[1].expr)*psi2;
}

void runge_kutta(problem *p, int j, double *x, int xsize, double *y1,
		double *y2)
{
  int i;
  double phi[4], psi[4], h = x[1]-x[0], t = p->t;
  y1[0] = p->phi[j][0];
  y2[0] = p->phi[j][1];
  for(i = 1; i < xsize; i++) {
    phi[0] = h*dp_t(p->a[0], t - x[i-1], y1[i-1], y2[i-1]);
    psi[0] = h*dp_t(p->a[1], t - x[i-1], y1[i-1], y2[i-1]);
    phi[1] = h*dp_t(p->a[0], t - (x[i-1]+h/2.0), y1[i-1]+phi[0]/2.0,
				y2[i-1]+psi[0]/2.0);
    psi[1] = h*dp_t(p->a[1], t - (x[i-1]+h/2.0), y1[i-1]+phi[0]/2.0,
				y2[i-1]+psi[0]/2.0);
    phi[2] = h*dp_t(p->a[0], t - (x[i-1]+h/2.0), y1[i-1]+phi[1]/2.0,
				y2[i-1]+psi[1]/2.0);
    psi[2] = h*dp_t(p->a[1], t - (x[i-1]+h/2.0), y1[i-1]+phi[1]/2.0,
				y2[i-1]+psi[1]/2.0);
    phi[3] = h*dp_t(p->a[0], t - (x[i-1]+h), y1[i-1]+phi[2], y2[i-1]+psi[2]);
    psi[3] = h*dp_t(p->a[1], t - (x[i-1]+h), y1[i-1]+phi[2], y2[i-1]+psi[2]);
    y1[i] = y1[i-1] + 1.0/6.0*(phi[0] + 2*phi[1] + 2*phi[2] + phi[3]);
    y2[i] = y2[i-1] + 1.0/6.0*(psi[0] + 2*psi[1] + 2*psi[2] + psi[3]);
  }
}

void integration_by_rectangle_h(problem *p, int j, double h,
		double *integral, double *psi_t)
{
  int node_count, i, psi_node_count;
  double sum = 0.0, *t, *psi1, *psi2, psi_h, a = 0, b = p->t;
	func2 *c_u = &p->c_u;
	node_count = (int)trunc((b-a)/h)+1;
	h = (b-a)/(double)node_count;
	node_count++;
	psi_node_count = node_count*2 - 1;
	psi_h = (b-a) / (double) (psi_node_count-1);
	t = malloc(sizeof(double)*psi_node_count);
	psi1 = malloc(sizeof(double)*psi_node_count);
	psi2 = malloc(sizeof(double)*psi_node_count);
	for(i = 0; i < psi_node_count; i++)
		t[i] = a + i*psi_h;
	runge_kutta(p, j, t, psi_node_count, psi1, psi2);
	for(i = 1; i < node_count; i++) {
		c_u->x = psi1[2*i-1];
		c_u->y = psi2[2*i-1];
		sum += te_eval(c_u->expr);
	}
	psi_t[0] = psi1[psi_node_count-1];
	psi_t[1] = psi2[psi_node_count-1];
	free(t);
	free(psi1);
	free(psi2);
	*integral = h*sum;
}

double calc_c_j(problem *p, int j, double eps)
{
  double h = 0.05, delta = 1.0, i_h, i_half_h, psi_t[2];
  int m = 2;
  while(fabs(delta)>eps) {
    integration_by_rectangle_h(p, j, h, &i_h, psi_t);
    integration_by_rectangle_h(p, j, h/2.0, &i_half_h, psi_t);
    delta = (i_half_h-i_h)/(pow(2,m)-1);
    h = h/2.0;
  }
  //printf("delta = %.128lf\n", delta);
  //printf("h = %lf\n", h);
	//printf("1: %lf\n", dot_product(psi_t, p->x0, 2));
	//printf("2: %lf\n", i_half_h);
	//printf("3: %lf\n", delta);
	//exit(1);
  return dot_product(psi_t, p->x0, 2)+i_half_h+delta;
}

void get_omega(problem *p, double ***omega, int *omega_size)
{
	int i, j, n;
	double *c;
	n = p->n;
	c = (double *) malloc(sizeof(double) * n);
	for(j = 0; j < n; j++)
		c[j] = calc_c_j(p, j, 0.001);
	//printf("VECTOR c\n");
	//printvec(c, n);
	//printf("T = %lf\n", p->t);
	*omega = (double **) malloc(sizeof(double *) * n);
	for(i = 0; i < n; i++)
		(*omega)[i] = (double *) malloc(sizeof(double) * 2);
	*omega_size = n;
	for(i = 0; i < n; i++) {
		(*omega)[i][0] = c[i];
		(*omega)[i][1] = -p->phi[i][0];
		(*omega)[i][2] = -p->phi[i][1];
	}
	free(c);
}

void get_dots_pos(double *cp_v, int cp_v_count, func2 *f,
		vec3 **dots_p, int *dots_p_count)
{
	int i, j, *cp_num_v, max_count;
	double *cp_dist_v;
	cp_dist_v = malloc(sizeof(double) * cp_v_count);
	cp_num_v = malloc(sizeof(int) * cp_v_count);
	for(i = 0; i < cp_v_count; i++) {
		f->x = cp_v[i*6];
		f->y = cp_v[i*6+1];
		cp_dist_v[i] = te_eval(f->expr);
		cp_num_v[i] = i;
	}
	sort(cp_num_v, cp_dist_v, cp_v_count);
	max_count = 1;
	for(i = cp_v_count - 1; i > 0; i--)
		if(fabs(cp_dist_v[i] - cp_dist_v[i-1]) < ALMOSTZERO)
			max_count++;
		else
			break;

	/*
	for(j = 0; j < max_count; j++) {
		i = cp_num_v[cp_v_count - 1 - j];
		printf("extreme dot (x,y) = (%lf,%lf)\n", cp_v[i*6], cp_v[i*6+1]);
		f->x = cp_v[i*6];
		f->y = cp_v[i*6+1];
		printf("f(%lf,%lf) = %lf\n", (*f).x, (*f).y, te_eval((*f).expr));
	}
	*/
	/*
	for(j = 0; j < cp_v_count; j++) {
		printf("dot (x,y) = (%lf,%lf)\n", cp_v[j*6], cp_v[j*6+1]);
		f->x = cp_v[j*6];
		f->y = cp_v[j*6+1];
		printf("f(%lf,%lf) = %lf\n", (*f).x, (*f).y, te_eval((*f).expr));
	}
	*/

	*dots_p = malloc(sizeof(vec3) * max_count);
	*dots_p_count = max_count;
	for(j = 0; j < max_count; j++) {
		i = cp_num_v[cp_v_count - 1 - j];
		(*dots_p)[j][0] = cp_v[i*6];
		(*dots_p)[j][1] = cp_v[i*6+1];
		(*dots_p)[j][2] = 0;
	}
	free(cp_dist_v);
	free(cp_num_v);
}

void get_direction(double ***phi, int n)
{
	int i;
	*phi = (double **) malloc(sizeof(double *) * n);
	for(i = 0; i < n ; i++)
		(*phi)[i] = (double *) malloc(sizeof(double)*2);
	for(i = 0; i < n; i++) {
		(*phi)[i][0] = cos((2*M_PI/n)*(i+1));
		(*phi)[i][1] = sin((2*M_PI/n)*(i+1));
	}
}

void deinit_problem(problem *p)
{
	int i, j;
	freemat(p->phi, p->n);
	for(i = 0; i < 2; i++) {
		for(j = 0; j < 2; j++)
			te_free(p->a[i][j].expr);
		free(p->a[i]);
	}
	free(p->a);
	free(p->x0);
	te_free(p->c_u.expr);
	te_free(p->f.expr);
}

void process_input(GLFWwindow *window, int *is_animation, int
		*is_extreme_problem)
{
	if(glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
		*is_animation = 1;
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		*is_extreme_problem = 1;
}

void get_rs(problem *p, unsigned int *VBO, unsigned int *VAO,
		int *cp_count_vertices, vec3 **dots_pos, int *dots_pos_count)
{
	int omega_size, i;
	double **omega, *cp_data;
	double prev_t, cur_t;

	prev_t = glfwGetTime();
	get_omega(p, &omega, &omega_size);
	cur_t = glfwGetTime();
	printf("DEBUG: get omega in H-format time: %lf\n", cur_t-prev_t);

	prev_t = glfwGetTime();
	get_cp_vertices(omega, omega_size, &cp_data, cp_count_vertices);
	get_dots_pos(cp_data, *cp_count_vertices, &p->f, dots_pos, dots_pos_count);
	prepare_object(VBO, VAO, *cp_count_vertices, cp_data);
	cur_t = glfwGetTime();
	printf("DEBUG: sort cp_data, solve extreme problem & create cp buffers: "
			"%lf\n", cur_t-prev_t);

	freemat(omega, omega_size);
	free(cp_data);
	printf("cp_count_vertices: %d\n", *cp_count_vertices);
	printf("dots_pos_count: %d\n", *dots_pos_count);
	printf("maxima\n");
	if(*dots_pos_count < 5) {
		for(i = 0; i < *dots_pos_count; i++)
			printf("(%lf,%lf)\n", (*dots_pos)[i][0], (*dots_pos)[i][1]);
	}
	else {
		for(i = 0; i < 5; i++)
			printf("(%lf,%lf)\n", (*dots_pos)[i][0], (*dots_pos)[i][1]);
		printf("and so on... (%d) \n", *dots_pos_count - 5);
	}
}

void parse_cmdline(int argc, char **argv, coordplane *plane)
{
	int i;
	char *s, *endptr;
	if(argc > 1)
		for(i = 1; i < argc; i++) {
			s = argv[i];
			while(isspace(*s))
				s++;
			if(strlen(s) == 0)
				continue;
			if(strcmp("-clip", s) == 0) {
				if(i == argc-1) {
					fprintf(stderr, "Error: clip requires an argument\n");
					exit(1);
				}
				plane->clip = strtof(argv[i+1], &endptr);
				if(endptr == argv[i+1]) {
					fprintf(stderr, "Error: you must specify a float\n");
					exit(1);
				}
			}
		}
}

int main(int argc, char **argv)
{
	coordplane *plane;
	unsigned int cp_VBO, cp_VAO, dot_VBO, dot_VAO;
	int cp_count_vertices, dots_pos_count, is_animation = 0,
			is_extreme_problem = 0;
	vec3 *dots_pos;
	const double dot_data[24] = {
		1.0, -1.0, 0, .322, .576, .839,
		1.0, 1.0, 0, .322, .576, .839,
		-1.0, 1.0, 0, .322, .576, .839,
		-1.0, -1.0, 0, .322, .576, .839,
	};
	problem p;

	parse_input(&p.a, &p.x0, &p.t, &p.n, &p.c_u, &p.f);
	print_input(&p.a, &p.x0, &p.t, &p.n, &p.c_u, &p.f);
	coordplane_create(&plane);
	get_direction(&p.phi, p.n);
  dd_set_global_constants(); /* First, this must be called once to use cddlib. */
	prepare_object(&dot_VBO, &dot_VAO, 4, dot_data);
	parse_cmdline(argc, argv, plane);

	get_rs(&p, &cp_VBO, &cp_VAO, &cp_count_vertices, &dots_pos,
			&dots_pos_count);

	while(!glfwWindowShouldClose(plane->window))
	{
		if(is_animation) {
			p.t += 0.1f;
			printf("t: %f\n", p.t);

			delete_object(cp_VBO, cp_VAO);
			free(dots_pos);

			get_rs(&p, &cp_VBO, &cp_VAO, &cp_count_vertices, &dots_pos,
					&dots_pos_count);

			is_animation = 0;
		}
		coordplane_process_input(plane);
		process_input(plane->window, &is_animation, &is_extreme_problem);
		coordplane_fill_with_color(0.2f, 0.3f, 0.3f);
		coordplane_shader_set_up(plane);
		coordplane_draw_axes(plane);
		draw_cp(cp_VAO, cp_count_vertices, plane->shader_id);
		if(is_extreme_problem) {
			draw_dots(plane, dot_VAO, dots_pos, dots_pos_count, plane->shader_id);
		}
		coordplane_draw_numbering(plane);
		glfwSwapBuffers(plane->window);
		glfwPollEvents();
	}
	delete_object(cp_VBO, cp_VAO);
	free(dots_pos);

	delete_object(dot_VBO, dot_VAO);
	coordplane_delete(plane);
	deinit_problem(&p);
  dd_free_global_constants();  /* At the end, this should be called. */
	
	return 0;
}
