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
  let userVerify = {};
  let found = {};
  let location = {};
  gMail.GetDefaultLocation(location, found, userVerify);
  assert.isTrue(userVerify.value);
  assert.isFalse(found.value);
}

testFindMailboxes.description = "FindMailboxes test";
testFindMailboxes.priority = 'must';
function testFindMailboxes() {
  testCreate();
  let mailboxes = gMail.FindMailboxes(utils.normalizeToFile(utils.baseURL + 'fixtures/mailboxes'));
  assert.isDefined(mailboxes);
  assert.isInstanceOf(Ci.nsISupportsArray, mailboxes);
  assert.equals(1, mailboxes.Count());

  let descriptor;
  descriptor = mailboxes.QueryElementAt(0, Ci.nsIImportMailboxDescriptor);
  assert.isDefined(descriptor);

  assert.equals("mailboxes", descriptor.GetDisplayName());
}

testTranslateFolderName.description = "TranslateFolderName test";
testTranslateFolderName.priority = 'must';
function testTranslateFolderName() {
  testCreate();
  assert.equals("Sent", gMail.translateFolderName("sent"));
  assert.equals("Trash", gMail.translateFolderName("trash"));
  assert.equals("Inbox", gMail.translateFolderName("inbox"));
  assert.equals("Unsent Messages", gMail.translateFolderName("unsent"));
}

testGetImportProgress.description = "GetImportProgress test";
testGetImportProgress.priority = 'must';
function testGetImportProgress() {
  testCreate();
  assert.equals(0, gMail.GetImportProgress());
}

