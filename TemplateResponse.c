#import "TemplateResponse.h"

void TemplateResponse(ResponseInstance resp, Template tpl) {
	String out = String_New(0);

	callback(tpl, &out);

	Response_SetBufferBody (resp, String_ToCarrier(out));
	Response_SetContentType(resp, String_ToCarrier($$("text/html; charset=utf-8")));
}
