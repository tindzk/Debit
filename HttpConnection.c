#import "HttpConnection.h"

extern Logger logger;

class {
	Connection base;

	bool replied;
	bool incomplete;
	bool persistent;

	SocketSession session;

	HTTP_Method  method;
	HTTP_Server  server;
	HTTP_Version version;

	Response        resp;
	FrontController controller;
};

static def(void, OnHeader, String name, String value);
static def(void, OnVersion, HTTP_Version version);
static def(void, OnMethod, HTTP_Method method);
static def(void, OnPath, String path);
static def(String *, OnQueryParameter, String name);
static def(String *, OnBodyParameter, String name);
static def(void, OnRespond, bool persistent);

def(void, Init, SocketConnection *conn) {
	this->method     = HTTP_Method_Get;
	this->version    = HTTP_Version_1_0;
	this->incomplete = true;
	this->replied    = false;

	FrontController_Init(&this->controller);

	SocketSession_Init(&this->session, conn);

	HTTP_Server_Events events;
	events.onHeader         = (HTTP_OnHeader) Callback(this, ref(OnHeader));
	events.onVersion        = (HTTP_OnVersion) Callback(this, ref(OnVersion));
	events.onMethod         = (HTTP_OnMethod) Callback(this, ref(OnMethod));
	events.onPath           = (HTTP_OnPath) Callback(this, ref(OnPath));
	events.onQueryParameter = (HTTP_OnParameter) Callback(this, ref(OnQueryParameter));
	events.onBodyParameter  = (HTTP_OnParameter) Callback(this, ref(OnBodyParameter));
	events.onRespond        = (HTTP_Server_OnRespond) Callback(this, ref(OnRespond));

	HTTP_Server_Init(&this->server, events, conn, 2048, 4096);

	Logger_Debug(&logger, $("Connection initialized"));
}

def(void, Destroy) {
	Logger_Debug(&logger, $("Connection destroyed"));
	HTTP_Server_Destroy(&this->server);
	FrontController_Destroy(&this->controller);
}

static def(void, Error, HTTP_Status status, String msg);

def(void, Process) {
	this->incomplete = true;

	try {
		this->incomplete = HTTP_Server_Process(&this->server);
	} clean catch(HTTP_Query, excExceedsPermittedLength) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("A parameter exceeds its permitted length."));
	} catch(HTTP_Header, excUnknownVersion) {
		call(Error, HTTP_Status_ServerError_VersionNotSupported,
			$("Unknown HTTP version."));
	} catch(HTTP_Header, excUnknownMethod) {
		call(Error, HTTP_Status_ServerError_NotImplemented,
			$("Method is not implemented."));
	} catch(HTTP_Server, excBodyUnexpected) {
		String fmt = String_Format(
			$("Body not expected with method '%'."),
			HTTP_Method_ToString(this->method));
		call(Error, HTTP_Status_ClientError_ExpectationFailed, fmt);
		String_Destroy(&fmt);
	} catch(HTTP_Header, excRequestMalformed) {
		call(Error, HTTP_Status_ClientError_BadRequest,
			$("Request malformed."));
	} catch(HTTP_Server, excHeaderTooLarge) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("The header exceeds its permitted length."));
	} catch(HTTP_Server, excBodyTooLarge) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("The body exceeds its permitted length."));
	} catch(HTTP_Server, excUnknownContentType) {
		call(Error, HTTP_Status_ClientError_NotAcceptable,
			$("Content-Type not recognized."));
	} catch(HTTP_Header, excEmptyRequestUri) {
		call(Error, HTTP_Status_ClientError_BadRequest,
			$("Empty request URI."));
	} catchAny {
		Logger_Error(&logger, $("Uncaught exception in HTTP server"));
		__exc_rethrow = true;
	} finally {

	} tryEnd;
}

static def(void, OnVersion, HTTP_Version version) {
	this->version = version;
}

static def(void, OnMethod, HTTP_Method method) {
	this->method = method;
}

/* Parameters to extract from URL. */
static def(void, OnPath, String path) {
	FrontController_Reset(&this->controller);

	Logger_Info(&logger, $("% % HTTP/%"),
		HTTP_Method_ToString(this->method),
		path,
		this->version == HTTP_Version_1_0
			? $("1.0")
			: $("1.1"));

	RouterInstance router = Router_GetInstance();

	MatchingRoute match = Router_FindRoute(router, path);

	if (match.route != NULL) {
		FrontController_SetMethod(&this->controller, this->method);
		FrontController_SetRoute(&this->controller, match.route);
		FrontController_SetResource(&this->controller, match.resource);
		FrontController_CreateResource(&this->controller);

		Router_ExtractParts(router,
			match.routeElems,
			match.pathElems,
			Callback(
				&this->controller,
				FrontController_Store));

		Router_DestroyMatch(router, match);
	}
}

static def(void, OnHeader, String name, String value) {
	Logger_Debug(&logger, $("Header: % = %"), name, value);

	if (String_Equals(name, $("Cookie"))) {
		StringArray *items = String_Split(value, '=');

		if (items->len > 1) {
			FrontController_SetCookie(&this->controller,
				items->buf[0],
				items->buf[1]);
		}

		StringArray_Free(items);
	} else {
		FrontController_SetHeader(&this->controller, name, value);
	}
}

static def(String *, OnQueryParameter, String name) {
	Logger_Debug(&logger, $("Received GET parameter '%'"), name);

	return FrontController_GetMemberAddr(&this->controller, name);
}

static def(String *, OnBodyParameter, String name) {
	Logger_Debug(&logger, $("Received POST parameter '%'"), name);

	return FrontController_GetMemberAddr(&this->controller, name);
}

static def(void, OnSent, bool flush) {
	/* When the connection is closed, the buffer is flushed
	 * anyway. Therefore, we can save a syscall.
	 */
	if (flush && Response_IsPersistent(&this->resp)) {
		SocketSession_Flush(&this->session);
	}

	this->replied    = true;
	this->persistent = Response_IsPersistent(&this->resp);

	Response_Destroy(&this->resp);
}

static def(void, OnFileSent,   __unused File *file);
static def(void, OnBufferSent, __unused String *str);

static def(void, OnHeadersSent, String *s) {
	Logger_Debug(&logger, $("Response headers sent (% bytes)"),
		Integer_ToString(s->len));

	String_Destroy(s);

	Response_Body *body = Response_GetBody(&this->resp);

	switch (body->type) {
		case (Response_BodyType_Buffer):
			SocketSession_Write(&this->session, body->buf,
				Callback(this, ref(OnBufferSent)));

			break;

		case Response_BodyType_File:
			SocketSession_SendFile(&this->session,
				body->file.file,
				body->file.size,
				Callback(this, ref(OnFileSent)));

			break;

		/* TODO */
		case Response_BodyType_Stream:
			call(OnSent, true);
			break;

		case Response_BodyType_Empty:
			call(OnSent, true);
			break;
	}
}

static def(void, OnBufferSent, String *str) {
	Logger_Debug(&logger, $("Buffer sent (% bytes)"),
		Integer_ToString(str->len));

	call(OnSent, true);
}

static def(void, OnFileSent, __unused File *file) {
	Logger_Debug(&logger, $("File sent"));

	/* File transfers don't require flushing. */
	call(OnSent, false);
}

static def(void, ProcessResponse, bool persistent) {
	Response_Process(&this->resp, persistent);

	SocketSession_Write(&this->session,
		Response_GetHeaders(&this->resp),
		Callback(this, ref(OnHeadersSent)));
}

static def(void, OnRespond, bool persistent) {
	Response_Init(&this->resp, this->version);

	if (FrontController_HasResource(&this->controller)) {
		FrontController_HandleRequest(&this->controller,
			&this->resp);
	} else {
		Response_SetStatus(&this->resp,
			HTTP_Status_ClientError_NotFound);

		Response_SetBufferBody(&this->resp,
			$("Sorry, no matching route found"));
	}

	call(ProcessResponse, persistent);
}

static def(void, Error, HTTP_Status status, String msg) {
	this->incomplete = false;

	Logger_Error(&logger, $("Client error: %"), msg);

	Response_Init(&this->resp, this->version);
	Response_SetStatus(&this->resp, status);

	HTTP_Status_Item st = HTTP_Status_GetItem(status);

	Response_SetBufferBody(&this->resp, String_Format(
		String(
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
								"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
			"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
					"<head>"
							"<title>% - %</title>"
					"</head>"
					"<body>"
							"<h1>% - %</h1>"
							"<h2>%</h2>"
					"</body>"
			"</html>"),

		Integer_ToString(st.code), st.msg,
		Integer_ToString(st.code), st.msg,

		msg));

	call(ProcessResponse, false);
}

static def(bool, Close) {
	if (this->incomplete) {
		return false;
	}

	if (this->replied) {
		this->replied    = false;
		this->incomplete = true;

		return !this->persistent;
	}

	return false;
}

static def(Connection_Status, Push) {
	Logger_Debug(&logger, $("Got push"));

	call(Process);

	return SocketSession_IsIdle(&this->session) && call(Close)
		? Connection_Status_Close
		: Connection_Status_Open;
}

static def(Connection_Status, Pull) {
	Logger_Debug(&logger, $("Got pull"));

	SocketSession_Continue(&this->session);

	return SocketSession_IsIdle(&this->session) && call(Close)
		? Connection_Status_Close
		: Connection_Status_Open;
}

Impl(Connection) = {
	.size    = sizeof(self),
	.init    = (void *) ref(Init),
	.destroy = (void *) ref(Destroy),
	.push    = (void *) ref(Push),
	.pull    = (void *) ref(Pull)
};
