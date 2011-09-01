var description = 'Migrator module tests'
var gMigrator;

function setUp() {
}

function tearDown() {
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gMigrator = Cc["@mozilla.org/profile/migrator;1?app=mail&type=becky"].createInstance(Ci.nsIMailProfileMigrator);
  assert.isDefined(gMigrator);
}

testSourceExists.description = "create instance test";
testSourceExists.priority = 'must';
function testSourceExists() {
  testCreate();
  assert.isFalse(gMigrator.sourceExists);
}

