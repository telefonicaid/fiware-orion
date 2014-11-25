# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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

SET (HEADERS
	paBuiltin.h
	paConfig.h
	paDefaultValues.h
	paEnvVals.h
	paFrom.h
	paFullName.h
	paGetVal.h
	paIsOption.h
	paIsSet.h
	paIterate.h
	paIxLookup.h
	paLimitCheck.h
	paLog.h
	paLogSetup.h
	paOptions.h
	paOptionsParse.h
	paParse.h
	paPrivate.h
	paRcFile.h
	paTraceLevels.h
	paUsage.h
	paValueFrom.h
	paWarning.h
	parseArgs.h
	baStd.h
)

SET (SOURCES
	paBuiltin.cpp
	paConfig.cpp
	paDefaultValues.cpp
	paEnvVals.cpp
	paFrom.cpp
	paFullName.cpp
	paGetVal.cpp
	paIsOption.cpp
	paIsSet.cpp
	paIterate.cpp
	paIxLookup.cpp
	paLimitCheck.cpp
	paLogSetup.cpp
	paOptions.cpp
	paOptionsParse.cpp
	paParse.cpp
	paRcFile.cpp
	paUsage.cpp
	paValueFrom.cpp
	paWarning.cpp
	baStd.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")

# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(pa STATIC ${SOURCES} ${HEADERS})
SET (LIBPA ${PROJECT_BINARY_DIR}/src/parseArgs/libpa.a CACHE INTERNAL "")
