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
GetFolderListFile(nsIFile **aFile)
{
  nsCOMPtr<nsIFile> folderListFile;
  nsresult rv = BeckyUtils::FindUserDirectory(getter_AddRefs(folderListFile));
  if (NS_FAILED(rv))
    return rv;

  rv = folderListFile->AppendNative(NS_LITERAL_CSTRING("Folder.lst"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = folderListFile->Exists(&exists);
  if (NS_FAILED(rv))
    return rv;

  NS_IF_ADDREF(*aFile = folderListFile);

  return NS_OK;
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
  nsCOMPtr<nsIFile> folderListFile;
  nsresult rv = GetFolderListFile(getter_AddRefs(folderListFile));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString defaultFolderName;
  rv = GetDefaultFolderName(folderListFile, defaultFolderName);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFile> defaultFolder;
  rv = BeckyUtils::FindUserDirectory(getter_AddRefs(defaultFolder));
  if (NS_FAILED(rv))
    return rv;

  rv = defaultFolder->AppendNative(defaultFolderName);
  if (NS_FAILED(rv))
    return rv;

  PRBool exists;
  rv = folderListFile->Exists(&exists);
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

NS_IMETHODIMP
BeckyMailImporter::FindMailboxes(nsIFile *aLocation,
                                 nsISupportsArray **_retval NS_OUTPARAM)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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

