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
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsILocalFile.h>
#include <nsISimpleEnumerator.h>
#include <nsIDirectoryEnumerator.h>
#include <nsISupportsArray.h>
#include <nsStringGlue.h>
#include <nsAbBaseCID.h>
#include <nsIAbManager.h>
#include <nsIImportService.h>
#include <nsIImportABDescriptor.h>
#include <nsMsgUtils.h>

#include "BeckyAddressBooksImporter.h"
#include "BeckyStringBundle.h"
#include "BeckyVCardAddress.h"

NS_IMPL_ISUPPORTS1(BeckyAddressBooksImporter, nsIImportAddressBooks)

BeckyAddressBooksImporter::BeckyAddressBooksImporter()
: mReadBytes(0)
{
  /* member initializers and constructor code */
}

BeckyAddressBooksImporter::~BeckyAddressBooksImporter()
{
  /* destructor code */
}

NS_IMETHODIMP
BeckyAddressBooksImporter::GetSupportsMultiple(PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
BeckyAddressBooksImporter::GetAutoFind(PRUnichar **aDescription NS_OUTPARAM, PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aDescription);
  NS_ENSURE_ARG_POINTER(_retval);

  nsString description;
  BeckyStringBundle::GetStringByID(BECKYIMPORT_DESCRIPTION, description);

  *aDescription = ToNewUnicode(description);
  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
BeckyAddressBooksImporter::GetNeedsFieldMap(nsIFile *aLocation, PRBool *_retval NS_OUTPARAM)
{
  return NS_OK;
}

static nsresult
FindUserDirectory(nsIFile **aLocation NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> folder = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = folder->InitWithNativePath(NS_LITERAL_CSTRING("C:\\Becky!"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = folder->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDirectoryEnumerator> files;
  files = do_QueryInterface(entries, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more;
  nsCOMPtr<nsIFile> entry;
  while (NS_SUCCEEDED(entries->HasMoreElements(&more)) && more) {
    rv = files->GetNextFile(getter_AddRefs(entry));
    PRBool isDirectory = PR_FALSE;
    rv = entry->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isDirectory)
      return CallQueryInterface(entry, aLocation);
  }

  return NS_ERROR_FILE_NOT_FOUND;
}

static nsresult
FindAddressBookDirectory(nsIFile **aAddressBookDirectory)
{
  nsCOMPtr<nsIFile> userDirectory;
  nsresult rv = FindUserDirectory(getter_AddRefs(userDirectory));
  if (NS_FAILED(rv))
    return rv;

  rv = userDirectory->AppendNative(NS_LITERAL_CSTRING("AddrBook"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = userDirectory->Exists(&exists);
  if (NS_FAILED(rv))
    return rv;
  if (!exists)
    return NS_ERROR_FILE_NOT_FOUND;

  return CallQueryInterface(userDirectory, aAddressBookDirectory);
}

NS_IMETHODIMP
BeckyAddressBooksImporter::GetDefaultLocation(nsIFile **aLocation NS_OUTPARAM,
                                              PRBool *aFound NS_OUTPARAM,
                                              PRBool *aUserVerify NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aFound);
  NS_ENSURE_ARG_POINTER(aLocation);
  NS_ENSURE_ARG_POINTER(aUserVerify);

  *aLocation = nsnull;
  *aFound = PR_FALSE;
  *aUserVerify = PR_TRUE;

  if (NS_SUCCEEDED(FindAddressBookDirectory(aLocation))) {
    *aFound = PR_TRUE;
    *aUserVerify = PR_FALSE;
  }

  return NS_OK;
}

static nsresult
CreateAddressBookDescriptor(nsIImportABDescriptor **aDescriptor)
{
  nsresult rv;
  nsCOMPtr<nsIImportService> importService;

  importService = do_GetService(NS_IMPORTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return importService->CreateNewABDescriptor(aDescriptor);
}

static nsresult
AppendAddressBookDescriptor(nsIFile *aEntry, nsISupportsArray *aCollected)
{
  nsresult rv;
  nsCOMPtr<nsIImportABDescriptor> descriptor;
  rv = CreateAddressBookDescriptor(getter_AddRefs(descriptor));
  if (NS_FAILED(rv))
    return rv;

  PRInt64 size;
  aEntry->GetFileSize(&size);
  descriptor->SetSize(size);
  descriptor->SetAbFile(aEntry);

  nsCOMPtr<nsIFile> parent;
  rv = aEntry->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoString name;
  parent->GetLeafName(name);
  descriptor->SetPreferredName(name);

  nsCOMPtr<nsISupports> interface;
  interface = do_QueryInterface(descriptor, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return aCollected->AppendElement(interface);
}

static nsresult
CollectAddressBooks(nsIFile *aTarget, nsISupportsArray *aCollected)
{
  PRBool isDirectory = PR_FALSE;
  nsresult rv = aTarget->IsDirectory(&isDirectory);
  if (!isDirectory)
    return AppendAddressBookDescriptor(aTarget, aCollected);

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = aTarget->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDirectoryEnumerator> files;
  files = do_QueryInterface(entries);
  if (files) {
    PRBool more;
    nsCOMPtr<nsIFile> entry;
    while (NS_SUCCEEDED(entries->HasMoreElements(&more)) && more) {
      rv = files->GetNextFile(getter_AddRefs(entry));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = CollectAddressBooks(entry, aCollected);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
BeckyAddressBooksImporter::FindAddressBooks(nsIFile *aLocation,
                                            nsISupportsArray **_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aLocation);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsCOMPtr<nsISupportsArray> array;

  rv = NS_NewISupportsArray(getter_AddRefs(array));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CollectAddressBooks(aLocation, array);
  if (NS_FAILED(rv))
    return rv;

  array.swap(*_retval);

  return NS_OK;
}

NS_IMETHODIMP
BeckyAddressBooksImporter::InitFieldMap(nsIImportFieldMap *aFieldMap)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
BeckyAddressBooksImporter::ImportAddressBook(nsIImportABDescriptor *aSource,
                                             nsIAddrDatabase *aDestination,
                                             nsIImportFieldMap *aFieldMap,
                                             nsISupports *aSupportService,
#ifdef MOZ_IMPORTADDRESSBOOK_NEED_ISADDRLOCHOME
                                             PRBool aIsAddrLocHome,
#endif
                                             PRUnichar **aErrorLog NS_OUTPARAM,
                                             PRUnichar **aSuccessLog NS_OUTPARAM,
                                             PRBool *aFatalError NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aDestination);
  NS_ENSURE_ARG_POINTER(aErrorLog);
  NS_ENSURE_ARG_POINTER(aSuccessLog);
  NS_ENSURE_ARG_POINTER(aFatalError);

  mReadBytes = 0;

  nsCOMPtr<nsIFile> file;
  nsresult rv = aSource->GetAbFile(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return rv;

  nsAutoString error;
  rv = BeckyVCardAddress::ImportAddresses(file, aDestination, error, &mReadBytes);
  if (!error.IsEmpty()) {
    *aErrorLog = ToNewUnicode(error);
  }

  return rv;
}

NS_IMETHODIMP
BeckyAddressBooksImporter::GetImportProgress(PRUint32 *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mReadBytes;
  return NS_OK;
}

NS_IMETHODIMP
BeckyAddressBooksImporter::SetSampleLocation(nsIFile *aLocation)
{
  return NS_OK;
}

NS_IMETHODIMP
BeckyAddressBooksImporter::GetSampleData(PRInt32 aRecordNumber,
                                         PRBool *aRecordExists NS_OUTPARAM,
                                         PRUnichar **_retval NS_OUTPARAM)
{
  return NS_ERROR_FAILURE;
}

