/*
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
 */

/*

IT IS HIGHLY ADVISABLE TO PERFORM A DATABASE BACKUP BEFORE USING THIS SCRIPT. AS
STATED IN THE LICENSE TEST ABOVE, THIS SCRIPT IS PROVIDED IN THE HOPE IT WILL BE
USEFUL BUT WITHOUT ANY WARRANTY.

This script is aimed to swap the position of the coords in the location.coords field in the
entities collection. It is needed during the 0.14.0 (or before) to 0.14.1 upgrage
procedure in the case you have geo-tagged information in your Orion database.

Usage: mongo <db> swap_coords.js

where <db> is the name of the database in which you want to perform the operation (typically
"orion" or "orion_something" if you use -multiservice option).

The script prints to standard output the entities being processed.

*/

cursor = db.entities.find();
while ( cursor.hasNext() ) {
   doc = cursor.next();
   entity = '<id=' + doc._id.id + ', type=' + doc._id.type + '>';
   if (doc.location) {
      old0 = doc.location.coords[0];
      old1 = doc.location.coords[1];
      doc.location.coords[0] = old1;
      doc.location.coords[1] = old0;
      print (entity + ' swapping [ ' + old0 + ' , ' + old1 + ' ] -> [ ' + doc.location.coords[0] + ' , ' + doc.location.coords[1] + ' ]');
      db.entities.save(doc);
   }
   else {
      print (entity + ' skipped (no location)');
   }
}
