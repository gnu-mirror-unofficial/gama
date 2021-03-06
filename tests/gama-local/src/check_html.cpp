/* GNU Gama -- testing adjustment results from different algorithms
   Copyright (C) 2012, 2016, 2022  Ales Cepek <cepek@gnu.org>

   This file is part of the GNU Gama C++ library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <iostream>
#include <iomanip>
#include <fstream>
#include "compare_xml_adjustment.h"


std::string help_notice = R"help_notice(

Compare HTML and XML output files from gama-local

     check_html  one_word_label  file.html  file.xml

)help_notice";

int help(int argc)
{
  if (argc != 4) {
      std::cerr << help_notice;
      return 1;
    }

  return 0;
}


using GNU_gama::LocalNetworkAdjustmentResults;

int main(int argc, char* argv[])
{
  if (help(argc)) return 1;

  std::cout << "max.diff between XML and HTML rounding output for "
            << argv[1] << "\n";

  LocalNetworkAdjustmentResults* html = new LocalNetworkAdjustmentResults;
  {
    std::ifstream inp_html(argv[2]);
    if (!inp_html) {
      std::cout << "   ####  ERROR ON OPENING FILE " << argv[2] << "\n";
      return 1;
    }
    try {
      html->read_html(inp_html);
    } catch (...) {
      std::cout << "   ####  ERROR ON READING " << argv[2] << "\n";
      return 1;
    }
  }

  LocalNetworkAdjustmentResults* xml = new LocalNetworkAdjustmentResults;
  {
    std::ifstream inp_xml(argv[3]);
    if (!inp_xml) {
      std::cout << "   ####  ERROR ON OPENING FILE " << argv[3] << "\n";
      return 1;
    }
    try {
      xml->read_xml(inp_xml);
    } catch (...) {
      std::cout << "   ####  ERROR ON READING FILE " << argv[3] << "\n";
      return 1;
    }
  }

  try
    {
      /* GNU gama 2.10: Because there was a problem in gama-q2 with
       * rendering of HTML table of adjustment results (Qt 5.14.2),
       * see id='confidence_scale', the output was changed and
       * confidence coefficent is printed with 3 decimal digits only
       * (was 5). To keep original precision for XML XML check a new
       * parameter was added to compare_xml_adjustement() with
       * implicit value 1e-5 and for the check of HTML output it is
       * explicitly set to 1e-3.
       */
      return compare_xml_adjustment(html, xml, 1e-1, 1e-3);
    }
  catch (...)
    {
      std::cout << "   ####  EXCEPTION FROM compare_xml_adjustment\n";
    }
}
