Old versions of Orion (0.28.0 and before) supported XML. This directory contains some stuff
related with that functionality that is not longer needed.

In particular, the xmlCheck checker (which was used to check that XML in unittest testData/
and test harness fragments were compliant with NGSI XSD).

In addition, the following XML-based unit tests were removed. If you want to have a look to them,
have a look to code just before merging PR #2104 (this is the commit number: 
https://github.com/telefonicaid/fiware-orion/commit/d6969490c8db77b2147fccbd986aab49303b5e8c).

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

The following XML-based test were also removed (most of them were in DISABLED
state just before removing it):

* TEST(compoundValue, DISABLED_updateContextValueVectorFiveItemsPlusBadOne)
* TEST(Convenience, DISABLED_badPathNgsi10)
* TEST(Convenience, DISABLED_badPathNgsi9)
* TEST(Convenience, DISABLED_shortPath)
* TEST(DiscoverContextAvailabilityRequest, DISABLED_entityIdIdAsAttribute_xml)
* TEST(DiscoverContextAvailabilityRequest, DISABLED_entityIdIsPattern_xml)
* TEST(DiscoverContextAvailabilityRequest, DISABLED_entityIdTypeAsField_xml)
* TEST(DiscoverContextAvailabilityRequest, DISABLED_entityIdType_xml)
* TEST(NotifyContextAvailabilityRequest, DISABLED_badEntityAttribute_xml)
* TEST(NotifyContextRequest, DISABLED_predetectedError)
* TEST(NotifyContextRequest, DISABLED_xml_invalidEntityIdAttribute)
* TEST(QueryContextRequest, DISABLED_emptyEntityList_xml)
* TEST(QueryContextRequest, DISABLED_emptyScopeType_xml)
* TEST(QueryContextRequest, DISABLED_emptyScopeValue_xml)
* TEST(QueryContextRequest, DISABLED_entityIdIdAsAttribute_xml)
* TEST(QueryContextRequest, DISABLED_entityIdIsPattern_xml)
* TEST(QueryContextRequest, DISABLED_entityIdType_xml)
* TEST(QueryContextRequest, DISABLED_fill)
* TEST(QueryContextRequest, DISABLED_noAttributeExpression_xml)
* TEST(QueryContextRequest, DISABLED_noEntityList_xml)
* TEST(QueryContextRequest, DISABLED_noRestriction_xml)
* TEST(QueryContextRequest, DISABLED_noScopeType_xml)
* TEST(QueryContextRequest, DISABLED_noScopeValue_xml)
* TEST(QueryContextRequest, DISABLED_overwriteEntityIdIsPattern_xml)
* TEST(QueryContextRequest, DISABLED_overwriteEntityIdType_xml)
* TEST(QueryContextRequest, DISABLED_unsupportedEntityIdAttribute_xml)
* TEST(QueryContextRequest, emptyEntityIdId_xml)
* TEST(QueryContextResponse, DISABLED_ok_xml)
* TEST(RegisterContextRequest, DISABLED_durationError)
* TEST(RegisterContextRequest, DISABLED_emptyContextMetadataName)
* TEST(RegisterContextRequest, DISABLED_emptyContextMetadataValue)
* TEST(RegisterContextRequest, DISABLED_emptyContextRegistration)
* TEST(RegisterContextRequest, DISABLED_emptyContextRegistrationAttributeIsDomain)
* TEST(RegisterContextRequest, DISABLED_emptyContextRegistrationAttributeName)
* TEST(RegisterContextRequest, DISABLED_emptyEntityIdList)
* TEST(RegisterContextRequest, DISABLED_emptyRegistrationMetadataValue)
* TEST(RegisterContextRequest, DISABLED_entityIdWithEmptyId)
* TEST(RegisterContextRequest, DISABLED_entityIdWithNoId)
* TEST(RegisterContextRequest, DISABLED_invalidAttributeName)
* TEST(RegisterContextRequest, DISABLED_noEntityIdList)
* TEST(RegisterContextRequest, DISABLED_present)
* TEST(restReply, DISABLED_restErrorReplyGet)
* TEST(SubscribeContextAvailabilityRequest, DISABLD_xml_noReference)
* TEST(SubscribeContextAvailabilityRequest, DISABLED_xml_badEntityId)
* TEST(SubscribeContextAvailabilityRequest, DISABLED_xml_entityIdIsPatternAsBothFieldAndAttribute)
* TEST(SubscribeContextAvailabilityRequest, DISABLED_xml_entityIdTypeAsBothFieldAndAttribute)
* TEST(SubscribeContextRequest, DISABLED_invalidEntityIdAttribute_xml)
* TEST(UpdateContextAvailabilitySubscriptionRequest, DISABLED_xml_invalidEntityAttribute)
* TEST(UpdateContextResponse, DISABLED_constructors)
* TEST(UpdateContextSubscriptionResponse, DISABLED_constructors)
