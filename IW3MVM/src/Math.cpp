#include "stdafx.hpp"
#include "Math.hpp"
#include "Game.hpp"
#include "Freecam.hpp"

float Deg2Rad(float degrees)
{
	return (degrees * 3.14159265359f / 180.0f);
}

float DotProduct(vec3_t a, vec3_t b)
{
	return ((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2]);
}

float VectorDistance(vec3_t a, vec3_t b)
{
	return (sqrt(powf(a[0] - b[0], 2) + powf(a[1] - b[1], 2) + powf(a[2] - b[2], 2)));
}

bool WorldToScreen(vec3_t worldPosition, vec2_t& screenPosition)
{
	vec3_t position{ worldPosition[0] - refdef->position[0], worldPosition[1] - refdef->position[1], worldPosition[2] - refdef->position[2] };
	vec3_t transform;

	vec3_t viewaxis1 { refdef->viewmatrix[0][0], refdef->viewmatrix[0][1], refdef->viewmatrix[0][2] };
	vec3_t viewaxis2 { refdef->viewmatrix[1][0], refdef->viewmatrix[1][1], refdef->viewmatrix[1][2] };
	vec3_t viewaxis3 { refdef->viewmatrix[2][0], refdef->viewmatrix[2][1], refdef->viewmatrix[2][2] };

	transform[0] = DotProduct(position, viewaxis2);
	transform[1] = DotProduct(position, viewaxis3);
	transform[2] = DotProduct(position, viewaxis1);

	if (transform[2] < 0.1f)
		return false;

	uint32_t widthCenter = (uint32_t)((refdef->windowSize[0]) / 2.0f);
	uint32_t heightCenter = (uint32_t)((refdef->windowSize[1]) / 2.0f);

	float multiplier = currentView.isPOV ? 1.0f : 0.75f;

	float x = (1 - (transform[0] / (refdef->tanHalfFovX * multiplier) / transform[2]));
	float y = (1 - (transform[1] / (refdef->tanHalfFovY * multiplier) / transform[2]));

	screenPosition[0] = (widthCenter * x);
	screenPosition[1] = (heightCenter * y);

	return true;
}

double unvec(double a[], double au[])
{
	double amag;

	amag = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);

	if (amag > 0.0)
	{
		au[0] = a[0] / amag;
		au[1] = a[1] / amag;
		au[2] = a[2] / amag;
	}
	else
	{
		au[0] = 0.0;
		au[1] = 0.0;
		au[2] = 0.0;
	}

	return amag;
}

double getang(double qi[], double qf[], double e[])
{
	double dtheta, sa, ca, temp[3];

	temp[0] = qi[3] * qf[0] - qi[0] * qf[3] - qi[1] * qf[2] + qi[2] * qf[1];
	temp[1] = qi[3] * qf[1] - qi[1] * qf[3] - qi[2] * qf[0] + qi[0] * qf[2];
	temp[2] = qi[3] * qf[2] - qi[2] * qf[3] - qi[0] * qf[1] + qi[1] * qf[0];

	ca = qi[0] * qf[0] + qi[1] * qf[1] + qi[2] * qf[2] + qi[3] * qf[3];

	sa = unvec(temp, e);

	dtheta = 2.0*atan2(sa, ca);

	return dtheta;
}

void crossp(double b[], double c[], double a[])
{
	a[0] = b[1] * c[2] - b[2] * c[1];
	a[1] = b[2] * c[0] - b[0] * c[2];
	a[2] = b[0] * c[1] - b[1] * c[0];
}

void CrossProduct(vec3_t one, vec3_t two, vec3_t& out)
{
	out[0] = one[1] * two[2] - one[2] * two[1];
	out[1] = one[2] * two[0] - one[0] * two[2];
	out[2] = one[0] * two[1] - one[1] * two[0];
}

void rf(double e[], double dtheta, double win[], double rhs[])
{
	int i;
	double sa, ca, dot, mag, c1, r0, r1, temp1[3], temp2[3];

	if (dtheta > EPS)
	{
		ca = cos(dtheta);
		sa = sin(dtheta);

		crossp(e, win, temp2);

		crossp(temp2, e, temp1);

		dot = win[0] * e[0] + win[1] * e[1] + win[2] * e[2];

		mag = win[0] * win[0] + win[1] * win[1] + win[2] * win[2];

		c1 = (1.0 - ca);

		r0 = 0.5*(mag - dot*dot)*(dtheta - sa) / c1;

		r1 = dot*(dtheta*sa - 2.0*c1) / (dtheta*c1);

		for (i = 0; i < 3; i++)
			rhs[i] = r0*e[i] + r1*temp1[i];
	}
	else
	{
		for (i = 0; i < 3; i++)
			rhs[i] = 0.0;
	}
}

int bd(double e[], double dtheta, int flag, double xin[], double xout[])
{
	int i;
	double sa, ca, b0, b1, b2, temp1[3], temp2[3];

	if (dtheta > EPS)
	{
		ca = cos(dtheta);
		sa = sin(dtheta);

		if (flag == 0)
		{
			b1 = 0.5*dtheta*sa / (1.0 - ca);
			b2 = 0.5*dtheta;
		}
		else if (flag == 1)
		{
			b1 = sa / dtheta;
			b2 = (ca - 1.0) / dtheta;
		}
		else
			return -1;

		b0 = xin[0] * e[0] + xin[1] * e[1] + xin[2] * e[2];

		crossp(e, xin, temp2);

		crossp(temp2, e, temp1);

		for (i = 0; i < 3; i++)
			xout[i] = b0*e[i] + b1*temp1[i] + b2*temp2[i];
	}
	else
	{
		for (i = 0; i < 3; i++)
			xout[i] = xin[i];
	}

	return 0;
}

void rates(int n, int maxit, double tol, double wi[], double wf[], double h[], double a[], double b[], double c[], double dtheta[], double e[][3], double w[][3], double wprev[][3])
{
	int i, j, iter;
	double dw, temp1[3], temp2[3];

	iter = 0;

	do                                                 /* start iteration loop. */
	{
		for (i = 1; i < n - 1; i++)
			for (j = 0; j < 3; j++)
				wprev[i][j] = w[i][j];

		/* set up the tridiagonal matrix. d initially holds the RHS vector array;
		it is then overlaid with the calculated angular rate vector array. */

		for (i = 1; i < n - 1; i++)
		{
			a[i] = 2.0 / h[i - 1];
			b[i] = 4.0 / h[i - 1] + 4.0 / h[i];
			c[i] = 2.0 / h[i];

			rf(e[i - 1], dtheta[i - 1], wprev[i], temp1);

			for (j = 0; j < 3; j++)
				w[i][j] = 6.0*(dtheta[i - 1] * e[i - 1][j] / (h[i - 1] * h[i - 1]) +
					dtheta[i] * e[i][j] / (h[i] * h[i])) -
				temp1[j];
		}

		bd(e[0], dtheta[0], 1, wi, temp1);
		bd(e[n - 2], dtheta[n - 2], 0, wf, temp2);

		for (j = 0; j < 3; j++)
		{
			w[1][j] -= a[1] * temp1[j];
			w[n - 2][j] -= c[n - 2] * temp2[j];
		}

		/* reduce the matrix to upper triangular form. */

		for (i = 1; i < n - 2; i++)
		{
			b[i + 1] -= c[i] * a[i + 1] / b[i];

			for (j = 0; j < 3; j++)
			{
				bd(e[i], dtheta[i], 1, w[i], temp1);

				w[i + 1][j] -= temp1[j] * a[i + 1] / b[i];
			}
		}

		/* solve using back substitution. */

		for (j = 0; j < 3; j++)
			w[n - 2][j] /= b[n - 2];

		for (i = n - 3; i > 0; i--)
		{
			bd(e[i], dtheta[i], 0, w[i + 1], temp1);

			for (j = 0; j < 3; j++)
				w[i][j] = (w[i][j] - c[i] * temp1[j]) / b[i];
		}

		dw = 0.0;

		for (i = 1; i < n - 1; i++)
			for (j = 0; j < 3; j++)
				dw += (w[i][j] - wprev[i][j])*(w[i][j] - wprev[i][j]);

		dw = sqrt(dw);
	} while (iter++ < maxit && dw > tol);

	/* solve for end conditions. */

	for (j = 0; j < 3; j++)
	{
		w[0][j] = wi[j];
		w[n - 1][j] = wf[j];
	}
}

void splint(double xa[], double ya[], double y2a[], int n, double x, double *y)
{
	int klo, khi, k;
	double h, b, a;

	klo = 0;
	khi = n - 1;
	while (khi - klo > 1)
	{
		k = (khi + klo) >> 1;
		if (xa[k] > x) khi = k;
		else klo = k;
	}
	h = xa[khi] - xa[klo];
	if (h == 0.0) throw "splint: Bad xa input.";
	a = (xa[khi] - x) / h;
	b = (x - xa[klo]) / h;
	*y = a * ya[klo] + b * ya[khi] + ((a * a * a - a) * y2a[klo] + (b * b * b - b) * y2a[khi]) * (h * h) / 6.0f;
}

void spline(double x[], double y[], int n, bool y1Natural, double yp1, bool ynNatural, double ypn, double y2[])
{
	int i, k;
	double p, qn, sig, un;
	double *u = new double[n - 1 - 1 + 1];

	if (y1Natural)
		y2[0] = u[0] = 0.0f;
	else
	{
		y2[0] = -0.5f;
		u[0] = (3.0f / (x[1] - x[0])) * ((y[1] - y[0]) / (x[1] - x[0]) - yp1);
	}

	for (i = 1; i <= n - 2; i++)
	{
		sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
		p = sig * y2[i - 1] + 2.0f;
		y2[i] = (sig - 1.0f) / p;
		u[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]);
		u[i] = (6.0f * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
	}

	if (ynNatural)
		qn = un = 0.0f;
	else
	{
		qn = 0.5f;
		un = (3.0f / (x[n - 1] - x[n - 2])) * (ypn - (y[n - 1] - y[n - 2]) / (x[n - 1] - x[n - 2]));
	}

	y2[n - 1] = (un - qn * u[n - 2]) / (qn * y2[n - 2] + 1.0f);

	for (k = n - 2; k >= 0; k--)
		y2[k] = y2[k] * y2[k + 1] + u[k];

	delete u;
}

void qspline_init(int n, int maxit, double tol, double wi[], double wf[], double x[], double y[][4], double h[], double dtheta[], double e[][3], double w[][3])
{
	int i, j;
	double *a, *b, *c, (*wprev)[3];

	if (n < 4) throw "qspline_init: insufficient input data.\n";

	wprev = new double[n][3];
	a = new double[n - 1];
	b = new double[n - 1];
	c = new double[n - 1];

	for (i = 0; i < n; i++)
		for (j = 0; j < 3; j++)
			w[i][j] = 0.0;

	for (i = 0; i < n - 1; i++)
	{
		h[i] = x[i + 1] - x[i];

		if (h[i] <= 0.0) throw "qspline_init: x is not monotonic.\n";
	}

	/* compute spline coefficients. */

	for (i = 0; i < n - 1; i++)
		dtheta[i] = getang(y[i], y[i + 1], e[i]);

	rates(n, maxit, tol, wi, wf, h, a, b, c, dtheta, e, w, wprev);

	delete c;
	delete b;
	delete a;
	delete[] wprev;
}

static double a[3][3], b[3][3], c[2][3], d[3];

void slew3_init(double dt, double dtheta, double e[], double wi[], double ai[], double wf[], double af[])
{
	int i;
	double sa, ca, c1, c2;
	double b0, bvec1[3], bvec2[3], bvec[3];

	if (dt <= 0.0)
		return;

	sa = sin(dtheta);
	ca = cos(dtheta);

	/* final angular rate terms. */

	if (dtheta > EPS)
	{
		c1 = 0.5*sa*dtheta / (1.0 - ca);

		c2 = 0.5*dtheta;

		b0 = e[0] * wf[0] + e[1] * wf[1] + e[2] * wf[2];

		crossp(e, wf, bvec2);

		crossp(bvec2, e, bvec1);

		for (i = 0; i < 3; i++)
			bvec[i] = b0*e[i] + c1*bvec1[i] + c2*bvec2[i];
	}
	else
	{
		for (i = 0; i < 3; i++)
			bvec[i] = wf[i];
	}

	/* compute coefficients. */

	for (i = 0; i < 3; i++)
	{
		b[0][i] = wi[i];
		a[2][i] = e[i] * dtheta;
		b[2][i] = bvec[i];

		a[0][i] = b[0][i] * dt;
		a[1][i] = (b[2][i] * dt - 3.0*a[2][i]);

		b[1][i] = (2.0*a[0][i] + 2.0*a[1][i]) / dt;
		c[0][i] = (2.0*b[0][i] + b[1][i]) / dt;
		c[1][i] = (b[1][i] + 2.0*b[2][i]) / dt;

		d[i] = (c[0][i] + c[1][i]) / dt;
	}
}

void slew3(double t, double dt, double qi[], double q[], double omega[], double alpha[], double jerk[])
{
	int i;
	double x, ang, sa, ca, u[3], x1[2];
	double th0[3], th1[3], th2[3], th3[3], temp0[3], temp1[3], temp2[3];
	double thd1, thd2, thd3, w2, td2, ut2, wwd;
	double w[3], udot[3], wd1[3], wd1xu[3], wd2[3], wd2xu[3];

	if (dt <= 0.0)
		return;

	x = t / dt;

	x1[0] = x - 1.0;

	for (i = 1; i < 2; i++)
		x1[i] = x1[i - 1] * x1[0];

	for (i = 0; i < 3; i++)
	{
		th0[i] = ((x*a[2][i] + x1[0] * a[1][i])*x + x1[1] * a[0][i])*x;

		th1[i] = (x*b[2][i] + x1[0] * b[1][i])*x + x1[1] * b[0][i];

		th2[i] = x*c[1][i] + x1[0] * c[0][i];

		th3[i] = d[i];
	}

	ang = unvec(th0, u);

	ca = cos(0.5*ang);
	sa = sin(0.5*ang);

	q[0] = ca*qi[0] + sa*(u[2] * qi[1] - u[1] * qi[2] + u[0] * qi[3]);
	q[1] = ca*qi[1] + sa*(-u[2] * qi[0] + u[0] * qi[2] + u[1] * qi[3]);
	q[2] = ca*qi[2] + sa*(u[1] * qi[0] - u[0] * qi[1] + u[2] * qi[3]);
	q[3] = ca*qi[3] + sa*(-u[0] * qi[0] - u[1] * qi[1] - u[2] * qi[2]);

	ca = cos(ang);
	sa = sin(ang);

	if (ang > EPS)
	{
		/* compute angular rate vector. */

		crossp(u, th1, temp1);

		for (i = 0; i < 3; i++)
			w[i] = temp1[i] / ang;

		crossp(w, u, udot);

		thd1 = u[0] * th1[0] + u[1] * th1[1] + u[2] * th1[2];

		for (i = 0; i < 3; i++)
			omega[i] = thd1*u[i] + sa*udot[i] - (1.0 - ca)*w[i];

		/* compute angular acceleration vector. */

		thd2 = udot[0] * th1[0] + udot[1] * th1[1] + udot[2] * th1[2] +
			u[0] * th2[0] + u[1] * th2[1] + u[2] * th2[2];

		crossp(u, th2, temp1);

		for (i = 0; i < 3; i++)
			wd1[i] = (temp1[i] - 2.0*thd1*w[i]) / ang;

		crossp(wd1, u, wd1xu);

		for (i = 0; i < 3; i++)
			temp0[i] = thd1*u[i] - w[i];

		crossp(omega, temp0, temp1);

		for (i = 0; i < 3; i++)
			alpha[i] = thd2*u[i] + sa*wd1xu[i] - (1.0 - ca)*wd1[i] +
			thd1*udot[i] + temp1[i];

		/* compute angular jerk vector. */

		w2 = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];

		thd3 = wd1xu[0] * th1[0] + wd1xu[1] * th1[1] + wd1xu[2] * th1[2] -
			w2*(u[0] * th1[0] + u[1] * th1[1] + u[2] * th1[2]) +
			2.0*(udot[0] * th2[0] + udot[1] * th2[1] + udot[2] * th2[2]) +
			u[0] * th3[0] + u[1] * th3[1] + u[2] * th3[2];

		crossp(th1, th2, temp1);

		for (i = 0; i < 3; i++)
			temp1[i] /= ang;

		crossp(u, th3, temp2);

		td2 = (th1[0] * th1[0] + th1[1] * th1[1] + th1[2] * th1[2]) / ang;

		ut2 = u[0] * th2[0] + u[1] * th2[1] + u[2] * th2[2];

		wwd = w[0] * wd1[0] + w[1] * wd1[1] + w[2] * wd1[2];

		for (i = 0; i < 3; i++)
			wd2[i] = (temp1[i] + temp2[i] - 2.0*(td2 + ut2)*w[i] -
				4.0*thd1*wd1[i]) / ang;

		crossp(wd2, u, wd2xu);

		for (i = 0; i < 3; i++)
			temp2[i] = thd2*u[i] + thd1*udot[i] - wd1[i];

		crossp(omega, temp2, temp1);

		crossp(alpha, temp0, temp2);

		for (i = 0; i < 3; i++)
			jerk[i] = thd3*u[i] + sa*wd2xu[i] - (1.0 - ca)*wd2[i] +
			2.0*thd2*udot[i] + thd1*((1.0 + ca)*wd1xu[i] - w2*u[i] - sa*wd1[i]) -
			wwd*sa*u[i] + temp1[i] + temp2[i];
	}
	else
	{
		crossp(th1, th2, temp1);

		for (i = 0; i < 3; i++)
		{
			omega[i] = th1[i];
			alpha[i] = th2[i];
			jerk[i] = th3[i] - 0.5*temp1[i];
		}
	}
}

void qspline_interp(int n, double xi, double x[], double y[][4], double h[], double dtheta[], double e[][3], double w[][3], double q[4], double omega[3], double alpha[3])
{
	double dum1[3], dum2[3];

	int klo, khi, k;

	klo = 0;
	khi = n - 1;
	while (khi - klo > 1)
	{
		k = (khi + klo) >> 1;
		if (x[k] > xi) khi = k;
		else klo = k;
	}

	/* interpolate and output results. */

	slew3_init(h[klo], dtheta[klo], e[klo], w[klo], dum1, w[klo + 1], dum2);

	slew3(xi - x[klo], h[klo], y[klo], q, omega, alpha, dum1);
}