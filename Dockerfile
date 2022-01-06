# Copyright 2021 Telefonica Investigacion y Desarrollo, S.A.U
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
#

FROM ubuntu:latest
RUN apt-get update -y --fix-missing
RUN apt-get install python3.6
RUN apt-get install -y python3-pip
RUN pip install Flask==1.0.2
RUN pip install pyOpenSSL==19.0.0
RUN pip install paho-mqtt==1.5.1
COPY . /app
WORKDIR /app
ENTRYPOINT [ "python3", "./accumulator-server.py"]
CMD ["1028", "/accumulate", "0.0.0.0", "on"]
