#import <File.h>
#import <String.h>
#import <HTTP/Envelope.h>

#define self Response

set(ref(BodyType)) {
	ref(BodyType_File),
	ref(BodyType_Buffer),
	ref(BodyType_Stream),
	ref(BodyType_Empty)
};

record(ref(Body)) {
	ref(BodyType) type;

	union {
		struct {
			File file;
			u64  size;
		} file;

		CarrierString buf;
	};
};

class {
	HTTP_Version version;
	ref(Body) body;
	CarrierString headers;
	HTTP_Envelope envelope;
};

rsdef(self, New);
def(void, Reset);
def(void, Destroy);
def(void, SetVersion, HTTP_Version version);
def(void, SetStatus, HTTP_Status status);
def(bool, IsStream);
def(bool, IsPersistent);
def(void, SetCookie, CarrierString name, CarrierString value);
def(void, SetLocation, CarrierString path);
def(void, SetLastModified, DateTime lastModified);
def(void, SetFileBody, File file, u64 size);
def(void, SetBufferBody, CarrierString buf);
def(void, SetContentType, CarrierString contentType);
def(void, Process, bool persistent);
rdef(ref(Body) *, GetBody);
rdef(ProtString, GetHeaders);

#undef self
