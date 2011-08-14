var description = 'Filters component tests'
var gFilters;

function setUp() {
}

function tearDown() {
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
  gFilters.SetLocation(utils.normalizeToFile(utils.baseURL));
}

testImport.description = "Import test";
testImport.priority = 'must';
function testImport() {
  testSetLocation();
  let errorString = {}
  assert.isTrue(gFilters.Import(errorString));
}

