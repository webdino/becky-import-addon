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
#include <nsILineInputStream.h>
#include <nsIStringBundle.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIMsgFilter.h>
#include <nsIMsgFilterList.h>
#include <nsIMsgAccountManager.h>
#include <nsIMsgAccount.h>
#include <nsIMsgFolder.h>
#include <nsCOMPtr.h>
#include <nsMsgSearchCore.h>
#include <nsMsgBaseCID.h>

#include "BeckyFiltersImporter.h"
#include "BeckyStringBundle.h"
#include "BeckyUtils.h"

NS_IMPL_ISUPPORTS1(BeckyFiltersImporter, nsIImportFilters)

BeckyFiltersImporter::BeckyFiltersImporter()
: mLocation(nsnull),
  mServer(nsnull)
{
  /* member initializers and constructor code */
}

BeckyFiltersImporter::~BeckyFiltersImporter()
{
  /* destructor code */
}

nsresult
BeckyFiltersImporter::GetDefaultFilterFile(nsIFile **aFile)
{
  nsresult rv;
  nsCOMPtr<nsIFile> filter;
  rv = BeckyUtils::GetDefaultMailboxDirectory(getter_AddRefs(filter));
  if (NS_FAILED(rv))
    return rv;

  rv = filter->AppendNative(NS_LITERAL_CSTRING("IFilter.def"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = filter->Exists(&exists);
  if (exists)
    return CallQueryInterface(filter, aFile);

  return NS_ERROR_FILE_NOT_FOUND;
}

NS_IMETHODIMP
BeckyFiltersImporter::AutoLocate(PRUnichar **aDescription NS_OUTPARAM,
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

  nsresult rv;
  nsCOMPtr<nsIFile> location;
  rv = GetDefaultFilterFile(getter_AddRefs(location));
  if (NS_FAILED(rv)) {
    location = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    return CallQueryInterface(location, aLocation);
  }

  *_retval = PR_TRUE;
  return CallQueryInterface(location, aLocation);
}

NS_IMETHODIMP
BeckyFiltersImporter::SetLocation(nsIFile *aLocation)
{
  mLocation = aLocation;
  return NS_OK;
}

static nsMsgSearchAttribValue
ConvertSearchKeyToAttrib(const nsDependentCSubstring &aKey)
{
  if (aKey.Equals("From") ||
      aKey.Equals("Sender") ||
      aKey.Equals("From, Sender, X-Sender")) {
    return nsMsgSearchAttrib::Sender;
  } else if (aKey.Equals("Subject")) {
    return nsMsgSearchAttrib::Subject;
  } else if (aKey.Equals("[body]")) {
    return nsMsgSearchAttrib::Body;
  } else if (aKey.Equals("Date")) {
    return nsMsgSearchAttrib::Date;
  } else if (aKey.Equals("To")) {
    return nsMsgSearchAttrib::To;
  } else if (aKey.Equals("Cc")) {
    return nsMsgSearchAttrib::CC;
  } else if (aKey.Equals("To,  Cc,  Bcc:")) {
    return nsMsgSearchAttrib::ToOrCC;
  }
  return -1;
}

static nsMsgSearchOpValue
ConvertSearchFlagsToOperator(const nsDependentCSubstring &aFlags)
{
  PRInt32 lastTabPosition = aFlags.RFindChar('\t');
  if (lastTabPosition < -1 ||
     aFlags.Length() == static_cast<PRUint32>(lastTabPosition)) {
    return NS_ERROR_FAILURE;
  }

  nsDependentCSubstring operatorFlags(aFlags, lastTabPosition + 1);
  nsDependentCSubstring negativeFlag(aFlags, 0, aFlags.FindChar('\t') - 1);

  switch (negativeFlag.CharAt(0)) {
  case 'X':
    return nsMsgSearchOp::DoesntContain;
    break;
  case 'O':
    if (operatorFlags.FindChar('T') >= 0)
      return nsMsgSearchOp::BeginsWith;
    else
      return nsMsgSearchOp::Contains;
    break;
  default:
    return -1;
  }

  return -1;
}

nsresult
BeckyFiltersImporter::ParseRuleLine(const nsCString &aLine,
                                    nsMsgSearchAttribValue *aSearchAttribute NS_OUTPARAM,
                                    nsMsgSearchOpValue *aSearchOperator NS_OUTPARAM,
                                    nsString &aSearchKeyword NS_OUTPARAM)
{
  PRInt32 firstColonPosition = aLine.FindChar(':');
  if (firstColonPosition < -1 ||
      aLine.Length() == static_cast<PRUint32>(firstColonPosition)) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 secondColonPosition = aLine.FindChar(':', firstColonPosition + 1);
  if (secondColonPosition < -1 ||
      aLine.Length() == static_cast<PRUint32>(secondColonPosition)) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 length = secondColonPosition - firstColonPosition - 1;
  nsMsgSearchAttribValue searchAttribute;
  searchAttribute = ConvertSearchKeyToAttrib(nsDependentCSubstring(aLine, firstColonPosition + 1, length));
  if (searchAttribute < 0)
    return NS_ERROR_FAILURE;

  PRInt32 tabPosition = aLine.FindChar('\t');
  if (tabPosition < -1 ||
      aLine.Length() == static_cast<PRUint32>(tabPosition)) {
    return NS_ERROR_FAILURE;
  }

  nsMsgSearchOpValue searchOperator;
  searchOperator = ConvertSearchFlagsToOperator(nsDependentCSubstring(aLine, tabPosition + 1));
  if (searchOperator < 0)
    return NS_ERROR_FAILURE;


  *aSearchOperator = searchOperator;
  *aSearchAttribute = searchAttribute;
  length = tabPosition - secondColonPosition - 1;
  nsDependentCSubstring nativeString(aLine, secondColonPosition + 1, length);
  return BeckyUtils::ConvertNativeStringToUTF16(nativeString, aSearchKeyword);
}

nsresult
BeckyFiltersImporter::SetSearchTerm(const nsCString &aLine, nsIMsgFilter *aFilter)
{
  if (!aFilter)
    return NS_ERROR_FAILURE;

  nsresult rv;
  nsMsgSearchAttribValue searchAttribute = -1;
  nsMsgSearchOpValue searchOperator = -1;
  nsAutoString searchKeyword;
  rv = ParseRuleLine(aLine, &searchAttribute, &searchOperator, searchKeyword);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgSearchTerm> term;
  rv = aFilter->CreateTerm(getter_AddRefs(term));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = term->SetAttrib(searchAttribute);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = term->SetOp(searchOperator);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgSearchValue> value;
  rv = term->GetValue(getter_AddRefs(value));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = value->SetAttrib(searchAttribute);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = value->SetStr(searchKeyword);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = term->SetValue(value);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = term->SetBooleanAnd(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!searchKeyword.IsEmpty())
    rv = aFilter->SetFilterName(searchKeyword);
  else
    rv = aFilter->SetFilterName(NS_LITERAL_STRING("No name"));
  NS_ENSURE_SUCCESS(rv, rv);

  return aFilter->AppendTerm(term);
}

nsresult
BeckyFiltersImporter::CreateRuleAction(nsIMsgFilter *aFilter,
                                       nsMsgRuleActionType actionType,
                                       nsIMsgRuleAction **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsIMsgRuleAction> action;
  rv = aFilter->CreateAction(getter_AddRefs(action));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = action->SetType(actionType);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*_retval = action);
  return NS_OK;
}

nsresult
BeckyFiltersImporter::GetActionTarget(const nsCString &aLine, nsCString &aTarget NS_OUTPARAM)
{
  PRInt32 firstColonPosition = aLine.FindChar(':');
  if (firstColonPosition < -1 ||
      aLine.Length() == static_cast<PRUint32>(firstColonPosition)) {
    return NS_ERROR_FAILURE;
  }

  aTarget.Assign(nsDependentCSubstring(aLine, firstColonPosition + 1));

  return NS_OK;
}

nsresult
BeckyFiltersImporter::GetResendTarget(const nsCString &aLine,
                                      nsCString &aTemplate NS_OUTPARAM,
                                      nsCString &aTargetAddress NS_OUTPARAM)
{
  nsresult rv;
  nsCAutoString target;
  rv = GetActionTarget(aLine, target);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 asteriskPosition = target.FindChar('*');
  if (asteriskPosition < 0) {
    aTemplate.Assign(target);
    return NS_OK;
  }

  if (target.Length() == static_cast<PRUint32>(asteriskPosition))
    return NS_ERROR_FAILURE;

  aTemplate.Assign(nsDependentCSubstring(target, 0, asteriskPosition - 1));
  aTargetAddress.Assign(nsDependentCSubstring(target, asteriskPosition + 1));

  return NS_OK;
}

nsresult
BeckyFiltersImporter::CreateResendAction(const nsCString &aLine,
                                         nsIMsgFilter *aFilter,
                                         const nsMsgRuleActionType &aActionType,
                                         nsIMsgRuleAction **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsIMsgRuleAction> action;
  rv = CreateRuleAction(aFilter, aActionType, getter_AddRefs(action));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString templateString;
  nsCAutoString targetAddress;
  rv = GetResendTarget(aLine, templateString, targetAddress);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aActionType == nsMsgFilterAction::Forward)
    rv = action->SetStrValue(targetAddress);
  else
    rv = action->SetStrValue(templateString);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = action);
  return NS_OK;
}

static nsresult
GetFolderName(const nsCString &aTarget, nsCString &aName)
{
  PRInt32 backslashPosition = aTarget.RFindChar('\\');
  if (backslashPosition > 0)
    aName.Assign(nsDependentCSubstring(aTarget, backslashPosition + 1));

  return NS_OK;
}

nsresult
BeckyFiltersImporter::GetDistributeTarget(const nsCString &aLine,
                                          nsCString &aTargetFolder NS_OUTPARAM)
{
  nsresult rv;
  nsCAutoString target;
  rv = GetActionTarget(aLine, target);
  NS_ENSURE_SUCCESS(rv, rv);

  target.Trim("\\", PR_FALSE, PR_TRUE);
  rv = GetFolderName(target, target);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr <nsIMsgFolder> folder;
  rv = FindMessageFolder(target, getter_AddRefs(folder));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString folderURL;
  if (!folder) {
    rv = mServer->GetRootMsgFolder(getter_AddRefs(folder));
    if (NS_FAILED(rv))
      return rv;
  }
  rv = folder->GetFolderURL(folderURL);
  NS_ENSURE_SUCCESS(rv, rv);

  aTargetFolder.Assign(folderURL);

  return NS_OK;
}

nsresult
BeckyFiltersImporter::CreateDistributeAction(const nsCString &aLine,
                                             nsIMsgFilter *aFilter,
                                             const nsMsgRuleActionType &aActionType,
                                             nsIMsgRuleAction **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr<nsIMsgRuleAction> action;
  rv = CreateRuleAction(aFilter, aActionType, getter_AddRefs(action));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString targetFolder;
  rv = GetDistributeTarget(aLine, targetFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = action->SetTargetFolderUri(targetFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = action);
  return NS_OK;
}

nsresult
BeckyFiltersImporter::CreateLeaveOrDeleteAction(const nsCString &aLine,
                                                nsIMsgFilter *aFilter,
                                                nsIMsgRuleAction **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsMsgRuleActionType actionType;
  if (aLine.CharAt(3) == '0') {
    actionType = nsMsgFilterAction::LeaveOnPop3Server;
  } else if (aLine.CharAt(3) == '1') {
    if (aLine.CharAt(5) == '1')
      actionType = nsMsgFilterAction::Delete;
    else
      actionType = nsMsgFilterAction::DeleteFromPop3Server;
  } else {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIMsgRuleAction> action;
  rv = CreateRuleAction(aFilter, actionType, getter_AddRefs(action));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = action);
  return NS_OK;
}

nsresult
BeckyFiltersImporter::SetRuleAction(const nsCString &aLine, nsIMsgFilter *aFilter)
{
  if (!aFilter || aLine.Length() < 4)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIMsgRuleAction> action;
  switch (aLine.CharAt(1)) {
  case 'R': // Reply
    rv = CreateResendAction(aLine,
                            aFilter,
                            nsMsgFilterAction::Reply,
                            getter_AddRefs(action));
    break;
  case 'F': // Forward
    rv = CreateResendAction(aLine,
                            aFilter,
                            nsMsgFilterAction::Forward,
                            getter_AddRefs(action));
    break;
  case 'L': // Leave or delete
    rv = CreateLeaveOrDeleteAction(aLine, aFilter, getter_AddRefs(action));
    break;
  case 'Y': // Copy
    rv = CreateDistributeAction(aLine,
                                aFilter,
                                nsMsgFilterAction::CopyToFolder,
                                getter_AddRefs(action));
    break;
  case 'M': // Move
    rv = CreateDistributeAction(aLine,
                                aFilter,
                                nsMsgFilterAction::MoveToFolder,
                                getter_AddRefs(action));
    break;
  case 'G': // Set flag
    if (aLine.CharAt(3) == 'R') { // Read
      rv = CreateRuleAction(aFilter, nsMsgFilterAction::MarkRead, getter_AddRefs(action));
    }
    break;
  default:
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (action) {
    rv = aFilter->AppendAction(action);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
BeckyFiltersImporter::CreateFilter(nsIMsgFilter **_retval NS_OUTPARAM)
{
  if (!mServer)
    return NS_ERROR_FAILURE;
  nsresult rv;
  nsCOMPtr <nsIMsgFilterList> filterList;
  rv = mServer->GetFilterList(nsnull, getter_AddRefs(filterList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgFilter> filter;
  rv = filterList->CreateFilter(NS_LITERAL_STRING(""), getter_AddRefs(filter));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = filter);

  return NS_OK;
}

nsresult
BeckyFiltersImporter::AppendFilter(nsIMsgFilter *aFilter)
{
  nsresult rv;
  if (!mServer)
    return NS_ERROR_FAILURE;

  nsCOMPtr <nsIMsgFilterList> filterList;
  rv = mServer->GetFilterList(nsnull, getter_AddRefs(filterList));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count;
  rv = filterList->GetFilterCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  return filterList->InsertFilterAt(count, aFilter);
}

nsresult
BeckyFiltersImporter::ParseFilterFile(nsIFile *aFile)
{
  nsresult rv;
  nsCOMPtr<nsILineInputStream> lineStream;
  rv = BeckyUtils::CreateLineInputStream(aFile, getter_AddRefs(lineStream));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more = PR_TRUE;
  nsCAutoString line;

  nsCOMPtr<nsIMsgFilter> filter;
  while (NS_SUCCEEDED(rv) && more) {
    rv = lineStream->ReadLine(line, &more);

    switch (line.CharAt(0)) {
    case ':':
      if (line.Equals(NS_LITERAL_CSTRING(":Begin \"\""))) {
        CreateFilter(getter_AddRefs(filter));
      } else if (line.Equals(NS_LITERAL_CSTRING(":End \"\""))) {
        if (filter)
          AppendFilter(filter);
        filter = nsnull;
      } else {
        continue;
      }
      break;
    case '!':
      SetRuleAction(line, filter);
      break;
    case '@':
      SetSearchTerm(line, filter);
      break;
    case '$': // $X: disabled
      if (line.Length() > 2 &&
          line.CharAt(1) == 'X' &&
          filter) {
        filter->SetEnabled(PR_FALSE);
      }
      break;
    default:
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
BeckyFiltersImporter::Import(PRUnichar **aError NS_OUTPARAM, PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aError);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_FALSE;

  nsresult rv;
  if (!mLocation && NS_FAILED(GetDefaultFilterFile(getter_AddRefs(mLocation))))
    return NS_ERROR_FILE_NOT_FOUND;

  rv = CollectServers();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ParseFilterFile(mLocation);
  if (NS_SUCCEEDED(rv))
    *_retval = PR_TRUE;

  return rv;
}

nsresult
BeckyFiltersImporter::FindMessageFolderInServer(const nsCString &aName,
                                                nsIMsgIncomingServer *aServer,
                                                nsIMsgFolder **_retval NS_OUTPARAM)
{
  nsresult rv;
  nsCOMPtr <nsIMsgFolder> rootFolder;
  rv = aServer->GetRootMsgFolder(getter_AddRefs(rootFolder));
  NS_ENSURE_SUCCESS(rv, rv);

  return rootFolder->GetChildNamed(NS_ConvertUTF8toUTF16(aName), _retval);
}

nsresult
BeckyFiltersImporter::FindMessageFolder(const nsCString &aName, nsIMsgFolder **_retval NS_OUTPARAM)
{
  nsresult rv;

  nsCOMPtr<nsIMsgAccountManager> accountManager;
  accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsArray> accounts;
  rv = accountManager->GetAccounts(getter_AddRefs(accounts));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 accountCount;
  rv = accounts->Count(&accountCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgFolder> found;
  for (PRUint32 i = 0; i < accountCount; i++) {
    nsCOMPtr<nsIMsgAccount> account(do_QueryElementAt(accounts, i));
    if (!account)
      continue;

    nsCOMPtr<nsIMsgIncomingServer> server;
    account->GetIncomingServer(getter_AddRefs(server));
    if (!server)
      continue;
    FindMessageFolderInServer(aName, server,getter_AddRefs(found));
    if (found)
      break;
  }

  if (!found) {
    nsCOMPtr<nsIMsgIncomingServer> server;
    rv = accountManager->GetLocalFoldersServer(getter_AddRefs(server));
    NS_ENSURE_SUCCESS(rv, rv);

    FindMessageFolderInServer(aName, server,getter_AddRefs(found));
  }

  NS_IF_ADDREF(*_retval = found);

  return NS_OK;
}

nsresult
BeckyFiltersImporter::CollectServers()
{
  nsresult rv;
  nsCOMPtr<nsIMsgAccountManager> accountManager;
  accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgAccount> defaultAccount;
  rv = accountManager->GetDefaultAccount(getter_AddRefs(defaultAccount));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgIncomingServer> server;
  defaultAccount->GetIncomingServer(getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(mServer = server);

  return NS_OK;
}

