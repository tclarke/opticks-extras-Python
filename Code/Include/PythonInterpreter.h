/*
 * The information in this file is
 * Copyright(c) 2011 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PYTHONINTERPRETER_H
#define PYTHONINTERPRETER_H

#include "ConfigurationSettings.h"
#include "Interpreter.h"

class PythonInterpreter : public Interpreter
{
public:
   SETTING_PTR(UserFile, PythonEngine, Filename);
   SETTING(InteractiveAvailable, PythonEngine, bool, true);
   SETTING(PythonHome, PythonEngine, std::string, "");

   virtual bool isPythonRunning() const = 0;
   virtual bool startPython() = 0;
   virtual std::string getStartupMessage() const = 0;
};

#endif