#pragma once 

struct QEulerAngles
{
	double pitch;
	double yaw;
	double roll;

	QEulerAngles(double pitch, double yaw, double roll);
};

struct QREulerAngles
{
	static QREulerAngles FromQEulerAngles(QEulerAngles a);

	double pitch;
	double yaw;
	double roll;

	QREulerAngles(double pitch, double yaw, double roll);

	QEulerAngles ToQEulerAngles(void);
};

struct Quaternion
{
	static Quaternion FromQREulerAngles(QREulerAngles a);

	double w;
	double x;
	double y;
	double z;

	Quaternion();
	Quaternion(double w, double x, double y, double z);

	double Norm();

	QREulerAngles ToQREulerAngles();
};

extern double Q_DotProduct(Quaternion a, Quaternion b);
