// This code is in the public domain -- castanyo@yahoo.es

#include "Box.h"
#include "Box.inl"
//#include "Sphere.h"

using namespace nv;



float nv::distanceSquared(const Box &box, const Vector3 &point) {
    Vector3 closest;

    if (point.x < box.minCorner.x) closest.x = box.minCorner.x;
    else if (point.x > box.maxCorner.x) closest.x = box.maxCorner.x;
    else closest.x = point.x;

    if (point.y < box.minCorner.y) closest.y = box.minCorner.y;
    else if (point.y > box.maxCorner.y) closest.y = box.maxCorner.y;
    else closest.y = point.y;

    if (point.z < box.minCorner.z) closest.z = box.minCorner.z;
    else if (point.z > box.maxCorner.z) closest.z = box.maxCorner.z;
    else closest.z = point.z;

    return lengthSquared(point - closest);
}

/*bool nv::overlap(const Box &box, const Sphere &sphere) {
    return distanceSquared(box, sphere.center) < sphere.radius * sphere.radius;
}*/
