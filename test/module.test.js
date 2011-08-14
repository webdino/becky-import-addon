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

testMailInterface.description = "mail interface test";
testMailInterface.priority = 'must';
function testMailInterface() {
  testCreate();
  var generic = gModule.GetImportInterface("mail");
  assert.isDefined(generic);
  assert.isInstanceOf(Ci.nsIImportGeneric, generic);

  var mail = generic.GetData("mailInterface");
  assert.isDefined(mail);
  assert.isInstanceOf(Ci.nsIImportMail, mail);
}

testAddressBooksInterface.description = "addressbooks interface test";
testAddressBooksInterface.priority = 'must';
function testAddressBooksInterface() {
  testCreate();
  var generic = gModule.GetImportInterface("addressbook");
  assert.isDefined(generic);
  assert.isInstanceOf(Ci.nsIImportGeneric, generic);

  var addressBooks = generic.GetData("addressInterface");
  assert.isDefined(addressBooks);
  assert.isInstanceOf(Ci.nsIImportAddressBooks, addressBooks);
}

testFiltersInterface.description = "filters interface test";
testFiltersInterface.priority = 'must';
function testFiltersInterface() {
  testCreate();
  var interface = gModule.GetImportInterface("filters");
  assert.isDefined(interface);
  assert.isInstanceOf(Ci.nsIImportFilters, interface);
}

testSettingsInterface.description = "settings interface test";
testSettingsInterface.priority = 'must';
function testSettingsInterface() {
  testCreate();
  var interface = gModule.GetImportInterface("settings");
  assert.isDefined(interface);
  assert.isInstanceOf(Ci.nsIImportSettings, interface);
}

