var description = 'Mail component tests'
var gMail;

function setUp() {
}

function tearDown() {
}

testCreate.description = "create instance test";
testCreate.priority = 'must';
function testCreate() {
  gMail = Cc["@mozilla-japan.org/import/becky/mail;1"].getService(Ci.nsIImportMail);
  assert.isDefined(gMail);
}

testGetDefaultLocation.description = "GetDefaultLocation test";
testGetDefaultLocation.priority = 'must';
function testGetDefaultLocation() {
  testCreate();
  var userVerify = {};
  var found = {};
  var location = {};
  gMail.GetDefaultLocation(location, found, userVerify);
  assert.isTrue(userVerify.value);
  assert.isTrue(userVerify.found);
}

testFindMailboxes.description = "FindMailboxes test";
testFindMailboxes.priority = 'must';
function testFindMailboxes() {
  testCreate();
  var mailboxes = gMail.FindMailboxes(utils.normalizeToFile(utils.baseURL));
  assert.isDefined(mailboxes);
}

testTranslateFolderName.description = "TranslateFolderName test";
testTranslateFolderName.priority = 'must';
function testTranslateFolderName() {
  testCreate();
  assert.equals("Sent", gMail.TranslateFolderName("sent"));
  assert.equals("Trash", gMail.TranslateFolderName("trash"));
  assert.equals("Inbox", gMail.TranslateFolderName("inbox"));
  assert.equals("Unsent Messages", gMail.TranslateFolderName("unsent"));
}

