/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PYTHONTESTS_H__
#define PYTHONTESTS_H__

#include "PlugInShell.h"
#include "Testable.h"

class PythonTests : public PlugInShell, public Testable
{
public:
   PythonTests();
   virtual ~PythonTests();

   virtual bool runOperationalTests(Progress* pProgress, std::ostream& failure);
   virtual bool runAllTests(Progress* pProgress, std::ostream& failure);
};

#endif