/*  
    Geodesy and Mapping C++ Library (GNU GaMa / GaMaLib)
    Copyright (C) 2002  Ales Cepek <cepek@fsv.cvut.cz>

    This file is part of the GNU GaMa / GaMaLib C++ Library.
    
    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 *  $Id: dataobject.h,v 1.2 2002/10/19 13:05:29 cepek Exp $
 */

#ifndef GaMaLib_GaMa_XML_Data_Object__object___h_
#define GaMaLib_GaMa_XML_Data_Object__object___h_

#include <string>

namespace GaMaLib {

  class DataObject {
  public:

    ~DataObject() 
      {
      }
    virtual std::string xml() const = 0;

  };


  class TextDataObject : public DataObject {
  public:
  
    std::string text;
  
    TextDataObject() 
      {
      }    
    TextDataObject(std::string s) : text(s) 
      {
      }    
    std::string xml() const 
      {
        return "\n<text>" + text + "</text>\n";
      }
  };

}       // namespace GaMaLib


#endif

















