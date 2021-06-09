#ifndef SRC_LIB_NGSINOTIFY_NOTIFIER_H_
#define SRC_LIB_NGSINOTIFY_NOTIFIER_H_

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Fermin Galan
*/
#include <map>
#include <pthread.h>

#include "apiTypesV2/HttpInfo.h"
#include "common/RenderFormat.h"
#include "ngsi10/NotifyContextRequest.h"

#include "ngsiNotify/senderThread.h"



/* ****************************************************************************
*
* Notifier -
*/
class Notifier
{
public:
  virtual ~Notifier(void);

  virtual void sendNotifyContextRequest(NotifyContextRequest&            ncr,
                                        const ngsiv2::HttpInfo&          httpInfo,
                                        const std::string&               tenant,
                                        const std::string&               xauthToken,
                                        const std::string&               fiwareCorrelator,
                                        unsigned int                     correlatorCounter,
                                        RenderFormat                     renderFormat,
                                        const std::vector<std::string>&  attrsFilter,
                                        bool                             blacklist,
                                        const std::vector<std::string>&  metadataFilter);

protected:
  static std::vector<SenderThreadParams*>* buildSenderParams(NotifyContextRequest&            ncr,
                                                             const ngsiv2::HttpInfo&          httpInfo,
                                                             const std::string&               tenant,
                                                             const std::string&               xauthToken,
                                                             const std::string&               fiwareCorrelator,
                                                             unsigned int                     correlatorCounter,
                                                             RenderFormat                     renderFormat,
                                                             const std::vector<std::string>&  attrsFilter,
                                                             bool                             blacklist,
                                                             const std::vector<std::string>&  metadataFilter);
};

#endif  // SRC_LIB_NGSINOTIFY_NOTIFIER_H_
