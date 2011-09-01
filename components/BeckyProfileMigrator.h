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

#ifndef BeckyProfileMigrator_h___
#define BeckyProfileMigrator_h___

#include <nsIMailProfileMigrator.h>
#include <nsITimer.h>
#include <nsIObserverService.h>
#include <nsIImportModule.h>
#include <nsIImportGeneric.h>

#define MJ_BECKY_PROFILE_MIGRATOR_CONTRACT_ID \
  "@mozilla.org/profile/migrator;1?app=mail&type=becky"

#define MJ_BECKY_PROFILE_MIGRATOR_CID               \
{                                                   \
  0xca226414, 0x03fa, 0x4c50,                       \
  {0x9a, 0xc7, 0x06, 0x70, 0xd1, 0x85, 0x44, 0xe2}}

class BeckyProfileMigrator : public nsIMailProfileMigrator,
                             public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMAILPROFILEMIGRATOR
  NS_DECL_NSITIMERCALLBACK

  BeckyProfileMigrator();
  virtual ~BeckyProfileMigrator();

  virtual nsresult ContinueImport();

private:
  nsresult ImportSettings();
  nsresult ImportAddressBook();
  nsresult ImportMailData();
  nsresult ImportFilters();
  nsresult FinishCopyingAddressBookData();
  nsresult FinishCopyingMailFolders();

  nsCOMPtr<nsITimer> mFileIOTimer;
  nsCOMPtr<nsIObserverService> mObserverService;
  nsCOMPtr<nsIImportModule> mImportModule;
  nsCOMPtr<nsIImportGeneric> mGenericImporter;
  PRBool mProcessingMailFolders;
};

#endif /* BeckyProfileMigrator_h___ */
