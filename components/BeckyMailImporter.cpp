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
#include <nsIOutputStream.h>
#include <nsILineInputStream.h>
#include <nsNetUtil.h>
#include <nsISupportsArray.h>
#include <nsIImportService.h>
#include <nsIImportMailboxDescriptor.h>
#include <nsMsgUtils.h>
#include <nsMsgLocalFolderHdrs.h>
#include <nsMsgMessageFlags.h>
#include <nsTArray.h>
#include <nspr.h>
#include <nsIStringBundle.h>
#include <nsIEnumerator.h>

#include "BeckyMailImporter.h"
#include "BeckyUtils.h"
#include "BeckyStringBundle.h"

#define FROM_LINE "From - Mon Jan 1 00:00:00 1965" MSG_LINEBREAK
#define X_BECKY_STATUS_HEADER "X-Becky-Status"
#define X_BECKY_INCLUDE_HEADER "X-Becky-Include"

enum {
  BECKY_STATUS_READ      = 1 << 0,
  BECKY_STATUS_FORWARDED = 1 << 1,
  BECKY_STATUS_REPLIED   = 1 << 2
};

NS_IMPL_ISUPPORTS1(BeckyMailImporter, nsIImportMail)

BeckyMailImporter::BeckyMailImporter()
{
  /* member initializers and constructor code */
}

BeckyMailImporter::~BeckyMailImporter()
{
  /* destructor code */
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
  *aFound = NS_SUCCEEDED(BeckyUtils::GetDefaultMailboxDirectory(aLocation));

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
GetMailboxName(nsIFile *aMailbox, nsAString &aName)
{
  nsresult rv;

  nsCOMPtr<nsIFile> iniFile;
  rv = BeckyUtils::GetMailboxINIFile(aMailbox, getter_AddRefs(iniFile));
  if (iniFile) {
    nsCOMPtr<nsIFile> convertedFile;
    rv = BeckyUtils::ConvertToUTF8File(iniFile, getter_AddRefs(convertedFile));
    if (convertedFile) {
      nsCAutoString utf8Name;
      BeckyUtils::GetMaiboxNameFromINIFile(convertedFile, utf8Name);
      convertedFile->Remove(PR_FALSE);
      aName = NS_ConvertUTF8toUTF16(utf8Name);
    }
  }

  if (aName.IsEmpty()) {
    aMailbox->GetLeafName(aName);
    aName.Trim("!", PR_TRUE, PR_FALSE);
  }

  return NS_OK;
}

PRUint32
BeckyMailImporter::CountMailboxSize(nsIFile *aMailboxFolder)
{
  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = aMailboxFolder->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, 0);

  PRBool more;
  nsCOMPtr<nsIFile> entry;
  PRUint32 totalSize = 0;
  while (NS_SUCCEEDED(entries->HasMoreElements(&more)) && more) {
    rv = entries->GetNext(getter_AddRefs(entry));
    NS_ENSURE_SUCCESS(rv, 0);

    nsCAutoString name;
    rv = entry->GetNativeLeafName(name);
    if (!StringEndsWith(name, NS_LITERAL_CSTRING(".bmf")))
      continue;

    PRInt64 size;
    entry->GetFileSize(&size);
    if (totalSize + size > PR_UINT32_MAX)
      return totalSize;
    totalSize += size;
  }

  return totalSize;
}

nsresult
BeckyMailImporter::AppendMailboxDescriptor(nsIFile *aEntry,
                                           PRUint32 aDepth,
                                           nsISupportsArray *aCollected)
{
  nsresult rv;
  nsCOMPtr<nsIImportMailboxDescriptor> descriptor;
  rv = CreateMailboxDescriptor(getter_AddRefs(descriptor));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 size = CountMailboxSize(aEntry);
  rv = descriptor->SetSize(size);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString mailboxName;
  rv = GetMailboxName(aEntry, mailboxName);
  NS_ENSURE_SUCCESS(rv, rv);
  descriptor->SetDisplayName(mailboxName.get());

  nsCOMPtr<nsILocalFile> mailboxFile;
  rv = descriptor->GetFile(getter_AddRefs(mailboxFile));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILocalFile> localFile;
  localFile = do_QueryInterface(aEntry, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  descriptor->SetDepth(aDepth);

  mailboxFile->InitWithFile(localFile);
  aCollected->AppendElement(descriptor);

  return NS_OK;
}

nsresult
BeckyMailImporter::CollectMailboxesInFolderListFile(nsIFile *aListFile,
                                                    PRUint32 aDepth,
                                                    nsISupportsArray *aCollected)
{
  nsresult rv;
  nsCOMPtr<nsILineInputStream> lineStream;
  rv = BeckyUtils::CreateLineInputStream(aListFile,
                                         getter_AddRefs(lineStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> parent;
  rv = aListFile->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more = PR_TRUE;
  nsCAutoString folderName;
  while (more && NS_SUCCEEDED(rv)) {
    rv = lineStream->ReadLine(folderName, &more);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> folder;
    rv = parent->Clone(getter_AddRefs(folder));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = folder->AppendNative(folderName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CollectMailboxesInDirectory(folder, aDepth, aCollected);
  }

  return rv;
}

nsresult
BeckyMailImporter::CollectMailboxesInDirectory(nsIFile *aDirectory,
                                               PRUint32 aDepth,
                                               nsISupportsArray *aCollected)
{
  if (aDepth != 0)
    AppendMailboxDescriptor(aDirectory, aDepth, aCollected);

  nsresult rv;
  nsCOMPtr<nsIFile> folderListFile;
  rv = BeckyUtils::GetFolderListFile(aDirectory, getter_AddRefs(folderListFile));

  if (NS_SUCCEEDED(rv))
    CollectMailboxesInFolderListFile(folderListFile, ++aDepth, aCollected);

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

  rv = CollectMailboxesInDirectory(aLocation, 0, array);
  if (NS_FAILED(rv))
    return rv;

  array.swap(*_retval);

  return NS_OK;
}

static nsresult
GetBeckyStatusValue (const nsCString &aHeader, nsACString &aValue)
{
  PRUint32 valueStartPosition;

  valueStartPosition = aHeader.FindChar(':');
  if (valueStartPosition < 0)
    return NS_ERROR_FAILURE;

  valueStartPosition++;
  nsDependentCSubstring value(aHeader,
                              valueStartPosition,
                              aHeader.FindChar(',', valueStartPosition) - valueStartPosition);
  value.Trim(" \t");

  aValue.Assign(value);

  return NS_OK;
}

static nsresult
GetBeckyIncludeValue (const nsCString &aHeader, nsACString &aValue)
{
  PRUint32 valueStartPosition;

  valueStartPosition = aHeader.FindChar(':');
  if (valueStartPosition < 0)
    return NS_ERROR_FAILURE;

  valueStartPosition++;
  nsDependentCSubstring value(aHeader,
                              valueStartPosition);
  value.Trim(" \t");

  aValue.Assign(value);

  return NS_OK;
}

static PRBool
ConvertBeckyStatusToMozillaStatus(const nsCString &aHeader,
                                  nsMsgMessageFlagType *aMozillaStatusFlag)
{
  nsresult rv;
  nsCAutoString statusString;
  rv = GetBeckyStatusValue(aHeader, statusString);
  NS_ENSURE_SUCCESS(rv, rv);

  nsresult errorCode;
  PRUint32 beckyStatusFlag = static_cast<PRUint32>(statusString.ToInteger(&errorCode, 16));
  if (NS_FAILED(errorCode))
    return PR_FALSE;

  if (beckyStatusFlag & BECKY_STATUS_READ)
    *aMozillaStatusFlag |= nsMsgMessageFlags::Read;
  if (beckyStatusFlag & BECKY_STATUS_FORWARDED)
    *aMozillaStatusFlag |= nsMsgMessageFlags::Forwarded;
  if (beckyStatusFlag & BECKY_STATUS_REPLIED)
    *aMozillaStatusFlag |= nsMsgMessageFlags::Replied;

  return PR_TRUE;
}

static inline PRBool
CheckHeaderKey(const nsCString &aHeader, const char *aKeyString)
{
  nsDependentCSubstring key(aHeader, 0, aHeader.FindChar(':'));
  key.Trim(" \t");
  return key.Equals(aKeyString);
}

static inline PRBool
IsBeckyStatusHeader(const nsCString &aHeader)
{
  return CheckHeaderKey(aHeader, X_BECKY_STATUS_HEADER);
}

static nsresult
HandleHeaderLine(const nsCString &aHeaderLine, nsACString &aHeaders)
{
  aHeaders.Append(aHeaderLine);
  aHeaders.AppendLiteral(MSG_LINEBREAK);

  nsMsgMessageFlagType flag = 0;
  if (IsBeckyStatusHeader(aHeaderLine) && ConvertBeckyStatusToMozillaStatus(aHeaderLine, &flag)) {
    char *statusLine;
    statusLine = PR_smprintf(X_MOZILLA_STATUS_FORMAT MSG_LINEBREAK, flag);
    aHeaders.Append(statusLine);
    PR_smprintf_free(statusLine);
    aHeaders.Append(X_MOZILLA_KEYWORDS);
  }

  return NS_OK;
}

static nsresult
WriteHeaders(nsCString &aHeaders, nsIOutputStream *aOutputStream)
{
  nsresult rv;
  PRUint32 writtenBytes = 0;

  rv = aOutputStream->Write(FROM_LINE, strlen(FROM_LINE), &writtenBytes);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutputStream->Write(aHeaders.get(), aHeaders.Length(), &writtenBytes);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutputStream->Write(MSG_LINEBREAK, strlen(MSG_LINEBREAK), &writtenBytes);
  NS_ENSURE_SUCCESS(rv, rv);
  aHeaders.Truncate();

  return NS_OK;
}

static inline PRBool
IsBeckyIncludeLine(const nsCString &aLine)
{
  return CheckHeaderKey(aLine, X_BECKY_INCLUDE_HEADER);
}

static nsresult
GetAttachmentFile(nsIFile *aMailboxFile,
                  const nsCString &aHeader,
                  nsIFile **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsIFile> attachmentFile;

  rv = aMailboxFile->Clone(getter_AddRefs(attachmentFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = attachmentFile->AppendNative(NS_LITERAL_CSTRING("#Attach"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString nativeAttachmentPath;
  rv = GetBeckyIncludeValue(aHeader, nativeAttachmentPath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString utf8AttachmentPath;
  BeckyUtils::ConvertNativeStringToUTF8(nativeAttachmentPath, utf8AttachmentPath);

  nsTArray<nsCAutoString> directoryNames;
  ParseString(utf8AttachmentPath, '\\', directoryNames);
  for (nsTArray<nsCString>::index_type i = 0; i < directoryNames.Length(); i++)
    rv = attachmentFile->Append(NS_ConvertUTF8toUTF16(directoryNames[i]));

  PRBool exists = PR_FALSE;
  attachmentFile->Exists(&exists);

  NS_IF_ADDREF(*_retval = attachmentFile);

  return exists ? NS_OK : NS_ERROR_FILE_NOT_FOUND;
}

static nsresult
WriteAttachmentFile(nsIFile *aMailboxFile,
                    const nsCString &aHeader,
                    nsIOutputStream *aOutputStream)
{
  nsresult rv;
  nsCOMPtr<nsIFile> parentDirectory;
  rv = aMailboxFile->GetParent(getter_AddRefs(parentDirectory));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> attachmentFile;
  rv = GetAttachmentFile(parentDirectory,
                         aHeader,
                         getter_AddRefs(attachmentFile));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIInputStream> inputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream),
                                  attachmentFile);
  NS_ENSURE_SUCCESS(rv, rv);

  char buffer[4096];
  PRUint32 readBytes = 0;
  PRUint32 writtenBytes = 0;
  rv = aOutputStream->Write(MSG_LINEBREAK, strlen(MSG_LINEBREAK), &writtenBytes);
  while (NS_SUCCEEDED(inputStream->Read(buffer, sizeof(buffer), &readBytes)) &&
         readBytes > 0) {
    rv = aOutputStream->Write(buffer, readBytes, &writtenBytes);
  }

  return NS_OK;
}

static nsresult
_MsgNewBufferedFileOutputStream(nsIOutputStream **aResult,
                                nsIFile* aFile,
                                PRInt32 aIOFlags = -1,
                                PRInt32 aPerm = -1)
{
  nsCOMPtr<nsIOutputStream> stream;
  nsresult rv = NS_NewLocalFileOutputStream(getter_AddRefs(stream),
                                            aFile,
                                            aIOFlags,
                                            aPerm);
  if (NS_SUCCEEDED(rv))
    rv = NS_NewBufferedOutputStream(aResult, stream, 4096);
  return rv;
}

nsresult
BeckyMailImporter::ImportMailFile(nsIFile *aMailFile, nsIOutputStream *aOutputStream)
{
  nsresult rv;
  nsCOMPtr<nsILineInputStream> lineStream;
  rv = BeckyUtils::CreateLineInputStream(aMailFile,
                                         getter_AddRefs(lineStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString line;
  PRUint32 writtenBytes = 0;
  PRBool inHeader = PR_TRUE;
  PRBool more = PR_TRUE;
  nsCAutoString headers;
  while (more && NS_SUCCEEDED(rv)) {
    rv = lineStream->ReadLine(line, &more);
    if (NS_FAILED(rv))
      break;

    mReadBytes += line.Length(); // Actually this isn't correct.

    if (inHeader) {
      if (line.IsEmpty()) { // End of headers
        rv = WriteHeaders(headers, aOutputStream);
        if (NS_FAILED(rv))
          break;
        inHeader = PR_FALSE;
      } else {
        HandleHeaderLine(line, headers);
      }
      continue;
    }

    if (line.Equals(".")) { // End of messages
      inHeader = PR_TRUE;
      continue;
    }

    if (IsBeckyIncludeLine(line)) {
      rv = WriteAttachmentFile(aMailFile, line, aOutputStream);
      if (NS_FAILED(rv))
        break;
      continue;
    }
    if (StringBeginsWith(line, NS_LITERAL_CSTRING("..")))
      line.Cut(0, 1);

    line.AppendLiteral(MSG_LINEBREAK);
    rv = aOutputStream->Write(line.get(), line.Length(), &writtenBytes);
  }
  return NS_OK;
}

NS_IMETHODIMP
BeckyMailImporter::ImportMailbox(nsIImportMailboxDescriptor *aSource,
                                 nsIFile *aDestination,
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

  nsresult rv;
  nsCOMPtr<nsILocalFile> mailboxFolder;
  rv = aSource->GetFile(getter_AddRefs(mailboxFolder));
  if (NS_FAILED(rv))
    return rv;

  PRBool isDirectory = PR_FALSE;
  rv = mailboxFolder->IsDirectory(&isDirectory);
  if (!isDirectory)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIOutputStream> outputStream;
  rv = _MsgNewBufferedFileOutputStream(getter_AddRefs(outputStream), aDestination);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = mailboxFolder->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more;
  nsCOMPtr<nsIFile> entry;
  while (NS_SUCCEEDED(entries->HasMoreElements(&more)) && more) {
    rv = entries->GetNext(getter_AddRefs(entry));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString name;
    rv = entry->GetNativeLeafName(name);
    if (!StringEndsWith(name, NS_LITERAL_CSTRING(".bmf")))
      continue;

    ImportMailFile(entry, outputStream);
  }

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIStringBundle> bundle(dont_AddRef(BeckyStringBundle::GetStringBundleProxy()));
    if (bundle) {
      nsString successMessage;
      BeckyStringBundle::GetStringByID(BECKYIMPORT_MAILBOX_SUCCESS, successMessage, bundle);
      *aSuccessLog = ToNewUnicode(successMessage);
    }
  }

  return rv;
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

