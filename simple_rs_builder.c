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

typedef struct problem_s {
	double **a, *x0, t, **phi;
	int n;
	sup_f c_u;
} problem;

void draw_cp(unsigned int VAO, int count_vertices, unsigned int shader_id)
{
	mat4 model;
	glmc_mat4_identity(model);
	shader_set_matrix(shader_id, "model", model);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, count_vertices);
}

void draw_dots(unsigned int VAO, int count_vertices, unsigned int shader_id)
{
	int i;
	mat4 model;
	glmc_mat4_identity(model);
	shader_set_matrix(shader_id, "model", model);
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

void parse_input(double ***a, double **x0, double *t, int *n, sup_f *c_u)
{
	const int dim = 2;
	int i, j, err, pos, te_vars_count;
	char input[max_length_line], *s, *endptr;
	double data[8];
	te_variable vars[] = {{"max", (void*) max,
		TE_FUNCTION2}, {"phi1", &c_u->phi1}, {"phi2", &c_u->phi2}};
	te_vars_count = 3;
	(*a) = (double **) malloc(sizeof(double *) * dim);
	(*a)[0] = (double *) malloc(sizeof(double) * dim);
	(*a)[1] = (double *) malloc(sizeof(double) * dim);
	*x0 = (double *) malloc(sizeof(double)*dim);
	pos = 0;
	if(isatty(0))
		printf("Enter A (4), x0 (2), T (1), N (1), c_u (function(phi1,phi2))\n");
	s = get_stroka(input);
	while(pos < 8) {
		data[pos] = strtod(s, &endptr);
		if(endptr == s) {
			s = get_stroka(input);
		} else {
      pos++;
      s = endptr;
		}
	}
	while(isspace(*s))
		s++;
	c_u->expr = te_compile(s, vars, te_vars_count, &err);
	while(!c_u->expr) {
		if(err != 1)
			fprintf(stderr, "Parse error at %d\n", err);
		s = get_stroka(input);
		c_u->expr = te_compile(s, vars, te_vars_count, &err);
	}
	
	for(i = 0; i < dim; i++)
		for(j = 0; j < dim; j++)
			(*a)[i][j] = data[i*dim + j];
	(*x0)[0] = data[4];
	(*x0)[1] = data[5];
	*t = data[6];
	*n = (int) data[7];
}

void freemat(double **m, int size)
{
  int i;
  for(i = 0; i < size; i++)
    free(m[i]);
  free(m);
}

void print_input(double ***a, double **x0, double *t, int *n, sup_f *c_u)
{
	printf("MATRIX A\n");
	printmat(*a, 2);
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

void copy(double **dest, int n, double **src)
{
  int i, j;
  for(i = 0; i < n; i++)
    for(j = 0; j < n; j++)
      dest[i][j] = src[i][j];
}

void multiply(double **a, double **b, int n, double ***r)
{
  int i, j, k;
  double s = 0.0;
  double **c;
	c = (double **) malloc(sizeof(double *)*n);
  for(i = 0; i < n; i++)
    c[i] = (double *) malloc(sizeof(double)*n);
  for(i = 0; i < n; i++) {
    for(j = 0; j < n; j++) {
      s = 0.0;
      for(k = 0; k < n; k++)
        s += a[i][k]*b[k][j];
      c[i][j] = s;
    }
  }
  *r = c;
}

void multiply_vec(double **a, double *b, int n, double **r)
{
  double *res = (double *) malloc(sizeof(double)*n);
  int i, j;
  for(i = 0; i < n; i++) {
    res[i] = 0.0;
    for(j = 0; j < n; j++)
        res[i] += a[i][j] * b[j];
  }
  *r = res;
}

double dot_product(double *x1, double *x2, int size)
{
  int i;
  double s = 0.0;
  for(i = 0; i < size; i++)
    s += x1[i] * x2[i];
  return s;
}

long factorial(long n)
{
  if(n == 0)
    return 1;
  return n*factorial(n-1);
}

void mult_m_s(double **m, int size, double s)
{
	int i, j;
	for(i = 0; i < size; i++)
		for(j = 0; j < size; j++)
			m[i][j] = s*m[i][j];
}

void add(double **a, double **b, int n)
{
  int i, j;
  for(i = 0; i < n; i++)
    for(j = 0; j < n; j++)
      a[i][j] += b[i][j];
}

double **expm(double **mat, int size)
{
	int i, j;
	long k;
	double **res, **a_new, **a_prev;
	res = (double **) malloc(sizeof(double *)*size);
	a_prev = (double **) malloc(sizeof(double *)*size);
	for(i = 0; i < size; i++) {
		res[i] = (double *) malloc(sizeof(double)*size);
		a_prev[i] = (double *) malloc(sizeof(double)*size);
	}
	for(i = 0; i < size; i++)
		for(j = 0; j < size; j++)
			if(i == j)
				res[i][j] = 1;
			else
				res[i][j] = 0;

	for(k = 1; k < 20; k++) {
		copy(a_prev, size, mat);
		for(i = 1; i < k; i++) {
			multiply(a_prev, mat, size, &a_new);
			freemat(a_prev, size);
			a_prev = a_new;
		}
		mult_m_s(a_prev, size, 1.0/factorial(k));
		add(res, a_prev, size);
	}
	freemat(a_prev, size);
	return res;
}

double integrand(double **a, double t, double *phi, sup_f *c_u, double s)
{
	double **alfa, **exp_alfa, *beta;
	alfa = (double **) malloc(sizeof(double *) * 2);
	alfa[0] = (double *) malloc(sizeof(double) * 2);
	alfa[1] = (double *) malloc(sizeof(double) * 2);
	copy(alfa, 2, a);
	mult_m_s(alfa, 2, t-s);
	exp_alfa = expm(alfa, 2);
	freemat(alfa, 2);
	multiply_vec(exp_alfa, phi, 2, &beta);
	freemat(exp_alfa, 2);
	c_u->phi1 = beta[0];
	c_u->phi2 = beta[1];
	free(beta);
	return te_eval(c_u->expr);
}

double integration_by_rectangle_h(double **mat_a, double t, double *phi,
		sup_f *c_u, double a, double b, double h)
{
  int node_count, i;
  double sum = 0.0;
	node_count = (int)trunc((b-a)/h)+1;
	h = (b-a)/node_count;
	node_count++;
	for(i = 1; i < node_count; i++)
		sum += integrand(mat_a, t, phi, c_u, (a+h*i + a+h*(i-1))/2.0);
	return h*sum;
}

double integration_by_runge_romberg(double **mat_a, double t, double *phi,
		sup_f *c_u, double a, double b, double eps)
{
  double h = 0.05, delta = 1.0, i_h, i_half_h;
  int m = 2;
  while(fabs(delta)>eps) {
    i_half_h = integration_by_rectangle_h(mat_a, t, phi, c_u, a, b, h/2.0);
    i_h = integration_by_rectangle_h(mat_a, t, phi, c_u, a, b, h);
    delta = (i_half_h-i_h)/(pow(2,m)-1);
    h = h/2.0;
  }
  //printf("delta = %.128lf\n", delta);
  //printf("h = %lf\n", h);
  return i_half_h+delta;
}

void transposition(double **a, int n, double ***r)
{
  int i, j;
  double **a_t= (double **) malloc(sizeof(double *)*n);
  for(i = 0; i < n; i++)
    a_t[i] = (double *) malloc(sizeof(double)*n);
  for(i = 0; i < n; i++)
    for(j = 0; j < n; j++)
      a_t[i][j] = a[j][i];
  *r = a_t;
}

void calc_c_j(double *c, double **a, double *x0, double t, int n, double **phi,
		sup_f *c_u)
{
	int j;
	double **alfa, **exp_alfa, *beta, **a_star;
	alfa = (double **) malloc(sizeof(double *) * 2);
	alfa[0] = (double *) malloc(sizeof(double) * 2);
	alfa[1] = (double *) malloc(sizeof(double) * 2);
	copy(alfa, 2, a);
	mult_m_s(alfa, 2, t);
	exp_alfa = expm(alfa, 2);
	freemat(alfa, 2);
	multiply_vec(exp_alfa, x0, 2, &beta);
	freemat(exp_alfa, 2);
	transposition(a, 2, &a_star);
	for(j = 0; j < n; j++) {
		c[j] = dot_product(beta, phi[j], 2) +
			integration_by_runge_romberg(a_star, t, phi[j], c_u, 0, t, 0.001);
	}
	free(beta);
	freemat(a_star, 2);
}

void get_omega(problem *p, double ***omega, int *omega_size)
{
	int i, n;
	double *c;
	n = p->n;
	//print_input(&a, &x0, &t, &n, &c_u);
	c = (double *) malloc(sizeof(double) * n);
	calc_c_j(c, p->a, p->x0, p->t, n, p->phi, &p->c_u);
	//printf("VECTOR c\n");
	//printvec(c, n);
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
	freemat(p->phi, p->n);
	freemat(p->a, 2);
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
			p.t += 1.0f;
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
