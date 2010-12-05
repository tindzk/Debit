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

		String buf;
	};
};

class {
	HTTP_Version version;
	ref(Body) body;
	HTTP_Envelope envelope;
	String headers;
};

ExtendClass;

def(void, Init);
def(void, Destroy);
def(void, SetVersion, HTTP_Version version);
def(void, SetStatus, HTTP_Status status);
def(bool, IsStream);
def(bool, IsPersistent);
def(void, SetCookie, String name, String value);
def(void, SetLocation, String path);
def(void, SetLastModified, DateTime lastModified);
def(void, SetFileBody, File file, u64 size);
def(void, SetBufferBody, String buf);
def(void, SetContentType, String contentType);
def(void, Process, bool persistent);
def(ref(Body) *, GetBody);
def(String, GetHeaders);

#undef self
