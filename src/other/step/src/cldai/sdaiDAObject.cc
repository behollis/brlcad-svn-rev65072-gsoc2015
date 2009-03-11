
//#include "/proj/pdevel/autoc/arch.sunCC.iv/scl_cf.h"
// scl_cf.h is an scl configuration file generated by the configure script 
// generated using gnu's autoconf
#ifdef HAVE_CONFIG_H
#include <scl_cf.h>
#endif

#include <memory.h>
#include <math.h>

#ifdef __OSTORE__
#include <ostore/ostore.hh>    // Required to access ObjectStore Class Library
#endif

#include <sdai.h>

#include "impconv.hh"

// to help ObjectCenter
#ifndef HAVE_MEMMOVE
extern "C"
{
void * memmove(void *__s1, const void *__s2, size_t __n);
}
#endif

#ifdef __OSTORE__
void force_vfts (void*){
  force_vfts(new os_Array<int>);
  force_vfts(new os_Bag<int>);
  force_vfts(new os_Collection<int>);
  force_vfts(new os_List<int>);
  force_vfts(new os_Set<int>);
}
#endif

SCLP23(DAObject)::SCLP23_NAME(DAObject)()
{
  _pid_da = SCLP23(PID_DA)::_nil();
}

SCLP23(DAObject)::SCLP23_NAME(DAObject)(const SCLP23_NAME(DAObject) &da_obj)
{
  _pid_da = da_obj._pid_da;
}

SCLP23(DAObject)::~SCLP23_NAME(DAObject)()
{
}

SCLP23(DAObject_ptr) 
SCLP23(DAObject)::_duplicate(SCLP23(DAObject_ptr) object)
{
  return object;
}

SCLP23(DAObject_ptr) 
SCLP23(DAObject)::_narrow(SCLP23(Object_ptr) object)
{
  void * cast = (void *) object;

  return (SCLP23(DAObject_ptr)) cast;
}

SCLP23(DAObject_ptr) 
SCLP23(DAObject)::_nil()
{
  return (SCLP23(DAObject_ptr)) NULL;
}

#ifndef PART26
SCLBOOL(Bool) 
SCLP23(DAObject)::dado_same(SCLP23(DAObject_ptr) obj) 
{ 
    if(this == obj)
      return SCLBOOL(BTrue);
    else
      return SCLBOOL(BFalse);
/*
    if(obj->dado_oid() && _dado_oid)
    {
	if(strcmp(obj->dado_oid()->oid(),_dado_oid->oid())) 
	  return SCLBOOL(BFalse);
	else
	  return SCLBOOL(BTrue);
    }
    else
      return SCLBOOL(BUnset);
*/
}
#else

SCLBOOL(Bool) 
SCLP23(DAObject)::dado_same(SCLP22(DAObject_ptr) obj, 
			    CORBA_REF(Environment) &IT_env) 
		throw (CORBA_REF(SystemException)) 
{
  P22toP23Ptr(DAObject, obj, daobj);

  if (this == daobj)
    return SCLBOOL(BTrue);
  else
    return SCLBOOL(BFalse);
}
#endif

#ifndef PART26
//SCLP23_NAME(DAObjectID) dado_oid()
SCLP23(DAObjectID_ptr) 
SCLP23(DAObject)::dado_oid()
{
    if(_pid_da)
    {
	return _pid_da->oid();
    }
    else
      return 0;
}
#else
//SCLP22(DAObjectID) dado_oid (CORBA::Environment &IT_env=CORBA::IT_chooseDefaultEnv ()) throw (CORBA::SystemException)
//SCLP22_NAME(DAObjectID) dado_oid (

SCLP22(DAObjectID) 
SCLP23(DAObject)::dado_oid (CORBA_REF(Environment) &IT_env) 
	throw (CORBA_REF(SystemException))
{ 
    if(_pid_da)
    {
	return _pid_da->oid();
    }
    else
      return 0;
    /* return null until this is set up to return the 
       corba tie object for _dado_oid return NULL; */
}
//	{ return CORBA::string_dupl(_dado_oid); }
#endif

#ifndef PART26
SCLP23(PID_DA_ptr) 
SCLP23(DAObject)::dado_pid()
{
    return _pid_da;
}
#else
SCLP22(PID_DA_ptr) 
SCLP23(DAObject)::dado_pid (CORBA_REF(Environment) &IT_env)
	throw (CORBA_REF(SystemException))
{
    P23toP22Ptr(PID_DA, this, pidda);
    return pidda;
}
#endif
#ifndef PART26
void 
SCLP23(DAObject)::dado_remove()
{ }
#else
void 
SCLP23(DAObject)::dado_remove (CORBA_REF(Environment) &IT_env) 
	throw (CORBA_REF(SystemException))
{ }
#endif
#ifndef PART26
void 
SCLP23(DAObject)::dado_free()
{ }
#else
void 
SCLP23(DAObject)::dado_free (CORBA(Environment) &IT_env) 
	throw (CORBA_REF(SystemException))
{ }
#endif

#ifdef __OSTORE__
void 
SCLP23(DAObject)::Access_hook_in(void *object, 
	enum os_access_reason reason, void *user_data, 
	void *start_range, void *end_range)
{
    cout << "****WARNING: virtual DAObject::Access_hook_in() called" << endl;
    SCLP23(DAObject_ptr) ent = (SCLP23(DAObject_ptr))object;
/*
    ent->Access_hook_in(object, reason, user_data, start_range, end_range);
*/
}
#endif

#ifdef PART26
//SCLP26(Application_instance_ptr) 
SCLP22_NAME(Application_instance_ptr) 
SCLP23(DAObject)::create_TIE() 
{
    cout << "ERROR DAObject::create_TIE() called." << endl;
    return 0;
}
#endif

#ifdef PART26
SCLP22_BASE(Object_ptr) 
SCLP23(DAObject)::newTIE(void)
{
  SCLP22_BASE(Object_ptr) tie = new SDAI_TIE_DAObject(this);
  return tie;
}
#endif


#ifdef PART26
    virtual SCLP23_NAME(DAObject_ptr) create_DAObject(
           CORBA_REF(Environment) &IT_env = CORBA_REF(default_environment))
           throw (CORBA_REF(SystemException));
#else
    virtual SCLP23_NAME(DAObject_ptr) create_DAObject()
           throw (CORBA_REF(SystemException));
#endif


/*
 * Copyright (c) 1990, 1991 Stanford University
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Stanford not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Stanford makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * STANFORD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL STANFORD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * UArray implementation.
 */

/*****************************************************************************/

SCLP23(DAObject__set)::SCLP23_NAME(DAObject__set) (int defaultSize)
#ifdef __OSTORE__
// : _rep(os_Collection<DAObject_ptr>::create(os_database::of(this)) ), 
 : _rep(os_List<SCLP23_NAME(DAObject_ptr)>::create(os_database::of(this)) ), 
   _cursor(_rep),
   _first(SCLBOOL(BTrue))
{
//    _rep = os_Set<DAObject_ptr>::create(os_database::of(this));
}
#else
{
    _bufsize = defaultSize;
    _buf = new SCLP23(DAObject_ptr)[_bufsize];
    _count = 0;
}
#endif

SCLP23(DAObject__set)::~SCLP23_NAME(DAObject__set) () 
{
#ifndef __OSTORE__
    delete _buf;
#endif
}

void SCLP23(DAObject__set)::Check (int index) {

#ifdef __OSTORE__
#else
    SCLP23(DAObject_ptr)* newbuf;

    if (index >= _bufsize) {
        _bufsize = (index+1) * 2;
        newbuf = new SCLP23(DAObject_ptr)[_bufsize];
        memmove(newbuf, _buf, _count*sizeof(SCLP23(DAObject_ptr)));
        delete _buf;
        _buf = newbuf;
    }
#endif
}

void 
SCLP23(DAObject__set)::Insert (SCLP23(DAObject_ptr) v, int index) {

#ifdef __OSTORE__
    _rep.insert_before( (SCLP23(DAObject_ptr)) v, index);
#else
    SCLP23(DAObject_ptr)* spot;
    index = (index < 0) ? _count : index;

    if (index < _count) {
        Check(_count+1);
        spot = &_buf[index];
        memmove(spot+1, spot, (_count - index)*sizeof(SCLP23(DAObject_ptr)));

    } else {
        Check(index);
        spot = &_buf[index];
    }
    *spot = v;
    ++_count;
#endif
}

void SCLP23(DAObject__set)::Append (SCLP23(DAObject_ptr) v) {

#ifdef __OSTORE__
    _rep.insert_last( (SCLP23(DAObject_ptr)) v);
#else
    int index = _count;
    SCLP23(DAObject_ptr)* spot;

    if (index < _count) {
        Check(_count+1);
        spot = &_buf[index];
        memmove(spot+1, spot, (_count - index)*sizeof(SCLP23(DAObject_ptr)));

    } else {
        Check(index);
        spot = &_buf[index];
    }
    *spot = v;
    ++_count;
#endif
}

void SCLP23(DAObject__set)::Remove (int index) {

#ifdef __OSTORE__
    _rep.remove_at( index );
#else
    if (0 <= index && index < _count) {
        --_count;
        SCLP23(DAObject_ptr)* spot = &_buf[index];
        memmove(spot, spot+1, (_count - index)*sizeof(SCLP23(DAObject_ptr)));
    }
#endif
}

#ifndef __OSTORE__
int SCLP23(DAObject__set)::Index (SCLP23(DAObject_ptr) v) {

    for (int i = 0; i < _count; ++i) {
        if (_buf[i] == v) {
            return i;
        }
    }
    return -1;
}
#endif

SCLP23(DAObject_ptr) 
SCLP23(DAObject__set)::retrieve(int index)
{
#ifdef __OSTORE__
    return _rep.retrieve(index);
#else
    return operator[](index);
#endif
}

#ifndef __OSTORE__
SCLP23(DAObject_ptr)& SCLP23(DAObject__set)::operator[] (int index) {

    Check(index);
//    _count = max(_count, index+1);
    _count = ( (_count > index+1) ? _count : (index+1) );
    return _buf[index];
}
#endif

int 
SCLP23(DAObject__set)::Count()
{
#ifdef __OSTORE__
    return _rep.cardinality();
#else
    return _count; 
#endif
}

int 
SCLP23(DAObject__set)::is_empty()
{
#ifdef __OSTORE__
    return _rep.empty();
#else
    return _count; 
#endif
}

#ifndef __OSTORE__
void 
SCLP23(DAObject__set)::Clear()
{
    _count = 0; 
}
#endif

#ifdef __OSTORE__

SCLP23(DAObject_ptr) 
SCLP23(DAObject__set)::first()
{
    return _cursor.first();
}

SCLP23(DAObject_ptr) 
SCLP23(DAObject__set)::next()
{
    return _cursor.next();
}

SCLP23(Integer) 
SCLP23(DAObject__set)::more()
{
    return _cursor.more();
}

#endif
