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
#include <nsStringGlue.h>
#include <nsCOMPtr.h>
#include <nsIFile.h>
#include <nsIInputStream.h>
#include <nsILineInputStream.h>
#include <nsNetUtil.h>
#include <nsISupportsArray.h>
#include <nsIImportService.h>
#include <nsIImportMailboxDescriptor.h>

#include "BeckyMailImporter.h"
#include "BeckyUtils.h"

NS_IMPL_ISUPPORTS1(BeckyMailImporter, nsIImportMail)

BeckyMailImporter::BeckyMailImporter()
{
  /* member initializers and constructor code */
}

BeckyMailImporter::~BeckyMailImporter()
{
  /* destructor code */
}

static nsresult
GetFolderListFile(nsIFile *aLocation, nsIFile **aFile)
{
  nsresult rv;
  nsCOMPtr<nsIFile> folderListFile;
  rv = aLocation->Clone(getter_AddRefs(folderListFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = folderListFile->AppendNative(NS_LITERAL_CSTRING("Folder.lst"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = folderListFile->Exists(&exists);
  if (NS_FAILED(rv))
    return rv;

  NS_IF_ADDREF(*aFile = folderListFile);

  return exists ? NS_OK : NS_ERROR_FILE_NOT_FOUND;
}

static nsresult
GetDefaultFolderName(nsIFile *aFolderListFile, nsACString& name)
{
  nsresult rv;
  nsCOMPtr<nsIInputStream> inputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream),
                                  aFolderListFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(inputStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more = PR_TRUE;
  rv = lineStream->ReadLine(name, &more);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

static nsresult
GetDefaultFolder(nsIFile **aFolder)
{
  nsCOMPtr<nsIFile> userDirectory;
  nsresult rv = BeckyUtils::FindUserDirectory(getter_AddRefs(userDirectory));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFile> folderListFile;
  rv = GetFolderListFile(userDirectory, getter_AddRefs(folderListFile));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString defaultFolderName;
  rv = GetDefaultFolderName(folderListFile, defaultFolderName);
  if (NS_FAILED(rv))
    return rv;

  rv = userDirectory->AppendNative(defaultFolderName);
  if (NS_FAILED(rv))
    return rv;

  nsIFile *defaultFolder = userDirectory;
  PRBool exists;
  rv = defaultFolder->Exists(&exists);
  if (NS_FAILED(rv))
    return rv;

  NS_IF_ADDREF(*aFolder = defaultFolder);

  return NS_OK;
}

NS_IMETHODIMP
BeckyMailImporter::GetDefaultLocation(nsIFile **aLocation NS_OUTPARAM,
                                      PRBool *aFound NS_OUTPARAM,
                                      PRBool *aUserVerify NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aFound);
  NS_ENSURE_ARG_POINTER(aLocation);
  NS_ENSURE_ARG_POINTER(aUserVerify);

  *aLocation = nsnull;
  *aUserVerify = PR_TRUE;
  *aFound = NS_SUCCEEDED(GetDefaultFolder(aLocation));

  return NS_OK;
}

static nsresult
CreateMailboxDescriptor(nsIImportMailboxDescriptor **aDescriptor)
{
  nsresult rv;
  nsCOMPtr<nsIImportService> importService;
  importService = do_GetService(NS_IMPORTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return importService->CreateNewMailboxDescriptor(aDescriptor);
}

static nsresult
AppendMailboxDescriptor(nsIFile *aEntry, nsISupportsArray *aCollected)
{
  nsCAutoString name;
  nsresult rv = aEntry->GetNativeLeafName(name);
  if (!StringEndsWith(name, NS_LITERAL_CSTRING(".bmf")))
    return NS_OK;

  nsCOMPtr<nsIImportMailboxDescriptor> descriptor;
  rv = CreateMailboxDescriptor(getter_AddRefs(descriptor));

  PRInt64 size;
  aEntry->GetFileSize(&size);
  descriptor->SetSize(size);

  nsCOMPtr<nsIFile> parent;
  rv = aEntry->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString parentName;
  parent->GetLeafName(parentName);
  descriptor->SetDisplayName(parentName.get());

  nsCOMPtr<nsILocalFile> mailboxFile;
  rv = descriptor->GetFile(getter_AddRefs(mailboxFile));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILocalFile> localFile;
  localFile = do_QueryInterface(aEntry, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mailboxFile->InitWithFile(localFile);
  aCollected->AppendElement(descriptor);

  return NS_OK;
}

static nsresult
CollectFoldersInFolderListFile(nsIFile *aListFile, nsISupportsArray *aCollected)
{
  return NS_OK;
}

static nsresult
CollectMailboxesInDirectory(nsIFile *aDirectory, nsISupportsArray *aCollected)
{
  nsresult rv;
  nsCOMPtr<nsIFile> folderListFile;
  rv = GetFolderListFile(aDirectory, getter_AddRefs(folderListFile));

  if (NS_SUCCEEDED(rv)) {
    CollectFoldersInFolderListFile(folderListFile, aCollected);
  } else {
    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool more;
    nsCOMPtr<nsIFile> entry;
    while (NS_SUCCEEDED(entries->HasMoreElements(&more)) && more) {
      rv = entries->GetNext(getter_AddRefs(entry));
      PRBool isDirectory = PR_FALSE;
      rv = entry->IsDirectory(&isDirectory);
      NS_ENSURE_SUCCESS(rv, rv);
      if (isDirectory) {
        CollectMailboxesInDirectory(entry, aCollected);
      } else {
        AppendMailboxDescriptor(entry, aCollected);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
BeckyMailImporter::FindMailboxes(nsIFile *aLocation,
                                 nsISupportsArray **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsISupportsArray> array;
  rv = NS_NewISupportsArray(getter_AddRefs(array));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CollectMailboxesInDirectory(aLocation, array);
  if (NS_FAILED(rv))
    return rv;

  array.swap(*_retval);

  return NS_OK;
}

NS_IMETHODIMP
BeckyMailImporter::ImportMailbox(nsIImportMailboxDescriptor *aSource,
                                 nsIFile *aDestination,
                                 PRUnichar **aErrorLog NS_OUTPARAM,
                                 PRUnichar **aSuccessLog NS_OUTPARAM,
                                 PRBool *aFatalError NS_OUTPARAM)
{
  mReadBytes = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
BeckyMailImporter::GetImportProgress(PRUint32 *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mReadBytes;
  return NS_OK;
}

NS_IMETHODIMP
BeckyMailImporter::TranslateFolderName(const nsAString & aFolderName,
                                       nsAString & _retval NS_OUTPARAM)
{
  if (aFolderName.LowerCaseEqualsLiteral("trash")) {
    _retval = NS_LITERAL_STRING(kDestTrashFolderName);
  } else if (aFolderName.LowerCaseEqualsLiteral("inbox")) {
    _retval = NS_LITERAL_STRING(kDestInboxFolderName);
  } else if (aFolderName.LowerCaseEqualsLiteral("sent")) {
    _retval = NS_LITERAL_STRING(kDestSentFolderName);
  } else if (aFolderName.LowerCaseEqualsLiteral("unsent")) {
    _retval = NS_LITERAL_STRING(kDestUnsentMessagesFolderName);
  } else {
    _retval = aFolderName;
  }

  return NS_OK;
}

