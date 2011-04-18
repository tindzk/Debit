#import "Response.h"
#import "RequestPacket.h"

#define self Response

rsdef(self, New, struct RequestPacket *packet) {
	return (self) {
		.envelope  = HTTP_Envelope_New(),
		.body.type = ref(BodyType_Empty),
		.version   = HTTP_Version_1_0,
		.headers   = CarrierString_New(),
		.packet    = packet
	};
}

static def(void, DestroyBody) {
	if (this->body.type == ref(BodyType_Buffer)) {
		CarrierString_Destroy(&this->body.buf);
	} else if (this->body.type == ref(BodyType_File)) {
		File_Close(&this->body.file.file);
	} else if (this->body.type == ref(BodyType_Stream)) {
		/* TODO */
		assert(false);
	}

	this->body.type = ref(BodyType_Empty);
}

def(void, Destroy) {
	call(DestroyBody);
	CarrierString_Destroy(&this->headers);
	HTTP_Envelope_Destroy(&this->envelope);
}

def(void, Reset) {
	call(DestroyBody);

	HTTP_Envelope_Destroy(&this->envelope);
	this->envelope = HTTP_Envelope_New();
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

def(void, SetCookie, CarrierString name, CarrierString value) {
	HTTP_Envelope_SetCookie(&this->envelope, name, value);
}

def(void, SetLocation, CarrierString path) {
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

def(void, SetBufferBody, CarrierString buf) {
	call(DestroyBody);

	this->body.buf  = buf;
	this->body.type = ref(BodyType_Buffer);

	HTTP_Envelope_SetContentLength(&this->envelope, buf.len);
}

def(void, SetContentType, CarrierString contentType) {
	HTTP_Envelope_SetContentType(&this->envelope, contentType);
}

def(void, SetPersistent, bool persistent) {
	this->persistent = persistent;
}

rdef(ref(Body) *, GetBody) {
	return &this->body;
}

rdef(RdString, GetHeaders) {
	bool persistent = this->persistent;

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
				assert(false);
				break;

			case HTTP_Version_Unset:
				break;
		}
	}

	HTTP_Envelope_SetPersistent(&this->envelope, persistent);

	CarrierString_Assign(&this->headers,
		String_ToCarrier(HTTP_Envelope_GetString(&this->envelope)));

	return this->headers.rd;
}

def(void, Flush) {
	assert(this->packet != NULL);
	RequestPacket_Flush(this->packet);
}
