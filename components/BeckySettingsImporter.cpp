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
#include <nsMsgBaseCID.h>
#include <nsMsgCompCID.h>
#include <nsIMsgAccountManager.h>
#include <nsServiceManagerUtils.h>
#include <nsIINIParser.h>
#include <nsISmtpService.h>
#include <nsISmtpServer.h>
#include <nsIStringEnumerator.h>
#include <nsIDirectoryEnumerator.h>
#include <nsIInputStream.h>
#include <nsIOutputStream.h>
#include <nsILineInputStream.h>
#include <nsIUTF8ConverterService.h>
#include <nsUConvCID.h>
#include <nsNetUtil.h>
#include <nsDirectoryServiceDefs.h>
#include <nsDirectoryServiceUtils.h>
#include <nsStringGlue.h>

#include "BeckySettingsImporter.h"
#include "BeckyStringBundle.h"
#include "BeckyUtils.h"

NS_IMPL_ISUPPORTS1(BeckySettingsImporter, nsIImportSettings)

BeckySettingsImporter::BeckySettingsImporter()
: mLocation(nsnull),
  mConvertedFile(nsnull)
{
  /* member initializers and constructor code */
}

BeckySettingsImporter::~BeckySettingsImporter()
{
}

static
nsresult
GetMailboxDirectory(nsIFile **aDirectory)
{
  nsCOMPtr<nsIFile> baseDirectory;
  nsresult rv = BeckyUtils::FindUserDirectory(getter_AddRefs(baseDirectory));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = baseDirectory->GetDirectoryEntries(getter_AddRefs(entries));
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
      return CallQueryInterface(entry, aDirectory);
  }

  return NS_ERROR_FILE_NOT_FOUND;
}

NS_IMETHODIMP
BeckySettingsImporter::AutoLocate(PRUnichar **aDescription NS_OUTPARAM,
                                  nsIFile **aLocation NS_OUTPARAM,
                                  PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aDescription);
  NS_ENSURE_ARG_POINTER(aLocation);
  NS_ENSURE_ARG_POINTER(_retval);

  nsString description;
  BeckyStringBundle::GetStringByID(BECKYIMPORT_NAME, description);

  *aDescription = ToNewUnicode(description);
  *aLocation = nsnull;
  *_retval = PR_FALSE;

  nsCOMPtr<nsIFile> location;
  nsresult rv = GetMailboxDirectory(getter_AddRefs(location));
  if (NS_FAILED(rv))
    return NS_OK;

  *_retval = PR_TRUE;
  return CallQueryInterface(location, aLocation);
}

NS_IMETHODIMP
BeckySettingsImporter::SetLocation(nsIFile *aLocation)
{
  mLocation = aLocation;
  return NS_OK;
}

static nsresult
ConvertToUTF8File(nsIFile *aSourceFile, nsIFile **aConvertedFile)
{
  nsCOMPtr<nsIInputStream> source;
  nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(source),
                                           aSourceFile);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIOutputStream> destination;

  nsCOMPtr<nsIFile> convertedFile;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(convertedFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convertedFile->AppendNative(NS_LITERAL_CSTRING("becky-importer-addon"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convertedFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewLocalFileOutputStream(getter_AddRefs(destination),
                                   convertedFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(source, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUTF8ConverterService> converter;
  converter = do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID);
  nsCAutoString line;
  nsCAutoString utf8String;
  PRUint32 bytesWritten = 0;
  PRBool more = PR_TRUE;
  while (more) {
    rv = lineStream->ReadLine(line, &more);
    converter->ConvertStringToUTF8(line, "CP932", PR_FALSE, utf8String);
    utf8String.AppendLiteral("\r\n");
    rv = destination->Write(utf8String.get(), utf8String.Length(), &bytesWritten);
  }
  return CallQueryInterface(convertedFile, aConvertedFile);
}

nsresult
BeckySettingsImporter::CreateParser(nsIINIParser **aParser)
{
  if (!mLocation)
    return NS_ERROR_FILE_NOT_FOUND;

  nsresult rv;
  rv = ConvertToUTF8File(mLocation, getter_AddRefs(mConvertedFile));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIINIParserFactory> factory;
  factory = do_GetService("@mozilla.org/xpcom/ini-processor-factory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> file = do_QueryInterface(mConvertedFile);
  nsCOMPtr<nsIINIParser> parser;
  rv = factory->CreateINIParser(file, getter_AddRefs(parser));
  NS_IF_ADDREF(*aParser = parser);

  return rv;
}

static nsresult
GetSmtpServer(const nsCString &aUserName,
              const nsCString &aServerName,
              nsISmtpServer **aServer)
{
  nsresult rv;

  nsCOMPtr<nsISmtpService> smtpService;
  smtpService = do_GetService(NS_SMTPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISmtpServer> server;
  rv = smtpService->FindServer(aUserName.get(),
                               aServerName.get(),
                               getter_AddRefs(server));

  if (NS_FAILED(rv) || !server) {
    rv = smtpService->CreateSmtpServer(getter_AddRefs(server));
  }
  NS_IF_ADDREF(*aServer = server);

  return rv;
}

static nsresult
GetIncomingServer(const nsCString &aUserName,
                  const nsCString &aServerName,
                  const nsCString &aProtocol,
                  nsIMsgIncomingServer **aServer)
{
  nsresult rv;
  nsCOMPtr<nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgIncomingServer> incomingServer;
  rv = accountManager->FindServer(aUserName,
                                  aServerName,
                                  aProtocol,
                                  getter_AddRefs(incomingServer));

  if (NS_FAILED(rv) || !incomingServer) {
    rv = accountManager->CreateIncomingServer(aUserName,
                                              aServerName,
                                              aProtocol,
                                              getter_AddRefs(incomingServer));
  }
  NS_IF_ADDREF(*aServer = incomingServer);

  return rv;
}

static nsresult
CreateSmtpServer(nsIINIParser *aParser,
                 nsISmtpServer **aServer)
{
  nsresult rv;
  nsCAutoString userName, serverName;

  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("SMTPServer"),
                     serverName);
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("UserID"),
                     userName);

  nsCOMPtr<nsISmtpServer> server;
  rv = GetSmtpServer(userName, serverName, getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString value;
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("SMTPPort"),
                     value);
  nsresult errorCode;
  PRInt32 port = static_cast<PRInt32>(value.ToInteger(&errorCode, 10));
  server->SetPort(port);

  PRBool isSecure = PR_FALSE;
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("SSLSMTP"),
                     value);
  if (value.EqualsLiteral("1")) {
    isSecure = PR_TRUE;
  }
  if (isSecure)
    server->SetSocketType(nsMsgSocketType::SSL);

  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("SMTPAUTH"),
                     value);
  if (value.EqualsLiteral("1")) {
  }

  NS_IF_ADDREF(*aServer = server);

  return NS_OK;
}

static nsresult
CreateIncomingServer(nsIINIParser *aParser,
                     nsIMsgIncomingServer **aServer)
{
  nsCAutoString userName, serverName;
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("MailServer"),
                     serverName);
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("UserID"),
                     userName);

  nsCAutoString value;
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("Protocol"),
                     value);
  nsCString protocol;
  if (value.EqualsLiteral("0")) {
    protocol = NS_LITERAL_CSTRING("pop3");
  } else if (value.EqualsLiteral("1")) {
    protocol = NS_LITERAL_CSTRING("imap");
  } else {
    protocol = NS_LITERAL_CSTRING("pop3");
  }

  nsresult rv;
  nsCOMPtr<nsIMsgIncomingServer> server;
  rv = GetIncomingServer(userName, serverName, protocol, getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSecure = PR_FALSE;
  PRInt32 port = 0;
  nsresult errorCode;
  if (value.EqualsLiteral("0")) {
    protocol = NS_LITERAL_CSTRING("pop");
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("POP3Port"),
                       value);
    port = static_cast<PRInt32>(value.ToInteger(&errorCode, 10));
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("SSLPOP"),
                       value);
    if (value.EqualsLiteral("1")) {
      isSecure = PR_TRUE;
    }
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("POP3Auth"),
                       value); // 0: plain, 1: APOP, 2: CRAM-MD5, 3: NTLM
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("LeaveServer"),
                       value);
  } else if (value.EqualsLiteral("1")) {
    protocol = NS_LITERAL_CSTRING("imap");
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("IMAP4Port"),
                       value);
    port = static_cast<PRInt32>(value.ToInteger(&errorCode, 10));
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("SSLIMAP"),
                       value);
    if (value.EqualsLiteral("1")) {
      isSecure = PR_TRUE;
    }
  }

  server->SetPort(port);
  if (isSecure)
    server->SetSocketType(nsMsgSocketType::SSL);

  NS_IF_ADDREF(*aServer = server);

  return NS_OK;
}

static nsresult
CreateIdentity(nsIINIParser *aParser,
               nsIMsgIdentity **aIdentity)
{
  nsCAutoString email, fullName, identityName;

  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("Name"),
                     identityName);
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("YourName"),
                     fullName);
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("MailAddress"),
                     email);

  nsresult rv;
  nsCOMPtr<nsIMsgAccountManager> accountManager =
    do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgIdentity> identity;
  rv = accountManager->CreateIdentity(getter_AddRefs(identity));
  NS_ENSURE_SUCCESS(rv, rv);

  identity->SetIdentityName(NS_ConvertUTF8toUTF16(identityName));
  identity->SetFullName(NS_ConvertUTF8toUTF16(fullName));
  identity->SetEmail(email);

  NS_IF_ADDREF(*aIdentity = identity);

  return NS_OK;
}

static nsresult
CreateAccount(nsIMsgIdentity *aIdentity,
              nsIMsgIncomingServer *aIncomingServer,
              nsIMsgAccount **aAccount)
{
  nsresult rv;
  nsCOMPtr<nsIMsgAccountManager> accountManager =
    do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgAccount> account;
  rv = accountManager->CreateAccount(getter_AddRefs(account));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = account->AddIdentity(aIdentity);
  if (NS_FAILED(rv))
    return rv;

  rv = account->SetIncomingServer(aIncomingServer);
  if (NS_FAILED(rv))
    return rv;

  NS_ADDREF(*aAccount = account);

  return rv;
}

nsresult
BeckySettingsImporter::RemoveConvertedFile()
{
  if (mConvertedFile) {
    PRBool exists;
    mConvertedFile->Exists(&exists);
    if (exists)
      mConvertedFile->Remove(PR_FALSE);
    NS_RELEASE(mConvertedFile);
  }
  return NS_OK;
}

#define NS_RETURN_IF_FAILED_WITH_REMOVE_CONVERTED_FILE(expr, rv) \
  if (NS_FAILED(expr)) {                                         \
    RemoveConvertedFile();                                       \
    return rv;                                                   \
  }

NS_IMETHODIMP
BeckySettingsImporter::Import(nsIMsgAccount **aLocalMailAccount NS_OUTPARAM,
                              PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aLocalMailAccount);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIINIParser> parser;
  nsresult rv = CreateParser(getter_AddRefs(parser));
  NS_RETURN_IF_FAILED_WITH_REMOVE_CONVERTED_FILE(rv, rv);

  nsCOMPtr<nsIMsgIncomingServer> incomingServer;
  rv = CreateIncomingServer(parser, getter_AddRefs(incomingServer));
  NS_RETURN_IF_FAILED_WITH_REMOVE_CONVERTED_FILE(rv, rv);

  nsCOMPtr<nsISmtpServer> smtpServer;
  rv = CreateSmtpServer(parser, getter_AddRefs(smtpServer));
  NS_RETURN_IF_FAILED_WITH_REMOVE_CONVERTED_FILE(rv, rv);

  nsCOMPtr<nsIMsgIdentity> identity;
  rv = CreateIdentity(parser, getter_AddRefs(identity));
  NS_RETURN_IF_FAILED_WITH_REMOVE_CONVERTED_FILE(rv, rv);

  nsCOMPtr<nsIMsgAccount> account;
  rv = CreateAccount(identity, incomingServer, getter_AddRefs(account));
  NS_RETURN_IF_FAILED_WITH_REMOVE_CONVERTED_FILE(rv, rv);

  RemoveConvertedFile();
  if (aLocalMailAccount)
    account.forget(aLocalMailAccount);
  *_retval = PR_TRUE;
  return NS_OK;
}

