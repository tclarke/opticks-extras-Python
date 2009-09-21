/*
 * The information in this file is
 * Copyright(c) 2008 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PYTHONENGINE_H__
#define PYTHONENGINE_H__

#include "ExecutableShell.h"
#include "InterpreterShell.h"
#include "PythonCommon.h"
#include "WizardShell.h"
#include <stdexcept>

class PythonEngine : public ExecutableShell
{
public:
   static std::string PlugInName() { return "Python Engine"; }
   PythonEngine();
   virtual ~PythonEngine();

   virtual bool getInputSpecification(PlugInArgList*& pArgList);
   virtual bool getOutputSpecification(PlugInArgList*& pArgList);
   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);

   virtual std::string getPrompt() const;
   bool processCommand(const std::string& command, std::string& returnText, std::string& errorText, Progress* pProgress);

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
   auto_obj mInterpModule;
   auto_obj mStdin;
   auto_obj mStdout;
   auto_obj mStderr;
   auto_obj mInterpreter;
   std::string mPrompt;
};

class PythonInterpreter : public InterpreterShell
{
public:
   PythonInterpreter();
   virtual ~PythonInterpreter() {}

   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);
   virtual std::string getPrompt() const;
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
