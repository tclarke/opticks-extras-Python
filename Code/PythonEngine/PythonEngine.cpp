/*
 * The information in this file is
 * Copyright(c) 2009 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppVerify.h"
#include "AttachmentPtr.h"
#include "FileResource.h"
#include "MessageLogResource.h"
#include "OpticksModule.h"
#include "PythonEngine.h"
#include "PythonVersion.h"
#include "PlugInRegistration.h"
#include "PythonCommon.h"
#include <sstream>

#include <boost/tokenizer.hpp>

namespace
{
   PythonEngine* spEngine = NULL;
};

PyObject* transmitOutput(PyObject* pSelf, PyObject* pArgs)
{
   if (spEngine == NULL)
   {
      Py_RETURN_NONE;
   }
   PyObject* pString = NULL;
   unsigned char error = 1;
   if (!PyArg_ParseTuple(pArgs, "Sb", &pString, &error))
   {
      return NULL;
   }
   if (pString != NULL)
   {
      std::string message(PyString_AsString(pString));
      if (error != 0)
      {
         spEngine->sendError(message);
      }
      else
      {
         spEngine->sendOutput(message);
      }
   }
   Py_RETURN_NONE;
}

extern "C" LINKAGE PythonInterpreter* init_python_engine(External* pExternal)
{
   if (pExternal == NULL)
   {
      return NULL;
   }
   ModuleManager::instance()->setService(pExternal);
   if (spEngine == NULL)
   {
      spEngine = new PythonEngine();
   }
   else
   {
      spEngine->showGlobalOutput(false); //reset to default
      spEngine->setStartupMessage("The Python Interpreter was not restarted and the python environment was not reset. Variables which referenced Opticks data should be deleted or a crash may occur.");
   }
   return spEngine;
}

extern "C" LINKAGE void shutdown_python_engine()
{
   delete spEngine;
   spEngine = NULL;
}

PythonEngine::PythonEngine()
   : mPrompt(">>> "), mGlobalOutputShown(false), mPythonRunning(false),
   mAttemptedOneStart(false), mRunningScopedCommand(false)
{
}

PythonEngine::~PythonEngine()
{
   if (mRunModule.get() != NULL)
   {
      mInterpModule.reset(NULL);
      mStdin.reset(NULL);
      mInterpreter.reset(NULL);
      mGlobals.reset(NULL);
      mRunModule.reset(NULL);
      Py_Finalize();
   }
}

bool PythonEngine::isPythonRunning() const
{
   return mPythonRunning;
}

bool PythonEngine::startPython()
{
   if (mPythonRunning)
   {
      return true;
   }
   if (mAttemptedOneStart)
   {
      return false;
   }
   mAttemptedOneStart = true;
   mStartupMessage.clear();
   try
   {
      std::string pythonHome = PythonInterpreter::getSettingPythonHome();
      if (!pythonHome.empty())
      {
         Py_SetPythonHome(const_cast<char*>(pythonHome.c_str()));
      }
      Py_SetProgramName("opticks");
      Py_Initialize();

      mRunModule.reset(PyModule_New("__opticks_script__"), true);
      checkErr();
      PyModule_AddStringConstant(mRunModule, "__file__", "Scripting Window running in Opticks");
      checkErr();
      mGlobals.reset(PyModule_GetDict(mRunModule), true);
      checkErr();
      PyDict_SetItemString(mGlobals, "__builtins__", PyEval_GetBuiltins());
      checkErr();

      init_opticks();
      checkErr();
      auto_obj sysPath(PySys_GetObject("path"));
      std::string newPath =
         Service<ConfigurationSettings>()->getSettingSupportFilesPath()->getFullPathAndName() + "/site-packages";
      auto_obj newPathItem(PyString_FromString(newPath.c_str()), true);
      VERIFYNR(PyList_Append(sysPath, newPathItem) == 0);
      VERIFYNR(PySys_SetObject("path", sysPath) == 0);
      mInterpModule.reset(PyImport_ImportModule("interpreter"), true);
      checkErr();
      auto_obj interpDict(PyModule_GetDict(mInterpModule));
      checkErr();
      
      auto_obj strBufStream(PyDict_GetItemString(interpDict, "StrBufStream"));
      checkErr();
      mStdin.reset(PyObject_CallObject(strBufStream, NULL), true);
      checkErr();
      VERIFYNR(mStdin.get());

      auto_obj pythonInteractiveInterpreter(PyDict_GetItemString(interpDict, "PythonInteractiveInterpreter"));
      mInterpreter.reset(PyObject_CallObject(pythonInteractiveInterpreter, Py_BuildValue("(O)", mGlobals.get())), true);
      checkErr();

      PyObject_SetAttrString(mInterpreter, "stdin", mStdin);
      checkErr();
      mStartupMessage = "Python ";
      mStartupMessage += Py_GetVersion();
      mStartupMessage += " on ";
      mStartupMessage += Py_GetPlatform();
      mStartupMessage += "\nType \"help(opticks)\" for release notes.\n";
      auto_obj builtinModule(PyImport_Import(PyString_FromString("__builtin__")), true);
      checkErr();
      auto_obj opticksModule(PyImport_Import(PyString_FromString("opticks")), true);
      checkErr();
      PyModule_AddObject(builtinModule, "opticks", opticksModule.release());
      checkErr();
   }
   catch(const PythonError& err)
   {
      mStartupMessage = "Error initializing python engine.\n" + std::string(err.what());
      MessageResource msg("Error initializing python engine.", "python", "{d32b0337-63f2-43fc-8d82-0387a9b5d254}");
      msg->addProperty("Err", err.what());
      return false;
   }

   std::string userFileName;
   std::string errorMsg;
   try
   {
      mGatheredOutput.clear();
      AttachmentPtr<PythonEngine> attachments(this);
      attachments.addSignal(SIGNAL_NAME(Interpreter, OutputText), Slot(this, &PythonEngine::gatherOutput));
      attachments.addSignal(SIGNAL_NAME(Interpreter, ErrorText), Slot(this, &PythonEngine::gatherOutput));
      const Filename* pUserFile = PythonInterpreter::getSettingUserFile();
      userFileName = (pUserFile == NULL) ? "" : pUserFile->getFullPathAndName();
      FileResource userFile(userFileName.c_str(), "r");
      if (userFile.get() != NULL)
      {
         const unsigned int BUF_SIZE = 4096;
         char pBuf[BUF_SIZE];
         std::string allFileContents;
         while (!feof(userFile))
         {
            size_t readSize = fread(pBuf, sizeof(char), BUF_SIZE, userFile);
            if (ferror(userFile) || readSize == 0)
            {
               throw PythonError("Invalid user file!");
            }
            allFileContents = allFileContents + std::string(pBuf, readSize);
         }
         PyRun_String(allFileContents.c_str(), Py_file_input, mGlobals.get(), mGlobals.get());
         checkErr();
      }
   }
   catch(const PythonError& err)
   {
      errorMsg = "Error executing user python file at: " + userFileName + ".\n" + std::string(err.what());
   }
   if (!mGatheredOutput.empty())
   {
      mStartupMessage += mGatheredOutput;
      mGatheredOutput.clear();
   }
   if (!errorMsg.empty())
   {
      mStartupMessage += errorMsg;
   }

   if (!PythonInterpreter::getSettingInteractiveAvailable())
   {
      mStartupMessage = "The ability to type Python commands into the Scripting Window "
         "has been disabled by another extension.\n" + mStartupMessage;
   }

   mPythonRunning = true;
   return mPythonRunning;
}

std::string PythonEngine::getStartupMessage() const
{
   return mStartupMessage;
}

void PythonEngine::setStartupMessage(const std::string& msg)
{
   mStartupMessage = msg;
}

bool PythonEngine::executeCommand(const std::string& command)
{
   if (!mPythonRunning)
   {
      return false;
   }
   bool retVal = true;
   mRunningScopedCommand = false;
   try
   {
      std::string::size_type commandLen = command.size();
      if (!command.empty() && command[commandLen - 1] == '\n')
      {
         commandLen--;
      }
      auto_obj cnt(PyObject_CallMethod(mStdin, "write", "sl", command.c_str(), commandLen), true);
      checkErr();
      auto_obj useps1(PyObject_CallMethod(mInterpreter, "process_event", NULL), true);
      checkErr();
      if (useps1 == Py_True)
      {
         mPrompt = ">>> ";
      }
      else
      {
         mPrompt = "... ";
      }
   }
   catch(const PythonError& err)
   {
      sendError(err.what());
      retVal = false;
   }

   mRunningScopedCommand = false;
   return retVal;
}

bool PythonEngine::executeScopedCommand(const std::string& command, const Slot& output,
                                        const Slot& error, Progress* pProgress)
{
   if (!mPythonRunning)
   {
      return false;
   }
   bool retVal = true;
   mRunningScopedCommand = true;
   attach(SIGNAL_NAME(PythonEngine, ScopedOutputText), output);
   attach(SIGNAL_NAME(PythonEngine, ScopedErrorText), error);
   try
   {
      auto_obj scopedDict(PyDict_New(), true);
      PyRun_String(command.c_str(), Py_file_input, mGlobals.get(), scopedDict.get());
      checkErr();
   }
   catch(const PythonError& err)
   {
      sendError(err.what());
      retVal = false;
   }
   detach(SIGNAL_NAME(PythonEngine, ScopedErrorText), error);
   detach(SIGNAL_NAME(PythonEngine, ScopedOutputText), output);
   mRunningScopedCommand = false;
   return retVal;
}

void PythonEngine::gatherOutput(Subject& subject, const std::string& signal, const boost::any& data)
{
   std::string text = boost::any_cast<std::string>(data);
   mGatheredOutput += text;
}

void PythonEngine::sendOutput(const std::string& text)
{
   sendOutput(text, mRunningScopedCommand);
}

void PythonEngine::sendOutput(const std::string& text, bool scoped)
{
   if (text.empty())
   {
      return;
   }
   if (scoped)
   {
      notify(SIGNAL_NAME(PythonEngine, ScopedOutputText), text);
   }
   if (!scoped || mGlobalOutputShown)
   {
      notify(SIGNAL_NAME(Interpreter, OutputText), text);
   }
}

void PythonEngine::sendError(const std::string& text)
{
   sendError(text, mRunningScopedCommand);
}

void PythonEngine::sendError(const std::string& text, bool scoped)
{
   if (text.empty())
   {
      return;
   }
   if (scoped)
   {
      notify(SIGNAL_NAME(PythonEngine, ScopedErrorText), text);
   }
   if (!scoped || mGlobalOutputShown)
   {
      notify(SIGNAL_NAME(Interpreter, ErrorText), text);
   }
}

bool PythonEngine::isGlobalOutputShown() const
{
   return mGlobalOutputShown;
}

void PythonEngine::showGlobalOutput(bool newValue)
{
   mGlobalOutputShown = newValue;
}

std::string PythonEngine::getPrompt() const
{
   return mPrompt;
}

void PythonEngine::checkErr()
{
   if (PyErr_Occurred() != NULL)
   {
      PyObject* pException = NULL;
      PyObject* pVal = NULL;
      PyObject* pTb = NULL;
      PyErr_Fetch(&pException, &pVal, &pTb);
      PyErr_NormalizeException(&pException, &pVal, &pTb);
      PyErr_Clear();
      std::string errStr;
      auto_obj traceback(PyImport_ImportModule("traceback"), true);
      if (PyErr_Occurred() != NULL)
      {
         PyErr_Clear();
         auto_obj msg(PyObject_GetAttrString(pVal, "message"), true);
         errStr = "Unable to import traceback module.\n" + 
            std::string(PyObject_REPR(pException)) + ": " +
            std::string(PyObject_REPR(msg)) + "\npath=" +
            std::string(PyObject_REPR(PySys_GetObject("path"))) + "\n";
      }
      else
      {
         auto_obj tracebackDict(PyModule_GetDict(traceback));
         if (pTb != NULL)
         {
            auto_obj format_tb(PyDict_GetItemString(tracebackDict, "format_tb"));
            auto_obj tb(PyObject_CallFunction(format_tb, "O", pTb), true);
            auto_obj scopedDict(PyDict_New(), true);
            if (scopedDict.get() != NULL)
            {
               int retVal = PyDict_SetItemString(scopedDict, "tb", tb);
               if (retVal == 0)
               {
                  auto_obj retObj(PyRun_String("''.join(tb)", Py_eval_input, scopedDict.get(),
                     scopedDict.get()), true);
                  if (retObj.get() != NULL)
                  {
                     errStr = std::string(PyString_AsString(retObj));
                  }
               }
            }
            if (!errStr.empty())
            {
               errStr = "Traceback (most recent call last):\n" + errStr;
            }
         }
         auto_obj format_exception_only(PyDict_GetItemString(tracebackDict, "format_exception_only"));
         auto_obj excList(PyObject_CallFunction(format_exception_only, "OO", pException, pVal), true);
         auto_obj scopedDict(PyDict_New(), true);
         if (scopedDict.get() != NULL)
         {
            int retVal = PyDict_SetItemString(scopedDict, "excList", excList);
            if (retVal == 0)
            {
               auto_obj retObj(PyRun_String("''.join(excList)", Py_eval_input, scopedDict.get(),
                  scopedDict.get()), true);
               if (retObj.get() != NULL)
               {
                  errStr += std::string(PyString_AsString(retObj));
               }
            }
         }
      }
      if (errStr.empty())
      {
         errStr = "Could not retrieve detailed error message";
      }
      throw PythonError(errStr);
   }
}

const std::string& PythonEngine::getObjectType() const
{
   static std::string sType("PythonEngine");
   return sType;
}

bool PythonEngine::isKindOf(const std::string& className) const
{
   if (className == getObjectType())
   {
      return true;
   }

   return SubjectImp::isKindOf(className);
}
