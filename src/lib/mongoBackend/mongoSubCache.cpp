#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubCache.h"

using namespace mongo;
using std::auto_ptr;



/* ****************************************************************************
*
* subCacheItemUpdate - 
*/
static void subCacheItemUpdate(const char* tenant, BSONObj* subP)
{
  BSONElement  idField = subP->getField("_id");

  if (idField.eoo() == true)
  {
    LM_E(("Database Error (error retrieving _id field in doc: '%s')", subP->toString().c_str()));
    return;
  }


  std::string               subId             = idField.OID().toString();
  std::string               servicePath       = subP->hasField(CSUB_SERVICE_PATH)? subP->getField(CSUB_SERVICE_PATH).String() : "/";
  std::string               reference         = subP->getField(CSUB_REFERENCE).String();
  int64_t                   expiration        = subP->hasField(CSUB_EXPIRATION)? subP->getField(CSUB_EXPIRATION).Long() : 0;
  int64_t                   throttling        = subP->hasField(CSUB_THROTTLING)? subP->getField(CSUB_THROTTLING).Long() : -1;
  std::vector<BSONElement>  eVec              = subP->getField(CSUB_ENTITIES).Array();
  std::vector<BSONElement>  attrVec           = subP->getField(CSUB_ATTRS).Array();
  std::vector<BSONElement>  condVec           = subP->getField(CSUB_CONDITIONS).Array();
  std::string               formatString      = subP->hasField(CSUB_FORMAT)? subP->getField(CSUB_FORMAT).String() : "XML";
  Format                    format            = stringToFormat(formatString);
  int                       lastNotification  = subP->hasField(CSUB_LASTNOTIFICATION)? subP->getField(CSUB_LASTNOTIFICATION).Int() : 0;

  LM_M(("%s: TEN:%s, SPATH:%s, REF:%s, EXP:%lu, THR:%lu, FMT:%s/%d, LNOT:%d",
        subId.c_str(),
        tenant,
        servicePath.c_str(),
        reference.c_str(),
        expiration,
        throttling,
        formatString.c_str(),
        format,
        lastNotification));
}



/* ****************************************************************************
*
* mongoSubCacheRefresh -
*
* Lookup all subscriptions in the database and call a treat function for each
*/
static void mongoSubCacheRefresh(std::string database, MongoSubCacheTreat treatFunction)
{
  BSONObj                   query      = BSON("conditions.type" << "ONCHANGE" << CSUB_ENTITIES "." CSUB_ENTITY_ISPATTERN << "true");
  DBClientBase*             connection = getMongoConnection();
  auto_ptr<DBClientCursor>  cursor;

  std::string tenant = tenantFromDb(database);
  try
  {
    cursor = connection->query(getSubscribeContextCollectionName(tenant).c_str(), query);

    /*
     * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
     * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
     * exception ourselves
     */
    if (cursor.get() == NULL)
    {
      throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    releaseMongoConnection(connection);

    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);
    LM_E(("Database Error (DBException: %s)", e.what()));
    return;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    LM_E(("Database Error (generic exception)"));
    return;
  }

  bool        reqSemTaken;
  reqSemTake(__FUNCTION__, "cache refresh", SemWriteOp, &reqSemTaken);

  // Call the treat function for each subscription
#if SUB_CACHE_ON
  subCache->semTake();
#endif
  int subNo = 0;
  while (cursor->more())
  {
    BSONObj sub = cursor->next();

    treatFunction(tenant.c_str(), &sub);

#if SUB_CACHE_ON
    treatFunction(tenant, &sub);
#endif

    ++subNo;
  }
#if SUB_CACHE_ON
  subCache->semGive();
#endif

  LM_M(("Got %d subscriptions for tenant '%s'", subNo, tenant.c_str()));
  
  reqSemGive(__FUNCTION__, "cache refresh", reqSemTaken);
}



/* ****************************************************************************
*
* mongoSubCacheRefresh - 
*/
void mongoSubCacheRefresh(void)
{
  std::vector<std::string> databases;
  static int               refreshNo = 0;

  ++refreshNo;
  LM_M(("Refreshing mongo subscription cache [%d]", refreshNo));

  // Get list of databases
  getOrionDatabases(databases);

  // Add the 'default tenant'
  databases.push_back(dbPrefixGet());

  // Now refresh the subCache for each and every tenant
  for (unsigned int ix = 0; ix < databases.size(); ++ix)
  {
    mongoSubCacheRefresh(databases[ix], subCacheItemUpdate);
  }
}
