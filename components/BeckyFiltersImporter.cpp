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
#include <nsIStringBundle.h>
#include <nsComponentManagerUtils.h>
#include <nsCOMPtr.h>

#include "BeckyFiltersImporter.h"
#include "BeckyStringBundle.h"
#include "BeckyUtils.h"

NS_IMPL_ISUPPORTS1(BeckyFiltersImporter, nsIImportFilters)

BeckyFiltersImporter::BeckyFiltersImporter()
: mLocation(nsnull)
{
  /* member initializers and constructor code */
}

BeckyFiltersImporter::~BeckyFiltersImporter()
{
  /* destructor code */
}

static nsresult
GetDefaultFilterFile(nsIFile **aFile)
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

NS_IMETHODIMP
BeckyFiltersImporter::Import(PRUnichar **aError NS_OUTPARAM, PRBool *_retval NS_OUTPARAM)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

