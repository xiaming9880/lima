#   Copyright 2002-2013 CEA LIST
#    
#   This file is part of LIMA.
#
#   LIMA is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   LIMA is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
#
#   You should have received a copy of the GNU Affero General Public License
#   along with LIMA.  If not, see <http://www.gnu.org/licenses/>

ADD_SA_RULES_DEPENDS(fre
        chainsMatrix-fre.bin
        categoriesClassesDeclaration-fre.txt
        compoundTenses-fre.bin
)

set(FRE_RULES_FILES
  rules-fre-heterodeps.txt
  rules-fre-homodeps-pass1.txt
  rules-fre-homodeps-pass2.txt
  rules-fre-homodeps-pass0.txt
  rules-fre-pleonasticPronouns.txt
  rules-fre-heterodeps.txt
  rules-fre-heterodeps-specializer.txt
  simplification-first-rules-fre.txt
  simplification-quotes-rules-fre.txt
  simplification-last-rules-fre.txt
  simplification-rules-fre.txt
  rules-fre-dummy.txt
  rules-compoundTense.txt
  rules-fre-coord.txt 
)
COMPILE_SA_RULES_WRAPPER (fre ${FRE_RULES_FILES})


# install(
#   FILES
#     selectionalPreferences-fre.bin
#   COMPONENT fre DESTINATION share/apps/lima/resources/SyntacticAnalysis
# )
