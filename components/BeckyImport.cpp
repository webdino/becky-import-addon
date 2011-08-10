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

#include <nscore.h>
#include <nsIServiceManager.h>
#include <nsIImportService.h>
#include <nsIComponentManager.h>
#include <nsIMemory.h>
#include <nsIImportService.h>
#include <nsIImportMail.h>
#include <nsIImportMailboxDescriptor.h>
#include <nsIImportGeneric.h>
#include <nsIImportAddressBooks.h>
#include <nsIImportABDescriptor.h>
#include <nsIImportSettings.h>
#include <nsIImportFilters.h>
#include <nsIImportFieldMap.h>
#include <nsXPCOM.h>
#include <nsISupportsPrimitives.h>
#include <nsIOutputStream.h>
#include <nsIAddrDatabase.h>
#include <nsTextFormatter.h>
#include <nsIStringBundle.h>
#include <nsUnicharUtils.h>
#include <nsIMsgTagService.h>
#include <nsMsgBaseCID.h>
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>

#include "BeckyImport.h"
#include "BeckyMailImporter.h"
#include "BeckyAddressBooksImporter.h"
#include "BeckySettingsImporter.h"
#include "BeckyFiltersImporter.h"
#include "BeckyStringBundle.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

BeckyImport::BeckyImport()
{
}

BeckyImport::~BeckyImport()
{
}

NS_IMPL_ISUPPORTS1(BeckyImport, nsIImportModule)

NS_IMETHODIMP
BeckyImport::GetName(PRUnichar **aName)
{
  NS_ENSURE_ARG_POINTER(*aName);
  *aName = BeckyStringBundle::GetStringByID(BECKYIMPORT_NAME);
  return NS_OK;
}

NS_IMETHODIMP
BeckyImport::GetDescription(PRUnichar **aDescription)
{
  NS_ENSURE_ARG_POINTER(*aDescription);
  *aDescription = BeckyStringBundle::GetStringByID(BECKYIMPORT_DESCRIPTION);
  return NS_OK;
}

NS_IMETHODIMP
BeckyImport::GetSupports(char **aSupports)
{
  NS_ENSURE_ARG_POINTER(*aSupports);
  *aSupports = strdup(kBeckySupportsString);
  return NS_OK;
}

NS_IMETHODIMP
BeckyImport::GetSupportsUpgrade(PRBool *aUpgrade)
{
  NS_ENSURE_ARG_POINTER(aUpgrade);
  *aUpgrade = PR_TRUE;
  return NS_OK;
}

nsresult
BeckyImport::GetMailImportInterface(nsISupports **aInterface)
{
  nsIImportMail *importer = nsnull;
  nsIImportGeneric *generic = nsnull;

  nsresult rv = BeckyMailImporter::Create(&importer);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIImportService> importService(do_GetService(NS_IMPORTSERVICE_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv)) {
      rv = importService->CreateNewGenericMail(&generic);
      if (NS_SUCCEEDED(rv)) {
        generic->SetData("importerInterface", importer);
        nsString name;
        BeckyStringBundle::GetStringByID(BECKYIMPORT_NAME, name);
        nsCOMPtr<nsISupportsString> nameString(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv));
        if (NS_SUCCEEDED(rv)) {
          nameString->SetData(name);
          generic->SetData("name", nameString);
          rv = generic->QueryInterface(kISupportsIID, (void **)aInterface);
        }
      }
    }
  }
  NS_IF_RELEASE(importer);
  NS_IF_RELEASE(generic);
  return rv;
}

nsresult
BeckyImport::GetAddressBookImportInterface(nsISupports **aInterface)
{
  // create the nsIImportMail interface and return it!
  nsIImportAddressBooks *address = nsnull;
  nsIImportGeneric *generic = nsnull;
  nsresult rv = BeckyAddressBooksImporter::Create(&address);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIImportService> importService(do_GetService(NS_IMPORTSERVICE_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv)) {
      rv = importService->CreateNewGenericAddressBooks(&generic);
      if (NS_SUCCEEDED(rv)) {
        generic->SetData("addressInterface", address);
        rv = generic->QueryInterface(kISupportsIID, (void **)aInterface);
      }
    }
  }
  NS_IF_RELEASE(address);
  NS_IF_RELEASE(generic);
  return rv;
}

nsresult
BeckyImport::GetSettingsImportInterface(nsISupports **aInterface)
{
  nsIImportSettings *settings = nsnull;
  nsresult rv = BeckySettingsImporter::Create(&settings);
  if (NS_SUCCEEDED(rv))
    settings->QueryInterface(kISupportsIID, (void **)aInterface);
  NS_IF_RELEASE(settings);
  return rv;
}

nsresult
BeckyImport::GetFiltersImportInterface(nsISupports **aInterface)
{
  nsIImportFilters *filters = nsnull;
  nsresult rv = BeckyFiltersImporter::Create(&filters);
  if (NS_SUCCEEDED(rv))
    filters->QueryInterface(kISupportsIID, (void **)aInterface);
  NS_IF_RELEASE(filters);
  return rv;
}

NS_IMETHODIMP
BeckyImport::GetImportInterface(const char *aImportType, nsISupports **aInterface)
{
  NS_ENSURE_ARG_POINTER(aImportType);
  NS_ENSURE_ARG_POINTER(aInterface);

  *aInterface = nsnull;
  if (!strcmp(aImportType, "mail")) {
    return GetMailImportInterface(aInterface);
  } else if (!strcmp(aImportType, "addressbook")) {
    return GetAddressBookImportInterface(aInterface);
  } else if (!strcmp(aImportType, "settings")) {
    return GetSettingsImportInterface(aInterface);
  } else if (!strcmp(aImportType, "filters")) {
    return GetFiltersImportInterface(aInterface);
  }

  return NS_ERROR_NOT_AVAILABLE;
}
