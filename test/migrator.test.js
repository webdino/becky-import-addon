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

testSourceExists.description = "sourceExists test";
testSourceExists.priority = 'must';
function testSourceExists() {
  testCreate();
  assert.isFalse(gMigrator.sourceExists);
}

testSourceHasMultipleProfiles.description = "sourceHasMultipleProfiles test";
testSourceHasMultipleProfiles.priority = 'must';
function testSourceHasMultipleProfiles() {
  testCreate();
  assert.isFalse(gMigrator.sourceHasMultipleProfiles);
}

testSourceProfiles.description = "sourceProfiles test";
testSourceProfiles.priority = 'must';
function testSourceProfiles() {
  testCreate();
  assert.isNull(gMigrator.sourceProfiles);
}

