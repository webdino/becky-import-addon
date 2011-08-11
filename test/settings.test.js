var description = 'Setting component tests'
var gSettings;
var actualAccount;

function setUp() {
  actuaAccount = null;
}

function tearDown() {
  if (actualAccount) {
    var accountManager = Cc["@mozilla.org/messenger/account-manager;1"].getService(Ci.nsIMsgAccountManager);
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

function createExpectedAccount() {
  var expected = Cc["@mozilla.org/messenger/account;1"].createInstance(Ci.nsIMsgAccount);
  expected.key = "becky-import-test-account";
  var incomingServer = Cc["@mozilla.org/messenger/server;1?type=pop3"].createInstance(Ci.nsIMsgIncomingServer);
  incomingServer.key = "server2";
  expected.incomingServer = incomingServer;

  return expected;
}

testSetLocation.description = "SetLocation test";
testSetLocation.priority = 'must';
function testSetLocation() {
  testCreate();
  assert.notRaises(
    Cr.NS_ERROR_FAILURE,
    function() {
      gSettings.SetLocation(utils.normalizeToFile(utils.baseURL));
    },
    {}
  );
}

testImport.description = "import test";
testImport.priority = 'must';
function testImport() {
  testSetLocation();
  var container = {};
  assert.isTrue(gSettings.Import(container));
  assert.isDefined(container.value);

  actualAccount = container.value;
  var expected = createExpectedAccount();
  assert.equalAccount(expected, actualAccount);
}

