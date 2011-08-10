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

assert.equalAccount = function(expected, actual) {
  assert.isInstanceOf(Ci.nsIMsgAccount, actual);
  assert.equals(expected.toString(), actual.toString());
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gSettings = Cc["@mozilla-japan.org/import/becky/settings;1"].getService(Ci.nsIImportSettings);
  assert.isDefined(gSettings);
}

testImport.description = "import test";
testImport.priority = 'must';
function testImport() {
  testCreate();
  var container = {};
  assert.isTrue(gSettings.Import(container));
  assert.isDefined(container.value);

  actualAccount = container.value;
  var expected = {
  };
  assert.equalAccount(expected, actualAccount);
}

