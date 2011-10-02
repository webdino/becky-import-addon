/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
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
#include <nsILocalFile.h>
#include <nsISimpleEnumerator.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsStringGlue.h>
#include <nsIUTF8ConverterService.h>
#include <nsUConvCID.h>
#include <nsNativeCharsetUtils.h>
#include <nsIInputStream.h>
#include <nsILineInputStream.h>
#include <nsNetUtil.h>
#include <nsIINIParser.h>
#include <nsDirectoryServiceDefs.h>
#include <nsDirectoryServiceUtils.h>
#include <msgCore.h>
#include <nsIImportMail.h>

#include "BeckyUtils.h"

nsresult
BeckyUtils::FindUserDirectory(nsIFile **aLocation NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> folder = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = folder->InitWithNativePath(NS_LITERAL_CSTRING("C:\\Becky!"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = folder->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more;
  nsCOMPtr<nsIFile> entry;
  while (NS_SUCCEEDED(entries->HasMoreElements(&more)) && more) {
    rv = entries->GetNext(getter_AddRefs(entry));
    PRBool isDirectory = PR_FALSE;
    rv = entry->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isDirectory)
      return CallQueryInterface(entry, aLocation);
  }

  return NS_ERROR_FILE_NOT_FOUND;
}

#ifdef XP_WIN
#include <windows.h>
static PRBool
EnsureStringLength(nsAString& aString, PRUint32 aLength)
{
  aString.SetLength(aLength);
  return (aString.Length() == aLength);
}
#endif

nsresult
BeckyUtils::ConvertNativeStringToUTF8(const nsACString& aOriginal,
                                      nsACString& _retval NS_OUTPARAM)
{
  nsresult rv;
  nsAutoString unicodeString;
  rv = ConvertNativeStringToUTF16(aOriginal, unicodeString);
  if (NS_FAILED(rv))
    return rv;

  _retval = NS_ConvertUTF16toUTF8(unicodeString);
  return NS_OK;
}

nsresult
BeckyUtils::ConvertNativeStringToUTF16(const nsACString& aOriginal,
                                       nsAString& _retval NS_OUTPARAM)
{
#ifdef XP_WIN
  PRUint32 inputLen = aOriginal.Length();
  const char *buf = aOriginal.BeginReading();

  // determine length of result
  PRUint32 resultLen = 0;
  int n = ::MultiByteToWideChar(CP_ACP, 0, buf, inputLen, NULL, 0);
  if (n > 0)
    resultLen += n;

  // allocate sufficient space
  if (!EnsureStringLength(_retval, resultLen))
    return NS_ERROR_OUT_OF_MEMORY;
  if (resultLen > 0) {
    PRUnichar *result = _retval.BeginWriting();
    ::MultiByteToWideChar(CP_ACP, 0, buf, inputLen, result, resultLen);
  }

  return NS_OK;
#else
  nsresult rv;
  nsCOMPtr<nsIUTF8ConverterService> converter;
  converter = do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString utf8String;
  rv = converter->ConvertStringToUTF8(aOriginal, "CP932", PR_FALSE, utf8String);
  if (NS_FAILED(rv))
    return rv;

  _retval = NS_ConvertUTF8toUTF16(utf8String);
  return NS_OK;
#endif
}

nsresult
BeckyUtils::CreateLineInputStream(nsIFile *aFile,
                                  nsILineInputStream **_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIInputStream> inputStream;
  nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), aFile);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(inputStream, _retval);
}

nsresult
BeckyUtils::GetFolderListFile(nsIFile *aLocation, nsIFile **_retval NS_OUTPARAM)
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

  NS_IF_ADDREF(*_retval = folderListFile);

  return exists ? NS_OK : NS_ERROR_FILE_NOT_FOUND;
}

nsresult
BeckyUtils::GetDefaultFolderName(nsIFile *aFolderListFile, nsACString& name)
{
  nsresult rv;
  nsCOMPtr<nsILineInputStream> lineStream;
  rv = BeckyUtils::CreateLineInputStream(aFolderListFile,
                                         getter_AddRefs(lineStream));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more = PR_TRUE;
  rv = lineStream->ReadLine(name, &more);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
BeckyUtils::GetDefaultMailboxDirectory(nsIFile **_retval)
{
  nsCOMPtr<nsIFile> userDirectory;
  nsresult rv = FindUserDirectory(getter_AddRefs(userDirectory));
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

  NS_IF_ADDREF(*_retval = defaultFolder);

  return NS_OK;
}

nsresult
BeckyUtils::GetDefaultMailboxINIFile(nsIFile **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsIFile> mailboxDirectory;
  rv = BeckyUtils::GetDefaultMailboxDirectory(getter_AddRefs(mailboxDirectory));
  if (NS_FAILED(rv))
    return rv;

  return GetMailboxINIFile(mailboxDirectory, _retval);
}

nsresult
BeckyUtils::GetMailboxINIFile(nsIFile *aDirectory, nsIFile **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsIFile> target;
  rv = aDirectory->Clone(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->AppendNative(NS_LITERAL_CSTRING("Mailbox.ini"));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool exists;
  rv = target->Exists(&exists);
  if (exists)
    return CallQueryInterface(target, _retval);

  return NS_ERROR_FILE_NOT_FOUND;
}

nsresult
BeckyUtils::CreateINIParserForFile(nsIFile *aFile,
                                   nsIINIParser **aParser NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsIINIParserFactory> factory;
  factory = do_GetService("@mozilla.org/xpcom/ini-processor-factory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> file = do_QueryInterface(aFile);
  nsCOMPtr<nsIINIParser> parser;
  rv = factory->CreateINIParser(file, getter_AddRefs(parser));
  NS_IF_ADDREF(*aParser = parser);

  return rv;
}

nsresult
BeckyUtils::GetMaiboxNameFromINIFile(nsIFile *aFile, nsCString &aName)
{
  nsresult rv;
  nsCOMPtr<nsIINIParser> parser;
  rv = BeckyUtils::CreateINIParserForFile(aFile, getter_AddRefs(parser));
  NS_ENSURE_SUCCESS(rv, rv);

  return parser->GetString(NS_LITERAL_CSTRING("Account"),
                           NS_LITERAL_CSTRING("Name"),
                           aName);
}

nsresult
BeckyUtils::ConvertToUTF8File(nsIFile *aSourceFile,
                              nsIFile **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsIFile> convertedFile;

  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(convertedFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convertedFile->AppendNative(NS_LITERAL_CSTRING("becky-importer-addon"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convertedFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> destination;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(destination),
                                   convertedFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILineInputStream> lineStream;
  rv = BeckyUtils::CreateLineInputStream(aSourceFile,
                                         getter_AddRefs(lineStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString line;
  nsCAutoString utf8String;
  PRUint32 writtenBytes = 0;
  PRBool more = PR_TRUE;
  while (more) {
    rv = lineStream->ReadLine(line, &more);
    BeckyUtils::ConvertNativeStringToUTF8(line, utf8String);
    utf8String.AppendLiteral(MSG_LINEBREAK);
    rv = destination->Write(utf8String.get(), utf8String.Length(), &writtenBytes);
  }
  return CallQueryInterface(convertedFile, _retval);
}

nsresult
BeckyUtils::TranslateFolderName(const nsAString & aFolderName,
                                nsAString & _retval NS_OUTPARAM)
{
  if (aFolderName.LowerCaseEqualsLiteral("!trash")) {
    _retval = NS_LITERAL_STRING(kDestTrashFolderName);
  } else if (aFolderName.LowerCaseEqualsLiteral("!!!!inbox")) {
    _retval = NS_LITERAL_STRING(kDestInboxFolderName);
  } else if (aFolderName.LowerCaseEqualsLiteral("!!!!outbox")) {
    _retval = NS_LITERAL_STRING(kDestSentFolderName);
  } else if (aFolderName.LowerCaseEqualsLiteral("!!!!unsent")) {
    _retval = NS_LITERAL_STRING(kDestUnsentMessagesFolderName);
  } else {
    _retval = aFolderName;
  }

  return NS_OK;
}

static PRInt32
DefaultComparator(const PRUnichar *a, const PRUnichar *b, PRUint32 len)
{
  for (const PRUnichar *end = a + len; a < end;
      ++a, ++b) {
    if (*a == *b)
      continue;

    return *a < *b ? -1 : 1;
  }

  return 0;
}

static PRInt32
StringRFind(const nsAString &aHaystack, const nsAString &aNeedle)
{
  const PRUnichar *rawHaystackData;
  PRUint32 haystackLength = NS_StringGetData(aHaystack, &rawHaystackData);
  const PRUnichar *begin, *end;
  begin = rawHaystackData;
  end = begin + haystackLength;

  const PRUnichar *rawNeedleData;
  PRUint32 needleLength = NS_StringGetData(aNeedle, &rawNeedleData);

  if (haystackLength < needleLength)
    return -1;

  end -= needleLength;

  for (const PRUnichar *cur = end; cur >= begin; --cur) {
    if (!DefaultComparator(cur, rawNeedleData, needleLength))
      return cur - begin;
  }
  return -1;
}

static PRInt32
StringFind(const nsAString &aHaystack, const nsAString &aNeedle)
{
  const PRUnichar *rawHaystackData;
  PRUint32 haystackLength = NS_StringGetData(aHaystack, &rawHaystackData);
  const PRUnichar *begin, *end;
  begin = rawHaystackData;
  end = begin + haystackLength;

  const PRUnichar *rawNeedleData;
  PRUint32 needleLength = NS_StringGetData(aNeedle, &rawNeedleData);

  if (haystackLength < needleLength)
    return -1;

  end -= needleLength;

  for (const PRUnichar *cur = begin; cur <= end; ++cur) {
    if (!DefaultComparator(cur, rawNeedleData, needleLength))
      return cur - begin;
  }
  return -1;
}

PRBool
BeckyUtils::StringEndsWith(const nsAString &aHaystack, const nsAString &aNeedle)
{
  PRInt32 idx = StringRFind(aHaystack, aNeedle);
  if (idx == -1)
    return false;

  return (idx + aNeedle.Length() == aHaystack.Length());
}

PRBool
BeckyUtils::StringBeginsWith(const nsAString &aHaystack, const nsAString &aNeedle)
{
  return (StringFind(aHaystack, aNeedle) == 0);
}

