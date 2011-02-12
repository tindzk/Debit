#import "TemplateResponse.h"

void TemplateResponse(ResponseInstance resp, Template tpl) {
	String out = $("");

	callback(tpl, &out);

	Response_SetBufferBody(resp, out);
	Response_SetContentType(resp, $("text/html; charset=utf-8"));
}
