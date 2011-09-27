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
#include <nsIPop3IncomingServer.h>
#include <nsIStringEnumerator.h>
#include <nsIInputStream.h>
#include <nsIOutputStream.h>
#include <nsILineInputStream.h>
#include <nsNetUtil.h>
#include <nsStringGlue.h>
#include <msgCore.h>
#include <nsIStringBundle.h>

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

NS_IMETHODIMP
BeckySettingsImporter::AutoLocate(PRUnichar **aDescription NS_OUTPARAM,
                                  nsIFile **aLocation NS_OUTPARAM,
                                  PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aDescription);
  NS_ENSURE_ARG_POINTER(aLocation);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIStringBundle> bundle(dont_AddRef(BeckyStringBundle::GetStringBundleProxy()));
  if (bundle) {
    nsString description;
    BeckyStringBundle::GetStringByID(BECKYIMPORT_NAME, description, bundle);
    *aDescription = ToNewUnicode(description);
  }
  *aLocation = nsnull;
  *_retval = PR_FALSE;

  nsCOMPtr<nsIFile> location;
  nsresult rv = BeckyUtils::GetDefaultMailboxINIFile(getter_AddRefs(location));
  if (NS_FAILED(rv)) {
    location = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    return CallQueryInterface(location, aLocation);
  }

  *_retval = PR_TRUE;
  return CallQueryInterface(location, aLocation);
}

NS_IMETHODIMP
BeckySettingsImporter::SetLocation(nsIFile *aLocation)
{
  mLocation = aLocation;
  return NS_OK;
}

nsresult
BeckySettingsImporter::CreateParser(nsIINIParser **aParser)
{
  if (!mLocation) {
    nsresult rv = BeckyUtils::GetDefaultMailboxINIFile(getter_AddRefs(mLocation));
    if (NS_FAILED(rv))
      return rv;
  }

  nsresult rv;
  rv = BeckyUtils::ConvertToUTF8File(mLocation, getter_AddRefs(mConvertedFile));
  if (NS_FAILED(rv))
    return rv;

  return BeckyUtils::CreateINIParserForFile(mConvertedFile, aParser);
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
    if (NS_SUCCEEDED(rv)) {
      server->SetHostname(aServerName);
      server->SetUsername(aUserName);
    }
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
  rv = aParser->GetString(NS_LITERAL_CSTRING("Account"),
                          NS_LITERAL_CSTRING("SMTPPort"),
                          value);
  PRInt32 port = 25;
  if (NS_SUCCEEDED(rv)) {
    nsresult errorCode;
    port = static_cast<PRInt32>(value.ToInteger(&errorCode, 10));
  }
  server->SetPort(port);

  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("SSLSMTP"),
                     value);
  if (value.Equals("1"))
    server->SetSocketType(nsMsgSocketType::SSL);

  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("SMTPAUTH"),
                     value);
  if (value.Equals("1")) {
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("SMTPAUTHMODE"),
                       value);
    nsMsgAuthMethodValue authMethod = nsMsgAuthMethod::none;
    if (value.Equals("1")) {
      authMethod = nsMsgAuthMethod::passwordEncrypted;
    } else if (value.Equals("2") ||
               value.Equals("4") ||
               value.Equals("6")) {
      authMethod = nsMsgAuthMethod::passwordCleartext;
    } else {
      authMethod = nsMsgAuthMethod::anything;
    }
    server->SetAuthMethod(authMethod);
  }

  NS_IF_ADDREF(*aServer = server);

  return NS_OK;
}

static nsresult
SetPop3ServerProperties(nsIINIParser *aParser,
                        nsIMsgIncomingServer *aServer)
{
  nsCOMPtr<nsIPop3IncomingServer> pop3Server = do_QueryInterface(aServer);

  nsCAutoString value;
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("POP3Auth"),
                     value); // 0: plain, 1: APOP, 2: CRAM-MD5, 3: NTLM
  nsMsgAuthMethodValue authMethod = nsMsgAuthMethod::none;
  if (value.IsEmpty || value.Equals("0")) {
    authMethod = nsMsgAuthMethod::passwordCleartext;
  } else if (value.Equals("1")) {
    authMethod = nsMsgAuthMethod::old;
  } else if (value.Equals("2")) {
    authMethod = nsMsgAuthMethod::passwordEncrypted;
  } else if (value.Equals("3")) {
    authMethod = nsMsgAuthMethod::NTLM;
  }
  aServer->SetAuthMethod(authMethod);

  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("LeaveServer"),
                     value);
  if (value.Equals("1")) {
    pop3Server->SetLeaveMessagesOnServer(PR_TRUE);
    nsresult rv = aParser->GetString(NS_LITERAL_CSTRING("Account"),
                                     NS_LITERAL_CSTRING("KeepDays"),
                                     value);
    if (NS_SUCCEEDED(rv)) {
      nsresult errorCode;
      PRInt32 leftDays = static_cast<PRInt32>(value.ToInteger(&errorCode, 10));
      if (NS_SUCCEEDED(errorCode)) {
        pop3Server->SetNumDaysToLeaveOnServer(leftDays);
        pop3Server->SetDeleteByAgeFromServer(PR_TRUE);
      }
    }
  }

  return NS_OK;
}

static nsresult
CreateIncomingServer(nsIINIParser *aParser,
                     nsIMsgIncomingServer **aServer)
{
  nsCAutoString value;
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("Protocol"),
                     value);
  nsCString protocol;
  if (value.Equals("0")) {
    protocol = NS_LITERAL_CSTRING("pop3");
  } else if (value.Equals("1")) {
    protocol = NS_LITERAL_CSTRING("imap");
  } else {
    protocol = NS_LITERAL_CSTRING("pop3");
  }

  nsCAutoString userName, serverName;
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("MailServer"),
                     serverName);
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("UserID"),
                     userName);

  nsresult rv;
  nsCOMPtr<nsIMsgIncomingServer> server;
  rv = GetIncomingServer(userName, serverName, protocol, getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSecure = PR_FALSE;
  PRInt32 port = 0;
  nsresult errorCode;
  if (protocol.Equals("pop3")) {
    SetPop3ServerProperties(aParser, server);
    rv = aParser->GetString(NS_LITERAL_CSTRING("Account"),
                            NS_LITERAL_CSTRING("POP3Port"),
                            value);
    if (NS_SUCCEEDED(rv))
      port = static_cast<PRInt32>(value.ToInteger(&errorCode, 10));
    else
      port = 110;
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("SSLPOP"),
                       value);
    if (value.Equals("1"))
      isSecure = PR_TRUE;
  } else if (protocol.Equals("imap")) {
    rv = aParser->GetString(NS_LITERAL_CSTRING("Account"),
                            NS_LITERAL_CSTRING("IMAP4Port"),
                            value);
    if (NS_SUCCEEDED(rv))
      port = static_cast<PRInt32>(value.ToInteger(&errorCode, 10));
    else
      port = 143;
    aParser->GetString(NS_LITERAL_CSTRING("Account"),
                       NS_LITERAL_CSTRING("SSLIMAP"),
                       value);
    if (value.Equals("1"))
      isSecure = PR_TRUE;
  }

  server->SetPort(port);
  if (isSecure)
    server->SetSocketType(nsMsgSocketType::SSL);

  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("CheckInt"),
                     value);
  if (value.Equals("1"))
    server->SetDoBiff(PR_TRUE);
  rv = aParser->GetString(NS_LITERAL_CSTRING("Account"),
                          NS_LITERAL_CSTRING("CheckEvery"),
                          value);
  if (NS_SUCCEEDED(rv)) {
    PRInt32 minutes = static_cast<PRInt32>(value.ToInteger(&errorCode, 10));
    if (NS_SUCCEEDED(errorCode))
      server->SetBiffMinutes(minutes * 1);
  }

  NS_IF_ADDREF(*aServer = server);

  return NS_OK;
}

static nsresult
CreateIdentity(nsIINIParser *aParser,
               nsIMsgIdentity **aIdentity)
{
  nsCAutoString email, fullName, identityName, bccAddress;

  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("Name"),
                     identityName);
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("YourName"),
                     fullName);
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("MailAddress"),
                     email);
  aParser->GetString(NS_LITERAL_CSTRING("Account"),
                     NS_LITERAL_CSTRING("PermBcc"),
                     bccAddress);

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
  if (!bccAddress.IsEmpty()) {
    identity->SetDoBcc(PR_TRUE);
    identity->SetDoBccList(bccAddress);
  }

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
    mConvertedFile = nsnull;
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

