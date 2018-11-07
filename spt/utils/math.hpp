#pragma once

#ifndef M_PI
const double M_PI = 3.14159265358979323846;
#endif
const double M_RAD2DEG = 180 / M_PI;
const double M_DEG2RAD = M_PI / 180;

double NormalizeRad(double a);
double NormalizeDeg(double a);

#define dim(x) (sizeof(x) / sizeof((x)[0]))