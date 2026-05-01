#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <cglm/call.h>
#include "glad/glad.h"
#include "setoper.h"
#include "cdd.h"
#include "tinyexpr.h"
#include "coordinate_plane.h"
#include "shader_s.h"

#define ALMOSTZERO 0.00001

enum {
	max_length_line = 255,
};

typedef struct sup_f_s {
	te_expr *expr;
	double phi1, phi2;
} sup_f;

typedef struct func1_s {
	te_expr *expr;
	double t;
} func1;

typedef struct problem_s {
	func1 **a;
	double *x0, t, **phi;
	int n;
	sup_f c_u;
} problem;

void draw_cp(unsigned int VAO, int count_vertices, unsigned int shader_id)
{
	shader_use(shader_id);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, count_vertices);
}

void draw_dots(unsigned int VAO, int count_vertices, unsigned int shader_id)
{
	int i;
	shader_use(shader_id);
	glLineWidth(2.0f);
	glBindVertexArray(VAO);
	for(i = 0; i < count_vertices / 4; i++)
		glDrawArrays(GL_TRIANGLE_FAN, i*4, 4);
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
		double *vertices)
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

te_expr *get_func(char *s, te_variable *vars, int te_vars_count)
{
	int err;
	char input[max_length_line];
	te_expr *expr;
	while(isspace(*s))
		s++;
	expr = te_compile(s, vars, te_vars_count, &err);
	while(!expr) {
		if(err != 1)
			fprintf(stderr, "Parse error at %d\n", err);
		s = get_stroka(input);
		expr = te_compile(s, vars, te_vars_count, &err);
	}
	return expr;
}

void parse_input(func1 ***a, double **x0, double *t, int *n, sup_f *c_u)
{
	const int dim = 2;
	const int num_count = 4;
	int i, j, pos, te_vars_count_c_u, te_vars_count_aij;
	char input[max_length_line], *s, *endptr;
	double data[num_count];
	te_variable vars_c_u[] = {{"max", (void*) max,
		TE_FUNCTION2}, {"phi1", &c_u->phi1}, {"phi2", &c_u->phi2}};
	(*a) = malloc(sizeof(func1 *) * dim);
	(*a)[0] = malloc(sizeof(func1) * dim);
	(*a)[1] = malloc(sizeof(func1) * dim);
	te_variable vars_aij[2][2][2] = {{{{"t", &(*a)[0][0].t}},
		{{"t", &(*a)[1][0].t}}}, {{{"t", &(*a)[0][1].t}}, {{"t", &(*a)[1][1].t}}}};
	te_vars_count_c_u = 3;
	te_vars_count_aij = 1;
	for(i = 0; i < dim; i++)
		for(j = 0; j < dim; j++) {
			s = get_stroka(input);
			(*a)[j][i].expr = get_func(s, vars_aij[i][j], te_vars_count_aij);
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

	c_u->expr = get_func(s, vars_c_u, te_vars_count_c_u);
	
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

void print_input(func1 ***a, double **x0, double *t, int *n, sup_f *c_u)
{
	int i, j;
	printf("MATRIX A\n");
	for(i = 0; i < 2; i++)
		for(j = 0; j < 2; j++) {
			(*a)[i][j].t = 1.0;
			printf("a[%d][%d](%lf) = %lf\n", i, j, (*a)[i][j].t, te_eval((*a)[i][j].expr));
		}
	(*c_u).phi1 = 1;
	(*c_u).phi2 = 1;
	printf("f(%lf,%lf) = %lf\n", (*c_u).phi1, (*c_u).phi2, te_eval((*c_u).expr));
	(*c_u).phi1 = 3;
	(*c_u).phi2 = -2;
	printf("f(%lf,%lf) = %lf\n", (*c_u).phi1, (*c_u).phi2, te_eval((*c_u).expr));
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
  int node_count, i, psi_node_count, a = 0, b = p->t;
  double sum = 0.0, *t, *psi1, *psi2, psi_h;
	sup_f *c_u = &p->c_u;
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
		c_u->phi1 = psi1[2*i-1];
		c_u->phi2 = psi2[2*i-1];
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
  return dot_product(psi_t, p->x0, 2)+i_half_h+delta;
}

void get_omega(problem *p, double ***omega, int *omega_size)
{
	int i, j, n;
	double *c;
	n = p->n;
	//print_input(&a, &x0, &t, &n, &c_u);
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
		(*omega)[i][1] = -p->phi[i][1];
		(*omega)[i][2] = -p->phi[i][0];
	}
	free(c);
}

void get_dots_vertices(double *cp_v, int cp_v_count, double **dots_v,
		int *dots_v_count, float clip)
{
	int i, j, *cp_num_v, max_count, is_checked;
	float size;
	double *cp_dist_v;
	cp_dist_v = malloc(sizeof(double) * cp_v_count);
	cp_num_v = malloc(sizeof(int) * cp_v_count);
	for(i = 0; i < cp_v_count; i++) {
		cp_dist_v[i] = cp_v[i*6]*cp_v[i*6] + cp_v[i*6+1]*cp_v[i*6+1];
		cp_num_v[i] = i;
	}
	sort(cp_num_v, cp_dist_v, cp_v_count);
	max_count = 0;
	is_checked = 0;
	for(i = cp_v_count - 1; i > 0; i--)
		if(fabs(cp_dist_v[i] - cp_dist_v[i-1]) < ALMOSTZERO)
			max_count++;
		else {
			break;
			is_checked = 1;
		}
	if(!is_checked && (fabs(cp_dist_v[1] - cp_dist_v[0]) < ALMOSTZERO))
		max_count++;
	for(j = 0; j < max_count; j++) {
		i = cp_num_v[cp_v_count - 1 - j];
		printf("extreme dot (x,y) = (%lf,%lf)\n", cp_v[i*6], cp_v[i*6+1]);
	}
	size = clip/250;
	/* 
	 * If you want to select all vertices of convex polygon, uncomment this code
	 * and comment out code below
	dots_v = malloc(sizeof(double) * (cp_v_count*6*4));
	dots_v_count = 4*cp_v_count;
	for(i = 0; i < cp_v_count; i++) {
		(*dots_v)[i*6*4] = cp_v[i*6] + size;
		(*dots_v)[i*6*4+1] = cp_v[i*6+1] - size;
		(*dots_v)[i*6*4+2] = 0;
		(*dots_v)[i*6*4+3] = .322f;
		(*dots_v)[i*6*4+4] = .576f;
		(*dots_v)[i*6*4+5] = .839f;

		(*dots_v)[i*6*4+6] = cp_v[i*6] + size;
		(*dots_v)[i*6*4+7] = cp_v[i*6+1] + size;
		(*dots_v)[i*6*4+8] = 0;
		(*dots_v)[i*6*4+9] = .322f;
		(*dots_v)[i*6*4+10] = .576f;
		(*dots_v)[i*6*4+11] = .839f;

		(*dots_v)[i*6*4+12] = cp_v[i*6] - size;
		(*dots_v)[i*6*4+13] = cp_v[i*6+1] + size;
		(*dots_v)[i*6*4+14] = 0;
		(*dots_v)[i*6*4+15] = .322f;
		(*dots_v)[i*6*4+16] = .576f;
		(*dots_v)[i*6*4+17] = .839f;

		(*dots_v)[i*6*4+18] = cp_v[i*6] - size;
		(*dots_v)[i*6*4+19] = cp_v[i*6+1] - size;
		(*dots_v)[i*6*4+20] = 0;
		(*dots_v)[i*6*4+21] = .322f;
		(*dots_v)[i*6*4+22] = .576f;
		(*dots_v)[i*6*4+23] = .839f;
	}
	*/
	*dots_v = malloc(sizeof(double) * (max_count*6*4));
	*dots_v_count = 4*max_count;
	for(j = 0; j < max_count; j++) {
		i = cp_num_v[cp_v_count - 1 - j];
		(*dots_v)[j*6*4] = cp_v[i*6] + size;
		(*dots_v)[j*6*4+1] = cp_v[i*6+1] - size;
		(*dots_v)[j*6*4+2] = 0;
		(*dots_v)[j*6*4+3] = .322f;
		(*dots_v)[j*6*4+4] = .576f;
		(*dots_v)[j*6*4+5] = .839f;

		(*dots_v)[j*6*4+6] = cp_v[i*6] + size;
		(*dots_v)[j*6*4+7] = cp_v[i*6+1] + size;
		(*dots_v)[j*6*4+8] = 0;
		(*dots_v)[j*6*4+9] = .322f;
		(*dots_v)[j*6*4+10] = .576f;
		(*dots_v)[j*6*4+11] = .839f;

		(*dots_v)[j*6*4+12] = cp_v[i*6] - size;
		(*dots_v)[j*6*4+13] = cp_v[i*6+1] + size;
		(*dots_v)[j*6*4+14] = 0;
		(*dots_v)[j*6*4+15] = .322f;
		(*dots_v)[j*6*4+16] = .576f;
		(*dots_v)[j*6*4+17] = .839f;

		(*dots_v)[j*6*4+18] = cp_v[i*6] - size;
		(*dots_v)[j*6*4+19] = cp_v[i*6+1] - size;
		(*dots_v)[j*6*4+20] = 0;
		(*dots_v)[j*6*4+21] = .322f;
		(*dots_v)[j*6*4+22] = .576f;
		(*dots_v)[j*6*4+23] = .839f;
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
}

void process_input(GLFWwindow *window, int *is_animation)
{
	if(glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
		*is_animation = 1;
}

int main()
{
	coordplane *plane;
	unsigned int cp_VBO, cp_VAO, dots_VBO, dots_VAO;
	int omega_size, cp_count_vertices, dots_count_vertices, is_animation;
	double *cp_data, *dots_data, **omega, prev_t, cur_t;
	problem p;

	parse_input(&p.a, &p.x0, &p.t, &p.n, &p.c_u);
	//print_input(&p.a, &p.x0, &p.t, &p.n, &p.c_u);
	coordplane_create(&plane);
	get_direction(&p.phi, p.n);
  dd_set_global_constants(); /* First, this must be called once to use cddlib. */
	is_animation = 0;

	get_omega(&p, &omega, &omega_size);
	get_cp_vertices(omega, omega_size, &cp_data, &cp_count_vertices);
	freemat(omega, omega_size);
	get_dots_vertices(cp_data, cp_count_vertices, &dots_data,
			&dots_count_vertices, plane->clip);
	prepare_object(&cp_VBO, &cp_VAO, cp_count_vertices, cp_data);
	free(cp_data);
	prepare_object(&dots_VBO, &dots_VAO, dots_count_vertices, dots_data);
	free(dots_data);

	while(!glfwWindowShouldClose(plane->window))
	{
		if(is_animation) {
			p.t += 0.1f;
			printf("t: %f\n", p.t);
			printf("cp_count_vertices: %d\n", cp_count_vertices);

			prev_t = glfwGetTime();
			delete_object(cp_VBO, cp_VAO);
			delete_object(dots_VBO, dots_VAO);
			cur_t = glfwGetTime();
			printf("DEBUG: delete_object time: %lf\n", cur_t-prev_t);

			prev_t = glfwGetTime();
			get_omega(&p, &omega, &omega_size);
			cur_t = glfwGetTime();
			printf("DEBUG: get_omega time: %lf\n", cur_t-prev_t);

			prev_t = glfwGetTime();
			get_cp_vertices(omega, omega_size, &cp_data, &cp_count_vertices);
			cur_t = glfwGetTime();
			printf("DEBUG: get_cp_vertices time: %lf\n", cur_t-prev_t);

			prev_t = glfwGetTime();
			freemat(omega, omega_size);
			cur_t = glfwGetTime();
			printf("DEBUG: freemat time: %lf\n", cur_t-prev_t);

			prev_t = glfwGetTime();
			get_dots_vertices(cp_data, cp_count_vertices, &dots_data,
					&dots_count_vertices, plane->clip);
			cur_t = glfwGetTime();
			printf("DEBUG: get_dots_vertices time: %lf\n", cur_t-prev_t);

			prev_t = glfwGetTime();
			prepare_object(&cp_VBO, &cp_VAO, cp_count_vertices, cp_data);
			free(cp_data);
			prepare_object(&dots_VBO, &dots_VAO, dots_count_vertices, dots_data);
			free(dots_data);
			cur_t = glfwGetTime();
			printf("DEBUG: prepare_object time: %lf\n", cur_t-prev_t);

			is_animation = 0;
		}
		coordplane_process_input(plane);
		process_input(plane->window, &is_animation);
		coordplane_fill_with_color(0.2f, 0.3f, 0.3f);
		coordplane_shader_set_up(plane);
		coordplane_draw_axes(plane);
		draw_cp(cp_VAO, cp_count_vertices, plane->shader_id);
		draw_dots(dots_VAO, dots_count_vertices, plane->shader_id);
		coordplane_draw_numbering(plane);
		glfwSwapBuffers(plane->window);
		glfwPollEvents();
	}
	delete_object(cp_VBO, cp_VAO);
	delete_object(dots_VBO, dots_VAO);
	coordplane_delete(plane);
	deinit_problem(&p);
  dd_free_global_constants();  /* At the end, this should be called. */
	
	return 0;
}
