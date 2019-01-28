#pragma once
#ifndef TRIANGLEH
#define TRIANGLEH

#include "hitable.h"

class triangle : public hitable {
public:
	__device__ triangle() {}

	__device__ triangle(vec3& aa, vec3& bb, vec3& cc, vec3& nn, float s = 1.0) : a(aa), b(bb), c(cc), normal(nn), scale(s) {
		// edge vectors
		e1 = b - a;
		e2 = c - a;

		normal = cross(e1, e2);
		normal = unit_vector(normal);

		d = dot(normal, a);
	};

	__device__ virtual bool hit(const ray& r, float tmin, float t_max, hit_record&) const;
	

	vec3 a, b, c;
	vec3 e1, e2;
	vec3 normal; // face/plane normal
	float area;
	float d;
	float scale;
};

__device__ bool triangle::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
	vec3 pvec = cross(r.direction(), e2);
	float aNum = dot(pvec, e1);

	// Backfacing / nearly parallel, or close to the limit of precision ?
	if (abs(aNum) < 1E-8)
		return false;

	vec3 tvec = r.origin() - a;
	float u = dot(pvec, tvec) / aNum;
	if (u < 0.0 || u > 1.0)
		return false;

	vec3 qVec = cross(tvec, e1);
	float v = dot(qVec, r.direction()) / aNum;
	if (v < 0.0 || u + v > 1.0)
		return false;

	float t = dot(qVec, e2) / aNum;
	if (t < t_min || t > t_max)
		return false;

	// valid intersection
	//rec.u = (a.u() * (1.0 - u - v) + b.u() * u + c.u() * v);
	//rec.v = (a.v() * (1.0 - u - v) + b.v() * u + c.v() * v);
	rec.t = t;
	rec.p = r.point_at_parameter(rec.t);
	rec.normal = normal;
	return true;
}
#endif