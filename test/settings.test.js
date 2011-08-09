var description = 'Setting component tests'
var gSettings;

function setUp() {
}

function tearDown() {
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gSettings = Cc["@mozilla-japan.org/import/becky/settings;1"].getService(Ci.nsIImportSettings);
  assert.isDefined(gSettings);
}

