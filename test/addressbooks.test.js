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

testGetSupportsMultiple.description = "GetSupportsMultiple instance test";
testGetSupportsMultiple.priority = 'must';
function testGetSupportsMultiple() {
  testCreate();
  assert.isFalse(gAddressBooks.GetSupportsMultiple());
}

testSetSampleLocation.description = "SetSampleLocation instance test";
testSetSampleLocation.priority = 'must';
function testSetSampleLocation() {
  testCreate();
  gAddressBooks.SetSampleLocation(utils.normalizeToFile(utils.baseURL));
}

testGetSampleData.description = "GetSampleData instance test";
testGetSampleData.priority = 'must';
function testGetSampleData() {
  testSetSampleLocation();
  assert.raises(
    Cr.NS_ERROR_FAILURE,
    function() {
      gAddressBooks.GetSampleData(1, {});
    },
    {}
  );
}

