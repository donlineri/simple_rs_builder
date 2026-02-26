#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <setoper.h>
#include <cdd.h>
#include "tinyexpr.h"
#include "shader_s.h"
#include "space.hpp"

enum {
	max_pathname = 255,
	max_length_line = 255,
};

typedef struct sup_f_s {
	te_expr *expr;
	double phi1, phi2;
} sup_f;

void draw_cp(unsigned int VAO, int count_vertices, Shader *shader)
{
	glm::mat4 model;
	unsigned int modelLoc;
	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(shader->ID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, count_vertices);
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

void order_vertices(double **cp_vertices, double (**g)[1],
		unsigned long rowsize, unsigned long colsize)
{
  unsigned long i, j;
	double centroidX = 0.0, centroidY = 0.0, *cp_angle_vertices;
	int *cp_number_vertices;
	cp_angle_vertices = (double *) malloc(sizeof(double)*rowsize);
	cp_number_vertices = (int *) malloc(sizeof(int)*rowsize);
	*cp_vertices = (double *) malloc(sizeof(double)*(rowsize * 6));
	for(i = 0; i < rowsize; i++) {
		centroidX += g[i][1][0];
		centroidY += g[i][2][0];
	}
	centroidX /= rowsize;
	centroidY /= rowsize;
	for(i = 0; i < rowsize; i++) {
		cp_number_vertices[i] = i;
		cp_angle_vertices[i] = atan2(g[i][2][0] - centroidY, g[i][1][0] - centroidX);
	}
	for(j = rowsize; j > 1; j--)
		for(i = 1; i < j; i++)
			if(cp_angle_vertices[i-1] > cp_angle_vertices[i]) {
				swap_double(&cp_angle_vertices[i-1], &cp_angle_vertices[i]);
				swap_int(&cp_number_vertices[i-1], &cp_number_vertices[i]);
			}
	/*
	int frontFaceMode;
	//glFrontFace(GL_CW);
	glGetIntegerv(GL_FRONT_FACE, &frontFaceMode);
	if(frontFaceMode == GL_CW)
		printf("glFrontFace is GL_CW (clockwise)\n");
	else if(frontFaceMode == GL_CCW)
		printf("glFrontFace is GL_CCW (counter clockwise)\n");
	else
		printf("glFrontFace has an unexpected value\n");
		*/
  for(i = 0; i < rowsize; i++) {
    for(j = 0; j < colsize; j++)
      printf("%lf ", g[i][j][0]);
		(*cp_vertices)[i*6] = g[cp_number_vertices[i]][1][0];
		(*cp_vertices)[i*6+1] = g[cp_number_vertices[i]][2][0];
		if(g[i][0][0] != 1.0) {
			fprintf(stderr, "Error: extreme ray found\n");
			exit(1);
		}
		(*cp_vertices)[i*6+2] = 0.0;
		(*cp_vertices)[i*6+3] = 1.0;
		(*cp_vertices)[i*6+4] = 0.5;
		(*cp_vertices)[i*6+5] = 0.2;
    printf("\n");
  }
	free(cp_angle_vertices);
	free(cp_number_vertices);
}

void open_result_file(FILE **result_file, char *name)
{
	time_t timep;
	char *date;
	timep = time(NULL);
	date = asctime(localtime(&timep));
	date[strlen(date)-1] = '\0';
	sprintf(name, "/tmp/simple_rs_builder_%.100s.ine", date);
	*result_file = fopen(name,  "w");
	if(!*result_file) {
		perror(name);
		exit(1);
	}
}

void error_file()
{
	fprintf(stderr, "Error: invalid file\n");
	exit(1);
}

void generate_result_file(FILE *result_file, double **omega, int omega_size)
{
	int i;
	char s[max_length_line];
	fputs("H-representation\n", result_file);
	fputs("begin\n", result_file);
	snprintf(s, max_length_line, "%d 3 real\n", omega_size+4);
	fputs(s, result_file);
	for(i = 0; i < omega_size; i++) {
		snprintf(s, max_length_line, "%lf %lf %lf\n", omega[i][0], omega[i][1], omega[i][2]);
		fputs(s, result_file);
	}
	fputs("10000 1 0\n", result_file);
	fputs("10000 -1 0\n", result_file);
	fputs("10000 0 1\n", result_file);
	fputs("10000 0 -1\n", result_file);
	fputs("end\n", result_file);
}

void get_file(FILE **cddfile, char *cddfile_name, double **omega, int omega_size)
{
	FILE *t;
	open_result_file(&t, cddfile_name);
	generate_result_file(t, omega, omega_size);
	fclose(t);
	*cddfile = fopen(cddfile_name, "r");
	if(!*cddfile) {
		perror(cddfile_name);
		exit(1);
	}
}

void get_cp_vertices(FILE *reading, double **cp_vertices, int *cp_count_vertices)
{
  dd_ErrorType error=dd_NoError;
  dd_MatrixPtr M, G;

  dd_PolyhedraPtr poly;

  dd_set_global_constants(); /* First, this must be called once to use cddlib. */

/* Input an LP using the cdd library  */
  M = dd_PolyFile2Matrix(reading, &error);
  if (error!=dd_NoError) goto _L99;
  //dd_WriteMatrix(stdout, M);
/* Generate all vertices of the feasible reagion */
  poly = dd_DDMatrix2Poly(M, &error);
  G = dd_CopyGenerators(poly);
  //printf("rowsize: %ld\n", G->rowsize);
	order_vertices(cp_vertices, G->matrix, G->rowsize, G->colsize);
	*cp_count_vertices = G->rowsize;
  printf("\nGenerators (All the vertices of the feasible region if bounded.)\n");
  dd_WriteMatrix(stdout, G);

  /* Free allocated spaces. */
  dd_FreeMatrix(G);
  dd_FreePolyhedra(poly);

/* Free allocated spaces. */
  dd_FreeMatrix(M);
_L99:;
  fclose(reading);
  if (error!=dd_NoError) dd_WriteErrorMessages(stdout, error);
  dd_free_global_constants();  /* At the end, this should be called. */
}

void prepare_convex_polygon(unsigned int *cp_VBO, unsigned int *cp_VAO,
		unsigned int cp_count_vertices, double *cp_vertices)
{
	glGenVertexArrays(1, cp_VAO);
	glGenBuffers(1, cp_VBO);
	glBindVertexArray(*cp_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, *cp_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(double)*(cp_count_vertices)*6,
			cp_vertices, GL_STATIC_DRAW);
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
	free(cp_vertices);
}

void delete_convex_polygon(unsigned int cp_VBO, unsigned int cp_VAO)
{
	glDeleteVertexArrays(1, &cp_VAO);
	glDeleteBuffers(1, &cp_VBO);
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

void get_omega(double ***omega, int *omega_size)
{
	int i, n;
	double **a, *x0, t, **phi, *c;
	sup_f c_u;
	parse_input(&a, &x0, &t, &n, &c_u);
	//print_input(&a, &x0, &t, &n, &c_u);
	phi = (double **) malloc(sizeof(double *) * n);
	for(i = 0; i < n ; i++)
		phi[i] = (double *) malloc(sizeof(double)*2);
	for(i = 0; i < n; i++) {
		phi[i][0] = cos((2*M_PI/n)*(i+1));
		phi[i][1] = sin((2*M_PI/n)*(i+1));
	}
	c = (double *) malloc(sizeof(double) * n);
	calc_c_j(c, a, x0, t, n, phi, &c_u);
	//printf("VECTOR c\n");
	//printvec(c, n);
	*omega = (double **) malloc(sizeof(double *) * n);
	for(i = 0; i < n; i++)
		(*omega)[i] = (double *) malloc(sizeof(double) * 2);
	*omega_size = n;
	for(i = 0; i < n; i++) {
		(*omega)[i][0] = c[i];
		(*omega)[i][1] = -phi[i][1];
		(*omega)[i][2] = -phi[i][0];
	}
	freemat(phi, n);
	freemat(a, 2);
	free(x0);
	free(c);
	te_free(c_u.expr);
}

int main()
{
	space *plane;
	unsigned int cp_VBO, cp_VAO;
	int omega_size, cp_count_vertices;
	double *cp_vertices, **omega;
	char cddfile_name[max_pathname];
	FILE *cddfile;

	get_omega(&omega, &omega_size);
	get_file(&cddfile, cddfile_name, omega, omega_size);
	freemat(omega, omega_size);
	get_cp_vertices(cddfile, &cp_vertices, &cp_count_vertices);
#ifndef DONTDELETE
	unlink(cddfile_name);
#endif
	prepare_plane(&plane);
	prepare_convex_polygon(&cp_VBO, &cp_VAO, cp_count_vertices, cp_vertices);
	draw_graph(plane, draw_cp, cp_VAO, cp_count_vertices);
	delete_convex_polygon(cp_VBO, cp_VAO);
	delete_plane(plane);
	
	return 0;
}
