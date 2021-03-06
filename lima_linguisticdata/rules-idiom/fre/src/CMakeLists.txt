#   Copyright 2002-2019 CEA LIST
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
file (GLOB SYMBOLICCODESFRE ${PROJECT_SOURCE_DIR}/lima_linguisticdata/analysisDictionary/fre/code/symbolicCode*-fre.xml)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/idiomaticExpressions-fre.rules
  COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/buildSeparableVerbList.pl -list=${CMAKE_CURRENT_BINARY_DIR}/separableVerbs.lst -rules=${CMAKE_CURRENT_BINARY_DIR}/separableVerbsSecond.rules ${CMAKE_CURRENT_SOURCE_DIR}/idioms-fre.txt > ${CMAKE_CURRENT_BINARY_DIR}/idioms-fre.withoutVerbs.txt
  COMMAND perl ${PROJECT_SOURCE_DIR}/lima_linguisticdata/scripts/reformat_idioms.pl ${CMAKE_CURRENT_SOURCE_DIR}/idioms-fre.txt > ${CMAKE_CURRENT_BINARY_DIR}/idioms-fre.grace
  COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/separableVerbs.rules ${CMAKE_CURRENT_BINARY_DIR}/separableVerbsSecond.rules >> ${CMAKE_CURRENT_BINARY_DIR}/idioms-fre.grace
  COMMAND perl ${PROJECT_SOURCE_DIR}/lima_linguisticdata/scripts/transcodeIdiomatic.pl ${CMAKE_BINARY_DIR}/lima_linguisticprocessing/tools/common/convertSymbolicCodes --configDir=${PROJECT_SOURCE_DIR}/lima_common/conf/ ${PROJECT_SOURCE_DIR}/lima_linguisticdata/analysisDictionary/fre/code/code-fre.xml ${SYMBOLICCODESFRE} ${CMAKE_CURRENT_BINARY_DIR}/idioms-fre.grace > ${CMAKE_CURRENT_BINARY_DIR}/idiomaticExpressions-fre.rules
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/buildSeparableVerbList.pl
    ${CMAKE_CURRENT_SOURCE_DIR}/categories_convert
    ${PROJECT_SOURCE_DIR}/lima_linguisticdata/scripts/reformat_idioms.pl
    ${PROJECT_SOURCE_DIR}/lima_linguisticdata/scripts/transcodeIdiomatic.pl
    ${CMAKE_CURRENT_SOURCE_DIR}/idioms-fre.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/separableVerbs.rules
    ${SYMBOLICCODESFRE}
    convertSymbolicCodes
  COMMENT "create fre idiomatic expressions rules"
  VERBATIM
)

IDIOMATICENTITIES(fre)
