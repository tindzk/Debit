#import "ResponseSender.h"

#define self ResponseSender

rsdef(self, New, Server_Client *client, Logger *logger) {
	return (self) {
		.logger  = logger,
		.client  = client,
		.session = SocketSession_New(client->socket.conn),
		.packets = DoublyLinkedList_New()
	};
}

static def(void, DestroyPacket, void *item) {
	Logger_Info(this->logger, $("Destroying remaining/unsent packet."));
	RequestPacket_Destroy(item);
}

def(void, DropPacketsUntil, RequestPacket *except) {
	for (;;) {
		RequestPacket *cur = this->packets.first;

		if (cur == NULL || cur == except) {
			break;
		}

		DoublyLinkedList_Remove(&this->packets, cur);

		call(DestroyPacket, cur);
	}
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
	RequestPacket *packet = RequestPacket_New(this, this->logger);
	DoublyLinkedList_InsertEnd(&this->packets, packet);
}

static sdef(void, OnFileSent,   Instance inst, __unused void *ptr);
static sdef(void, OnBufferSent, Instance inst, __unused void *ptr);

static sdef(void, OnSent, RequestPacket *packet, bool flush) {
	RequestPacket *next = packet->next;

	self* _this = packet->sender;

	bool persistent = Response_IsPersistent(&packet->response);

	if (persistent) {
		if (flush) {
			/* When the connection is closed, the buffer is flushed anyway.
			 * Therefore, we can save a system call.
			 */

			SocketSession_Flush(&_this->session);
		}

		if (next != NULL && RequestPacket_IsReady(next)) {
			/* There are more packets in the queue. Continue with the next one. */
			scall(Flush, _this);
		}
	} else {
		if (next != NULL) {
			Logger_Debug(_this->logger,
				$("There are still requests but client requests closure."));
		}

		Server_Client_Close(_this->client);
	}

	DoublyLinkedList_Remove(&_this->packets, packet);

	RequestPacket_Destroy(packet);
}

static sdef(void, OnHeadersSent, Instance inst, void *ptr) {
	assert(ptr != NULL);
	assert(Instance_IsValid(inst));

	RequestPacket *packet = inst.addr;
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

static sdef(void, OnBufferSent, Instance inst, void *ptr) {
	assert(ptr != NULL);
	assert(Instance_IsValid(inst));

	RequestPacket *packet = inst.addr;
	RdString *str = ptr;

	String size = Integer_ToString(str->len);
	Logger_Debug(packet->sender->logger, $("Buffer sent (% bytes)"), size.rd);
	String_Destroy(&size);

	scall(OnSent, packet, true);
}

static sdef(void, OnFileSent, Instance inst, __unused void *ptr) {
	assert(ptr != NULL);
	assert(Instance_IsValid(inst));

	RequestPacket *packet = inst.addr;

	Logger_Debug(packet->sender->logger, $("File sent"));

	/* File transfers don't require flushing. */
	scall(OnSent, packet, false);
}

/* This method is called by the user (TemplateResponse() etc.) to initiate the
 * transmission.
 */

def(void, Flush) {
	Logger_Debug(this->logger, $("Received flush request."));

	if (!RequestPacket_IsReady(this->packets.first)) {
		Logger_Debug(this->logger, $("But first packet is not ready."));
		return;
	}

	/* We don't send the data right away using SocketSession_Write() but rather
	 * inject a push request to keep the code simpler as the resource's action
	 * (where this function call can be traced back to) should complete first.
	 * Otherwise we could run into a problem when the resource is destroyed in
	 * OnSent() and the action tries to access a resource's member variable.
	 *
	 * Even if the client is not in a state of receiving data, enqueuing a
	 * `fake' push request won't pose any problems as the socket is non-blocking
	 * and just returns EAGAIN if the client isn't ready. When the client can
	 * finally receive the sequel of the data, we receive a `real' push request
	 * and just send the rest of the response accordingly.
	 */

	Server_Client_Push(this->client);
}

/* This method is called when we receive a `push' request from the client.
 *
 * Packets cannot be sent in an arbitrary order. We have to start with the first
 * packet and process the rest step by step. If the first packet is not ready
 * yet, we have to wait.
 */
def(void, Continue) {
	/* Before we can move over to the next response, any remaining data
	 * belonging to the current response must be sent to the client first.
	 */
	if (!SocketSession_IsIdle(&this->session)) {
		Logger_Debug(this->logger, $("Complete existing session first."));
		SocketSession_Continue(&this->session);

		if (!SocketSession_IsIdle(&this->session)) {
			Logger_Debug(this->logger,
				$("Existing session still contains data."));
			return;
		} else {
			Logger_Debug(this->logger,
				$("Existing successfully sent. Continuing with next packet..."));
		}
	}

	if (this->packets.first == NULL) {
		Logger_Debug(this->logger, $("No requests are left for processing."));
		return;
	}

	if (!RequestPacket_IsReady(this->packets.first)) {
		Logger_Debug(this->logger, $("Packet is not ready."));
		return;
	}

	SocketSession_Write(&this->session,
		Response_GetHeaders(&this->packets.first->response),
		SocketSession_OnDone_For(this->packets.first, ref(OnHeadersSent)));
}
