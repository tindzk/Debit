#import <Array.h>
#import <String.h>
#import <Integer.h>

record(FormError) {
	ProtString field;
	ProtString msg;
};

Array(FormError, FormErrors);

ProtString Field_GetValue(ProtString val, ProtString field, FormErrors **err);

s8  Field_GetInt8(ProtString val, ProtString field, FormErrors **err);
u8 Field_GetUInt8(ProtString val, ProtString field, FormErrors **err);

s32  Field_GetInt32(ProtString val, ProtString field, FormErrors **err);
u32 Field_GetUInt32(ProtString val, ProtString field, FormErrors **err);

bool Field_IsSurroundedBySpaces(ProtString s);
bool Field_IsValidEmail(ProtString s);
