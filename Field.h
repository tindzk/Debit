#import <Array.h>
#import <String.h>
#import <Integer.h>

record(FormError) {
	RdString field;
	RdString msg;
};

Array(FormError, FormErrors);

RdString Field_GetValue(RdString val, RdString field, FormErrors **err);

s8  Field_GetInt8(RdString val, RdString field, FormErrors **err);
u8 Field_GetUInt8(RdString val, RdString field, FormErrors **err);

s32  Field_GetInt32(RdString val, RdString field, FormErrors **err);
u32 Field_GetUInt32(RdString val, RdString field, FormErrors **err);

bool Field_IsSurroundedBySpaces(RdString s);
bool Field_IsValidEmail(RdString s);
