//***************************************************************************
// Web Service
// File webservice.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 28.12.2021 - Jörg Wendel
//***************************************************************************

#pragma once

//***************************************************************************
// Class Web Service
//***************************************************************************

class cWebService
{
   public:

      enum Event
      {
         evUnknown,
         evLogin,
         evLogout,
         evPageChange,
         evData,
         evInit,
         evToggleIo,
         evToggleIoNext,
         evToggleMode,
         evStoreConfig,
         evGetToken,
         evSetup,
         evStoreIoSetup,
         evChartData,
         evLogMessage,

         evUserDetails,
         evStoreUserConfig,
         evChangePasswd,

         evReset,
         evGroups,
         evGroupConfig,
         evChartbookmarks,
         evStoreChartbookmarks,
         evSendMail,
         evSyslog,
         evSystem,
         evForceRefresh,
         evStoreDashboards,
         evAlerts,
         evStoreAlerts,
         evImageConfig,
         evSchema,
         evStoreSchema,

         evErrors,
         evMenu,
         evParEditRequest,
         evParStore,
         evInitTables,
         evUpdateTimeRanges,
         evPellets,
         evPelletsAdd,

         evCount
      };

      enum ClientType
      {
         ctWithLogin = 0,
         ctActive
      };

      enum UserRights
      {
         urView        = 0x01,
         urControl     = 0x02,
         urFullControl = 0x04,
         urSettings    = 0x08,
         urAdmin       = 0x10
      };

      static const char* toName(Event event);
      static Event toEvent(const char* name);

      static const char* events[];
};
