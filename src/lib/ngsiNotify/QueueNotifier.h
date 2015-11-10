#ifndef SRC_LIB_NGSINOTIFY_QUEUENOTIFIER_H
#define SRC_LIB_NGSINOTIFY_QUEUENOTIFIER_H

#include "common/SyncQOverflow.h"
#include "ngsiNotify/Notifier.h"
#include "ngsiNotify/senderThread.h"

// default queue size
#define DEFAULT_NOTIF_QS 100
// default number of threads
#define DEFAULT_NOTIF_TN 10

/* ****************************************************************************
*
* class QueueNotifier-
*/
class QueueNotifier : public Notifier
{
public:
  QueueNotifier(SyncQOverflow<SenderThreadParams*> *pQ): pQueue(pQ) {}
  void sendNotifyContextRequest(NotifyContextRequest* ncr, const std::string& url, const std::string& tenant, const std::string& xauthToken, Format format);

private:
 SyncQOverflow<SenderThreadParams*>* pQueue;
};

#endif // SRC_LIB_NGSINOTIFY_QUEUENOTIFIER_H
