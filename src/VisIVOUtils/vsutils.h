/***************************************************************************
 *   Copyright (C) 2008 by Marco Comparato   *
 *   marco.comparato@oact.inaf.it   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef VSUTILS_H
#define VSUTILS_H

/**
  @author Marco Comparato <marco.comparato@oact.inaf.it>
*/

#include <string>
// #include <vector>
#include <map>

class VSUtilOp
{

  static const unsigned int MAX_NUMBER_INT;

  std::map<std::string, std::string> m_parameters;
  int m_maxNumberInt;

protected:
public:
  VSUtilOp();
  ~VSUtilOp();

  // bool setParameters(std::string parameters);
  bool setParameters(std::map<std::string, std::string> parameters)
  {
    m_parameters = parameters;
    return true;
  };
  bool addParameter(std::string key, std::string value);
  //  bool addInput(VSTable *table);

  int isParameterPresent(std::string parameter) { return m_parameters.count(parameter); };

  std::string getParameterAsString(std::string parameter);
  int getParameterAsInt(std::string parameter);
  int getMaxNumberInt() { return m_maxNumberInt; };
  float getParameterAsFloat(std::string parameter);

  std::map<std::string, std::string> getParameters() { return m_parameters; };

  virtual bool execute() = 0;
  virtual void printHelp() = 0;
};

#endif
