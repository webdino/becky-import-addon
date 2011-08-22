var description = 'AddressBooks component tests'
var gImporter;
var gAddressBooks;

function setUp() {
}

function tearDown() {
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gImporter = Cc["@mozilla-japan.org/import/becky/addressbooks;1"].getService(Ci.nsIImportAddressBooks);
  assert.isDefined(gImporter);
}

testGetSupportsMultiple.description = "GetSupportsMultiple instance test";
testGetSupportsMultiple.priority = 'must';
function testGetSupportsMultiple() {
  testCreate();
  assert.isFalse(gImporter.GetSupportsMultiple());
}

testGetNeedsFieldMap.description = "GetNeedsFieldMap instance test";
testGetNeedsFieldMap.priority = 'must';
function testGetNeedsFieldMap() {
  testCreate();
  assert.isFalse(gImporter.GetNeedsFieldMap(utils.normalizeToFile(utils.baseURL)));
}

testInitFieldMap.description = "InitFieldMap instance test";
testInitFieldMap.priority = 'must';
function testInitFieldMap() {
  testCreate();
  assert.raises(
    Cr.NS_ERROR_FAILURE,
    function () {
      gImporter.InitFieldMap({});
    },
    {}
  );
}

testSetSampleLocation.description = "SetSampleLocation instance test";
testSetSampleLocation.priority = 'must';
function testSetSampleLocation() {
  testCreate();
  gImporter.SetSampleLocation(utils.normalizeToFile(utils.baseURL));
}

testGetSampleData.description = "GetSampleData instance test";
testGetSampleData.priority = 'must';
function testGetSampleData() {
  testSetSampleLocation();
  assert.raises(
    Cr.NS_ERROR_FAILURE,
    function() {
      gImporter.GetSampleData(1, {});
    },
    {}
  );
}

testGetImportProgress.description = "GetImportProgress test";
testGetImportProgress.priority = 'must';
function testGetImportProgress() {
  testCreate();
  assert.equals(0, gImporter.GetImportProgress());
}

testGetDefaultLocation.description = "GetDefaultLocation test";
testGetDefaultLocation.priority = 'must';
function testGetDefaultLocation() {
  testCreate();
  var location = {};
  var found = {};
  var userVerify = {};
  gImporter.GetDefaultLocation(location, found, userVerify);

  assert.isFalse(found.value);
  assert.isTrue(userVerify.value);
}

testFindAddressBooks.description = "FindAddressBooks test";
testFindAddressBooks.priority = 'must';
function testFindAddressBooks() {
  testCreate();

  var location = utils.normalizeToFile(utils.baseURL + 'fixtures/addressbooks');
  gAddressBooks = gImporter.FindAddressBooks(location);

  assert.isDefined(gAddressBooks);
  assert.equals(1, gAddressBooks.Count());

  var descriptor;
  descriptor = gAddressBooks.QueryElementAt(0, Ci.nsIImportABDescriptor);
  assert.isDefined(descriptor);

  assert.equals("addressbooks", descriptor.preferredName);
}

