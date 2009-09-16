/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PYTHONCOMMON_H__
#define PYTHONCOMMON_H__

#define PY_SSIZE_T_CLEAN // we use Py_ssize_t correctly
#undef _DEBUG // necessary to use release Python binaries with debug plug-in builds
#include <Python.h>

/**
 * auto ptr object for PyObject*
 */
class auto_obj
{
public:
   auto_obj() : mpObj(NULL), mOwned(false) {}
   auto_obj(PyObject* pObj, bool takeOwnership=false) : mpObj(pObj), mOwned(takeOwnership) {}
   ~auto_obj()
   {
      if (mOwned)
      {
         Py_XDECREF(mpObj);
      }
   }

   /**
    * Reset the internal pointer.
    */
   void reset(PyObject* pObj, bool takeOwnership=false)
   {
      release();
      mpObj = pObj;
      mOwned = takeOwnership;
   }

   /**
    * Release ownership doing a decref if needed.
    */
   PyObject* release()
   {
      if (mpObj == NULL)
      {
         return NULL;
      }
      if (mOwned)
      {
         Py_DECREF(mpObj);
         mOwned = false;
      }
      return mpObj;
   }

   /**
    * Returns a borrowed reference. This object may not own the reference.
    */
   PyObject* get()
   {
      return mpObj;
   }

   /**
    * Returns a borrowed reference. Ensures this object owns the reference.
    */
   PyObject* take()
   {
      if (mpObj == NULL)
      {
         return NULL;
      }
      if (!mOwned)
      {
         Py_INCREF(mpObj);
         mOwned = true;
      }
      return mpObj;
   }

   operator PyObject*()
   {
      return get();
   }

private:
   PyObject* mpObj;
   bool mOwned;
};

#endif