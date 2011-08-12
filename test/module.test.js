var description = 'Import module tests'
var gModule;

function setUp() {
}

function tearDown() {
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gModule = Cc["@mozilla-japan.org/import/becky;1"].getService(Ci.nsIImportModule);
  assert.isDefined(gModule);
}

testName.description = "name test";
testName.priority = 'must';
function testName() {
  testCreate();
  assert.equals("Becky! Internet Mail", gModule.name);
}

testDescription.description = "description test";
testDescription.priority = 'must';
function testDescription() {
  testCreate();
  assert.equals("Import Local Mail from Becky! Internet Mail", gModule.description);
}

testSupports.description = "supports test";
testSupports.priority = 'must';
function testSupports() {
  testCreate();
  assert.equals("mail,addressbook,settings,filters", gModule.supports);
}

testSupportsUpgrade.description = "supportsUpgrade test";
testSupportsUpgrade.priority = 'must';
function testSupportsUpgrade() {
  testCreate();
  assert.isTrue(gModule.supportsUpgrade);
}

