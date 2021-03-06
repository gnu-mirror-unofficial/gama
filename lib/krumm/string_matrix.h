/* Krumm2gama-local -- conversion from F. Krumm format to XML gama-local
   Copyright (C) 2022 Ales Cepek <cepek@gnu.org>

   This file is part of Krumm2gama-local.

   Krumm2gama-local is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Krumm2gama-local is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Krumm2gama-local. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef STRINGMATRIX_H
#define STRINGMATRIX_H

#include <map>
#include <string>

namespace GNU_gama { namespace local {

class StringMatrix : public std::map<int, std::map<int, std::string>>
{
public:
  StringMatrix();

  int rows() const;
  int cols() const;
};

}} // namespace GNU_gama::local

#endif // STRINGMATRIX_H
