#include "stdafx.hpp"

#include "Math.hpp"
#include "Dollycam.hpp"
#include "Utility.hpp"
#include "Game.hpp"
#include "Console.hpp"
#include "Demo.hpp"
#include "Freecam.hpp"
#include "Drawing.hpp"
#include "Streams.hpp"


nodes_t nodes;
Build_s build;

float dollyTimescale;
float dollyMaxFps;

uint64_t frozenDollyBaseFrame;
uint64_t frozenDollyCurrentFrame;

DollycamState currentDollycamState = DollycamState::Disabled;

const char* campathExportDirectory = "main";
const char* campathExportExtension = "cam";

void CameraExport()
{
	if (Demo_GetName().size() != 0 && nodes.size() > 0)
	{
		if (Cmd_Argc() == 2)
		{
			std::ofstream file;
			std::string path = va("%s/%s.%s", campathExportDirectory, Cmd_Argv(1), campathExportExtension);
			file.open(path);

			file << mvm_dolly_frozen->current.enabled << "_";

			for (auto it = nodes.begin(); it != nodes.end(); it++)
			{
				std::string tick = std::string(va("%u", it->first));
				std::string fov = std::string(va("%g", it->second.fov));
				std::string rw = std::string(va("%g", it->second.R.w));
				std::string rx = std::string(va("%g", it->second.R.x));
				std::string ry = std::string(va("%g", it->second.R.y));
				std::string rz = std::string(va("%g", it->second.R.z));
				std::string x = std::string(va("%g", it->second.T[0]));
				std::string y = std::string(va("%g", it->second.T[1]));
				std::string z = std::string(va("%g", it->second.T[2]));

				file << tick << "/" << fov << "/" << rw << "/" << rx << "/" << ry << "/" << rz <<
					"/" << x << "/" << y << "/" << z << ";";
			}

			Console::Log(GREEN, va("Successfully wrote cam data to '%s'!\n", path.c_str()));

			file.close();
		}
		else
		{
			Console::Log(YELLOW, "Usage: mvm_cam_export <filename>\n");
		}
	}
	else
	{
		Console::Log(RED, "Exporting camera nodes requires you to be in a demo, having placed at least 1 node!\n");
	}
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (getline(ss, item, delim))
	{
		elems.push_back(item);
	}
	return elems;
}

void CameraImport()
{
	if ((Demo_GetName().size() != 0))
	{
		if (nodes.size() == 0)
		{
			if (Cmd_Argc() == 2)
			{
				std::ifstream file;
				std::string path = va("%s/%s.%s", campathExportDirectory, Cmd_Argv(1), campathExportExtension);
				file.open(path);
				if (!file.good())
				{
					Console::Log(RED, va("Couldn't read file '%s'\n", path.c_str()));
					file.close();
					return;
				}

				std::stringstream strStream;
				strStream << file.rdbuf();
				std::string data = strStream.str();

				nodes.clear();

				std::vector<std::string> high = split(data, '_');
				mvm_dolly_frozen->current.enabled = (bool)std::atoi(high[0].c_str());

				std::vector<std::string> nodeList = split(high[1], ';');

				for (std::string nodestring : nodeList)
				{
					std::vector<std::string> info = split(nodestring, '/');

					double tick = atoll(info[0].c_str());
					double fov = strtod(info[1].c_str(), NULL);
					double rw = strtod(info[2].c_str(), NULL);
					double rx = strtod(info[3].c_str(), NULL);
					double ry = strtod(info[4].c_str(), NULL);
					double rz = strtod(info[5].c_str(), NULL);
					double x = strtod(info[6].c_str(), NULL);
					double y = strtod(info[7].c_str(), NULL);
					double z = strtod(info[8].c_str(), NULL);

					COSValue cos = COSValue();
					cos.fov = fov;
					cos.R = Quaternion(rw, rx, ry, rz);
					cos.T[0] = (float)x;
					cos.T[1] = (float)y;
					cos.T[2] = (float)z;

					nodes.insert(std::pair<double, COSValue>(tick, cos));
				}

				Console::Log(GREEN, va("Successfully read cam data from '%s'!\n", path.c_str()));

				file.close();
			}
			else
			{
				Console::Log(YELLOW, "Usage: mvm_cam_import <filename>\n");
			}
		}
		else
		{
			Console::Log(YELLOW, "Please clear your dollycam nodes before importing them from a file!");
		}
	}
	else
	{
		Console::Log(YELLOW, "Importing camera nodes requires you to be viewing a demo!");
	}
}


void StartDolly()
{
	currentDollycamState = DollycamState::Waiting;

	dollyTimescale = Dvar_FindVar("timescale")->current.value;
	dollyMaxFps = (float)maxfps_cap;
	Dvar_FindVar("com_maxfps")->current.value = (float)maxfps_cap;

	uint32_t requiredNodeCount = mvm_dolly_linear->current.enabled ? 2 : 4;

	if (nodes.size() < requiredNodeCount)
	{
		Console::Log(RED, "Place at least %u points to start the dollycam\n", requiredNodeCount);
		currentDollycamState = DollycamState::Disabled;
		return;
	}

	currentView.isPOV = false;
	
	if (!mvm_dolly_frozen->current.enabled)
	{
		if (Demo_IsPaused())
			Demo_TogglePause();

		if (Demo_GetCurrentTick() > nodes.begin()->first)
		{
			Console::Log(BLUE, "Reloading demo %s", Demo_GetName().c_str());

			Cbuf_AddText(va("demo %s\n", va("\"%s\"", Demo_GetName().c_str())));
			Console::Log(GREEN, "Playing campath!");
		}
	}
	else 
	{
		Console::Log(GREEN, "Playing frozen campath!");
	}

}

void StopDolly()
{
	currentDollycamState = DollycamState::Disabled;
	if (recordingStatus == RecordingStatus::Running)
		recordingStatus = RecordingStatus::Finish;
	demoDrawMenu = true;
	Console::Log(BLUE, "Stopped dolly!");
}

void AddNode()
{
	if (currentDollycamState != DollycamState::Disabled)
		return;

	if (nodes.count(Demo_GetCurrentTick()) && !mvm_dolly_frozen->current.enabled)
	{
		Console::Log(RED, "You can't place 2 nodes on the same tick.\nSet mvm_dolly_frozen to 1 if you intend on doing a frozen cinematic!");
		return;
	}

	double t = mvm_dolly_frozen->current.enabled ? (nodes.size() * 5000) : Demo_GetCurrentTick();

	COSValue value;

	value.T[0] = currentView.position[0];
	value.T[1] = currentView.position[1];
	value.T[2] = currentView.position[2];

	value.R = Quaternion::FromQREulerAngles(QREulerAngles::FromQEulerAngles(QEulerAngles(
		currentView.rotation[0],
		currentView.rotation[1],
		currentView.rotation[2]
		)));

	value.fov = Dvar_FindVar("cg_fov")->current.value;

	value.sunX = Dvar_FindVar("r_lightTweakSunDirection")->current.vector[0];
	value.sunY = Dvar_FindVar("r_lightTweakSunDirection")->current.vector[1];
	value.sunZ = Dvar_FindVar("r_lightTweakSunDirection")->current.vector[2];

	value.pUser = 0;

	nodes.insert(std::pair<double, COSValue>(t, value));

	Console::Log(GREEN, "Added node at ( %g | %g | %g )!", value.T[0], value.T[1], value.T[2]);
}

void ClearNodes()
{
	if (currentDollycamState == DollycamState::Disabled)
	{
		nodes.clear();
		Console::Log(GREEN, "Nodes cleared!");
	}
}

COSValue Eval(double t)
{
	COSValue result;

	int n = nodes.size();

#pragma region rebuild
#pragma region free
	delete build.Fov2;
	build.Fov2 = 0;
	delete build.Fov;
	build.Fov = 0;
	delete[] build.Q_w;
	build.Q_w = 0;
	delete[] build.Q_e;
	build.Q_e = 0;
	delete build.Q_dtheta;
	build.Q_dtheta = 0;
	delete build.Q_h;
	build.Q_h = 0;
	delete[] build.Q_y;
	build.Q_y = 0;
	delete build.Z2;
	build.Z2 = 0;
	delete build.Z;
	build.Z = 0;
	delete build.Y2;
	build.Y2 = 0;
	delete build.Y;
	build.Y = 0;
	delete build.X2;
	build.X2 = 0;
	delete build.X;
	build.X = 0;
	delete build.T;
	build.T = 0;
#pragma endregion

	build.T = new double[n];
	build.X = new double[n];
	build.X2 = new double[n];
	build.Y = new double[n];
	build.Y2 = new double[n];
	build.Z = new double[n];
	build.Z2 = new double[n];
	build.Q_y = new double[n][4];
	build.Q_h = new double[n - 1];
	build.Q_dtheta = new double[n - 1];
	build.Q_e = new double[n - 1][3];
	build.Q_w = new double[n][3];
	build.Fov = new double[n];
	build.Fov2 = new double[n];

	{
		Quaternion QLast;

		int i = 0;
		for (nodes_t::iterator it = nodes.begin(); it != nodes.end(); it++)
		{

			build.T[i] = it->first;
			build.X[i] = it->second.T[0];
			build.Y[i] = it->second.T[1];
			build.Z[i] = it->second.T[2];
			build.Fov[i] = it->second.fov;

			Quaternion Q = it->second.R;

			// Make sure we will travel the short way:
			if (0<i)
			{
				// hasLast.
				double dotProduct = Q_DotProduct(Q, QLast);
				if (dotProduct<0.0)
				{
					Q.x *= -1.0;
					Q.y *= -1.0;
					Q.z *= -1.0;
					Q.w *= -1.0;
				}
			}

			build.Q_y[i][0] = Q.x;
			build.Q_y[i][1] = Q.y;
			build.Q_y[i][2] = Q.z;
			build.Q_y[i][3] = Q.w;

			QLast = Q;
			i++;
		}
	}

	spline(build.T, build.X, n, false, 0.0, false, 0.0, build.X2);
	spline(build.T, build.Y, n, false, 0.0, false, 0.0, build.Y2);
	spline(build.T, build.Z, n, false, 0.0, false, 0.0, build.Z2);

	double wi[3] = { 0.0,0.0,0.0 };
	double wf[3] = { 0.0,0.0,0.0 };
	qspline_init(n, 2, EPS, wi, wf, build.T, build.Q_y, build.Q_h, build.Q_dtheta, build.Q_e, build.Q_w);

	spline(build.T, build.Fov, n, false, 0.0, false, 0.0, build.Fov2);

#pragma endregion

	double x, y, z;
	double Q[4], dum1[4], dum2[4];
	double fov;

	splint(build.T, build.X, build.X2, n, t, &x);
	splint(build.T, build.Y, build.Y2, n, t, &y);
	splint(build.T, build.Z, build.Z2, n, t, &z);

	qspline_interp(n, t, build.T, build.Q_y, build.Q_h, build.Q_dtheta, build.Q_e, build.Q_w, Q, dum1, dum2);

	splint(build.T, build.Fov, build.Fov2, n, t, &fov);

	result.T[0] = (float)x;
	result.T[1] = (float)y;
	result.T[2] = (float)z;
	result.R.w = Q[3];
	result.R.x = Q[0];
	result.R.y = Q[1];
	result.R.z = Q[2];
	result.fov = fov;
	result.pUser = 0;

	return result;
}

COSValue Lerp(double t)
{
	COSValue result;

	double startTick, endTick;
	COSValue from;
	for (std::pair<double, COSValue> n : nodes)
	{
		if (n.first < t)
		{
			startTick = n.first;
			from = n.second;
		}
	}

	COSValue to;
	for (std::pair<double, COSValue> n : nodes)
	{
		if (n.first > t)
		{
			endTick = n.first;
			to = n.second;
			break;
		}
	}

	t = (t - startTick) / (endTick - startTick);

	result.T[0] = (float)(from.T[0] + (to.T[0] - from.T[0]) * t);
	result.T[1] = (float)(from.T[1] + (to.T[1] - from.T[1]) * t);
	result.T[2] = (float)(from.T[2] + (to.T[2] - from.T[2]) * t);
	result.R.w = from.R.w + (to.R.w - from.R.w) * t;
	result.R.x = from.R.x + (to.R.x - from.R.x) * t;
	result.R.y = from.R.y + (to.R.y - from.R.y) * t;
	result.R.z = from.R.z + (to.R.z - from.R.z) * t;
	result.fov = from.fov + (to.fov - from.fov) * t;
	result.pUser = 0;
	
	return result;
}

Node GetNode(double t)
{
	COSValue val;
	if (mvm_dolly_linear->current.enabled)
		val = Lerp(t);
	else
		val = Eval(t);
	QEulerAngles angles = val.R.ToQREulerAngles().ToQEulerAngles();

	return Node(val.T[0], val.T[1], val.T[2], angles.pitch, angles.yaw, angles.roll, val.fov);
}

Node::Node()
	: x(0.0), y(0.0), z(0.0), pitch(0.0), yaw(0.0), roll(0.0), fov(90.0)
{
}

Node::Node(double x, double y, double z, double pitch, double yaw, double roll, double fov)
	: x(x), y(y), z(z), pitch(pitch), yaw(yaw), roll(roll), fov(fov)
{
}

Build_s::Build_s()
{
	this->X = 0;
	this->X2 = 0;
	this->Y = 0;
	this->Y2 = 0;
	this->Z = 0;
	this->Z2 = 0;
	this->T = 0;

	this->Fov = 0;
	this->Fov2 = 0;
	this->Q_dtheta = 0;
	this->Q_e = 0;
	this->Q_h = 0;
	this->Q_w = 0;
	this->Q_y = 0;

}
