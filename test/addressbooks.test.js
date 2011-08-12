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

testGetNeedsFieldMap.description = "GetNeedsFieldMap instance test";
testGetNeedsFieldMap.priority = 'must';
function testGetNeedsFieldMap() {
  testCreate();
  assert.isFalse(gAddressBooks.GetNeedsFieldMap(utils.normalizeToFile(utils.baseURL)));
}

testInitFieldMap.description = "InitFieldMap instance test";
testInitFieldMap.priority = 'must';
function testInitFieldMap() {
  testCreate();
  assert.raises(
    Cr.NS_ERROR_FAILURE,
    function () {
      gAddressBooks.InitFieldMap({});
    },
    {}
  );
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

testGetImportProgress.description = "GetImportProgress test";
testGetImportProgress.priority = 'must';
function testGetImportProgress() {
  testCreate();
  assert.equals(0, gAddressBooks.GetImportProgress());
}

