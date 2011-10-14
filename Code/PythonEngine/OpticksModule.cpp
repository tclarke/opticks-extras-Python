/*
 * The information in this file is
 * Copyright(c) 2009 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "OpticksModule.h"
#include "PlugInRegistration.h"
#include "PythonCommon.h"
#include "PythonEngine.h"
#include "PythonVersion.h"

namespace OpticksModule
{
   PyObject* get_handle(PyObject*, PyObject*)
   {
      auto_obj opaque(PyCObject_FromVoidPtr(
         reinterpret_cast<void*>(ModuleManager::instance()->getService()), NULL), true);
      return Py_BuildValue("O", opaque.take());
   }

   PyObject* get_python_version(PyObject*, PyObject*)
   {
      return PyString_FromString(PYTHON_VERSION_NUMBER);
   }

   PyMethodDef opticksMethods[] = {
      {"handle", get_handle, METH_NOARGS, "Retrieve an opaque handle to the Opticks services object. " \
                                          "This is used when initializing modules within a .pyd file."},
      {"pythonVersion", get_python_version, METH_NOARGS, "Retrieve the version of the Python plug-in as a string."},
      {"send_output", transmitOutput, METH_VARARGS, "Send output back to Opticks."},
      {NULL, NULL, 0, NULL} // sentinel
   };
} // namespace

PyMODINIT_FUNC init_opticks()
{
   auto_obj pModule(Py_InitModule3("_opticks", OpticksModule::opticksMethods, "Internal Opticks support module."));
   if (pModule.get() == NULL)
   {
      return;
   }
}
