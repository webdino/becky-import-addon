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

