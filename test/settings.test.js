var description = 'Setting component tests'
var gSettings;
var actualAccount;
var expectedAccount;

function setUp() {
  actuaAccount = null;
  expectedAccount = null;
}

function removeAccount(account) {
  if (account) {
    let accountManager = Cc["@mozilla.org/messenger/account-manager;1"].getService(Ci.nsIMsgAccountManager);
    accountManager.removeAccount(account);
    actuaAccount = null;
  }
}

function tearDown() {
  removeAccount(actualAccount);
  removeAccount(expectedAccount);
}

assert.equalIncomingServer = function(expected, actual) {
  assert.isInstanceOf(Ci.nsIMsgIncomingServer, actual);
  assert.equals(expected.type, actual.type);
  assert.equals(expected.hostName, actual.hostName);
  assert.equals(expected.port, actual.port);
  assert.equals(expected.username, actual.username);
  assert.equals(expected.isSecure, actual.isSecure);
  assert.equals(expected.authMethod, actual.authMethod);
  assert.equals(expected.socketType, actual.socketType);
  assert.equals(expected.emptyTrashOnExit, actual.emptyTrashOnExit);
}

assert.equalIdentity = function(expected, actual) {
  assert.isInstanceOf(Ci.nsIMsgIdentity, actual);
  assert.equals(expected.fullName, actual.fullName);
  assert.equals(expected.email, actual.email);
}

assert.equalIdentities = function(expected, actual) {
  assert.isInstanceOf(Ci.nsISupportsArray, actual);
  assert.equals(expected.Count(), actual.Count());
}

assert.equalAccount = function(expected, actual) {
  assert.isInstanceOf(Ci.nsIMsgAccount, actual);
  assert.equalIncomingServer(expected.incomingServer, actual.incomingServer);
  assert.equalIdentities(expected.identities, actual.identities);
  assert.equalIdentity(expected.defaultIdentity, actual.defaultIdentity);
  assert.equals(expected.toString(), actual.toString());
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gSettings = Cc["@mozilla-japan.org/import/becky/settings;1"].getService(Ci.nsIImportSettings);
  assert.isDefined(gSettings);
}

testAutoLocate.description = "AutoLocate test";
testAutoLocate.priority = 'must';
function testAutoLocate() {
  testCreate();
  let descriptionContainer = {};
  let fileContainer = {};
  assert.isFalse(gSettings.AutoLocate(descriptionContainer, fileContainer));
}

testSetLocation.description = "SetLocation test";
testSetLocation.priority = 'must';
function testSetLocation() {
  testCreate();
  gSettings.SetLocation(utils.normalizeToFile(utils.baseURL + 'fixtures/settings/Mailbox.ini'));
}

function createExpectedIncomingServer() {
  let incomingServer = Cc["@mozilla.org/messenger/server;1?type=imap"].createInstance(Ci.nsIMsgIncomingServer);
  incomingServer.key = "server2";
  incomingServer.type = "imap";
  incomingServer.hostName = "mail-server.expample.com";
  incomingServer.port = 993;
  incomingServer.username = "lonesome";
  incomingServer.socketType = Ci.nsMsgSocketType.SSL;
  incomingServer.authMethod = Ci.nsMsgAuthMethod.passwordCleartext;
  return incomingServer;
}

function createExpectedIdentity() {
  let expected = Cc["@mozilla.org/messenger/identity;1"].createInstance(Ci.nsIMsgIdentity);
  expected.key = "id1";
  expected.fullName = "ジョージ";
  expected.email = "lonesome@example.com";

  return expected;
}

function createExpectedAccount() {
  let expected = Cc["@mozilla.org/messenger/account;1"].createInstance(Ci.nsIMsgAccount);
  expected.key = "becky-import-test-account";
  expected.incomingServer = createExpectedIncomingServer();
  expected.addIdentity(createExpectedIdentity());

  return expected;
}

testImport.description = "import test";
testImport.priority = 'must';
function testImport() {
  testSetLocation();
  let container = {};
  assert.isTrue(gSettings.Import(container));
  assert.isDefined(container.value);

  actualAccount = container.value;
  expectedAccount = createExpectedAccount();
  assert.equalAccount(expectedAccount, actualAccount);
}

