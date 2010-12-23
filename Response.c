#import "Response.h"

#define self Response

static def(void, Defaults) {
	this->body.type = ref(BodyType_Empty);
	this->version   = HTTP_Version_1_0;

	HTTP_Envelope_SetStatus(&this->envelope, HTTP_Status_Success_Ok);
	HTTP_Envelope_SetContentLength(&this->envelope, 0);
	HTTP_Envelope_SetLocation(&this->envelope, $(""));
	HTTP_Envelope_SetContentType(&this->envelope, $(""));
	HTTP_Envelope_SetLastModified(&this->envelope, DateTime_Empty());
}

def(void, Init) {
	HTTP_Envelope_Init(&this->envelope);
	call(Defaults);

	this->headers = HeapString(0);
}

static def(void, DestroyBody) {
	if (this->body.type == ref(BodyType_Buffer)) {
		if (this->body.buf.mutable) {
			String_Destroy(&this->body.buf);
		}
	} else if (this->body.type == ref(BodyType_File)) {
		File_Close(&this->body.file.file);
	} else if (this->body.type == ref(BodyType_Stream)) {
		/* TODO */
	}
}

def(void, Reset) {
	call(DestroyBody);
	call(Defaults);

	String_Destroy(&this->headers);
	this->headers = HeapString(0);
}

def(void, Destroy) {
	call(Reset);
	HTTP_Envelope_Destroy(&this->envelope);
}

def(void, SetVersion, HTTP_Version version) {
	HTTP_Envelope_SetVersion(&this->envelope, version);
	this->version = version;
}

def(void, SetStatus, HTTP_Status status) {
	HTTP_Envelope_SetStatus(&this->envelope, status);
}

def(bool, IsStream) {
	return HTTP_Envelope_GetContentLength(&this->envelope) == -1;
}

def(bool, IsPersistent) {
	return HTTP_Envelope_IsPersistent(&this->envelope);
}

def(void, SetCookie, String name, String value) {
	HTTP_Envelope_SetCookie(&this->envelope, name, value);
}

def(void, SetLocation, String path) {
	HTTP_Envelope_SetLocation(&this->envelope, path);
}

def(void, SetLastModified, DateTime lastModified) {
	HTTP_Envelope_SetLastModified(&this->envelope, lastModified);
}

def(void, SetFileBody, File file, u64 size) {
	call(DestroyBody);

	this->body.type = ref(BodyType_File);

	this->body.file.file = file;
	this->body.file.size = size;

	HTTP_Envelope_SetContentLength(&this->envelope, size);
}

/* SetBufferBody() doesn't clone the buffer. Therefore, it must be
 * valid outside the caller scope, i.e. stack-allocated strings
 * cannot be used.
 *
 * The buffer's allocated memory is automatically freed.
 */
def(void, SetBufferBody, String buf) {
	call(DestroyBody);

	this->body.buf  = buf;
	this->body.type = ref(BodyType_Buffer);

	HTTP_Envelope_SetContentLength(&this->envelope, buf.len);
}

def(void, SetContentType, String contentType) {
	HTTP_Envelope_SetContentType(&this->envelope, contentType);
}

def(void, Process, bool persistent) {
	/* Are we dealing with a stream of which we don't know its length? */
	if (this->body.type == ref(BodyType_Stream)) {
		switch (this->version) {
			case HTTP_Version_1_0:
				/* Force the closure of the current connection
				 * because otherwise the client wouldn't know when
				 * the response has reached its end.
				 */

				persistent = false;
				break;

			case HTTP_Version_1_1:
				/* TODO Use chunked transfer. */
				break;

			case HTTP_Version_Unset:
				break;
		}
	}

	HTTP_Envelope_SetPersistent(&this->envelope, persistent);
}

def(ref(Body) *, GetBody) {
	return &this->body;
}

def(String, GetHeaders) {
	String_Destroy(&this->headers);
	this->headers = HTTP_Envelope_GetString(&this->envelope);

	return String_Disown(this->headers);
}
