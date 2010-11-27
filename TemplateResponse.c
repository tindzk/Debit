#import "TemplateResponse.h"

typedef void Render(void *, String *);

void TemplateResponse(ResponseInstance resp, void *ptr, void *context) {
	String out = HeapString(0);
	((Render *) ptr)(context, &out);

	Response_SetBufferBody(resp, out);
	Response_SetContentType(resp, String("text/html; charset=utf-8"));
}
