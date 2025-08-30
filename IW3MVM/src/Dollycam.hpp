#pragma once
#include "Quaternion.hpp"

struct Node
{
	double x;
	double y;
	double z;

	double pitch;
	double yaw;
	double roll;

	double fov;

	Node();

	Node(double x, double y, double z, double pitch, double yaw, double roll, double fov);
};

struct COSValue
{
	vec3_t T;
	Quaternion R;
	double fov;
	float sunX;
	float sunY;
	float sunZ;
	void * pUser;
};

struct Build_s
{
	double * T;
	double * X;
	double * X2;
	double * Y;
	double * Y2;
	double * Z;
	double * Z2;
	double(*Q_y)[4];
	double * Q_h;
	double * Q_dtheta;
	double(*Q_e)[3];
	double(*Q_w)[3];
	double * Fov;
	double * Fov2;

	Build_s();
};

extern Build_s build;

typedef std::map<double, COSValue> nodes_t;
typedef std::map<double, COSValue>::iterator nodes_t_it;
extern nodes_t nodes;

extern void StartDolly();
extern void StopDolly();
extern Node GetNode(double t);
extern void AddNode();
extern void ClearNodes();
static COSValue Eval(double t);

enum class DollycamState {
	Disabled, Waiting, Playing, Rendering
};

extern DollycamState currentDollycamState;

extern float dollyTimescale; 
extern float dollyMaxFps;

extern uint64_t frozenDollyBaseFrame;
extern uint64_t frozenDollyCurrentFrame;

extern const char* campathExportDirectory;
extern const char* campathExportExtension;