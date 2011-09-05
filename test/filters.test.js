var description = 'Filters component tests'
var gFilters;
var originalFilterListCount;

function setUp() {
  let originalFilterList = getFilterList();
  originalFilterListCount = originalFilterList.filterCount;
}

function tearDown() {
  let filterList = getFilterList();
  let filterCount = filterList.filterCount;
  if (filterCount > originalFilterListCount) {
    for (let i = filterCount - 1;
         i >= originalFilterListCount;
         i--) {
      filterList.removeFilterAt(i);
    }
  }
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gFilters = Cc["@mozilla-japan.org/import/becky/filters;1"].getService(Ci.nsIImportFilters);
  assert.isDefined(gFilters);
}

testAutoLocate.description = "AutoLocate instance test";
testAutoLocate.priority = 'must';
function testAutoLocate() {
  testCreate();
  let location = {};
  let description = {};
  assert.isTrue(gFilters.AutoLocate(description, location));
}

testSetLocation.description = "SetLocation test";
testSetLocation.priority = 'must';
function testSetLocation() {
  testCreate();
  gFilters.SetLocation(utils.normalizeToFile(utils.baseURL + 'fixtures/filters/IFilter.def'));
}

assert.equalSearchTerm = function(expected, actual, message) {
  assert.equal(expected.attrib, actual.attrib, message);
  assert.equal(expected.op, actual.op, message);
  assert.equal(expected.value.attrib, actual.value.attrib, message);
  assert.equal(expected.value.str, actual.value.str, message);
}

assert.equalSearchTerms = function(expected, actual, message) {
  assert.equals(expected.Count(), actual.Count(), message);
  for (let i = 0; i < expected.Count(); i++) {
    let expectedSearchTerm = expected.GetElementAt(i).QueryInterface(Ci.nsIMsgSearchTerm);
    let actualSearchTerm = actual.GetElementAt(i).QueryInterface(Ci.nsIMsgSearchTerm);
    assert.equalSearchTerm(expectedSearchTerm, actualSearchTerm, message);
  }
}

assert.equalAction = function(expected, actual) {
  assert.equals(expected.type, actual.type);

  if (expected.type == Ci.nsMsgFilterAction.MoveToFolder ||
      expected.type == Ci.nsMsgFilterAction.CopyToFolder) {
    assert.equals(expected.targetFolderUri, actual.targetFolderUri);
  } else if (expected.type == Ci.nsMsgFilterAction.Forward ||
             expected.type == Ci.nsMsgFilterAction.Reply) {
    assert.equals(expected.strValue, actual.strValue);
  }
}

assert.equalActionList = function(expected, actual) {
  assert.equals(expected.Count(), actual.Count());
  for (let i = 0; i < expected.Count(); i++) {
    let expectedAction = expected.GetElementAt(i).QueryInterface(Ci.nsIMsgRuleAction);
    let actualAction = actual.GetElementAt(i).QueryInterface(Ci.nsIMsgRuleAction);
    assert.equalAction(expectedAction, actualAction);
  }
}

assert.equalFilter = function(expected, actual) {
  assert.equals(expected.filterType, actual.filterType);
  assert.equalActionList(expected.actionList, actual.actionList);
  assert.equalSearchTerms(expected.searchTerms, actual.searchTerms, expected.filterName);
}

assert.equalFilters = function(expected, actual) {
  assert.equals(expected.length, actual.filterCount);

  for (let i = 0; i < expected.length; i++) {
    let expectedFilter = expected[i];
    let actualFilter = actual.getFilterAt(i + originalFilterListCount);
    assert.equalFilter(expectedFilter, actualFilter);
  }
}

function assertContainFilters(expected) {
  let actualFilters = getFilterList();
  assert.equalFilters(expected, actualFilters);
}

function getDefaultServer() {
  let accountManager = Cc["@mozilla.org/messenger/account-manager;1"].getService(Ci.nsIMsgAccountManager);
  let account = accountManager.defaultAccount;
  return account.incomingServer;
}

function getFilterList() {
  let server = getDefaultServer();
  return server.getFilterList(null);
}

function getRootFolderURL() {
  let server = getDefaultServer();
  return server.rootMsgFolder.URI;
}

var expectedFilters = [
  [ '1', [ Ci.nsMsgFilterAction.Reply, 'template' ], [ Ci.nsMsgSearchAttrib.CC, Ci.nsMsgSearchOp.DoesntContain, 'reply-to' ] ],
  [ '2', [ Ci.nsMsgFilterAction.MarkRead ], [ Ci.nsMsgSearchAttrib.To, Ci.nsMsgSearchOp.Contains, 'mozilla' ] ],
  [ '3', [ Ci.nsMsgFilterAction.Forward, 'forward@example.com' ], [ Ci.nsMsgSearchAttrib.Sender, Ci.nsMsgSearchOp.DoesntContain, 'ikezoe' ] ],
  [ '4', [ Ci.nsMsgFilterAction.MoveToFolder, 'mozilla' ], [ Ci.nsMsgSearchAttrib.To, Ci.nsMsgSearchOp.BeginsWith, 'hello' ] ],
];

function createAction(filter, properties) {
  let action = filter.createAction();
  action.type = properties[0];
  if (properties[1]) {
    if (action.type == Ci.nsMsgFilterAction.MoveToFolder ||
        action.type == Ci.nsMsgFilterAction.CopyToFolder) {
      action.targetFolderUri = getRootFolderURL() + "/" + properties[1];
    } else if (action.type == Ci.nsMsgFilterAction.Forward ||
               action.type == Ci.nsMsgFilterAction.Reply) {
      action.strValue = properties[1];
    }
  }

  return action;
}

function createSearchTerm(filter, properties) {
  let term = filter.createTerm();
  term.attrib = properties[0];
  term.op = properties[1];
  let value = term.value;
  value.attrib = properties[0];
  value.str = properties[2];
  term.value = value;

  return term;
}

function createFilter(filterList, properties) {
  let filter = filterList.createFilter(properties[0]);

  let action = createAction(filter, properties[1]);
  filter.appendAction(action);

  let term = createSearchTerm(filter, properties[2]);
  filter.appendTerm(term);

  return filter;
}

function createExpectedFilters() {
  let filters = [];
  let filterList = getFilterList();

  for (let i = 0; i < expectedFilters.length; i++)
    filters.push(createFilter(filterList, expectedFilters[i]));

  return filters;
}

testImport.description = "Import test";
testImport.priority = 'must';
function testImport() {
  testSetLocation();
  let errorString = {}
  assert.isTrue(gFilters.Import(errorString));

  let expected = createExpectedFilters();
  assertContainFilters(expected);
}

