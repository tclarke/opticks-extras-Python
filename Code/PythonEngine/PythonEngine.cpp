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
#include "InterpreterUtilities.h"
#include "MessageLogResource.h"
#include "OpticksModule.h"
#include "PythonEngine.h"
#include "PythonEngineOptions.h"
#include "PythonVersion.h"
#include "PlugInArgList.h"
#include "PlugInRegistration.h"
#include "PlugInManagerServices.h"
#include "ProgressTracker.h"
#include "PythonCommon.h"
#include <sstream>

#include <boost/tokenizer.hpp>

REGISTER_PLUGIN_BASIC(Python, PythonInterpreter);
REGISTER_PLUGIN_BASIC(Python, PythonInterpreterWizardItem);

namespace
{
   PythonEngine* spEngine = NULL;
   void transmitOutput(char* pText, int32_t length, int32_t error)
   {
      if (spEngine == NULL)
      {
         return;
      }
      std::string message(pText, length);
      if (error)
      {
         spEngine->sendError(message);
      }
      else
      {
         spEngine->sendOutput(message);
      }
   }
};

PythonEngine::PythonEngine()
   : mPrompt(">>> "), mGlobalOutputShown(false), mPythonRunning(false),
   mAttemptedOneStart(false), mRunningScopedCommand(false)
{
   spEngine = this;
}

PythonEngine::~PythonEngine()
{
   spEngine = NULL;
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
      std::string pythonHome = PythonEngineOptions::getSettingPythonHome();
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

      auto_obj sendOutputFuncPointer(PyCObject_FromVoidPtr(reinterpret_cast<void*>(&transmitOutput), NULL), true);
      checkErr();
      PyObject_SetAttrString(mInterpModule, "_send_output_c_func_pointer", sendOutputFuncPointer);
      checkErr();
      auto_obj strConnectIo(PyDict_GetItemString(interpDict, "connect_io"));
      checkErr();
      auto_obj strConnectIoRet(PyObject_CallObject(strConnectIo, NULL), true);
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
      const Filename* pUserFile = PythonEngineOptions::getSettingUserFile();
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

   if (!PythonEngineOptions::getSettingInteractiveAvailable())
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

PythonInterpreter::PythonInterpreter()
   : mpEngine(new PythonEngine())
{
   setName("Python");
   setDescription("Provides command line utilities to execute Python commands.");
   setDescriptorId("{171f067d-1927-4cf0-b505-f3d6bcc09493}");
   setCopyright(PYTHON_COPYRIGHT);
   setVersion(PYTHON_VERSION_NUMBER);
   setProductionStatus(PYTHON_IS_PRODUCTION_RELEASE);
   allowMultipleInstances(false);
   setWizardSupported(false);
   setInteractiveEnabled(PythonEngineOptions::getSettingInteractiveAvailable());
   addDependencyCopyright("Python", "<pre>A. HISTORY OF THE SOFTWARE\n"
"==========================\n"
"\n"
"Python was created in the early 1990s by Guido van Rossum at Stichting\n"
"Mathematisch Centrum (CWI, see http://www.cwi.nl) in the Netherlands\n"
"as a successor of a language called ABC.  Guido remains Python's\n"
"principal author, although it includes many contributions from others.\n"
"\n"
"In 1995, Guido continued his work on Python at the Corporation for\n"
"National Research Initiatives (CNRI, see http://www.cnri.reston.va.us)\n"
"in Reston, Virginia where he released several versions of the\n"
"software.\n"
"\n"
"In May 2000, Guido and the Python core development team moved to\n"
"BeOpen.com to form the BeOpen PythonLabs team.  In October of the same\n"
"year, the PythonLabs team moved to Digital Creations (now Zope\n"
"Corporation, see http://www.zope.com).  In 2001, the Python Software\n"
"Foundation (PSF, see http://www.python.org/psf/) was formed, a\n"
"non-profit organization created specifically to own Python-related\n"
"Intellectual Property.  Zope Corporation is a sponsoring member of\n"
"the PSF.\n"
"\n"
"All Python releases are Open Source (see http://www.opensource.org for\n"
"the Open Source Definition).  Historically, most, but not all, Python\n"
"releases have also been GPL-compatible; the table below summarizes\n"
"the various releases.\n"
"\n"
    "Release         Derived     Year        Owner       GPL-\n"
                    "from                                compatible? (1)\n"
"\n"
    "0.9.0 thru 1.2              1991-1995   CWI         yes\n"
    "1.3 thru 1.5.2  1.2         1995-1999   CNRI        yes\n"
    "1.6             1.5.2       2000        CNRI        no\n"
    "2.0             1.6         2000        BeOpen.com  no\n"
    "1.6.1           1.6         2001        CNRI        yes (2)\n"
    "2.1             2.0+1.6.1   2001        PSF         no\n"
    "2.0.1           2.0+1.6.1   2001        PSF         yes\n"
    "2.1.1           2.1+2.0.1   2001        PSF         yes\n"
    "2.2             2.1.1       2001        PSF         yes\n"
    "2.1.2           2.1.1       2002        PSF         yes\n"
    "2.1.3           2.1.2       2002        PSF         yes\n"
    "2.2.1           2.2         2002        PSF         yes\n"
    "2.2.2           2.2.1       2002        PSF         yes\n"
    "2.2.3           2.2.2       2003        PSF         yes\n"
    "2.3             2.2.2       2002-2003   PSF         yes\n"
    "2.3.1           2.3         2002-2003   PSF         yes\n"
    "2.3.2           2.3.1       2002-2003   PSF         yes\n"
    "2.3.3           2.3.2       2002-2003   PSF         yes\n"
    "2.3.4           2.3.3       2004        PSF         yes\n"
    "2.3.5           2.3.4       2005        PSF         yes\n"
    "2.4             2.3         2004        PSF         yes\n"
    "2.4.1           2.4         2005        PSF         yes\n"
    "2.4.2           2.4.1       2005        PSF         yes\n"
    "2.4.3           2.4.2       2006        PSF         yes\n"
    "2.5             2.4         2006        PSF         yes\n"
"\n"
"Footnotes:\n"
"\n"
"(1) GPL-compatible doesn't mean that we're distributing Python under\n"
    "the GPL.  All Python licenses, unlike the GPL, let you distribute\n"
    "a modified version without making your changes open source.  The\n"
    "GPL-compatible licenses make it possible to combine Python with\n"
    "other software that is released under the GPL; the others don't.\n"
"\n"
"(2) According to Richard Stallman, 1.6.1 is not GPL-compatible,\n"
    "because its license has a choice of law clause.  According to\n"
    "CNRI, however, Stallman's lawyer has told CNRI's lawyer that 1.6.1\n"
    "is \"not incompatible\" with the GPL.\n"
"\n"
"Thanks to the many outside volunteers who have worked under Guido's\n"
"direction to make these releases possible.\n"
"\n"
"\n"
"B. TERMS AND CONDITIONS FOR ACCESSING OR OTHERWISE USING PYTHON\n"
"===============================================================\n"
"\n"
"PYTHON SOFTWARE FOUNDATION LICENSE VERSION 2\n"
"--------------------------------------------\n"
"\n"
"1. This LICENSE AGREEMENT is between the Python Software Foundation\n"
"(\"PSF\"), and the Individual or Organization (\"Licensee\") accessing and\n"
"otherwise using this software (\"Python\") in source or binary form and\n"
"its associated documentation.\n"
"\n"
"2. Subject to the terms and conditions of this License Agreement, PSF\n"
"hereby grants Licensee a nonexclusive, royalty-free, world-wide\n"
"license to reproduce, analyze, test, perform and/or display publicly,\n"
"prepare derivative works, distribute, and otherwise use Python\n"
"alone or in any derivative version, provided, however, that PSF's\n"
"License Agreement and PSF's notice of copyright, i.e., \"Copyright (c)\n"
"2001, 2002, 2003, 2004, 2005, 2006 Python Software Foundation; All Rights\n"
"Reserved\" are retained in Python alone or in any derivative version \n"
"prepared by Licensee.\n"
"\n"
"3. In the event Licensee prepares a derivative work that is based on\n"
"or incorporates Python or any part thereof, and wants to make\n"
"the derivative work available to others as provided herein, then\n"
"Licensee hereby agrees to include in any such work a brief summary of\n"
"the changes made to Python.\n"
"\n"
"4. PSF is making Python available to Licensee on an \"AS IS\"\n"
"basis.  PSF MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR\n"
"IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, PSF MAKES NO AND\n"
"DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS\n"
"FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF PYTHON WILL NOT\n"
"INFRINGE ANY THIRD PARTY RIGHTS.\n"
"\n"
"5. PSF SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON\n"
"FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS\n"
"A RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON,\n"
"OR ANY DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.\n"
"\n"
"6. This License Agreement will automatically terminate upon a material\n"
"breach of its terms and conditions.\n"
"\n"
"7. Nothing in this License Agreement shall be deemed to create any\n"
"relationship of agency, partnership, or joint venture between PSF and\n"
"Licensee.  This License Agreement does not grant permission to use PSF\n"
"trademarks or trade name in a trademark sense to endorse or promote\n"
"products or services of Licensee, or any third party.\n"
"\n"
"8. By copying, installing or otherwise using Python, Licensee\n"
"agrees to be bound by the terms and conditions of this License\n"
"Agreement.\n"
"\n"
"\n"
"BEOPEN.COM LICENSE AGREEMENT FOR PYTHON 2.0\n"
"-------------------------------------------\n"
"\n"
"BEOPEN PYTHON OPEN SOURCE LICENSE AGREEMENT VERSION 1\n"
"\n"
"1. This LICENSE AGREEMENT is between BeOpen.com (\"BeOpen\"), having an\n"
"office at 160 Saratoga Avenue, Santa Clara, CA 95051, and the\n"
"Individual or Organization (\"Licensee\") accessing and otherwise using\n"
"this software in source or binary form and its associated\n"
"documentation (\"the Software\").\n"
"\n"
"2. Subject to the terms and conditions of this BeOpen Python License\n"
"Agreement, BeOpen hereby grants Licensee a non-exclusive,\n"
"royalty-free, world-wide license to reproduce, analyze, test, perform\n"
"and/or display publicly, prepare derivative works, distribute, and\n"
"otherwise use the Software alone or in any derivative version,\n"
"provided, however, that the BeOpen Python License is retained in the\n"
"Software, alone or in any derivative version prepared by Licensee.\n"
"\n"
"3. BeOpen is making the Software available to Licensee on an \"AS IS\"\n"
"basis.  BEOPEN MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR\n"
"IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, BEOPEN MAKES NO AND\n"
"DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS\n"
"FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF THE SOFTWARE WILL NOT\n"
"INFRINGE ANY THIRD PARTY RIGHTS.\n"
"\n"
"4. BEOPEN SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF THE\n"
"SOFTWARE FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS\n"
"AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THE SOFTWARE, OR ANY\n"
"DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.\n"
"\n"
"5. This License Agreement will automatically terminate upon a material\n"
"breach of its terms and conditions.\n"
"\n"
"6. This License Agreement shall be governed by and interpreted in all\n"
"respects by the law of the State of California, excluding conflict of\n"
"law provisions.  Nothing in this License Agreement shall be deemed to\n"
"create any relationship of agency, partnership, or joint venture\n"
"between BeOpen and Licensee.  This License Agreement does not grant\n"
"permission to use BeOpen trademarks or trade names in a trademark\n"
"sense to endorse or promote products or services of Licensee, or any\n"
"third party.  As an exception, the \"BeOpen Python\" logos available at\n"
"http://www.pythonlabs.com/logos.html may be used according to the\n"
"permissions granted on that web page.\n"
"\n"
"7. By copying, installing or otherwise using the software, Licensee\n"
"agrees to be bound by the terms and conditions of this License\n"
"Agreement.\n"
"\n"
"\n"
"CNRI LICENSE AGREEMENT FOR PYTHON 1.6.1\n"
"---------------------------------------\n"
"\n"
"1. This LICENSE AGREEMENT is between the Corporation for National\n"
"Research Initiatives, having an office at 1895 Preston White Drive,\n"
"Reston, VA 20191 (\"CNRI\"), and the Individual or Organization\n"
"(\"Licensee\") accessing and otherwise using Python 1.6.1 software in\n"
"source or binary form and its associated documentation.\n"
"\n"
"2. Subject to the terms and conditions of this License Agreement, CNRI\n"
"hereby grants Licensee a nonexclusive, royalty-free, world-wide\n"
"license to reproduce, analyze, test, perform and/or display publicly,\n"
"prepare derivative works, distribute, and otherwise use Python 1.6.1\n"
"alone or in any derivative version, provided, however, that CNRI's\n"
"License Agreement and CNRI's notice of copyright, i.e., \"Copyright (c)\n"
"1995-2001 Corporation for National Research Initiatives; All Rights\n"
"Reserved\" are retained in Python 1.6.1 alone or in any derivative\n"
"version prepared by Licensee.  Alternately, in lieu of CNRI's License\n"
"Agreement, Licensee may substitute the following text (omitting the\n"
"quotes): \"Python 1.6.1 is made available subject to the terms and\n"
"conditions in CNRI's License Agreement.  This Agreement together with\n"
"Python 1.6.1 may be located on the Internet using the following\n"
"unique, persistent identifier (known as a handle): 1895.22/1013.  This\n"
"Agreement may also be obtained from a proxy server on the Internet\n"
"using the following URL: http://hdl.handle.net/1895.22/1013\".\n"
"\n"
"3. In the event Licensee prepares a derivative work that is based on\n"
"or incorporates Python 1.6.1 or any part thereof, and wants to make\n"
"the derivative work available to others as provided herein, then\n"
"Licensee hereby agrees to include in any such work a brief summary of\n"
"the changes made to Python 1.6.1.\n"
"\n"
"4. CNRI is making Python 1.6.1 available to Licensee on an \"AS IS\"\n"
"basis.  CNRI MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR\n"
"IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, CNRI MAKES NO AND\n"
"DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS\n"
"FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF PYTHON 1.6.1 WILL NOT\n"
"INFRINGE ANY THIRD PARTY RIGHTS.\n"
"\n"
"5. CNRI SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON\n"
"1.6.1 FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS\n"
"A RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON 1.6.1,\n"
"OR ANY DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.\n"
"\n"
"6. This License Agreement will automatically terminate upon a material\n"
"breach of its terms and conditions.\n"
"\n"
"7. This License Agreement shall be governed by the federal\n"
"intellectual property law of the United States, including without\n"
"limitation the federal copyright law, and, to the extent such\n"
"U.S. federal law does not apply, by the law of the Commonwealth of\n"
"Virginia, excluding Virginia's conflict of law provisions.\n"
"Notwithstanding the foregoing, with regard to derivative works based\n"
"on Python 1.6.1 that incorporate non-separable material that was\n"
"previously distributed under the GNU General Public License (GPL), the\n"
"law of the Commonwealth of Virginia shall govern this License\n"
"Agreement only as to issues arising under or with respect to\n"
"Paragraphs 4, 5, and 7 of this License Agreement.  Nothing in this\n"
"License Agreement shall be deemed to create any relationship of\n"
"agency, partnership, or joint venture between CNRI and Licensee.  This\n"
"License Agreement does not grant permission to use CNRI trademarks or\n"
"trade name in a trademark sense to endorse or promote products or\n"
"services of Licensee, or any third party.\n"
"\n"
"8. By clicking on the \"ACCEPT\" button where indicated, or by copying,\n"
"installing or otherwise using Python 1.6.1, Licensee agrees to be\n"
"bound by the terms and conditions of this License Agreement.\n"
"\n"
        "ACCEPT\n"
"\n"
"\n"
"CWI LICENSE AGREEMENT FOR PYTHON 0.9.0 THROUGH 1.2\n"
"--------------------------------------------------\n"
"\n"
"Copyright (c) 1991 - 1995, Stichting Mathematisch Centrum Amsterdam,\n"
"The Netherlands.  All rights reserved.\n"
"\n"
"Permission to use, copy, modify, and distribute this software and its\n"
"documentation for any purpose and without fee is hereby granted,\n"
"provided that the above copyright notice appear in all copies and that\n"
"both that copyright notice and this permission notice appear in\n"
"supporting documentation, and that the name of Stichting Mathematisch\n"
"Centrum or CWI not be used in advertising or publicity pertaining to\n"
"distribution of the software without specific, written prior\n"
"permission.\n"
"\n"
"STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO\n"
"THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND\n"
"FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE\n"
"FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES\n"
"WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN\n"
"ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT\n"
"OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.</pre>");
}

bool PythonInterpreter::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   mpEngine->startPython();
   return true;
}

bool PythonInterpreter::isStarted() const
{
   return mpEngine->isPythonRunning();
}

bool PythonInterpreter::start()
{
   bool alreadyStarted = mpEngine->isPythonRunning();
   bool started = mpEngine->startPython();
   if (!alreadyStarted && started)
   {
      notify(SIGNAL_NAME(InterpreterManager, InterpreterStarted));
   }
   return started;
}

std::string PythonInterpreter::getStartupMessage() const
{
   return mpEngine->getStartupMessage();
}

Interpreter* PythonInterpreter::getInterpreter() const
{
   if (mpEngine->isPythonRunning())
   {
      return mpEngine.get();
   }
   return NULL;
}

const std::string& PythonInterpreter::getObjectType() const
{
   static std::string sType("PythonInterpreter");
   return sType;
}

bool PythonInterpreter::isKindOf(const std::string& className) const
{
   if (className == getObjectType())
   {
      return true;
   }

   return SubjectImp::isKindOf(className);
}

PythonInterpreterWizardItem::PythonInterpreterWizardItem()
{
   setName("Python Interpreter");
   setDescription("Allow execution of Python code from within a wizard. "
      "This is DEPRECATED, please use the RunInterpreterCommands wizard item.");
   setDescriptorId("{2103eba4-f8d8-44be-9fb3-47fbbe8c6cc3}");
   setCopyright(PYTHON_COPYRIGHT);
   setVersion(PYTHON_VERSION_NUMBER);
   setProductionStatus(PYTHON_IS_PRODUCTION_RELEASE);
}

bool PythonInterpreterWizardItem::getInputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<Progress>(Executable::ProgressArg(), NULL));
   VERIFY(pArgList->addArg<std::string>("Command", std::string()));

   return true;
}

bool PythonInterpreterWizardItem::getOutputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<std::string>("Output Text"));
   VERIFY(pArgList->addArg<std::string>("Return Type"));

   return true;
}

bool PythonInterpreterWizardItem::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   VERIFY(pInArgList != NULL && pOutArgList != NULL);

   ProgressTracker progress(pInArgList->getPlugInArgValue<Progress>(ProgressArg()),
      "Execute Python command.", "python", "{5b5d5de3-faad-41ed-894b-dbe5a90d6d4d}");

   progress.report("This wizard item is DEPRECATED, please use the RunInterpreterCommands wizard item.",
      0, WARNING, true);
   std::string command;
   if (!pInArgList->getPlugInArgValue("Command", command))
   {
      return false;
   }

   std::vector<PlugIn*> plugins = Service<PlugInManagerServices>()->getPlugInInstances("Python");
   if (plugins.size() != 1)
   {
      progress.report("Unable to locate the Python engine.", 0, ERRORS, true);
      return false;
   }
   PythonInterpreter* pMgr = dynamic_cast<PythonInterpreter*>(plugins.front());
   VERIFY(pMgr != NULL);
   pMgr->start();
   Interpreter* pInterpreter = pMgr->getInterpreter();
   if (pInterpreter == NULL)
   {
      progress.report("Unable to start Python interpreter. " + pMgr->getStartupMessage(), 0, ERRORS, true);
      return false;
   }

   progress.report("Executing Python command.", 1, NORMAL);
   std::string outputAndErrorText;
   bool hasErrorText = false;
   bool retVal = InterpreterUtilities::executeScopedCommand("Python", command, outputAndErrorText,
      hasErrorText, progress.getCurrentProgress());
   if (!retVal)
   {
      progress.report("Error executing Python command.", 0, ERRORS, true);
   }
   std::string returnType = (hasErrorText ? "Error" : "Output");
   VERIFY(pOutArgList->setPlugInArgValue("Return Type", &returnType));
   VERIFY(pOutArgList->setPlugInArgValue("Output Text", &outputAndErrorText));
   progress.report("Executing Python command.", 100, NORMAL);
   progress.upALevel();
   return retVal;
}
