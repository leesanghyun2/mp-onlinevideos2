/*
    Copyright (C) 2007-2010 Team MediaPortal
    http://www.team-mediaportal.com

    This file is part of MediaPortal 2

    MediaPortal 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MediaPortal 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaPortal 2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef __FILE_TYPE_BOX_DEFINED
#define __FILE_TYPE_BOX_DEFINED

#include "box.h"
#include "BrandCollection.h"

#define FILE_TYPE_BOX_TYPE                                                    L"ftyp"

class CFileTypeBox :
  public CBox
{
public:
  // initializes a new instance of CFileTypeBox class
  CFileTypeBox(void);

  // destructor
  virtual ~CFileTypeBox(void);

  /* get methods */

  // gets major brand
  // @return : major brand
  virtual CBrand *GetMajorBrand(void);

  // gets minor version
  // @return : minor version
  virtual unsigned int GetMinorVersion(void);

  // gets compatible brands
  // @return : compatible brands
  virtual CBrandCollection *GetCompatibleBrands(void);

  /* set methods */

  /* other methods */

  // parses data in buffer
  // @param buffer : buffer with box data for parsing
  // @param length : the length of data in buffer
  // @return : true if parsed successfully, false otherwise
  virtual bool Parse(const unsigned char *buffer, unsigned int length);

  // gets box data in human readable format
  // @param indent : string to insert before each line
  // @return : box data in human readable format or NULL if error
  virtual wchar_t *GetParsedHumanReadable(const wchar_t *indent);

protected:
  // stores major brand
  CBrand *majorBrand;
  // stores minor version
  unsigned int minorVersion;
  // stores compatible brands
  CBrandCollection *compatibleBrands;
};

#endif