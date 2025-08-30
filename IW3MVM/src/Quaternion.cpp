#include "stdafx.hpp"
#include "Math.hpp"
#include "Quaternion.hpp"

QEulerAngles::QEulerAngles(double pitch, double yaw, double roll)
{
	this->pitch = pitch;
	this->yaw = yaw;
	this->roll = roll;
}

QREulerAngles::QREulerAngles(double pitch, double yaw, double roll)
{
	this->pitch = pitch;
	this->yaw = yaw;
	this->roll = roll;
}

QREulerAngles QREulerAngles::FromQEulerAngles(QEulerAngles a)
{
	return QREulerAngles(
		M_PI * a.pitch / 180.0,
		M_PI * a.yaw / 180.0,
		M_PI * a.roll / 180.0
	);
}

QEulerAngles QREulerAngles::ToQEulerAngles(void)
{
	return QEulerAngles(
		180.0 * pitch / M_PI,
		180.0 * yaw / M_PI,
		180.0 * roll / M_PI
	);
}

Quaternion operator +(Quaternion a, Quaternion b)
{
	return Quaternion(
		a.w + b.w,
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	);
}

Quaternion operator *(double a, Quaternion b)
{
	return Quaternion(
		a*b.w,
		a*b.x,
		a*b.y,
		a*b.z
	);
}

Quaternion operator *(Quaternion a, Quaternion b)
{
	return Quaternion(
		a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
		a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
		a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
		a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w
	);
}

double Q_DotProduct(Quaternion a, Quaternion b)
{
	return a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
}

Quaternion Quaternion::FromQREulerAngles(QREulerAngles a)
{
	double pitchH = 0.5 * a.pitch;
	Quaternion qPitchY(cos(pitchH), 0.0, sin(pitchH), 0.0);

	double yawH = 0.5 * a.yaw;
	Quaternion qYawZ(cos(yawH), 0.0, 0.0, sin(yawH));

	double rollH = 0.5 * a.roll;
	Quaternion qRollX(cos(rollH), sin(rollH), 0.0, 0.0);

	return qYawZ * qPitchY * qRollX;
}

Quaternion::Quaternion()
{
	w = 1.0;
	x = 0.0;
	y = 0.0;
	z = 0.0;
}

Quaternion::Quaternion(double w, double x, double y, double z)
{
	this->w = w;
	this->x = x;
	this->y = y;
	this->z = z;
}

double Quaternion::Norm()
{
	return sqrt(w*w + x*x + y*y + z*z);
}

QREulerAngles Quaternion::ToQREulerAngles()
{
	// Quaternion to matrix conversion taken from:
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm

	// Quaternion to euler conversion analog (but changed) to:
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/index.htm

	double sqw = w*w;
	double sqx = x*x;
	double sqy = y*y;
	double sqz = z*z;

	double ssq = sqx + sqy + sqz + sqw;
	double invs = ssq ? 1 / ssq : 0;
	double m00 = (sqx - sqy - sqz + sqw)*invs;
	//double m11 = (-sqx + sqy - sqz + sqw)*invs;
	double m22 = (-sqx - sqy + sqz + sqw)*invs;

	double tmp1 = x*y;
	double tmp2 = z*w;
	double m10 = 2.0 * (tmp1 + tmp2)*invs;
	//double m01 = 2.0 * (tmp1 - tmp2)*invs;

	tmp1 = x*z;
	tmp2 = y*w;
	double m20 = 2.0 * (tmp1 - tmp2)*invs;
	//double m02 = 2.0 * (tmp1 + tmp2)*invs;

	tmp1 = y*z;
	tmp2 = x*w;
	double m21 = 2.0 * (tmp1 + tmp2)*invs;
	//double m12 = 2.0 * (tmp1 - tmp2)*invs;

	// X =            Y =            Z =
	// |1, 0 , 0  | |cp , 0, sp| |cy, -sy, 0|
	// |0, cr, -sr| |0  , 1, 0 | |sy, cy , 0|
	// |0, sr, cr | |-sp, 0, cp| |0 , 0  , 1|

	// Y*X =
	// |cp , sp*sr, sp*cr|
	// |0  , cr   , -sr  |
	// |-sp, cp*sr, cp*cr|

	// Z*(Y*X) =
	// |cy*cp, cy*sp*sr -sy*cr, cy*sp*cr +sy*sr |
	// |sy*cp, sy*sp*sr +cy*cr, sy*sp*cr +cy*-sr|
	// |-sp  , cp*sr          , cp*cr           |

	// 1) cy*cp = m00
	// 2) cy*sp*sr -sy*cr = m01
	// 3) cy*sp*cr +sy*sr = m02
	// 4) sy*cp = m10
	// 5) sy*sp*sr +cy*cr = m11
	// 6) sy*sp*cr +cy*-sr = m12
	// 7) -sp = m20
	// 8) cp*sr = m21
	// 9) cp*cr = m22
	//
	// 7=> p = arcsin( -m20 )
	//
	// 4/1=> y = arctan2( m10, m00 )
	//
	// 8/9=> r = arctan2( m21, m22 )

	double sinYPitch = -m20;
	double yPitch;
	double zYaw;
	double xRoll;

	if (sinYPitch > 1.0 - EPS)
	{
		// sout pole singularity:

		yPitch = M_PI / 2.0;

		xRoll = -2.0*atan2(z*invs, w*invs);
		zYaw = 0;
	}
	else
		if (sinYPitch < -1.0 + EPS)
		{
			// north pole singularity:

			yPitch = -M_PI / 2.0;
			xRoll = 2.0*atan2(z*invs, w*invs);
			zYaw = 0;
		}
		else
		{
			// hopefully no singularity:

			yPitch = asin(sinYPitch);
			zYaw = atan2(m10, m00);
			xRoll = atan2(m21, m22);
		}

	return QREulerAngles(
		yPitch,
		zYaw,
		xRoll
	);
}