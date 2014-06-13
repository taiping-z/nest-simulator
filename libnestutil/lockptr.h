/*
 *  lockptr.h
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LOCK_PTR_H
#define LOCK_PTR_H

#ifdef NDEBUG
#define LOCK_PTR_NDEBUG
#endif

#include <cassert>
#include <cstddef>

/**
\class lockPTR

 This template is the standard safe-pointer implementation
 of SYNOD.

 In order for this scheme to work smoothly, the user has to take some
 precautions:
 1. each pointer should only be used ONCE to initialize a lockPTR.
 2. The lockPTR assumes that there are no other access points to the
    protected pointer.
 3. The lockPTR can freely be copied and passed between objects and functions.
 4. lockPTR objects should be used like the pointer to the object.
 5. lockPTR objects should be passed as objects in function calls and
    function return values.
 6. There should be no pointers to lockPTR objects.

 Class lockPTR is designed to behave just like the pointer would.  You
 can use the dereference operators (* and ->) to access the protected
 object. However, the pointer itself is (with exceptions) never
 exposed to the user.

 Since all access to the referenced object is done via a lockPTR, it
 is possible to maintain a count of all active references. If this
 count dropts to zero, the referenced object can savely be destroyed.
 For dynamically allocated objects, delete is envoked on the stored
 pointer.
 
 class lockPTR distinguishes between dynamically and automatically
 allocated objects by the way it is initialised: 

 If a lockPTR is initialised with a pointer, it assumes that the
 referenced object was dynamically allocated and will call the
 destructor once the reference count drops to zero.

 If the lockPTR is initialised with a reference, it assumes that the
 object is automatically allocated. Thus, the lockPTR wil NOT call the
 destructor.

 In some cases it is necessary for a routine to actually get hold of
 the pointer, contained in the lockPTR object.  This can be done by
 using the member function get().  After the pointer has been exposed
 this way, the lockPTR will regard the referenced object as unsafe,
 since the user might call delete on the pointer. Thus, lockPTR will
 "lock" the referenced object and deny all further access.
 The object can be unlocked by calling the unlock() member.

 Equality for lockPTRs is defined as identity of the data object.
*/

template<class D>
class lockPTR
{  
  class PointerObject
  {
    
    private:

    D *pointee;                  // pointer to handled Datum object
    size_t number_of_references;
    bool deletable;
    bool locked;

// forbid this constructor!
    PointerObject(PointerObject const&);
    
    public:
    
    PointerObject(D* p = NULL)
      :pointee(p),
       number_of_references(1),
       deletable(true), locked(false){}
    
    PointerObject(D& p_o)
      :pointee(&p_o),
       number_of_references(1),
       deletable(false), locked(false){}

    ~PointerObject()
    { 
      assert(number_of_references == 0); // This will invalidate the still
                                         // existing pointer!      
      assert(!locked);
      if((pointee != NULL) && deletable && (!locked))
	delete pointee;
    }

    D * get(void) const
    {
      return pointee;
    }
    
    void addReference(void)
    {
      ++number_of_references;
    }

    void subReference(size_t s)
    {
      number_of_references-=s;
    }

    void removeReference(void)
    {
//      assert(number_of_references > 0);
      
      --number_of_references;
      if(number_of_references == 0)
      {
	delete this;
      }
    }

    size_t references(void) const
    {
      return number_of_references;
    }
    
    bool islocked(void) const
      {
	return locked;
      }

    bool isdeletable(void) const
      {
	return deletable;
      }

    void lock(void)
      {
	assert(locked == false);
	locked = true;
      }

    void unlock(void)
      {
	assert(locked == true);
	locked = false;
      }
    
  };
    
  PointerObject *obj;

  public:

  // lockPTR() ; // generated automatically.
  // The default, argumentless constructor is used in
  // class declarations and generates an empty lockPTR
  // object which must then be initialised, for example 
  // by assignement.

  explicit lockPTR(D* p=NULL)
  {
    obj = new PointerObject(p);
    assert(obj != NULL);
  }
  
  explicit lockPTR(D& p_o)
  {
    obj = new PointerObject(p_o);
    assert(obj != NULL);
  }

  lockPTR(const lockPTR<D>& spd)
    : obj(spd.obj)
  {
    assert(obj != NULL);
    obj->addReference();
  }
    
  virtual ~lockPTR()
  {
    //   std::cout << "~lockPTR";

    // MD experimental, we use obj==0 to indicate controlled detachment
    if (exists())
     obj->removeReference();
  }    

  
  // MD, experimental
  void detach(void)
  {
   assert(obj != NULL);
   assert(obj->references()-1>0); // somebody must remain do delete
   obj->subReference(1);

   //std::cout << "after detach ref: " << obj->references() << std::endl;
 
   obj=NULL;
  }


  lockPTR<D> operator=(const lockPTR<D>&spd)
  {
    //  assert(obj != NULL);
    // assert(spd.obj != NULL);
	    
// The following order of the expressions protects
// against a=a;
    
    spd.obj->addReference();
    obj->removeReference();
	    
    obj = spd.obj;
	    
    return *this;
  }

  lockPTR<D> operator=(D &s)
    {
      *this = lockPTR<D>(s);
      assert(!(obj->isdeletable()));
      return *this;
    }

  lockPTR<D> operator=(D const &s)
    {
      *this = lockPTR<D>(s);
      assert(!(obj->isdeletable()));
      return *this;
    }

  D*  get(void)
  {
    assert(!obj->islocked());
    obj->lock(); // Try to lock Object
    return obj->get(); 
  }

  D*  get(void) const
  {
    assert(!obj->islocked());

    obj->lock(); // Try to lock Object
    return obj->get(); 
  }

  D*  operator->() const
  {
    assert(obj->get() != NULL);
    
    return obj->get(); 
  }
  
  D*  operator->()
  {
    assert(obj->get() != NULL);
    
    return obj->get(); 
  }

  D&  operator*()
  {
    assert(obj->get() != NULL);
    
    return *(obj->get()); 
  }
  
  const D&  operator*() const
  {
    assert(obj->get() != NULL);
    return *(obj->get()); 
  }
  
  
  bool operator!() const //!< returns true if and only if obj->pointee == NULL
  {
    // assert(obj != NULL);
    
    return (obj->get() == NULL);
  }


  /* operator==, !=
     Identity operator. These are inherited by derived types, so they should only be called
       by the equals method of the derived class which checks for type identity
       or when both classes are known to be bare lockPTR<D>.

     These follow identity semantics rather than equality semantics.
     The underlying object should only ever be owned by a single PointerObject
     that are shared by lockPTR<D>s, so this is equivalent to comparing the address
     of the D objects.
  */
  bool operator==(const lockPTR<D>& p) const
  {
    return (obj == p.obj);
  }

  bool operator!=(const lockPTR<D>& p) const
  {
    return (obj != p.obj);
  }


  bool valid(void) const    //!< returns true if and only if obj->pointee != NULL
  {
    assert(obj != NULL);
    return (obj->get() != NULL);
  }

 //MD, experimental
  // this condition should only be false inside a recursive call to a datum with
  // self references like the DictionaryDatum
  bool exists(void) const    //!< returns true if and only if obj != NULL
  {
    return (obj != NULL);
  }

  bool islocked(void) const 
  {
    assert(obj != NULL);
    return (obj->islocked());
  }

  bool deletable(void) const 
  {
    assert(obj != NULL);
    return (obj->isdeletable());
  }

  void lock(void) const
    {
      assert(obj!=NULL);
      obj->lock();
    }

  void unlock(void) const
    {
      assert(obj!=NULL);
      obj->unlock();
    }

  void unlock(void)
    {
      assert(obj!=NULL);
      obj->unlock();
    }

  size_t references(void) const
    {
      return (obj==NULL)? 0: obj->references();
    }
};

#ifndef LOCK_PTR_NDEBUG
#undef NDEBUG
#endif

#endif