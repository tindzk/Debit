#import "HttpConnection.h"

#define self HttpConnection

extern Logger logger;

class {
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

static def(void, OnHeader, RdString name, RdString value);
static def(void, OnVersion, HTTP_Version version);
static def(void, OnMethod, HTTP_Method method);
static def(void, OnPath, RdString path);
static def(String *, OnQueryParameter, RdString name);
static def(String *, OnBodyParameter, RdString name);
static def(void, OnRespond, bool persistent);

def(void, Init, SocketConnection *conn) {
	this->method     = HTTP_Method_Get;
	this->version    = HTTP_Version_1_0;
	this->incomplete = true;
	this->replied    = false;
	this->controller = FrontController_New();
	this->session    = SocketSession_New(conn);
	this->server     = HTTP_Server_New(conn, 2048, 4096);

	HTTP_Server_BindHeader(&this->server, Callback(this, ref(OnHeader)));
	HTTP_Server_BindVersion(&this->server, Callback(this, ref(OnVersion)));
	HTTP_Server_BindMethod(&this->server, Callback(this, ref(OnMethod)));
	HTTP_Server_BindPath(&this->server, Callback(this, ref(OnPath)));
	HTTP_Server_BindQueryParameter(&this->server, Callback(this, ref(OnQueryParameter)));
	HTTP_Server_BindBodyParameter(&this->server, Callback(this, ref(OnBodyParameter)));
	HTTP_Server_BindRespond(&this->server, Callback(this, ref(OnRespond)));

	Logger_Debug(&logger, $("Connection initialized"));

	this->resp = Response_New();
}

def(void, Destroy) {
	Logger_Debug(&logger, $("Connection destroyed"));
	HTTP_Server_Destroy(&this->server);
	Response_Destroy(&this->resp);

	FrontController_Destroy(&this->controller);
}

static def(void, Error, HTTP_Status status, RdString msg);

def(void, Process) {
	this->incomplete = true;

	String fmt = String_New(0);

	try {
		this->incomplete = HTTP_Server_Process(&this->server);
	} catch(HTTP_Query, ExceedsPermittedLength) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("A parameter exceeds its permitted length."));
	} catch(HTTP_Header, UnknownVersion) {
		call(Error, HTTP_Status_ServerError_VersionNotSupported,
			$("Unknown HTTP version."));
	} catch(HTTP_Header, UnknownMethod) {
		call(Error, HTTP_Status_ServerError_NotImplemented,
			$("Method is not implemented."));
	} catch(HTTP_Server, BodyUnexpected) {
		fmt = String_Format(
			$("Body not expected with method '%'."),
			HTTP_Method_ToString(this->method));
		call(Error, HTTP_Status_ClientError_ExpectationFailed, fmt.rd);
	} catch(HTTP_Header, RequestMalformed) {
		call(Error, HTTP_Status_ClientError_BadRequest,
			$("Request malformed."));
	} catch(HTTP_Server, HeaderTooLarge) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("The header exceeds its permitted length."));
	} catch(HTTP_Server, BodyTooLarge) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("The body exceeds its permitted length."));
	} catch(HTTP_Server, UnknownContentType) {
		call(Error, HTTP_Status_ClientError_NotAcceptable,
			$("Content-Type not recognized."));
	} catch(HTTP_Header, EmptyRequestUri) {
		call(Error, HTTP_Status_ClientError_BadRequest,
			$("Empty request URI."));
	} catchAny {
		Logger_Error(&logger, $("Uncaught exception in HTTP server"));
		__exc_rethrow = true;
	} finally {
		String_Destroy(&fmt);
	} tryEnd;
}

static def(void, OnVersion, HTTP_Version version) {
	Response_SetVersion(&this->resp, version);
	this->version = version;
}

static def(void, OnMethod, HTTP_Method method) {
	this->method = method;
}

/* Parameters to extract from URL. */
static def(void, OnPath, RdString path) {
	Response_Reset(&this->resp);
	FrontController_Reset(&this->controller);

	Logger_Info(&logger, $("% % %"),
		HTTP_Method_ToString(this->method),
		path,
		HTTP_Version_ToString(this->version));

	RouterInstance router = Router_GetInstance();

	MatchingRoute match = Router_FindRoute(router, path);

	if (match.route == NULL) {
		match = Router_GetDefaultRoute(router);
	}

	if (match.route != NULL) {
		FrontController_SetMethod(&this->controller, this->method);
		FrontController_SetRoute(&this->controller, match.route);
		FrontController_SetResource(&this->controller, match.resource);
		FrontController_CreateResource(&this->controller);

		/* `routeElems' and `pathElems' aren't used for the default route. */
		if (match.routeElems != NULL) {
			Router_ExtractParts(router,
				match.routeElems,
				match.pathElems,
				Callback(
					&this->controller,
					FrontController_Store));
		}

		Router_DestroyMatch(router, match);
	}
}

static def(void, OnHeader, RdString name, RdString value) {
	Logger_Debug(&logger, $("Header: % = %"), name, value);

	if (String_Equals(name, $("Cookie"))) {
		RdStringArray *items = String_Split(value, '=');

		if (items->len > 1) {
			FrontController_SetCookie(&this->controller,
				items->buf[0],
				items->buf[1]);
		}

		RdStringArray_Free(items);
	} else {
		FrontController_SetHeader(&this->controller, name, value);
	}
}

static def(String *, OnQueryParameter, RdString name) {
	Logger_Debug(&logger, $("Received GET parameter '%'"), name);

	return FrontController_GetMemberAddr(&this->controller, name);
}

static def(String *, OnBodyParameter, RdString name) {
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
}

static def(void, OnFileSent,   __unused File *file);
static def(void, OnBufferSent, __unused RdString *str);

static def(void, OnHeadersSent, RdString *s) {
	String size = Integer_ToString(s->len);
	Logger_Debug(&logger, $("Response headers sent (% bytes)"), size.rd);
	String_Destroy(&size);

	Response_Body *body = Response_GetBody(&this->resp);

	switch (body->type) {
		case (Response_BodyType_Buffer):
			SocketSession_Write(&this->session, body->buf.rd,
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

static def(void, OnBufferSent, RdString *str) {
	String size = Integer_ToString(str->len);
	Logger_Debug(&logger, $("Buffer sent (% bytes)"), size.rd);
	String_Destroy(&size);

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
	if (FrontController_HasResource(&this->controller)) {
		FrontController_HandleRequest(&this->controller,
			&this->resp);
	} else {
		Response_SetStatus(&this->resp,
			HTTP_Status_ClientError_NotFound);

		Response_SetBufferBody(&this->resp,
			String_ToCarrier($$("Sorry, no matching route found")));
	}

	call(ProcessResponse, persistent);
}

static def(void, Error, HTTP_Status status, RdString msg) {
	this->incomplete = false;

	Logger_Error(&logger, $("Client error: %"), msg);

	Response_SetStatus(&this->resp, status);

	HTTP_Status_Item st = HTTP_Status_GetItem(status);

	String strCode = Integer_ToString(st.code);

	Response_SetBufferBody(&this->resp, String_ToCarrier(String_Format(
		$(
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

		strCode.rd, st.msg,
		strCode.rd, st.msg,
		msg)));

	String_Destroy(&strCode);

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

static def(ClientConnection_Status, Push) {
	Logger_Debug(&logger, $("Got push"));

	call(Process);

	return SocketSession_IsIdle(&this->session) && call(Close)
		? ClientConnection_Status_Close
		: ClientConnection_Status_Open;
}

static def(ClientConnection_Status, Pull) {
	Logger_Debug(&logger, $("Got pull"));

	SocketSession_Continue(&this->session);

	return SocketSession_IsIdle(&this->session) && call(Close)
		? ClientConnection_Status_Close
		: ClientConnection_Status_Open;
}

Impl(Connection) = {
	.size    = sizeof(self),
	.init    = (void *) ref(Init),
	.destroy = (void *) ref(Destroy),
	.push    = (void *) ref(Push),
	.pull    = (void *) ref(Pull)
};
