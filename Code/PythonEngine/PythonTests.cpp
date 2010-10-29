/*
 * The information in this file is
 * Copyright(c) 2009 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppVerify.h"
#include "InterpreterUtilities.h"
#include "PythonTests.h"
#include "PythonVersion.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "Progress.h"
#include "PythonEngine.h"

REGISTER_PLUGIN_BASIC(Python, PythonTests);

PythonTests::PythonTests()
{
   setName("PythonTests");
   setDescription("Python test suite");
   setDescriptorId("{2a4eeedc-54d1-47b0-bb49-5bba6088b189}");
   setCopyright(PYTHON_COPYRIGHT);
   setVersion(PYTHON_VERSION_NUMBER);
   setProductionStatus(PYTHON_IS_PRODUCTION_RELEASE);
   setType("Testable");
}

PythonTests::~PythonTests()
{
}

bool PythonTests::runOperationalTests(Progress* pProgress, std::ostream& failure)
{
   std::vector<PlugIn*> plugins = Service<PlugInManagerServices>()->getPlugInInstances("Python");
   if (plugins.size() != 1)
   {
      failure << "Unable to locate python engine. " \
         "Opticks may not be able to locate your Python installation. Try setting PYTHONHOME.";
      return false;
   }
   PythonInterpreter* pInterMgr = dynamic_cast<PythonInterpreter*>(plugins.front());
   VERIFY(pInterMgr != NULL);
   Interpreter* pInterpreter = pInterMgr->getInterpreter();
   if (pInterpreter == NULL)
   {
      failure << "Unable to locate python engine. " \
         "Opticks may not be able to locate your Python installation. Try setting PYTHONHOME.";
      return false;
   }

   if (pProgress != NULL)
   {
      pProgress->updateProgress("Executing Python tests.", 5, NORMAL);
   }
   std::string command = "import opticks.test\n"
      "result = opticks.test.run_tests()\n"
      "if not(result.wasSuccessful()):\n"
      "    raise Exception('One or more tests failed')";
   std::string returnText;
   bool hasErrorText = false;
   bool testsPassed = InterpreterUtilities::executeScopedCommand("Python", command, returnText,
      hasErrorText, pProgress);
   if (pProgress != NULL)
   {
      pProgress->updateProgress(returnText, 99, WARNING);
   }
   if (!testsPassed)
   {
      if (pProgress != NULL)
      {
         pProgress->updateProgress("Python tests failed.", 0, ERRORS);
      }
      return false;
   }
   if (pProgress != NULL)
   {
      pProgress->updateProgress("Python tests complete.", 100, NORMAL);
   }

   return true;
}

bool PythonTests::runAllTests(Progress* pProgress, std::ostream& failure)
{
   return runOperationalTests(pProgress, failure);
}
