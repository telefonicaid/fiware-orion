Old versions of Orion (0.28.0 and before) supported XML. This directory contains some stuff
related with that functionality that is not longer needed.

In particular, the xmlCheck checker (which was used to check that XML in unittest testData/
and test harness fragments were compliant with NGSI XSD).

In addition, the following XML-based unit tests were removed. If you want to have a look to them,
have a look to code just before merging PR #X (this is the commit number: X).

Deleted files containing only XML-based unit tests:

* serviceRoutines/badNgsi10Request_test.cpp
* serviceRoutines/badNgsi9Request_test.cpp
* serviceRoutines/badRequest_test.cpp
* serviceRoutines/deleteAttributeValueInstance_test.cpp
* serviceRoutines/deleteIndividualContextEntityAttribute_test.cpp
* serviceRoutines/deleteIndividualContextEntity_test.cpp
* serviceRoutines/getAttributeValueInstance_test.cpp
* serviceRoutines/getContextEntitiesByEntityId_test.cpp
* serviceRoutines/getContextEntityAttributes_test.cpp
* serviceRoutines/getContextEntityTypeAttributeContainer_test.cpp
* serviceRoutines/getContextEntityTypeAttribute_test.cpp
* serviceRoutines/getContextEntityTypes_test.cpp
* serviceRoutines/getEntityByIdAttributeByName_test.cpp
* serviceRoutines/getIndividualContextEntityAttribute_test.cpp
* serviceRoutines/getIndividualContextEntity_test.cpp
* serviceRoutines/getNgsi10ContextEntityTypesAttribute_test.cpp
* serviceRoutines/getNgsi10ContextEntityTypes_test.cpp
* serviceRoutines/leakTreat_test.cpp
* serviceRoutines/logTraceTreat_test.cpp
* serviceRoutines/postContextEntitiesByEntityId_test.cpp
* serviceRoutines/postContextEntityAttributes_test.cpp
* serviceRoutines/postContextEntityTypeAttributeContainer_test.cpp
* serviceRoutines/postContextEntityTypeAttribute_test.cpp
* serviceRoutines/postContextEntityTypes_test.cpp
* serviceRoutines/postDiscoverContextAvailability_test.cpp
* serviceRoutines/postEntityByIdAttributeByName_test.cpp
* serviceRoutines/postIndividualContextEntityAttribute_test.cpp
* serviceRoutines/postIndividualContextEntity_test.cpp
* serviceRoutines/postQueryContext_test.cpp
* serviceRoutines/postRegisterContext_test.cpp
* serviceRoutines/postSubscribeContextAvailability_test.cpp
* serviceRoutines/postSubscribeContext_test.cpp
* serviceRoutines/postUnsubscribeContextAvailability_test.cpp
* serviceRoutines/postUnsubscribeContext_test.cpp
* serviceRoutines/postUpdateContextAvailabilitySubscription_test.cpp
* serviceRoutines/postUpdateContextSubscription_test.cpp
* serviceRoutines/postUpdateContext_test.cpp
* serviceRoutines/putAttributeValueInstance_test.cpp
* serviceRoutines/putIndividualContextEntity_test.cpp

