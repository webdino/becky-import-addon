/* vim: set ts=2 et sw=2 tw=80: */
/*
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org Code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <mozilla-config.h>
#include <nsServiceManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsIProfileMigrator.h>
#include <nsIImportGeneric.h>
#include <nsIImportSettings.h>
#include <nsIImportFilters.h>
#include <nsComponentManagerUtils.h>
#include <nsIMsgAccount.h>
#include <nsISupportsPrimitives.h>

#include "BeckyProfileMigrator.h"
#include "BeckyUtils.h"
#include "BeckyMailImporter.h"
#include "BeckyAddressBooksImporter.h"
#include "BeckySettingsImporter.h"
#include "BeckyFiltersImporter.h"

NS_IMPL_ISUPPORTS2(BeckyProfileMigrator, nsIMailProfileMigrator, nsITimerCallback)

BeckyProfileMigrator::BeckyProfileMigrator()
{
  mProcessingMailFolders = PR_FALSE;
  mObserverService = do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  mImportModule = do_CreateInstance("@mozilla-japan.org/import/becky;1");
}

BeckyProfileMigrator::~BeckyProfileMigrator()
{
  /* destructor code */
}

nsresult
BeckyProfileMigrator::ContinueImport()
{
  return Notify(nsnull);
}

NS_IMETHODIMP
BeckyProfileMigrator::Notify(nsITimer *aTimer)
{
  PRInt32 progress;
  mGenericImporter->GetProgress(&progress);

  nsAutoString index;
  index.AppendInt(progress);
  mObserverService->NotifyObservers(nsnull, "Migration:Progress", index.get());

  if (progress == 100) {
    if (mProcessingMailFolders)
      return FinishCopyingMailFolders();
    else
      return FinishCopyingAddressBookData();
  } else {
    // fire a timer to handle the next one.
    mFileIOTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mFileIOTimer)
      mFileIOTimer->InitWithCallback(static_cast<nsITimerCallback *>(this), 100, nsITimer::TYPE_ONE_SHOT);
  }
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// nsIMailProfileMigrator

NS_IMETHODIMP
BeckyProfileMigrator::Migrate(PRUint16 aItems,
                              nsIProfileStartup *aStartup,
                              const PRUnichar *aProfile)
{
  nsresult rv;

  if (aStartup) {
    rv = aStartup->DoStartup();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mObserverService->NotifyObservers(nsnull, "Migration:Started", nsnull);
  rv = ImportSettings();

  return ImportAddressBook();
}

NS_IMETHODIMP
BeckyProfileMigrator::GetMigrateData(const PRUnichar *aProfile,
                                     PRBool aDoingStartup,
                                     PRUint16 *_retval NS_OUTPARAM)
{
  *_retval = nsIMailProfileMigrator::ACCOUNT_SETTINGS |
             nsIMailProfileMigrator::ADDRESSBOOK_DATA |
             nsIMailProfileMigrator::MAILDATA |
             nsIMailProfileMigrator::FILTERS;

  return NS_OK;
}

NS_IMETHODIMP
BeckyProfileMigrator::GetSourceExists(PRBool *aSourceExists)
{
  *aSourceExists = PR_FALSE;

  nsCOMPtr<nsIImportSettings> importSettings;
  mImportModule->GetImportInterface(NS_IMPORT_SETTINGS_STR, getter_AddRefs(importSettings));

  if (importSettings) {
    nsString description;
    nsCOMPtr<nsIFile> location;
    importSettings->AutoLocate(getter_Copies(description),
                               getter_AddRefs(location), aSourceExists);
  }

  return NS_OK;
}

NS_IMETHODIMP
BeckyProfileMigrator::GetSourceHasMultipleProfiles(PRBool *aSourceHasMultipleProfiles)
{
  *aSourceHasMultipleProfiles = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
BeckyProfileMigrator::GetSourceProfiles(nsIArray * *aSourceProfiles)
{
  *aSourceProfiles = nsnull;
  return NS_OK;
}

nsresult
BeckyProfileMigrator::ImportSettings()
{
  nsAutoString index;
  index.AppendInt(nsIMailProfileMigrator::ACCOUNT_SETTINGS);
  mObserverService->NotifyObservers(nsnull,
                                    "Migration:ItemBeforeMigrate",
                                    index.get());

  nsresult rv;
  nsCOMPtr<nsIImportSettings> importer;
  rv = mImportModule->GetImportInterface(NS_IMPORT_SETTINGS_STR,
                                         getter_AddRefs(importer));
  PRBool imported;
  nsCOMPtr<nsIMsgAccount> account;
  rv = importer->Import(getter_AddRefs(account), &imported);

  mObserverService->NotifyObservers(nsnull,
                                    "Migration:ItemAfterMigrate",
                                    index.get());
  return rv;
}

nsresult
BeckyProfileMigrator::ImportAddressBook()
{
  nsAutoString index;
  index.AppendInt(nsIMailProfileMigrator::ADDRESSBOOK_DATA);
  mObserverService->NotifyObservers(nsnull,
                                    "Migration:ItemBeforeMigrate",
                                    index.get());

  nsresult rv;
  rv = mImportModule->GetImportInterface(NS_IMPORT_ADDRESS_STR,
                                         getter_AddRefs(mGenericImporter));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsCString> pabString = do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // we want to migrate the outlook express addressbook into our personal address book
  pabString->SetData(nsDependentCString("moz-abmdbdirectory://abook.mab"));
  mGenericImporter->SetData("addressDestination", pabString);

  PRBool imported;
  PRBool wantsProgress;
  mGenericImporter->WantsProgress(&wantsProgress);
#ifdef MOZ_IMPORTGENERIC_NEED_ISADDRLOCHOME
  rv = mGenericImporter->BeginImport(nsnull, nsnull, PR_TRUE, &imported);
#else
  rv = mGenericImporter->BeginImport(nsnull, nsnull, &imported);
#endif

  if (wantsProgress)
    ContinueImport();
  else
    FinishCopyingAddressBookData();

  return NS_OK;
}

nsresult
BeckyProfileMigrator::ImportMailData()
{
  nsAutoString index;
  index.AppendInt(nsIMailProfileMigrator::MAILDATA);
  mObserverService->NotifyObservers(nsnull,
                                    "Migration:ItemBeforeMigrate",
                                    index.get());

  nsresult rv;
  rv = mImportModule->GetImportInterface(NS_IMPORT_MAIL_STR,
                                         getter_AddRefs(mGenericImporter));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsPRBool> migrating = do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // by setting the migration flag, we force the import utility to install local folders from OE
  // directly into Local Folders and not as a subfolder
  migrating->SetData(PR_TRUE);
  mGenericImporter->SetData("migration", migrating);

  PRBool importResult;
  PRBool wantsProgress;
  mGenericImporter->WantsProgress(&wantsProgress);
#ifdef MOZ_IMPORTGENERIC_NEED_ISADDRLOCHOME
  rv = mGenericImporter->BeginImport(nsnull, nsnull, PR_TRUE, &importResult);
#else
  rv = mGenericImporter->BeginImport(nsnull, nsnull, &importResult);
#endif

  mProcessingMailFolders = PR_TRUE;

  if (wantsProgress)
    ContinueImport();
  else
    FinishCopyingMailFolders();

  return NS_OK;
}

nsresult
BeckyProfileMigrator::ImportFilters()
{
  nsAutoString index;
  index.AppendInt(nsIMailProfileMigrator::MAILDATA);
  mObserverService->NotifyObservers(nsnull,
                                    "Migration:ItemBeforeMigrate",
                                    index.get());

  nsresult rv;
  nsCOMPtr<nsIImportFilters> importer;
  rv = mImportModule->GetImportInterface(NS_IMPORT_FILTERS_STR,
                                         getter_AddRefs(importer));
  if (NS_SUCCEEDED(rv)) {
    PRBool imported;
    PRUnichar *error;
    rv = importer->Import(&error, &imported);
    mObserverService->NotifyObservers(nsnull,
                                      "Migration:ItemAfterMigrate",
                                      index.get());
  }

  mObserverService->NotifyObservers(nsnull, "Migration:Ended", nsnull);
  return NS_OK;
}

nsresult
BeckyProfileMigrator::FinishCopyingAddressBookData()
{
  nsAutoString index;
  index.AppendInt(nsIMailProfileMigrator::ADDRESSBOOK_DATA);
  mObserverService->NotifyObservers(nsnull,
                                    "Migration:ItemAfterMigrate",
                                    index.get());

  // now kick off the mail migration code
  ImportMailData();

  return NS_OK;
}

nsresult
BeckyProfileMigrator::FinishCopyingMailFolders()
{
  nsAutoString index;
  index.AppendInt(nsIMailProfileMigrator::MAILDATA);
  mObserverService->NotifyObservers(nsnull,
                                    "Migration:ItemAfterMigrate",
                                    index.get());

  // now kick off the filters migration code
  return ImportFilters();
}

