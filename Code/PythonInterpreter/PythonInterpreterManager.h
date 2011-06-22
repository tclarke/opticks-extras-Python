/*
 * The information in this file is
 * Copyright(c) 2009 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PYTHONINTERPRETERMANAGER_H__
#define PYTHONINTERPRETERMANAGER_H__

#include "ApplicationServices.h"
#include "AttachmentPtr.h"
#include "InterpreterManagerShell.h"
#include "SubjectImp.h"
#include <string>

class DynamicModule;
class PythonInterpreter;

class PythonInterpreterManager : public InterpreterManagerShell, public SubjectImp
{
public:
   PythonInterpreterManager();
   virtual ~PythonInterpreterManager() {}

   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);

   virtual bool isStarted() const;
   virtual bool start();
   virtual std::string getStartupMessage() const;
   virtual Interpreter* getInterpreter() const;

   virtual const std::string& getObjectType() const;
   virtual bool isKindOf(const std::string& className) const;

   SUBJECTADAPTER_METHODS(SubjectImp)

private:
   AttachmentPtr<ApplicationServices> mpAppServices;
   void applicationClosed(Subject& subject, const std::string& signal, const boost::any& data);

   bool mAppShuttingDown;
   DynamicModule* mpModule;
   PythonInterpreter* mpInterpreter;
   std::string mStartupMessage;
};
#endif
