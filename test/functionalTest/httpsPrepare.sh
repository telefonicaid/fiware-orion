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

# -----------------------------------------------------------------------------
#
# usage
#
function usage()
{
  fileName=$(basename $0)
  echo $fileName "[--keyFileName (name of key file)] [--certFileName (name of certificate file)]"
  echo "                [--country (country)] [--state (state)] [--city (city)] [--company (company)] [--unit (unit)] [--name (name)] [--email (email)]"
  echo
  echo "  The last seven options are input for the creation of the certificate and they all have 'decent' default values."
  exit $1
}



keyFileName="localhost.key"
certFileName="localhost.pem"
country="ES"
state="Madrid"
city="Madrid"
company="Telefonica"
unit="I+D"
name="localhost"
email="noone@nowhere.com"


while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];             then usage;
  elif [ "$1" == "--keyFileName" ];  then keyFileName=$2;  shift;
  elif [ "$1" == "--certFileName" ]; then certFileName=$2; shift;
  elif [ "$1" == "--state" ];        then state=$2;        shift;
  elif [ "$1" == "--city" ];         then city=$2;         shift;
  elif [ "$1" == "--company" ];      then company=$2;      shift;
  elif [ "$1" == "--unit" ];         then unit=$2;         shift;
  elif [ "$1" == "--name" ];         then name=$2;         shift;
  elif [ "$1" == "--email" ];        then email=$2;        shift;
  else
    echo $0: bad parameter/option: "'"${1}"'";
    usage 1
  fi

  shift
done


OPTIONS="/C="$country"/ST="$state"/L="$city"/O="$company"/OU="$unit"/CN="$name"/"

openssl genrsa -out "$keyFileName" 2048 > /dev/null 2>&1
openssl req -days 365 -out "$certFileName" -new -x509 -key "$keyFileName" -subj "$OPTIONS" > /dev/null 2>&1
