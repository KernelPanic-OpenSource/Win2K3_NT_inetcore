//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  MAPICALL.C - Functions to call MAPI for internet mail profile configuration
//      
//

//  HISTORY:
//  
//  1/25/95    jeremys    Created.
//  96/03/09  markdu    Added a wait cursor during loading of MAPI
//

#include "wizard.h"

/**********************************************************************

  Terminology:

  profile - a collection of settings for Exchange that determine
    the services that are used and the address book and message store
  service - a MAPI plug-in that talks to a mail back end
    (or address book or message store)

  There can be a number of profiles installed on a particular machine.
  Each profile contains a set of services.

  Stategy:

  To configure Microsoft Exchange, we need to do the following:

  1) Establish a profile to modify
    - If any profiles currently exist, find the default profile.
    Otherwise create a profile, which will be initially empty.

  2) Populate the profile with the required services.
    - The profile needs to contain the Internet Mail service, an
    address book and a message store.  If any of these items are
    not present, we add them to the profile.

  3) Configure the internet mail service for this profile.
    - stick in the user's email address, email server, etc.

**********************************************************************/

// instance handle must be in per-instance data segment
#pragma data_seg(DATASEG_PERINSTANCE)
HINSTANCE hInstMAPIDll=NULL;  // handle to MAPI dll we load explicitly
DWORD dwMAPIRefCount = 0;

// global function pointers for MAPI apis
LPMAPIINITIALIZE     lpMAPIInitialize     = NULL;
LPMAPIADMINPROFILES   lpMAPIAdminProfiles   = NULL;
LPMAPIUNINITIALIZE     lpMAPIUninitialize     = NULL;
LPMAPIALLOCATEBUFFER  lpMAPIAllocateBuffer  = NULL;
LPMAPIFREEBUFFER    lpMAPIFreeBuffer     = NULL;
LPHRQUERYALLROWS    lpHrQueryAllRows    = NULL;

#define NUM_MAPI_PROCS   6
// API table for function addresses to fetch
APIFCN MAPIApiList[NUM_MAPI_PROCS] = {
  { (PVOID *) &lpMAPIInitialize,szMAPIInitialize},
  { (PVOID *) &lpMAPIUninitialize,szMAPIUninitialize},
  { (PVOID *) &lpMAPIAdminProfiles,szMAPIAdminProfiles},
  { (PVOID *) &lpMAPIAllocateBuffer,szMAPIAllocateBuffer},
  { (PVOID *) &lpMAPIFreeBuffer,szMAPIFreeBuffer},
  { (PVOID *) &lpHrQueryAllRows,szHrQueryAllRows}};

#pragma data_seg(DATASEG_DEFAULT)

// function prototypes
HRESULT GetProfileList(LPPROFADMIN lpProfAdmin,LPSRowSet * ppRowSet);
HRESULT GetServicesList(LPSERVICEADMIN lpServiceAdmin, LPSRowSet *ppRowSet);
HRESULT CreateProfileIfNecessary(LPPROFADMIN lpProfAdmin,TCHAR * pszSelProfileName);
HRESULT InstallRequiredServices(LPSERVICEADMIN pServiceAdmin,
  LPSRowSet pServiceRowSet);
VOID FreeSRowSet(LPSRowSet prws);
BOOL ValidateProperty(LPSPropValue pval, ULONG cVal, ULONG ulPropTag);
BOOL DoesFileExist(TCHAR * pszPath,TCHAR * pszFileName);
HRESULT ConfigInternetService(MAILCONFIGINFO * pMailConfigInfo,
  LPSERVICEADMIN lpServiceAdmin);
HRESULT   GetServiceUID(TCHAR * pszName,LPSERVICEADMIN lpServiceAdmin,
  LPMAPIUID *ppMapiUID);
BOOL MakeUniqueFilename(UINT uIDFilename,UINT uIDAltFilename,
  TCHAR * pszFilename,DWORD cbFilename);
extern BOOL GetApiProcAddresses(HMODULE hModDLL,APIFCN * pApiProcList,
  UINT nApiProcs);
HRESULT ConfigNewService(LPSERVICEADMIN lpServiceAdmin,LPMAPIUID lpMapiUID,
  UINT uIDFilename,UINT uIDFilename1,UINT uPropValID);

// enums
enum { ivalDisplayName, ivalServiceName, ivalResourceFlags, ivalServiceDllName,
  ivalServiceEntryName, ivalServiceUID, ivalServiceSupportFiles,
  cvalMsgSrvMax };

/*******************************************************************

  NAME:    InitMAPI

  SYNOPSIS:  Loads the MAPI dll, gets proc addresses and initializes
        MAPI

  EXIT:    TRUE if successful, or FALSE if fails.  Displays its
        own error message upon failure.

  NOTES:    We load MAPI explicitly because the DLL may not be installed
        when we begin the wizard... we may have to install it and
        then load it.
        

********************************************************************/
BOOL InitMAPI(HWND hWnd)
{
  TCHAR szMAPIDll[SMALL_BUF_LEN+1];
  HRESULT hr;

  // load MAPI only if not already loaded... otherwise just increment
  // reference count

  if (!hInstMAPIDll) {

    // set an hourglass cursor
    WAITCURSOR WaitCursor;

    // get the filename (MAPI32.DLL) out of resource
    LoadSz(IDS_MAPIDLL_FILENAME,szMAPIDll,ARRAYSIZE(szMAPIDll));
    // load the MAPI dll
    DEBUGMSG("Loading MAPI DLL");
    hInstMAPIDll = LoadLibrary(szMAPIDll);
    if (!hInstMAPIDll) {
      UINT uErr = GetLastError();
      DisplayErrorMessage(hWnd,IDS_ERRLoadMAPIDll1,uErr,ERRCLS_STANDARD,
        MB_ICONSTOP,szMAPIDll);
      return FALSE;
    }

    // cycle through the API table and get proc addresses for all the APIs we
    // need
    BOOL fSuccess = GetApiProcAddresses(hInstMAPIDll,MAPIApiList,NUM_MAPI_PROCS);

    if (!fSuccess) {
      MsgBoxParam(hWnd,IDS_ERRLoadMAPIDll2,MB_ICONSTOP,MB_OK,szMAPIDll);
      DeInitMAPI();
      return FALSE;
    }

    // initialize MAPI
    ASSERT(lpMAPIInitialize);
    hr = lpMAPIInitialize(NULL);
    if (HR_FAILED(hr)) {
       MsgBox(hWnd,IDS_ERRInitMAPI,MB_ICONSTOP,MB_OK);
      DeInitMAPI();
      return FALSE;
    }

    gpWizardState->fMAPIActive = TRUE;
  }

  dwMAPIRefCount ++;

   return TRUE;
}

/*******************************************************************

  NAME:    DeInitMAPI

  SYNOPSIS:  Uninitializes MAPI and unloads MAPI dlls

********************************************************************/
VOID DeInitMAPI(VOID)
{
  // decrease reference count
  if (dwMAPIRefCount) {
    dwMAPIRefCount--;
  }

  // shut down and unload MAPI if reference count hits zero
  if (!dwMAPIRefCount) {
    // uninitialize MAPI
    if (gpWizardState->fMAPIActive && lpMAPIUninitialize) {
      lpMAPIUninitialize();
      gpWizardState->fMAPIActive = FALSE;
    }

    // free the MAPI dll
    if (hInstMAPIDll) {
      DEBUGMSG("Unloading MAPI DLL");
      FreeLibrary(hInstMAPIDll);
      hInstMAPIDll = NULL;
    }
  
    // set function pointers to NULL
    for (UINT nIndex = 0;nIndex<NUM_MAPI_PROCS;nIndex++) 
      *MAPIApiList[nIndex].ppFcnPtr = NULL;
  }
}

/*******************************************************************

  NAME:    SetMailProfileInformation

  SYNOPSIS:  Sets up MAPI profile for internet mail and sets
        user information in profile.

  ENTRY:    pMailConfigInfo - pointer to struct with configuration info

  EXIT:    returns an HRESULT
  
  NOTES:    See strategy statement above
  
********************************************************************/
HRESULT SetMailProfileInformation(MAILCONFIGINFO * pMailConfigInfo)
{
  HRESULT hr;
  LPPROFADMIN   pProfAdmin=NULL;  // interface to administer profiles
  LPSERVICEADMIN  pServiceAdmin=NULL; // interface to administer services
  LPSRowSet    pServiceRowSet=NULL;
  TCHAR      szSelProfileName[cchProfileNameMax+1]=TEXT("");

  ASSERTSZ(gpWizardState->fMAPIActive,"MAPI not initialized!");
  ASSERT(pMailConfigInfo);

  // get a pointer to the interface to administer profiles
  ASSERT(lpMAPIAdminProfiles);
  hr = lpMAPIAdminProfiles(0,&pProfAdmin);
  if (HR_FAILED(hr)) {
    DEBUGMSG("MAPIAdminProfiles returned 0x%lx",hr);
    return (hr);
  }
  ASSERT(pProfAdmin);
  // release this interface when we leave the function
  RELEASE_ME_LATER ReleaseProfAdminLater(pProfAdmin);

  // get profile name from passed-in struct, if specified
  if (pMailConfigInfo->pszProfileName && lstrlen(pMailConfigInfo->pszProfileName)) {
    lstrcpy(szSelProfileName,pMailConfigInfo->pszProfileName);
  } else {
    // no profile specified, use default name
    LoadSz(IDS_DEFAULT_PROFILE_NAME,szSelProfileName,ARRAYSIZE(szSelProfileName));
  }

  // create profile if we need to
  hr = CreateProfileIfNecessary(pProfAdmin,szSelProfileName);
  if (HR_FAILED(hr))
    return hr;

  // set this profile as default if appropriate
  if (pMailConfigInfo->fSetProfileAsDefault) {
    hr = pProfAdmin->SetDefaultProfile(szSelProfileName,0);
    if (HR_FAILED(hr))
      return hr;
  }

  ASSERT(lstrlen(szSelProfileName));  // should have profile name at this point
  DEBUGMSG("Modifying MAPI profile: %s",szSelProfileName);

  // get a pointer to the interface to administer services for this profile
  hr = pProfAdmin->AdminServices(szSelProfileName,NULL,NULL,0,
    &pServiceAdmin);

  if (HR_FAILED(hr))
    return hr;
  ASSERT(pServiceAdmin);
  // release pServiceAdmin interface when done
  RELEASE_ME_LATER rlServiceAdmin(pServiceAdmin);  

  // get a list of services for this profile
  hr = GetServicesList(pServiceAdmin,&pServiceRowSet);
  if (HR_FAILED(hr))
    return hr;
  ASSERT(pServiceRowSet);

  // install any services we need which aren't already present in the profile
  hr = InstallRequiredServices(pServiceAdmin,pServiceRowSet);
    // done with profile row set, free the table
  FreeSRowSet(pServiceRowSet);
  pServiceRowSet = NULL;
  if (HR_FAILED(hr))
    return hr;

  // configure the internet mail service with the passed in email name,
  // server, etc.
  hr = ConfigInternetService(pMailConfigInfo,pServiceAdmin);
  if (HR_FAILED(hr)) {
    DEBUGMSG("ConfigInternetService returned 0x%x" , hr);
    return hr;
  }

  return hr;
}

/*******************************************************************

  NAME:    GetProfileList

  SYNOPSIS:  retrieves a list of MAPI profiles 

  ENTRY:    lpProfAdmin - pointer to profile admin interface
        ppRowSet - pointer to an SRowSet pointer that is filled in

  EXIT:    returns an HRESULT.  If successful, *ppRowSet contains
        pointer to SRowSet with profile list.

  NOTES:    Cloned from MAPI profile control panel code.
        Caller MUST call MAPIFreeBuffer to free *ppRowSet when done.
          
********************************************************************/
HRESULT GetProfileList(LPPROFADMIN lpProfAdmin,LPSRowSet * ppRowSet)
{
  HRESULT hr;
  LPMAPITABLE   pMapiTable=NULL;
  SPropTagArray   TagArray= {3,{PR_DISPLAY_NAME,
            PR_COMMENT,PR_DEFAULT_PROFILE}};

  ASSERT(lpProfAdmin);
  ASSERT(ppRowSet);

  // call the lpProfAdmin interface to get the MAPI profile table
  hr = lpProfAdmin->GetProfileTable(0,&pMapiTable);
  if (HR_FAILED(hr))
    return hr;
  ASSERT(pMapiTable);
  // release this interface when we leave the function
  RELEASE_ME_LATER ReleaseMapiTableLater(pMapiTable);

  // set properties of table columns
  hr = pMapiTable->SetColumns(&TagArray,0);
  if (!HR_FAILED(hr)) {
    // get row set information from table
    hr = pMapiTable->QueryRows(4000,0,ppRowSet);
  }

  return hr;
}

/*******************************************************************

  NAME:    GetServicesList

  SYNOPSIS:  retrieves a list of MAPI services in a profile

  ENTRY:    lpProfAdmin - pointer to service admin interface
        ppRowSet - pointer to an SRowSet pointer that is filled in

  EXIT:    returns an HRESULT.  If successful, *ppRowSet contains
        pointer to SRowSet with service list.

  NOTES:    Cloned from MAPI profile control panel code.
        Caller MUST call MAPIFreeBuffer to free *ppRowSet when done.
          
********************************************************************/
HRESULT GetServicesList(LPSERVICEADMIN lpServiceAdmin, LPSRowSet *ppRowSet)
{
  HRESULT      hr;
  ULONG      iRow;
  LPMAPITABLE    pMapiTable   = NULL;
  SCODE      sc    = S_OK;
  static SPropTagArray taga  = {7, { PR_DISPLAY_NAME,
                    PR_SERVICE_NAME,
                    PR_RESOURCE_FLAGS,
                    PR_SERVICE_DLL_NAME,
                    PR_SERVICE_ENTRY_NAME,
                    PR_SERVICE_UID,
                    PR_SERVICE_SUPPORT_FILES }};

  *ppRowSet = NULL;

  hr = lpServiceAdmin->GetMsgServiceTable(0, &pMapiTable);
  if (HR_FAILED(hr))
    return hr;
  // free this interface when function exits
  RELEASE_ME_LATER rlTable(pMapiTable);


  hr = pMapiTable->SetColumns(&taga, 0);
  if (!HR_FAILED(hr)) {
    // BUGBUG get rid of 'magic number' (appears in MAPI
    // ctrl panel code, need to find out what it is)  jeremys 1/30/95
    hr = pMapiTable->QueryRows(4000,0,ppRowSet);
  }
  if (HR_FAILED(hr))
    return hr;

  for(iRow = 0; iRow < (*ppRowSet)->cRows; iRow++)
  {
    // make sure properties are valid, if not then slam something in
    ValidateProperty((*ppRowSet)->aRow[iRow].lpProps, 0, PR_DISPLAY_NAME);
    ValidateProperty((*ppRowSet)->aRow[iRow].lpProps, 1, PR_SERVICE_NAME);
  }

  return hr;
}

/*******************************************************************

  NAME:    CreateProfileIfNecessary

  SYNOPSIS:  Creates profile if it doesn't already exist

  ENTRY:    lpProfAdmin - pointer to profile admin interface
        pszSelProfileName - name of profile to create

  EXIT:    returns an HRESULT. 

********************************************************************/
HRESULT CreateProfileIfNecessary(LPPROFADMIN pProfAdmin,TCHAR * pszSelProfileName)
{
  HRESULT hr = hrSuccess;
  LPTSTR lpProfileName=NULL;
  BOOL  fDefault;

  ASSERT(pProfAdmin);
  ASSERT(pszSelProfileName);

  ENUM_MAPI_PROFILE EnumMAPIProfile;

  // walk through the profile names, see if we have a match
  while (EnumMAPIProfile.Next(&lpProfileName,&fDefault)) {
    ASSERT(lpProfileName);

    if (!lstrcmpi(lpProfileName,pszSelProfileName)) {
      return hrSuccess;  // found a match, nothing to do
    }
  }

  // no match, need to create profile
  DEBUGMSG("Creating MAPI profile: %s",pszSelProfileName);
  // call MAPI to create the profile
  hr = pProfAdmin->CreateProfile(pszSelProfileName,
    NULL, (ULONG) 0, (ULONG) 0);

  return hr;
}


/*******************************************************************

  NAME:    InstallRequiredServices

  SYNOPSIS:  Installs the 3 services we need (message store,
        address book, internet mail) in the profile
        if they're not already present.  Calls functions to configure
        message store and address book (they both need a filename
        to use) if we're adding them.

  ENTRY:    lpServiceAdmin - pointer to service admin interface
        pServiceRowSet - MAPI table with list of installed services
        
  EXIT:    returns an HRESULT. 

  NOTES:    We deliberately don't configure internet mail service here--
        we do that in the main routine.  The reason is that we
        need to configure internet mail whether it's already installed
        or not, address book and message store we only need to configure
        if they're brand new.

********************************************************************/
HRESULT InstallRequiredServices(LPSERVICEADMIN pServiceAdmin,
  LPSRowSet pServiceRowSet)
{
  ULONG       iRow,iService;
  TCHAR       szServiceName[SMALL_BUF_LEN+1];
  LPMAPIUID     pMapiUID=NULL;
  HRESULT      hr=hrSuccess;

  // table for MAPI services we need to make sure are installed in profile
  MSGSERVICE MAPIServiceList[NUM_SERVICES] = {
    { FALSE, IDS_INTERNETMAIL_SERVICENAME, IDS_INTERNETMAIL_DESCRIPTION,FALSE, 0,0,0},
    { FALSE, IDS_MESSAGESTORE_SERVICENAME, IDS_MESSAGESTORE_DESCRIPTION,TRUE,
      IDS_MESSAGESTORE_FILENAME,IDS_MESSAGESTORE_FILENAME1,PR_PST_PATH},
    { FALSE, IDS_ADDRESSBOOK_SERVICENAME, IDS_ADDRESSBOOK_DESCRIPTION,TRUE,
      IDS_ADDRESSBOOK_FILENAME,IDS_ADDRESSBOOK_FILENAME1,PR_PAB_PATH}};

  // walk through the list of services
  for (iRow = 0;iRow < pServiceRowSet->cRows;iRow ++) {
    DEBUGMSG("Profile contains service: %s (%s)",
      pServiceRowSet->aRow[iRow].lpProps[ivalDisplayName].Value.LPSZ,
      pServiceRowSet->aRow[iRow].lpProps[ivalServiceName].Value.LPSZ);

    // for each service, walk through our array of required services,
    // and see if there's a match
    for (iService = 0;iService < NUM_SERVICES;iService ++) {
      // load the name for this service out of resource
      LoadSz(MAPIServiceList[iService].uIDServiceName,
        szServiceName,ARRAYSIZE(szServiceName));

      // compare it to the service name in the table of
      // installed services for this profile
      if (!lstrcmpi(szServiceName,
        pServiceRowSet->aRow[iRow].lpProps[ivalServiceName].Value.LPSZ)) {
         // this is a match!
        MAPIServiceList[iService].fPresent = TRUE;
        break;  // break the inner 'for' loop
      }
    }
  }


  // install any services we need which are not already present
  for (iService = 0;iService < NUM_SERVICES;iService ++) {

    if (!MAPIServiceList[iService].fPresent) {
      TCHAR szServiceDesc[MAX_RES_LEN+1];
      MSGSERVICE * pMsgService = &MAPIServiceList[iService];        

      // load the service name and description
      LoadSz(pMsgService->uIDServiceName,
        szServiceName,ARRAYSIZE(szServiceName));
      LoadSz(pMsgService->uIDServiceDescription,
        szServiceDesc,ARRAYSIZE(szServiceDesc));
      DEBUGMSG("Adding service: %s (%s)",
        szServiceDesc,szServiceName);

      // create the service
      hr = pServiceAdmin->CreateMsgService(szServiceName,
        szServiceDesc,0,0);
      if (HR_FAILED(hr))
        return hr;

      // call a creation-time config procedure if specified
      if (pMsgService->fNeedConfig) {

        // get the UID (identifier) for this service
        // based on service name, APIs downstream need this
        hr = GetServiceUID(szServiceName,pServiceAdmin,
          &pMapiUID);
        if (HR_FAILED(hr))
          return hr;
        ASSERT(pMapiUID);

        // call the proc to configure newly-created service
        hr = ConfigNewService(pServiceAdmin,pMapiUID,
          pMsgService->uIDStoreFilename,pMsgService->uIDStoreFilename1,
          pMsgService->uPropID);

        // free the buffer with the UID
        ASSERT(lpMAPIFreeBuffer);
        lpMAPIFreeBuffer(pMapiUID);
        pMapiUID = NULL;
      }
    }
  }

  return hr;
}

#define NUM_MAIL_PROPS   11
/*******************************************************************

  NAME:    ConfigInternetService

  SYNOPSIS:  Configures the Internet Mail service (route 66) with
        user's email name, email server, etc.

  ENTRY:    pMailConfigInfo - pointer to struct with configuration info
        pServiceAdmin - pointer to service admin interface

  EXIT:    returns an HRESULT

  NOTES:    will stomp any existing settings for properties that it
        sets.

********************************************************************/
HRESULT ConfigInternetService(MAILCONFIGINFO * pMailConfigInfo,
  LPSERVICEADMIN pServiceAdmin)
{
  HRESULT     hr;
  SPropValue  PropValue[NUM_MAIL_PROPS];
  TCHAR       szServiceName[SMALL_BUF_LEN+1];
  LPMAPIUID   pMapiUID=NULL;
  UINT        nProps = NUM_MAIL_PROPS;

  ASSERT(pMailConfigInfo);
  ASSERT(pServiceAdmin);

  // get service UID for internet mail service
  LoadSz(IDS_INTERNETMAIL_SERVICENAME,szServiceName,ARRAYSIZE(szServiceName));
  hr = GetServiceUID(szServiceName,pServiceAdmin,&pMapiUID);
  if (HR_FAILED(hr)) {
    return hr;
  }
  ASSERT(pMapiUID);


  // set the property value for each property.  Note that the order
  // of items in the array doesn't mattter. The ulPropTag member indicates
  // what property the PropValue item is for, and the lpszA, b or l member
  // contains the data for that property.

  // need to "encrypt" mail account password with xor bit mask.  Mail client
  // expects it to be thusly "encrypted" when it reads it out.  It's stored
  // in the registry in this securely "encrypted" format.  Somebody pretty
  // smart must have thought of this.

  // configure mail service properties
  PropValue[0].ulPropTag  = PR_CFG_EMAIL_ADDRESS;
  PropValue[0].Value.LPSZ = pMailConfigInfo->pszEmailAddress;
  PropValue[1].ulPropTag  = PR_CFG_EMAIL_DISPLAY_NAME;
  PropValue[1].Value.LPSZ = pMailConfigInfo->pszEmailDisplayName;
  PropValue[2].ulPropTag  = PR_CFG_SERVER_PATH;
  PropValue[2].Value.LPSZ = pMailConfigInfo->pszEmailServer;
  PropValue[3].ulPropTag  = PR_CFG_EMAIL_ACCOUNT;
  PropValue[3].Value.LPSZ = pMailConfigInfo->pszEmailAccountName;
  PropValue[4].ulPropTag  = PR_CFG_PASSWORD;
  PropValue[4].Value.LPSZ = (LPTSTR) szNull;
  PropValue[5].ulPropTag  = PR_CFG_REMEMBER;
  PropValue[5].Value.b    = (USHORT) TRUE;
  // configure for RNA or LAN as appropriate
  PropValue[6].ulPropTag  = PR_CFG_RNA_PROFILE;
  PropValue[7].ulPropTag  = PR_CFG_CONN_TYPE;
  PropValue[8].ulPropTag  = PR_CFG_DELIVERY_OPTIONS;
  if (pMailConfigInfo->pszConnectoidName &&
    lstrlen(pMailConfigInfo->pszConnectoidName)) {
    PropValue[6].Value.LPSZ = pMailConfigInfo->pszConnectoidName;
    PropValue[7].Value.l = (long) CONNECT_TYPE_REMOTE;
    // set transfer mode for "selective"..
    PropValue[8].Value.l = DOWNLOAD_OPTION_HEADERS;
  } else {
    PropValue[6].Value.LPSZ = (LPTSTR) szNull;
    PropValue[7].Value.l = (long) CONNECT_TYPE_LAN;
    // set automatic transfer mode... mail guys made up the weird
    // define name, not me!
    PropValue[8].Value.l = DOWNLOAD_OPTION_MAIL_DELETE;
  }
  PropValue[9].ulPropTag   = PR_CFG_REMOTE_USERNAME;
  PropValue[9].Value.LPSZ  = pMailConfigInfo->pszEmailAccountName;
  PropValue[10].ulPropTag  = PR_CFG_REMOTE_PASSWORD;
  PropValue[10].Value.LPSZ = pMailConfigInfo->pszEmailAccountPwd;

  // call the service admin interface to configure the service with these
  // properties
  hr = pServiceAdmin->ConfigureMsgService(pMapiUID,NULL,0,
    nProps,PropValue);
  if (HR_FAILED(hr)) {
    DEBUGMSG("ConfigureMsgService returned 0x%x", hr);
  }

  // free the buffer with the UID
  ASSERT(lpMAPIFreeBuffer);
  lpMAPIFreeBuffer(pMapiUID);
  pMapiUID = NULL;

  return hr;
}

/*******************************************************************

  NAME:    ConfigMessageStore

  SYNOPSIS:  Generates a unique filename and sets it as the
        message store

  ENTRY:    lpServiceAdmin - pointer to service admin interface
        lpMapiUID - UID for this service (message store)

  EXIT:    returns an HRESULT

  NOTES:    This code expects to be called only when the service is
        newly created.  Calling it on an existing service will
        cause it to stomp existing settings.
  
********************************************************************/
HRESULT ConfigNewService(LPSERVICEADMIN lpServiceAdmin,LPMAPIUID lpMapiUID,
  UINT uIDFilename,UINT uIDFilename1,UINT uPropValID)
{
  TCHAR szMsgStorePath[MAX_PATH+1];
  HRESULT hr=hrSuccess;

  ASSERT(lpServiceAdmin);
  ASSERT(lpMapiUID);

  // build a path for the message store
  if (!MakeUniqueFilename(uIDFilename,uIDFilename1,
    szMsgStorePath,sizeof(szMsgStorePath))) {
     DEBUGTRAP("Unable to create unique filename");
    return MAPI_E_COLLISION;
  }
  DEBUGMSG("Creating MAPI store %s",szMsgStorePath);

  // set this filename for the message store
  SPropValue PropVal;
  PropVal.ulPropTag = uPropValID;
  PropVal.Value.LPSZ = szMsgStorePath;
  hr = lpServiceAdmin->ConfigureMsgService(lpMapiUID,NULL,0,1,&PropVal);
  if (HR_FAILED(hr)) {
    DEBUGMSG("ConfigureMsgService returned 0x%x", hr);
  }

  return hr;
}

/*******************************************************************

  NAME:    FindInternetMailService

  SYNOPSIS:  Detects if internet mail is installed, returns
        email address and email server if it is.
  
********************************************************************/
BOOL FindInternetMailService(TCHAR * pszEmailAddress,DWORD cbEmailAddress,
  TCHAR * pszEmailServer, DWORD cbEmailServer)
{
  ASSERT(pszEmailAddress);
  ASSERT(pszEmailServer);

  if (!hInstMAPIDll && !InitMAPI(NULL))
    return FALSE;

  // look through all profiles.  For each profile, look through all
  // services.  If we find an instance of the internet mail service,
  // then return email address and password to caller.  If there is
  // more than one profile with the internet mail service, we
  // will return the first one we find.

  ENUM_MAPI_PROFILE EnumMAPIProfile;
  LPTSTR lpProfileName,lpServiceName;
  BOOL fDefault;
  // walk through the list of profiles...
  while (EnumMAPIProfile.Next(&lpProfileName,&fDefault)) {
    ASSERT(lpProfileName);

    DEBUGMSG("Found profile: %s",lpProfileName);
    
    // for each profile, walk through the list of services...
    ENUM_MAPI_SERVICE EnumMAPIService(lpProfileName);
    while (EnumMAPIService.Next(&lpServiceName)) {
      TCHAR szSmallBuf[SMALL_BUF_LEN+1];

      DEBUGMSG("Found service: %s",lpServiceName);

      if (!lstrcmpi(lpServiceName,LoadSz(IDS_INTERNETMAIL_SERVICENAME,
        szSmallBuf,ARRAYSIZE(szSmallBuf)))) {

//BUGBUG 21-May-1995 jeremys Get e-mail server & address from MAPI profile

        return TRUE;
      }
    }
  }

  return FALSE;
}               

ENUM_MAPI_PROFILE::ENUM_MAPI_PROFILE(VOID)
{
  LPPROFADMIN   pProfAdmin=NULL;  // interface to administer profiles
  HRESULT hr;

  ASSERTSZ(gpWizardState->fMAPIActive,"MAPI not initialized!");

  _pProfileRowSet = NULL;
  _nEntries = 0;
  _iRow = 0;

  // get a pointer to the interface to administer profiles
  ASSERT(lpMAPIAdminProfiles);
  hr = lpMAPIAdminProfiles(0,&pProfAdmin);
  if (HR_FAILED(hr)) {
    DEBUGMSG("MAPIAdminProfiles returned 0x%lx",hr);
    return;
  }
  ASSERT(pProfAdmin);
  // release this interface when we leave the function
  RELEASE_ME_LATER ReleaseProfAdminLater(pProfAdmin);

  // get the rows in the profile table
  hr = GetProfileList(pProfAdmin,&_pProfileRowSet);
  if (HR_FAILED(hr))
    return;
  ASSERT(_pProfileRowSet);

  _nEntries = _pProfileRowSet->cRows;

}

ENUM_MAPI_PROFILE::~ENUM_MAPI_PROFILE(VOID)
{
  if (_pProfileRowSet) {
    // done with profile row set, free the table
    FreeSRowSet(_pProfileRowSet);
    _pProfileRowSet = NULL;
  }
}

BOOL ENUM_MAPI_PROFILE::Next(LPTSTR * ppProfileName,BOOL * pfDefault)
{
  ASSERT(pfDefault);

  if (!_pProfileRowSet)
    return FALSE;

  if (_iRow < _pProfileRowSet->cRows) {
    LPSPropValue pPropVal = _pProfileRowSet->aRow[_iRow].lpProps;
    ASSERT(pPropVal);

    // get pointer to profile name
    *ppProfileName = pPropVal[0].Value.LPSZ;
    ASSERT(*ppProfileName);
    // set 'this profile is default' flag
    *pfDefault = pPropVal[2].Value.b;

    _iRow++;
    return TRUE;
  }
  
  return FALSE;
}

ENUM_MAPI_SERVICE::ENUM_MAPI_SERVICE(LPTSTR pszProfileName)
{
  LPPROFADMIN   pProfAdmin=NULL;  // interface to administer profiles
  LPSERVICEADMIN  pServiceAdmin=NULL;  // interface to administer services
  HRESULT hr;

  ASSERT(pszProfileName);
  ASSERTSZ(gpWizardState->fMAPIActive,"MAPI not initialized!");

  _pServiceRowSet = NULL;
  _nEntries = 0;
  _iRow = 0;

  // get a pointer to the interface to administer profiles
  ASSERT(lpMAPIAdminProfiles);
  hr = lpMAPIAdminProfiles(0,&pProfAdmin);
  if (HR_FAILED(hr)) {
    DEBUGMSG("MAPIAdminProfiles returned 0x%lx",hr);
    return;
  }
  ASSERT(pProfAdmin);
  // release this interface when we leave the function
  RELEASE_ME_LATER ReleaseProfAdminLater(pProfAdmin);

  // get a pointer to the interface to administer services for this profile
  hr = pProfAdmin->AdminServices(pszProfileName,NULL,NULL,0,
    &pServiceAdmin);
  if (HR_FAILED(hr)) {
    DEBUGMSG("AdminServices returned 0x%lx",hr);
    return;
  }
  // release this interface when we leave the function
  RELEASE_ME_LATER ReleaseServiceAdminLater(pServiceAdmin);

  // get the rows in the profile table
  hr = GetServicesList(pServiceAdmin,&_pServiceRowSet);
  if (HR_FAILED(hr))
    return;
  ASSERT(_pServiceRowSet);

  _nEntries = _pServiceRowSet->cRows;

}

ENUM_MAPI_SERVICE::~ENUM_MAPI_SERVICE(VOID)
{
  if (_pServiceRowSet) {
    // done with profile row set, free the table
    FreeSRowSet(_pServiceRowSet);
    _pServiceRowSet = NULL;
  }
}

BOOL ENUM_MAPI_SERVICE::Next(LPTSTR * ppServiceName)
{
  if (!_pServiceRowSet)
    return FALSE;

  if (_iRow < _pServiceRowSet->cRows) {
    LPSPropValue pPropVal = _pServiceRowSet->aRow[_iRow].lpProps;
    ASSERT(pPropVal);

    // get pointer to profile name
    *ppServiceName = pPropVal[ivalServiceName].Value.LPSZ;
    ASSERT(*ppServiceName);

    _iRow++;
    return TRUE;
  }
  
  return FALSE;
}

/*******************************************************************

  NAME:    MakeUniqueFilename

  SYNOPSIS:  Generates a filename in the Windows directory that
        does not already exist

  ENTRY:    uIDFilename - ID of string resource for desired name
          for the file
        uIDAltFilename - ID of string resource with template
          for filename to use if file with uIDFilename's name
          already exists.  Template should contain %u into
          which numbers will be inserted to make filename unique.
        pszFilename - buffer to return path and filename into
        cbFilename - size of pszFilename buffer

  EXIT:    returns TRUE if successful, FALSE if couldn't make
        unique filename within MAX_FILENAME_TRIES tries
  
********************************************************************/
// number of times we'll try to create a unique filename before giving up
#define MAX_MAKEFILENAME_TRIES  20
BOOL MakeUniqueFilename(UINT uIDFilename,UINT uIDAltFilename,
  TCHAR * pszFilename,DWORD cbFilename)
{
  TCHAR szFileName[SMALL_BUF_LEN+1];
  BOOL fSuccess = FALSE;

  ASSERT(pszFilename);

  // build a path for the filename
  UINT uRet=GetWindowsDirectory(pszFilename,cbFilename);
  ASSERTSZ(uRet,"GetWindowsDirectory failed");

  // choose a file name that doesn't already exist

  // first, try using the string resource specified by uIDFilename
  LoadSz(uIDFilename,szFileName,ARRAYSIZE(szFileName));
  if (DoesFileExist(pszFilename,szFileName)) {

    // if that file exists, then use the string resource uIDAltFilename
    // which has a replacable parameter.  We'll try adding numbers to
    // the filename to make it unique.
    
    TCHAR szFileFmt[SMALL_BUF_LEN+1];
    LoadSz(uIDAltFilename,szFileFmt,ARRAYSIZE(szFileFmt));

    for (UINT nIndex = 0; nIndex < MAX_MAKEFILENAME_TRIES; nIndex ++) {
      // make a name e.g. "mailbox4.pst"
      wsprintf(szFileName,szFileFmt,nIndex);
      if (!DoesFileExist(pszFilename,szFileName)) {
        // OK, found a filename that doesn't exist
        fSuccess = TRUE;
        break;
      }
    }
  } else {
    // first try succeeded  
    fSuccess = TRUE;
  }

  if (fSuccess) {
    // now we have unique filename, build the full path

    lstrcat(pszFilename,szSlash);
    lstrcat(pszFilename,szFileName);
  }

  return fSuccess;
}

/*******************************************************************

  NAME:    DoesFileExist

  SYNOPSIS:  Checks to see whether the specified file exists

  ENTRY:    pszPath - path to directory
        pszFilename - name of file

  EXIT:    returns TRUE if file exists, FALSE if it doesn't
  
********************************************************************/
BOOL DoesFileExist(TCHAR * pszPath,TCHAR * pszFileName)
{
  CHAR      szPath[MAX_PATH+1];
  OFSTRUCT  of;

  ASSERT(pszPath);
  ASSERT(pszFileName);

  // concatenate the path and file name
#ifdef UNICODE
  TCHAR     szTmp[MAX_PATH+1];

  lstrcpy(szTmp,pszPath);
  lstrcat(szTmp,szSlash);
  lstrcat(szTmp,pszFileName);
  
  wcstombs(szPath, szTmp, MAX_PATH+1);
#else
  lstrcpy(szPath,pszPath);
  lstrcat(szPath,szSlash);
  lstrcat(szPath,pszFileName);
#endif

  // find out if this file exists
  return (OpenFile(szPath,&of,OF_EXIST) != HFILE_ERROR);
}

/*******************************************************************

  NAME:    GetServiceUID

  SYNOPSIS:  Given a MAPI service name, gets the MAPIUID associated
        with it.

  ENTRY:    pszServiceName - name of MAPI service (e.g. "IMAIL","MSPST AB")
        lpServiceAdmin - pointer to service admin interface
        ppMapiUID - pointer to pointer for MAPIUID struct
  
  EXIT:    returns an HRESULT

  NOTES:    Cloned from MAPI profile wizard code, if you think this
        function is big and ugly now you should have seen it before
        I cleaned it up.

        This function allocates a MAPIUID, the caller is responsible
        for freeing this (use MAPIFreeBuffer) when done.
  
********************************************************************/
HRESULT   GetServiceUID(TCHAR * pszServiceName,LPSERVICEADMIN lpServiceAdmin,
  LPMAPIUID *ppMapiUID)
{
  HRESULT      hr =hrSuccess;
  LPSPropValue  pTblProp =NULL;
  DWORD         iRow,iColumn;
  LPMAPITABLE     pTable =NULL;
  LPSRowSet    pRowSet =NULL;
  LPSRow      pRow =NULL;
  int        nFound =0;
  LPMAPIUID    pMapiUID =NULL;
  BOOL      fContinue = TRUE;
  SizedSPropTagArray(2, Tbltaga) = { 2, { PR_SERVICE_NAME,
                      PR_SERVICE_UID }};

  ASSERT(pszServiceName);
  ASSERT(lpServiceAdmin);
  ASSERT(ppMapiUID);

  // get table of message services
  hr = lpServiceAdmin->GetMsgServiceTable(0, &pTable);
  if (HR_FAILED(hr))
  {
    DEBUGMSG("GetMsgServiceTable returned 0x%x", hr);
    return hr;
  }
  ASSERT(pTable);
  // release this table when we exit this function
  RELEASE_ME_LATER rlTable(pTable);

  ASSERT(lpHrQueryAllRows);
  hr = lpHrQueryAllRows(pTable, (LPSPropTagArray) &Tbltaga, NULL, NULL, 0, &pRowSet);
  if (HR_FAILED(hr))
  {
    DEBUGMSG("HrQueryAllRows returned 0x%x", hr);
    return hr;
  }
  ASSERT(pRowSet);

  iRow =0;
  while (fContinue && iRow< pRowSet->cRows)
  {
    pRow = &pRowSet->aRow[iRow];
    pTblProp = pRow->lpProps;
    nFound = 0;
    for (iColumn=0; iColumn<pRow->cValues; iColumn++)
    {   //Check each property
      if (pTblProp->ulPropTag ==PR_SERVICE_UID)
      {
        nFound++;
        ASSERT(lpMAPIAllocateBuffer);
        lpMAPIAllocateBuffer(pTblProp->Value.bin.cb, (LPVOID FAR *) &pMapiUID);
        if (!pMapiUID)
        {
          hr = MAPI_E_NOT_ENOUGH_MEMORY;
          fContinue = FALSE;
          break;
        }
        memcpy(pMapiUID, pTblProp->Value.bin.lpb, (size_t) pTblProp->Value.bin.cb);
        *ppMapiUID = pMapiUID;
      }
      else if ((pTblProp->ulPropTag ==PR_SERVICE_NAME) &&
           !lstrcmpi(pTblProp->Value.LPSZ, pszServiceName))
      {
        nFound++;
      }
      pTblProp++;

      if (nFound == 2) {
        // found it!
        fContinue = FALSE;
        break;
      }
    }
    iRow++;

    if (nFound < 2) {    
    // if one but not both items matched above, then deallocate buffer
      if (pMapiUID) {
        ASSERT(lpMAPIFreeBuffer);
        lpMAPIFreeBuffer(pMapiUID);
        pMapiUID =NULL;
      }
      if (*ppMapiUID)
        *ppMapiUID = NULL;
    }
  }

  if (HR_FAILED(hr) || nFound < 2) {
    // free buffer if we didn't find the UID
    if (pMapiUID) {
      ASSERT(lpMAPIFreeBuffer);
      lpMAPIFreeBuffer(pMapiUID);
    }
    if (*ppMapiUID)
      *ppMapiUID = NULL;
  }

  if (pRowSet)
    FreeSRowSet(pRowSet);

  return hr;
}

/*******************************************************************

  NAME:    FreeSRowSet

  SYNOPSIS:  Frees an SRowSet structure and the rows therein

  ENTRY:    prws - the row set to free

  NOTES:    Cloned from MAPI profile ctrl panel code

********************************************************************/
VOID FreeSRowSet(LPSRowSet prws)
{
  ULONG irw;

  if (!prws)
    return;

  ASSERT(lpMAPIFreeBuffer);

  // Free each row
  for (irw = 0; irw < prws->cRows; irw++)
    lpMAPIFreeBuffer(prws->aRow[irw].lpProps);

  // Free the top level structure
  lpMAPIFreeBuffer(prws);
}

/*
 *  ValidateProperty
 *
 *  Purpose:
 *    Given a string prop, make sure it contains a valid string.
 *
 *  Arguments:
 *    pval
 *    cVal
 *    ulPropTag
 *
 *  Returns:
 *    BOOL
 */
TCHAR szUnk[] = TEXT("???");
BOOL ValidateProperty(LPSPropValue pval, ULONG cVal, ULONG ulPropTag)
{
  if(pval[cVal].ulPropTag != ulPropTag)
  {
    // make sure we're not stomping on good properties.
    ASSERT(PROP_TYPE(pval[cVal].ulPropTag) == PT_ERROR);

    pval[cVal].ulPropTag = ulPropTag;
    pval[cVal].Value.LPSZ = szUnk;

    return TRUE;
  }

  return FALSE;
}

#pragma data_seg(DATASEG_READONLY)
// note: this array depends on errors in rc file being in this order
// starting with S_OK.  Don't rearrange one without rearranging the other.
static SCODE mpIdsScode[] =
{
    S_OK,
    MAPI_E_NO_ACCESS,
    E_NOINTERFACE,
    E_INVALIDARG,
    MAPI_E_CALL_FAILED,
    MAPI_E_NOT_FOUND,
    MAPI_E_NO_SUPPORT,
    MAPI_W_ERRORS_RETURNED,
    MAPI_W_PARTIAL_COMPLETION,
    MAPI_E_BAD_CHARWIDTH,
    MAPI_E_BAD_VALUE,
    MAPI_E_BUSY,
    MAPI_E_COLLISION,
    MAPI_E_COMPUTED,
    MAPI_E_CORRUPT_DATA,
    MAPI_E_CORRUPT_STORE,
    MAPI_E_DISK_ERROR,
    MAPI_E_HAS_FOLDERS,
    MAPI_E_HAS_MESSAGES,
    MAPI_E_INVALID_ENTRYID,
    MAPI_E_INVALID_OBJECT,
    MAPI_E_LOGON_FAILED,
    MAPI_E_NETWORK_ERROR,
    MAPI_E_NON_STANDARD,
    MAPI_E_NOT_ENOUGH_DISK,
    MAPI_E_NOT_ENOUGH_MEMORY,
    MAPI_E_NOT_ENOUGH_RESOURCES,
    MAPI_E_NOT_IN_QUEUE,
    MAPI_E_OBJECT_CHANGED,
    MAPI_E_OBJECT_DELETED,
    MAPI_E_STRING_TOO_LONG,
    MAPI_E_SUBMITTED,
    MAPI_E_TOO_BIG,
    MAPI_E_UNABLE_TO_ABORT,
    MAPI_E_UNCONFIGURED,
    MAPI_E_UNEXPECTED_TYPE,
    MAPI_E_UNKNOWN_FLAGS,
    MAPI_E_USER_CANCEL,
    MAPI_E_VERSION
};
#pragma data_seg()

/*******************************************************************

  NAME:    GetMAPIErrorText

  SYNOPSIS:  Gets text string corresponding to MAPI error code

  ENTRY:    uErr - MAPI error code
        pszErrText - buffer to retrieve error text description
        cbErrText - size of pszErrText buffer

  NOTES:    based on MapScodeSz from MAPI SDK.  There is, alas,
        no function built into MAPI to do this.  Every app
        must include all text strings for every error code,
        and will be broken on any error codes MAPI invents
        after the app ships.  Numbskulls.

********************************************************************/
VOID GetMAPIErrorText(UINT uErr,TCHAR * pszErrText,DWORD cbErrText)
{
  ASSERT(pszErrText);
  
  SCODE scErr = (SCODE) uErr;
  UINT nIndex,nMax;

  // get facility code, facility is bits 17-30 in scode
  DWORD dwFacility = (scErr >> 16) & (0x7FFF);

  // if this is a WIN32 code wrapped in an scode, call GetErrorDescription
  // to get the text, which will get the text out of Windows and do a much
  // better job than this crappy little function will
  if (dwFacility == FACILITY_WIN32) 
    GetErrorDescription(pszErrText,cbErrText,
      (scErr & 0xFFFF),ERRCLS_STANDARD);

    /* Linear search in mpIdsScode for scArg.  When found, index is IDS. */
    nMax = sizeof mpIdsScode / sizeof mpIdsScode[0];
    for (nIndex = 0; nIndex < nMax; nIndex++)
    {
        if (mpIdsScode[nIndex] == scErr)
            break;
    }

  if (nIndex < nMax) {
    // found the code in the table
    LoadSz(IDS_MAPIERROR_BASE + nIndex,pszErrText,cbErrText);
  } else {
    // didn't find the code in the table, make a generic string
    // with the error number
    TCHAR szFmt[SMALL_BUF_LEN+1];
    LoadSz(IDS_GENERIC_MAPI_ERROR,szFmt,ARRAYSIZE(szFmt));
    wsprintf(pszErrText,szFmt,scErr);
  }
}

