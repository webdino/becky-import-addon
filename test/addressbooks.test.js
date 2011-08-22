/* vim: set ts=2 et sw=2 tw=80: */

var description = 'AddressBooks component tests'
var gImporter;
var gAddressBooks;
var gMDB;

assert.equalAbCard = function(expected, actual) {
  assert.equals(expected, actual.translateTo("vcard"));
}

function setUp() {
  gMDB = null;
}

function tearDown() {
  if (gMDB && gMDB.exists())
    gMDB.remove(false);
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

function createMDB() {
  var destination =
    Cc["@mozilla.org/addressbook/carddatabase;1"].getService(Ci.nsIAddrDatabase);
  assert.isDefined(destination);
  gMDB = utils.normalizeToFile(utils.baseURL + 'fixtures/db/test.mdb');
  destination.openMDB(gMDB, true);

  return destination;
}

testImportAddressBook.description = "ImportAddressBook test";
testImportAddressBook.priority = 'must';
function testImportAddressBook() {
  testFindAddressBooks();

  var descriptor = gAddressBooks.QueryElementAt(0, Ci.nsIImportABDescriptor);
  assert.isDefined(descriptor);

  var destination = createMDB();
  gImporter.ImportAddressBook(descriptor,
                              destination,
                              null,
                              null,
                              false,
                              {},
                              {},
                              {});
  var container = {};
  destination.getCardCount(container);
  assert.isDefined(container.value);
  assert.equals(1, container.value);

  var cards = destination.enumerateCards(null);
  assert.isDefined(cards);
  var card = cards.getNext().QueryInterface(Ci.nsIAbCard);
  assert.equalAbCard("begin%3Avcard%0D%0Afn%3Bquoted-printable%3A%3DE3%3D81%3D84%3DE3%3D81%3D91%3DE3%3D81%3D9E%3DE3%3D81%3D88%3DE3%3D81%3DA7%3DE3%3D81%3D99%0D%0An%3Bquoted-printable%3Bquoted-printable%3A%3DE6%3DB1%3DA0%3DE6%3DB7%3DBB%3B%3DE6%3DB5%3DA9%3DE4%3DB9%3D8B%0D%0Aorg%3AGnome%3Blibrsvg%20maintainer%0D%0Aadr%3Bquoted-printable%3Bquoted-printable%3Bquoted-printable%3Bquoted-printable%3A%3B%3B%3DE7%3D9A%3D87%3DE5%3DB1%3D85%3B%3DE5%3D8D%3D83%3DE4%3DBB%3DA3%3DE7%3D94%3DB0%3DE5%3D8C%3DBA%3B%3DE6%3D9D%3DB1%3DE4%3DBA%3DAC%3B111%3B%3DE6%3D97%3DA5%3DE6%3D9C%3DAC%0D%0Aemail%3Binternet%3Ahiikezoe%40gnome.org%0D%0Atel%3Bwork%3A11-1111-1111%0D%0Atel%3Bhome%3Ahiikezoe%40gnome.org%0D%0Atel%3Bcell%3A090-0000-0000%0D%0Aversion%3A2.1%0D%0Aend%3Avcard%0D%0A%0D%0A", card);
}

