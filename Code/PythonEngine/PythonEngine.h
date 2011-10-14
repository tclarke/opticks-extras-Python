/*
 * The information in this file is
 * Copyright(c) 2009 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PYTHONENGINE_H__
#define PYTHONENGINE_H__

#include "AppConfig.h"
#include "PythonCommon.h"
#include "PythonInterpreter.h"
#include "SubjectImp.h"
#include <stdexcept>
#include <string>
#include <vector>

class External;

extern "C" PyObject* transmitOutput(PyObject* pSelf, PyObject* pArgs);
extern "C" LINKAGE PythonInterpreter* init_python_engine(External* pServices);
extern "C" LINKAGE void shutdown_python_engine();

class PythonEngine : public PythonInterpreter, public SubjectImp
{
public:
   PythonEngine();
   virtual ~PythonEngine();

   bool isPythonRunning() const;
   bool startPython();
   std::string getStartupMessage() const;
   void setStartupMessage(const std::string& msg);

   virtual std::string getPrompt() const;
   virtual bool executeCommand(const std::string& command);
   virtual bool executeScopedCommand(const std::string& command, const Slot& output,
      const Slot& error, Progress* pProgress);
   virtual bool isGlobalOutputShown() const;
   virtual void showGlobalOutput(bool newValue);

   void sendOutput(const std::string& text);
   void sendError(const std::string& text);

   virtual const std::string& getObjectType() const;
   virtual bool isKindOf(const std::string& className) const;

   SUBJECTADAPTER_METHODS(SubjectImp)

   class PythonError : public std::exception
   {
   public:
      PythonError(const std::string& what) : mWhat(what) {}
      virtual ~PythonError() throw() {}
      virtual const char* what() const throw()
      {
         return mWhat.c_str();
      }

   private:
      std::string mWhat;
   };

protected:
   void checkErr();

private:
   void sendOutput(const std::string& text, bool scoped);
   void sendError(const std::string& text, bool scoped);

   void gatherOutput(Subject& subject, const std::string& signal, const boost::any& data);

   SIGNAL_METHOD(PythonEngine, ScopedOutputText);
   SIGNAL_METHOD(PythonEngine, ScopedErrorText);

   auto_obj mInterpModule;
   auto_obj mStdin;
   auto_obj mInterpreter;
   auto_obj mGlobals;
   auto_obj mRunModule;
   std::string mPrompt;
   bool mGlobalOutputShown;
   bool mPythonRunning;
   bool mAttemptedOneStart;
   bool mRunningScopedCommand;
   std::string mStartupMessage;
   std::string mGatheredOutput;
};

#endif
