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
# fermin at tid dot es

echo "-----------------------------------------------------------"
echo "  Creating private key and certificate files for localhost"
echo "  Suggested answers to the question about the certificate:"
echo "    - Country:  ES"
echo "    - State:    Madrid"
echo "    - City:     Madrid"
echo "    - Company:  Telefonica"
echo "    - Unit:     I+D"
echo "    - NAME:     localhost   # This one seems pretty important!"
echo "    - e-mail:   you@where.com"
echo "-----------------------------------------------------------"

openssl genrsa -out localhost.key 1024
openssl req -days 365 -out localhost.pem -new -x509 -key localhost.key
