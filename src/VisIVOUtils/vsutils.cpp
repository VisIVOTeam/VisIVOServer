/***************************************************************************
 *   Copyright (C) 2008 by Marco Comparato                                 *
 *   marco.comparato@oact.inaf.it                                          *
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

#include "vsutils.h"

#include <sstream>
// #include <iostream>
// #include <set>

// const unsigned int VSUtilOp::MAX_NUMBER_INT = 2147483647;
const unsigned int VSUtilOp::MAX_NUMBER_INT = 250000000;
// const unsigned int VSUtilOp::MAX_NUMBER_INT = 66000;

VSUtilOp::VSUtilOp()
{
  m_maxNumberInt = MAX_NUMBER_INT;
  //  m_maxNumberInt = 13; Only for debug
}

VSUtilOp::~VSUtilOp()
{
}

// bool VSUtilOp::addInput(VSTable *table)
// {
//   if(table)
//   {
//     m_tables.push_back(table);
//
//     return true;
//   }
//
//   return false;
// }

bool VSUtilOp::addParameter(std::string key, std::string value)
{

  /*  key = trim(key);
    value = trim(value);*/

  // Erase  "-" or "--" in key value if any.
  std::map<std::string, std::string>::iterator iter;

  if (key[0] == '-')
    key.erase(0, 1);

  if (key[0] == '-')
    key.erase(0, 1);

  if (!m_parameters.count(key))
    m_parameters.insert(make_pair(key, value)); // insert new parameter
  else
  {
    iter = m_parameters.find(key);
    m_parameters.erase(iter);
    m_parameters.insert(make_pair(key, value)); // substitute the existing parameter
  }
  return true;
}

// bool VSUtilOp::setParameters(std::string parameters)
// {
//   std::string dummy = "";
//   std::string key   = "";
//   std::string value = "";
//
//   std::stringstream ss(parameters);
//   std::streampos pos;
//
//   while(!ss.eof())
//   {
//     ss >> dummy;
//
//     if(dummy[0] != '-')
//       return false;
//
//     dummy.erase(0, 1);
//
//     if(dummy[0] == '-')
//       dummy.erase(0, 1);
//
//     key = dummy;
//
//     if(ss.eof())
//     {
//       value = "unknown";
//
//       m_parameters.insert(make_pair(key, value));
//
//       break;
//     }
//
//     pos = ss.tellg();
//
//     ss >> dummy;
//
//     if(dummy[0] == '-')
//     {
//       if(ss.eof())
//         ss.clear();
//
//       ss.seekg(pos);
//
//       value = "unknown";
//
//       m_parameters.insert(make_pair(key, value));
//
//       continue;
//     }
//
//     value = dummy;
//
//     m_parameters.insert(make_pair(key, value));
//   }
//
//   return false;
// }

std::string VSUtilOp::getParameterAsString(std::string parameter)
{
  if (!m_parameters.count(parameter))
    return "";

  return m_parameters.find(parameter)->second;
}

int VSUtilOp::getParameterAsInt(std::string parameter)
{
  if (!m_parameters.count(parameter))
    return 0;

  std::stringstream ss(m_parameters.find(parameter)->second);

  int ret = 0;

  ss >> ret;

  return ret;
}

float VSUtilOp::getParameterAsFloat(std::string parameter)
{
  if (!m_parameters.count(parameter))
    return 0;

  std::stringstream ss(m_parameters.find(parameter)->second);

  float ret = 0;

  ss >> ret;

  return ret;
}
