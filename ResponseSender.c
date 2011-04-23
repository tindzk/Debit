#import "ResponseSender.h"

#define self ResponseSender

def(void, Init, Server_Client *client, Logger *logger) {
	this->complete   = false;
	this->persistent = false;
	this->logger     = logger;
	this->client     = client;
	this->session    = SocketSession_New(client->socket.conn);

	DoublyLinkedList_Init(&this->packets);
}

def(void, DestroyPacket, RequestPacketExtendedInstance packet) {
	RequestPacket_Destroy(packet);
}

def(void, Destroy) {
	DoublyLinkedList_Destroy(&this->packets,
		LinkedList_OnDestroy_For(this, ref(DestroyPacket)));
}

def(RequestPacket *, GetPacket) {
	assert(this->packets.last != NULL);
	return this->packets.last;
}

def(void, NewPacket) {
	RequestPacket *packet = Pool_Alloc(Pool_GetInstance(), sizeof(RequestPacket));
	RequestPacket_Init(packet, this, this->logger);

	DoublyLinkedList_InsertEnd(&this->packets, packet);
}

static sdef(void, OnFileSent,   GenericInstance inst, __unused void *ptr);
static sdef(void, OnBufferSent, GenericInstance inst, __unused void *ptr);

static sdef(void, OnSent, RequestPacket *packet, bool flush) {
	RequestPacket *next = packet->next;

	self* _this = packet->sender;

	/* When the connection is closed, the buffer is flushed anyway. Therefore,
	 * we can save a system call.
	 */
	if (flush && Response_IsPersistent(&packet->response)) {
		SocketSession_Flush(&_this->session);
	}

	/* Remember the `persistent' state as the object won't be accessible any
	 * longer.
	 */
	_this->persistent = Response_IsPersistent(&packet->response);

	DoublyLinkedList_Remove(&_this->packets, packet);

	RequestPacket_Destroy(packet);
	Pool_Free(Pool_GetInstance(), packet);

	if (next != NULL && RequestPacket_IsReady(next)) {
		/* There might be more packets in the queue. Continue with the next one
		 * if it already has a response.
		 */
		scall(SendResponse, _this, next);
	} else if (_this->packets.first == NULL) {
		/* We have replied to all requests, i.e. this is the last request. */
		if (!_this->persistent) {
			/* Therefore we can take its `persistent' status and close the
			 * connection if necessary.
			 */
			if (_this->complete) {
				/* `complete' means that the buffer in HTTP.Server is processed.
				 * This is only the case for asynchronous resources.
				 */
				Server_Client_Close(_this->client);
			}
		}
	}
}

static sdef(void, OnHeadersSent, GenericInstance inst, void *ptr) {
	assert(ptr != NULL);
	assert(!Generic_IsNull(inst));

	RequestPacket *packet = inst.object;
	String *s = ptr;

	String size = Integer_ToString(s->len);
	Logger_Debug(packet->sender->logger,
		$("Response headers sent (% bytes)"), size.rd);
	String_Destroy(&size);

	Response_Body *body = Response_GetBody(&packet->response);

	switch (body->type) {
		case Response_BodyType_Buffer:
			SocketSession_Write(&packet->sender->session, body->buf.rd,
				SocketSession_OnDone_For(packet, ref(OnBufferSent)));

			break;

		case Response_BodyType_File:
			SocketSession_SendFile(&packet->sender->session,
				body->file.file, body->file.size,
				SocketSession_OnDone_For(packet, ref(OnFileSent)));

			break;

		case Response_BodyType_Stream:
			scall(OnSent, packet, true);
			break;

		case Response_BodyType_Empty:
			scall(OnSent, packet, true);
			break;
	}
}

static sdef(void, OnBufferSent, GenericInstance inst, void *ptr) {
	assert(ptr != NULL);
	assert(!Generic_IsNull(inst));

	RequestPacket *packet = inst.object;
	RdString *str = ptr;

	String size = Integer_ToString(str->len);
	Logger_Debug(packet->sender->logger, $("Buffer sent (% bytes)"), size.rd);
	String_Destroy(&size);

	scall(OnSent, packet, true);
}

static sdef(void, OnFileSent, GenericInstance inst, __unused void *ptr) {
	assert(ptr != NULL);
	assert(!Generic_IsNull(inst));

	RequestPacket *packet = inst.object;

	Logger_Debug(packet->sender->logger, $("File sent"));

	/* File transfers don't require flushing. */
	scall(OnSent, packet, false);
}

/* This method is called by the user (TemplateResponse() etc.) to initiate the
 * transmission.
 *
 * Packets cannot be sent in an arbitrary order. We have to start with the first
 * packet and process the rest step by step. If the first packet is not ready
 * yet, we have to wait.
 */

def(void, SendResponse, RequestPacket *packet) {
	/* This should not be called when we're not done with sending the existing
	 * session.
	 */
	assert(SocketSession_IsIdle(&this->session));

	if (this->packets.first != packet) {
		call(SendResponse, this->packets.first);
		return;
	}

	if (!RequestPacket_IsReady(packet)) {
		Logger_Debug(this->logger, $("Packet is not ready."));
		return;
	}

	SocketSession_Write(&this->session,
		Response_GetHeaders(&packet->response),
		SocketSession_OnDone_For(packet, ref(OnHeadersSent)));
}

/* This method is called when we receive a `push' request from the client. */
def(void, Continue) {
	/* Before we can move over to the next response, any remaining data
	 * belonging to the current response must be sent to the client first.
	 */
	if (!SocketSession_IsIdle(&this->session)) {
		SocketSession_Continue(&this->session);
	}

	if (this->packets.first == NULL) {
		Logger_Debug(this->logger, $("No requests are left for processing."));
		return;
	}

	call(SendResponse, this->packets.first);
}

/* Returns true if we can close the connection. */
def(bool, Close) {
	/* Have we replied to all requests? */
	if (this->packets.first == NULL) {
		/* Use the `persistent' status from the last request. */
		return this->complete && !this->persistent;
	}

	return false;
}
