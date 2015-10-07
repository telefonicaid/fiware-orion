# Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es

VERSION=$1

############################### SONAR_RUNNER ###############################
################################ COMMON PROPERTIES:BEGIN
### PROJECT PROPERTIES ###
echo sonar.projectName=fiware-orion
echo sonar.projectKey=com.telefonica.iot:orion
echo sonar.projectVersion=$VERSION
### SOURCES
echo sonar.sources=src/
echo sonar.tests=test/
### TELEFONICA I+D PROPERTIES ###
# This properties will be added to sonar as manual measures.
echo product.area.name=\"iotplatform\"
echo product.name=\"orion\"
echo product.release=\"$VERSION\"
################################ COMMON PROPERTIES:END
################################ SPECIFIC PROPERTIES(language dependency):BEGIN
### LANGUAGE
echo sonar.language=c++
### EXCLUSIONS
echo sonar.exclusions=**/lib/parseArgs/**,**/lib/logMsg/**,/usr/**
### TESTS 
echo sonar.cxx.xunit.reportPath=BUILD_UNITTEST/unit_test.xml
echo sonar.cxx.coverage.reportPath=coverage_sonar.xml
echo sonar.cxx.cppcheck.reportPath=cppcheck-result.xml
################################ SPECIFIC PROPERTIES:END"

