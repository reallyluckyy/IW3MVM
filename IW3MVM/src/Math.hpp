#pragma once

#define M_PI 3.14159265358979323846
#define EPS 1.0e-6

typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];

extern float Deg2Rad(float degrees);
extern float DotProduct(vec3_t a, vec3_t b);
extern void CrossProduct(vec3_t one, vec3_t two, vec3_t& out);
extern float VectorDistance(vec3_t a, vec3_t b);

extern bool WorldToScreen(vec3_t worldPosition, vec2_t& screenPosition);

extern void splint(double xa[], double ya[], double y2a[], int n, double x, double *y);
extern void spline(double x[], double y[], int n, bool y1Natural, double yp1, bool ynNatural, double ypn, double y2[]);
extern void qspline_init(int n, int maxit, double tol, double wi[], double wf[], double x[], double y[][4], double h[], double dtheta[], double e[][3], double w[][3]);
extern void qspline_interp(int n, double xi, double x[], double y[][4], double h[], double dtheta[], double e[][3], double w[][3], double q[4], double omega[3], double alpha[3]);