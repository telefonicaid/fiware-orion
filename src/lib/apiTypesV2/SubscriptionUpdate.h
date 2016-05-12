#ifndef SRC_LIB_APITYPESV2_SUBSCRIPTIONUPDATE_H_
#define SRC_LIB_APITYPESV2_SUBSCRIPTIONUPDATE_H_

#include "apiTypesV2/Subscription.h"

namespace ngsiv2
{

  class SubscriptionUpdate : public Subscription
  {
  public:

    bool  descriptionProvided;
    bool  subjectProvided;
    bool  expiresProvided;
    bool  statusProvided;
    bool  notificationProvided;
    bool  attrsFormatProvided;
    bool  throttlingProvided;

    SubscriptionUpdate():
      descriptionProvided  (false),
      subjectProvided      (false),
      expiresProvided      (false),
      statusProvided       (false),
      notificationProvided (false),
      attrsFormatProvided  (false),
      throttlingProvided   (false)
      {}

  };

}

#endif // SRC_LIB_APITYPESV2_SUBSCRIPTIONUPDATE_H_
