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

#include "Interpreter.h"
#include "InterpreterManagerShell.h"
#include "PythonCommon.h"
#include "SubjectImp.h"
#include "WizardShell.h"
#include <stdexcept>
#include <string>
#include <vector>

class PythonEngine : public Interpreter, public SubjectImp
{
public:
   PythonEngine();
   virtual ~PythonEngine();

   bool isPythonRunning() const;
   bool startPython();
   std::string getStartupMessage() const;

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

class PythonInterpreter : public InterpreterManagerShell, public SubjectImp
{
public:
   PythonInterpreter();
   virtual ~PythonInterpreter() {}

   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);

   virtual bool isStarted() const;
   virtual bool start();
   virtual std::string getStartupMessage() const;
   virtual Interpreter* getInterpreter() const;

   virtual const std::string& getObjectType() const;
   virtual bool isKindOf(const std::string& className) const;

   SUBJECTADAPTER_METHODS(SubjectImp)
private:
   std::auto_ptr<PythonEngine> mpEngine;

};

class PythonInterpreterWizardItem : public WizardShell
{
public:
   PythonInterpreterWizardItem();
   virtual ~PythonInterpreterWizardItem() {}
   virtual bool getInputSpecification(PlugInArgList*& pArgList);
   virtual bool getOutputSpecification(PlugInArgList*& pArgList);
   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);
};

#endif
