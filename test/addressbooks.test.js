var description = 'AddressBooks component tests'
var gAddressBooks;

function setUp() {
}

function tearDown() {
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gAddressBooks = Cc["@mozilla-japan.org/import/becky/addressbooks;1"].getService(Ci.nsIImportAddressBooks);
  assert.isDefined(gAddressBooks);
}

