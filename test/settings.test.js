var description = 'Setting component tests'
var gSettings;
var actualAccount;

function setUp() {
  actuaAccount = null;
}

function tearDown() {
  if (actualAccount) {
    let accountManager = Cc["@mozilla.org/messenger/account-manager;1"].getService(Ci.nsIMsgAccountManager);
    accountManager.removeAccount(actualAccount);
  }
}

assert.equalIncomingServer = function(expected, actual) {
  assert.isInstanceOf(Ci.nsIMsgIncomingServer, actual);
  assert.equals(expected.type, actual.type);
  assert.equals(expected.hostName, actual.hostName);
  assert.equals(expected.port, actual.port);
  assert.equals(expected.username, actual.username);
  assert.equals(expected.password, actual.password);
  assert.equals(expected.isSecure, actual.isSecure);
  assert.equals(expected.authMethod, actual.authMethod);
  assert.equals(expected.socketType, actual.socketType);
  assert.equals(expected.emptyTrashOnExit, actual.emptyTrashOnExit);
}

assert.equalAccount = function(expected, actual) {
  assert.isInstanceOf(Ci.nsIMsgAccount, actual);
  assert.equalIncomingServer(expected.incomingServer, actual.incomingServer);
  assert.equals(expected.identities, actual.identities);
  assert.equals(expected.defaultIdentity, actual.defaultIdentity);
  assert.equals(expected.toString(), actual.toString());
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gSettings = Cc["@mozilla-japan.org/import/becky/settings;1"].getService(Ci.nsIImportSettings);
  assert.isDefined(gSettings);
}

testSetLocation.description = "SetLocation test";
testSetLocation.priority = 'must';
function testSetLocation() {
  testCreate();
  gSettings.SetLocation(utils.normalizeToFile(utils.baseURL));
}

function createExpectedAccount() {
  let expected = Cc["@mozilla.org/messenger/account;1"].createInstance(Ci.nsIMsgAccount);
  expected.key = "becky-import-test-account";
  let incomingServer = Cc["@mozilla.org/messenger/server;1?type=pop3"].createInstance(Ci.nsIMsgIncomingServer);
  incomingServer.key = "server2";
  expected.incomingServer = incomingServer;

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
  let expected = createExpectedAccount();
  assert.equalAccount(expected, actualAccount);
}

