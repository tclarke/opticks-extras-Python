/*
 * The information in this file is
 * Copyright(c) 2009 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PYTHONINTERPRETEROPTIONS_H
#define PYTHONINTERPRETEROPTIONS_H

#include "Filename.h"
#include "LabeledSectionGroup.h"
#include "PythonVersion.h"
#include <string>
#include <vector>

class FileBrowser;
class QLineEdit;

class PythonInterpreterOptions : public LabeledSectionGroup
{
   Q_OBJECT

public:
   PythonInterpreterOptions();
   virtual ~PythonInterpreterOptions();

   void setUserFile(const Filename* pUserFile);
   void applyChanges();

   static const std::string& getName()
   {
      static std::string var = "Python Engine Options";
      return var;
   }

   static const std::string& getOptionName()
   {
      static std::string var = "Scripting/Python Engine";
      return var;
   }

   static const std::string& getDescription()
   {
      static std::string var = "Configuration options for the Python engine";
      return var;
   }

   static const std::string& getShortDescription()
   {
      static std::string var = "Configuration options for the Python engine";
      return var;
   }

   static const std::string& getCreator()
   {
      static std::string var = "Ball Aerospace & Technologies Corp.";
      return var;
   }

   static const std::string& getCopyright()
   {
      static std::string var = PYTHON_COPYRIGHT;
      return var;
   }

   static const std::string& getVersion()
   {
      static std::string var = PYTHON_VERSION_NUMBER;
      return var;
   }

   static bool isProduction()
   {
      return PYTHON_IS_PRODUCTION_RELEASE;
   }

   static const std::string& getDescriptorId()
   {
      static std::string var = "{7fdc8adc-c0d4-49d7-8aad-b4d8a51f12af}";
      return var;
   }

private:
   FileBrowser* mpUserConfig;
   QLineEdit* mpPythonHome;
};

#endif
