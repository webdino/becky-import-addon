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
#include <nsIGenericFactory.h>
#include <nsICategoryManager.h>
#include <nsServiceManagerUtils.h>
#include <nsCOMPtr.h>

#include "BeckyMailImporter.h"
#include "BeckyImport.h"
#include "BeckyStringBundle.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(BeckyMailImporter)
NS_GENERIC_FACTORY_CONSTRUCTOR(BeckyImport)
static NS_DEFINE_CID(kMJ_BECKYIMPORT_CID, MJ_BECKYIMPORT_CID);

NS_METHOD
BeckyRegister(nsIComponentManager *aCompManager,
              nsIFile *aPath,
              const char *aRegistryLocation,
              const char *aComponentType,
              const nsModuleComponentInfo *aInfo)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCString replace;
    char *theCID = kMJ_BECKYIMPORT_CID.ToString();
    rv = catMan->AddCategoryEntry("mailnewsimport",
                                  theCID,
                                  kBeckySupportsString,
                                  PR_TRUE,
                                  PR_TRUE,
                                  getter_Copies(replace));
    NS_Free(theCID);
  }

  return rv;
}

static const nsModuleComponentInfo components[] = {
  { "Becky! Import Component", MJ_BECKYIMPORT_CID,
    "@mozilla.org/import/becky;1", BeckyImportConstructor,
    BeckyRegister, nsnull }
};

static void
importModuleDtor(nsIModule* self)
{
  BeckyStringBundle::Cleanup();
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsImportServiceModule,
                              components,
                              importModuleDtor) 
