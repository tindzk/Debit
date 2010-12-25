#import "TemplateResponse.h"

void TemplateResponse(ResponseInstance resp, Template tpl) {
	String out = HeapString(0);
	callback(tpl, &out);

	Response_SetBufferBody(resp, out);
	Response_SetContentType(resp, $("text/html; charset=utf-8"));
}
