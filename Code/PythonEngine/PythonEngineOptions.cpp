/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppConfig.h"
#include "AppVerify.h"
#include "FileBrowser.h"
#include "Filename.h"
#include "LabeledSection.h"
#include "OptionQWidgetWrapper.h"
#include "PlugInRegistration.h"
#include "PythonEngineOptions.h"
#include <QtGui/QLabel>
#include <QtGui/QWidget>

REGISTER_PLUGIN(Python, PythonEngineOptions, OptionQWidgetWrapper<PythonEngineOptions>());

PythonEngineOptions::PythonEngineOptions()
{
   QWidget* pPythonConfigWidget = new QWidget(this);
   QLabel* pUserConfLabel = new QLabel("Python User Configuration File Location", pPythonConfigWidget);
   mpUserConfig = new FileBrowser(pPythonConfigWidget);
   mpUserConfig->setBrowseCaption("Locate the Python user configuration file");
   mpUserConfig->setBrowseFileFilters("Python file (*.py *.pyc *.pyo *.pyd)");

   QGridLayout* pPythonConfigLayout = new QGridLayout(pPythonConfigWidget);
   pPythonConfigLayout->addWidget(pUserConfLabel, 0, 0);
   pPythonConfigLayout->addWidget(mpUserConfig, 0, 1);
   pPythonConfigLayout->setColumnStretch(1, 10);
   pPythonConfigLayout->setRowStretch(1, 10);

   LabeledSection* pPythonConfigSection = new LabeledSection(pPythonConfigWidget, "Python Configuration", this);
   const Filename* pTmpFile = PythonEngineOptions::getSettingUserFile();
   setUserFile(pTmpFile);

   // Initialization
   addSection(pPythonConfigSection, 100);
   addStretch(1);
}

PythonEngineOptions::~PythonEngineOptions()
{
}

void PythonEngineOptions::setUserFile(const Filename* pUserFile)
{
   if (pUserFile != NULL)
   {
      mpUserConfig->setFilename(*pUserFile);
   }
}

void PythonEngineOptions::applyChanges()
{
   FactoryResource<Filename> pTmpFile;
   pTmpFile->setFullPathAndName(mpUserConfig->getFilename().toStdString());
   PythonEngineOptions::setSettingUserFile(pTmpFile.get());
}
