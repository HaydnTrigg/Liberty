/* ---------- headers */

#include "EngineObject.h"

#include <Dump.h>
#include <Core.h>

/* ---------- prototypes */

_extern void __thiscall sub_627CAF0(EngineObject const*);

/* ---------- public code */

EngineObject::EngineObject(const EngineObject& copy)
{
    mangled_assert("??0EngineObject@@QAE@ABU0@@Z");

    *this = copy;
}

EngineObject::EngineObject() :
    index(-1),
    transform(),
    radius(),
    centerOfMass(),
    instanceFlags()
{
    mangled_assert("??0EngineObject@@QAE@XZ");
}

COMMON_DEC EngineObject& EngineObject::operator=(EngineObject const& copy)
{
    mangled_assert("??4EngineObject@@QAEAAU0@ABU0@@Z");

    this->index = copy.index;
    this->transform = copy.transform;

    return *this;
}

COMMON_DEC const Matrix& EngineObject::get_orientation() const
{
    mangled_assert("?get_orientation@EngineObject@@QBEABVMatrix@@XZ");

    return transform.get_orientation();
}

COMMON_DEC const Vector& EngineObject::get_position() const
{
    mangled_assert("?get_position@EngineObject@@QBEABVVector@@XZ");

    return transform.get_position();
}

COMMON_DEC const float EngineObject::get_radius() const
{
    mangled_assert("?get_radius@EngineObject@@QBE?BMXZ");

    return radius;
}

COMMON_DEC const Transform& EngineObject::get_transform() const
{
    mangled_assert("?get_transform@EngineObject@@QBEABVTransform@@XZ");

    return transform;
}

COMMON_DEC void EngineObject::set_orientation(const Matrix& orientation)
{
    mangled_assert("?set_orientation@EngineObject@@QAEXABVMatrix@@@Z");

    transform.set_orientation(orientation);
}

COMMON_DEC void EngineObject::set_position(const Vector& pos)
{
    mangled_assert("?set_position@EngineObject@@QAEXABVVector@@@Z");

    transform.set_position(pos);
}

COMMON_DEC void EngineObject::set_transform(const Transform& transform)
{
    mangled_assert("?set_transform@EngineObject@@QAEXABVTransform@@@Z");

    this->transform = transform;
}

COMMON_DEC void EngineObject::update_tree() const
{
    mangled_assert("?update_tree@EngineObject@@QBEXXZ");

    sub_627CAF0(this);
}

/* ---------- private code */

COMMON_DEC void __stdcall EngineObject::initialize_instance(long index)
{
    mangled_assert("?initialize_instance@EngineObject@@EAGXJ@Z");

    this->index = index;
}

COMMON_DEC void __stdcall EngineObject::create_instance(long index)
{
    mangled_assert("?create_instance@EngineObject@@EAGXJ@Z");
    unused(index);

    noop();
}

COMMON_DEC void __stdcall EngineObject::destroy_instance(long index)
{
    mangled_assert("?destroy_instance@EngineObject@@EAGXJ@Z");
    unused(index);

    this->index = -1;
}

COMMON_DEC void __stdcall EngineObject::set_position(long index, const Vector& pos)
{
    mangled_assert("?set_position@EngineObject@@EAGXJABVVector@@@Z");
    unused(index);

    transform.set_position(pos);
}

COMMON_DEC const Vector& __stdcall EngineObject::get_position(long index) const
{
    mangled_assert("?get_position@EngineObject@@EBGABVVector@@J@Z");
    unused(index);

    return transform.get_position();
}

COMMON_DEC void __stdcall EngineObject::set_orientation(long index, const Matrix& rotation)
{
    mangled_assert("?set_orientation@EngineObject@@EAGXJABVMatrix@@@Z");
    unused(index);

    transform.set_orientation(rotation);
}

COMMON_DEC const Matrix& __stdcall EngineObject::get_orientation(long index) const
{
    mangled_assert("?get_orientation@EngineObject@@EBGABVMatrix@@J@Z");
    unused(index);

    return transform.get_orientation();
}

COMMON_DEC void __stdcall EngineObject::set_transform(long index, const Transform& transform)
{
    mangled_assert("?set_transform@EngineObject@@EAGXJABVTransform@@@Z");
    unused(index);

    this->transform = transform;
}

COMMON_DEC const Transform& __stdcall EngineObject::get_transform(long index) const
{
    mangled_assert("?get_transform@EngineObject@@EBGABVTransform@@J@Z");
    unused(index);

    return transform;
}

COMMON_DEC void __stdcall EngineObject::get_centered_radius(long index, float* out_radius, Vector* out_centerOfMass) const
{
    mangled_assert("?get_centered_radius@EngineObject@@EBGXJPAMPAVVector@@@Z");
    unused(index);

    *out_radius = radius;
    *out_centerOfMass = centerOfMass;
}

COMMON_DEC void __stdcall EngineObject::set_centered_radius(long index, float radius, Vector const& centerOfMass)
{
    mangled_assert("?set_centered_radius@EngineObject@@EAGXJMABVVector@@@Z");
    unused(index);

    this->radius = radius;
    this->centerOfMass = centerOfMass;
}

COMMON_DEC void __stdcall EngineObject::set_instance_flags(long index, unsigned long flags)
{
    mangled_assert("?set_instance_flags@EngineObject@@EAGXJK@Z");
    unused(index);

    instanceFlags = flags;
}

COMMON_DEC unsigned long __stdcall EngineObject::get_instance_flags(long index) const
{
    mangled_assert("?get_instance_flags@EngineObject@@EBGKJ@Z");
    unused(index);

    return instanceFlags;
}

COMMON_DEC bool __stdcall EngineObject::joint_changed(long index)
{
    mangled_assert("?joint_changed@EngineObject@@EAG_NJ@Z");
    unused(index);

    return false;
}

EngineObject::~EngineObject()
{
    noop();
}

COMMON_DEC const Vector& __stdcall EngineObject::get_velocity(long index) const
{
    mangled_assert("?get_velocity@EngineObject@@EBGABVVector@@J@Z");
    unused(index);

    static const Vector v;
    return v;
}

COMMON_DEC void __stdcall EngineObject::set_velocity(long index, Vector const& velocity)
{
    mangled_assert("?set_velocity@EngineObject@@EAGXJABVVector@@@Z");
    unused(index);
    unused(velocity);

    noop();
}

COMMON_DEC const Vector& __stdcall EngineObject::get_angular_velocity(long index) const
{
    mangled_assert("?get_angular_velocity@EngineObject@@EBGABVVector@@J@Z");
    unused(index);

    static const Vector v;
    return v;
}

COMMON_DEC void __stdcall EngineObject::set_angular_velocity(long index, Vector const& angularVelocity)
{
    mangled_assert("?set_angular_velocity@EngineObject@@EAGXJABVVector@@@Z");
    unused(index);
    unused(angularVelocity);

    noop();
}

COMMON_DEC void EngineObject::set_centered_radius(float radius, Vector const& centerOfMass)
{
    mangled_assert("?set_centered_radius@EngineObject@@AAEXMABVVector@@@Z");

    this->centerOfMass = centerOfMass;
    this->radius = radius;
}

/* ---------- reverse engineering */

#include <new>
_extern EngineObject* __thiscall _sub_63098A0(EngineObject* _this);
_extern EngineObject* __thiscall sub_63098A0(EngineObject* _this)
{
    return new(_this) EngineObject();
}