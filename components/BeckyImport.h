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

#ifndef BeckyImport_h___
#define BeckyImport_h___

#include <nsIImportModule.h>

#define MJ_BECKYIMPORT_CID          \
{                                   \
  0xf7a0d2c3, 0x61ea, 0x4754,       \
  {0x9b, 0xc3, 0xd9, 0x48, 0xf3, 0x6b, 0x4c, 0xbb}}

#define kBeckySupportsString NS_IMPORT_MAIL_STR "," NS_IMPORT_ADDRESS_STR "," NS_IMPORT_SETTINGS_STR "," NS_IMPORT_FILTERS_STR

class BeckyImport : public nsIImportModule
{
public:
  BeckyImport();
  virtual ~BeckyImport();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMPORTMODULE

private:
  nsresult GetMailImportInterface(nsISupports **aInterface);
  nsresult GetAddressBookImportInterface(nsISupports **aInterface);
  nsresult GetSettingsImportInterface(nsISupports **aInterface);
  nsresult GetFiltersImportInterface(nsISupports **aInterface);

};

#endif /* BeckyImport_h___ */
