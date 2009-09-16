/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppVerify.h"
#include "FileResource.h"
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

REGISTER_PLUGIN_BASIC(Python, PythonEngine);
REGISTER_PLUGIN_BASIC(Python, PythonInterpreter);
REGISTER_PLUGIN_BASIC(Python, PythonInterpreterWizardItem);

PythonEngine::PythonEngine() : mPrompt(">>> ")
{
   setName(PlugInName());
   setDescription("Python execution engine");
   setDescriptorId("{d47d3e19-5b7f-49aa-b02a-c8fbb1255591}");
   setCopyright(PYTHON_COPYRIGHT);
   setVersion(PYTHON_VERSION_NUMBER);
   setProductionStatus(PYTHON_IS_PRODUCTION_RELEASE);
   setType("Manager");
   executeOnStartup(true);
   destroyAfterExecute(false);
   allowMultipleInstances(false);
   setWizardSupported(false);
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

PythonEngine::~PythonEngine()
{
   if (mInterpreter.get() != NULL)
   {
      mInterpModule.reset(NULL);
      mStdin.reset(NULL);
      mStdout.reset(NULL);
      mStderr.reset(NULL);
      mInterpreter.reset(NULL);
      Py_Finalize();
   }
}

bool PythonEngine::getInputSpecification(PlugInArgList*& pArgList)
{
   pArgList = NULL;
   return true;
}

bool PythonEngine::getOutputSpecification(PlugInArgList*& pArgList)
{
   pArgList = NULL;
   return true;
}

bool PythonEngine::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   try
   {
      Py_SetProgramName("opticks");
      Py_Initialize();
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
      auto_obj strBufStream(PyDict_GetItemString(interpDict, "StrBufStream"));
      checkErr();
      mStdin.reset(PyObject_CallObject(strBufStream, NULL), true);
      checkErr();
      VERIFYNR(mStdin.get());
      mStdout.reset(PyObject_CallObject(strBufStream, NULL), true);
      checkErr();
      VERIFYNR(mStdout.get());
      mStderr.reset(PyObject_CallObject(strBufStream, NULL), true);
      checkErr();
      VERIFYNR(mStderr.get());

      auto_obj pythonInteractiveInterpreter(PyDict_GetItemString(interpDict, "PythonInteractiveInterpreter"));
      mInterpreter.reset(PyObject_CallObject(pythonInteractiveInterpreter, NULL), true);
      checkErr();

      PyObject_SetAttrString(mInterpreter, "stdin", mStdin);
      PyObject_SetAttrString(mInterpreter, "stdout", mStdout);
      PyObject_SetAttrString(mInterpreter, "stderr", mStderr);
      checkErr();
      std::string welcomeBanner = "Python ";
      welcomeBanner += Py_GetVersion();
      welcomeBanner += " on ";
      welcomeBanner += Py_GetPlatform();
      welcomeBanner += "\nType \"help(opticks)\" for release notes.\n\n";
      auto_obj cnt(PyObject_CallMethod(mStdout, "write", "sl", welcomeBanner.c_str(), welcomeBanner.size()), true);
      checkErr();
      auto_obj builtinModule(PyImport_Import(PyString_FromString("__builtin__")), true);
      checkErr();
      auto_obj opticksModule(PyImport_Import(PyString_FromString("opticks")), true);
      checkErr();
      PyModule_AddObject(builtinModule, "opticks", opticksModule.release());
      checkErr();

      const Filename* pUserFile = PythonEngineOptions::getSettingUserFile();
      std::string userFileName = (pUserFile == NULL) ? "" : pUserFile->getFullPathAndName();
      FileResource userFile(userFileName.c_str(), "rt");
      if (userFile.get() != NULL)
      {
         while (!feof(userFile))
         {
            char pBuf[4096];
            size_t readSize = fread(pBuf, sizeof(char), 4096, userFile);
            if (readSize == 0)
            {
               break;
            }
            auto_obj cnt(PyObject_CallMethod(mStdin, "write", "sl", pBuf, readSize), true);
            checkErr();
         }
         auto_obj useps1(PyObject_CallMethod(mInterpreter, "processEvent", NULL), true);
         checkErr();
         if (useps1 == Py_True)
         {
            mPrompt = ">>> ";
         }
         else
         {
            throw PythonError("Invalid user file!");
         }
      }
   }
   catch(const PythonError& err)
   {
      MessageResource msg("Error initializing python engine.", "python", "{d32b0337-63f2-43fc-8d82-0387a9b5d254}");
      msg->addProperty("Err", err.what());
      return false;
   }
   return true;
}

std::string PythonEngine::getPrompt() const
{
   return mPrompt;
}

bool PythonEngine::processCommand(const std::string& command,
                                  std::string& returnText,
                                  std::string& errorText,
                                  Progress* pProgress)
{
   if (mInterpreter.get() == NULL)
   {
      return false;
   }
   try
   {
      std::string tmpCommand = command;
      if (tmpCommand.empty() || tmpCommand[tmpCommand.size() - 1] != '\n')
      {
         tmpCommand.append("\n");
      }
      auto_obj cnt(PyObject_CallMethod(mStdin, "write", "sl", tmpCommand.c_str(), tmpCommand.size()), true);
      checkErr();
      auto_obj useps1(PyObject_CallMethod(mInterpreter, "processEvent", NULL), true);
      checkErr();
      if (useps1 == Py_True)
      {
         mPrompt = ">>> ";
      }
      else
      {
         mPrompt = "... ";
      }

      auto_obj stderrAvailable(PyObject_CallMethod(mStderr, "available", NULL), true);
      checkErr();
      long stderrCount = PyInt_AsLong(stderrAvailable);
      if (stderrCount > 0)
      {
         auto_obj stderrStr(PyObject_CallMethod(mStderr, "read", "O", stderrAvailable.get()), true);
         checkErr();
         errorText = PyString_AsString(stderrStr);
      }
      auto_obj stdoutAvailable(PyObject_CallMethod(mStdout, "available", NULL), true);
      checkErr();
      long stdoutCount = PyInt_AsLong(stdoutAvailable);
      if (stdoutCount > 0)
      {
         auto_obj stdoutStr(PyObject_CallMethod(mStdout, "read", "O", stdoutAvailable.get()), true);
         checkErr();
         returnText = PyString_AsString(stdoutStr);
      }
   }
   catch(const PythonError& err)
   {
      errorText = err.what();
      return false;
   }
   return true;
}

void PythonEngine::checkErr()
{
   if (PyErr_Occurred() != NULL)
   {
      PyObject* pException=NULL;
      PyObject* pVal=NULL;
      PyObject* pTb=NULL;
      PyErr_Fetch(&pException, &pVal, &pTb);
      PyErr_NormalizeException(&pException, &pVal, &pTb);
      PyErr_Clear();
      std::stringstream errStr;
      auto_obj traceback(PyImport_ImportModule("traceback"), true);
      if (PyErr_Occurred() != NULL)
      {
         PyErr_Clear();
         auto_obj msg(PyObject_GetAttrString(pVal, "message"), true);
         errStr << "Unable to import traceback module.\n"
            << PyObject_REPR(pException) << ": "
            << PyObject_REPR(msg) << "\npath="
            << PyObject_REPR(PySys_GetObject("path")) << std::endl;
      }
      else
      {
         auto_obj tracebackDict(PyModule_GetDict(traceback));
         auto_obj format_exception_only(PyDict_GetItemString(tracebackDict, "format_exception_only"));
         auto_obj exc(PyObject_CallFunction(format_exception_only, "OO", pException, pVal), true);
         errStr << PyObject_REPR(exc) << std::endl;
         if (pTb != NULL)
         {
            auto_obj format_tb(PyDict_GetItemString(tracebackDict, "format_tb"));
            auto_obj tb(PyObject_CallFunction(format_tb, "O", pTb), true);
            errStr << PyObject_REPR(tb) << std::endl;
         }
      }
      std::string tmpStr = errStr.str();
      for (std::string::size_type loc = tmpStr.find("\\n"); loc != std::string::npos; loc = tmpStr.find("\\n"))
      {
         tmpStr.replace(loc, 2, "\n");
      }
      throw PythonError(tmpStr);
   }
}

PythonInterpreter::PythonInterpreter()
{
   setName("Python");
   setDescription("Provides command line utilities to execute Python commands.");
   setDescriptorId("{171f067d-1927-4cf0-b505-f3d6bcc09493}");
   setCopyright(PYTHON_COPYRIGHT);
   setVersion(PYTHON_VERSION_NUMBER);
   setProductionStatus(PYTHON_IS_PRODUCTION_RELEASE);
   allowMultipleInstances(false);
   setWizardSupported(false);
}

bool PythonInterpreter::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   VERIFY(pInArgList != NULL && pOutArgList != NULL);
   if (!PythonEngineOptions::getSettingInteractiveAvailable())
   {
      std::string returnType("Error");
      std::string returnText = "Interactive interpreter has been disabled.";
      VERIFY(pOutArgList->setPlugInArgValue(ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(OutputTextArg(), &returnText));
      return true;
   }

   std::string command;
   if (!pInArgList->getPlugInArgValue(Interpreter::CommandArg(), command))
   {
      std::string returnType("Error");
      std::string errorText("Invalid command argument.");
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::OutputTextArg(), &errorText));
      return true;
   }
   Progress* pProgress = pInArgList->getPlugInArgValue<Progress>(Executable::ProgressArg());
   std::vector<PlugIn*> plugins = Service<PlugInManagerServices>()->getPlugInInstances(PythonEngine::PlugInName());
   if (plugins.size() != 1)
   {
      std::string returnType("Error");
      std::string errorText("Unable to locate python engine.");
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::OutputTextArg(), &errorText));
      return true;
   }
   PythonEngine* pEngine = dynamic_cast<PythonEngine*>(plugins.front());
   VERIFY(pEngine != NULL);

   std::string returnText;
   std::string errorText;
   if (!pEngine->processCommand(command, returnText, errorText, pProgress))
   {
      std::string returnType("Error");
      returnText += "\n" + errorText;
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::OutputTextArg(), &returnText));
      return true;
   }
   // Populate the output arg list
   if (errorText.empty())
   {
      std::string returnType("Output");
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::OutputTextArg(), &returnText));
   }
   else
   {
      std::string returnType("Error");
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::OutputTextArg(), &errorText));
   }

   return true;
}

std::string PythonInterpreter::getPrompt() const
{
   std::vector<PlugIn*> plugins = Service<PlugInManagerServices>()->getPlugInInstances(PythonEngine::PlugInName());
   if (plugins.size() != 1)
   {
      return ">>> ";
   }
   PythonEngine* pEngine = dynamic_cast<PythonEngine*>(plugins.front());
   VERIFYRV(pEngine != NULL, ">>> ");
   return pEngine->getPrompt();
}

PythonInterpreterWizardItem::PythonInterpreterWizardItem()
{
   setName("Python Interpreter");
   setDescription("Allow execution of Python code from within a wizard.");
   setDescriptorId("{2103eba4-f8d8-44be-9fb3-47fbbe8c6cc3}");
   setCopyright(PYTHON_COPYRIGHT);
   setVersion(PYTHON_VERSION_NUMBER);
   setProductionStatus(PYTHON_IS_PRODUCTION_RELEASE);
}

bool PythonInterpreterWizardItem::getInputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<Progress>(Executable::ProgressArg(), NULL));
   VERIFY(pArgList->addArg<std::string>(Interpreter::CommandArg(), std::string()));

   return true;
}

bool PythonInterpreterWizardItem::getOutputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<std::string>(Interpreter::OutputTextArg()));
   VERIFY(pArgList->addArg<std::string>(Interpreter::ReturnTypeArg()));

   return true;
}

bool PythonInterpreterWizardItem::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   VERIFY(pInArgList != NULL && pOutArgList != NULL);

   std::string command;
   if (!pInArgList->getPlugInArgValue(Interpreter::CommandArg(), command))
   {
      return false;
   }
   ProgressTracker progress(pInArgList->getPlugInArgValue<Progress>(ProgressArg()),
      "Execute Python command.", "python", "{5b5d5de3-faad-41ed-894b-dbe5a90d6d4d}");

   std::vector<PlugIn*> plugins = Service<PlugInManagerServices>()->getPlugInInstances(PythonEngine::PlugInName());
   if (plugins.size() != 1)
   {
      progress.report("Unable to locate the Python engine.", 0, ERRORS, true);
      return false;
   }
   PythonEngine* pEngine = dynamic_cast<PythonEngine*>(plugins.front());
   VERIFY(pEngine != NULL);

   progress.report("Executing Python command.", 1, NORMAL);
   std::string returnText;
   std::string errorText;
   if (!pEngine->processCommand(command, returnText, errorText, progress.getCurrentProgress()))
   {
      progress.report("Error executing Python command.", 0, ERRORS, true);
      return false;
   }
   // Populate the output arg list
   if (errorText.empty())
   {
      std::string returnType("Output");
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::OutputTextArg(), &returnText));
   }
   else
   {
      std::string returnType("Error");
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(Interpreter::OutputTextArg(), &errorText));
   }

   progress.report("Executing Python command.", 100, NORMAL);
   progress.upALevel();
   return true;
}
