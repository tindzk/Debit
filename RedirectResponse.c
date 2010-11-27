#import "RedirectResponse.h"

void RedirectResponse(ResponseInstance resp, String location) {
	Response_SetStatus(resp, HTTP_Status_Redirection_Found);
	Response_SetLocation(resp, location);
}
