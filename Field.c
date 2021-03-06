#import "Field.h"

RdString Field_GetValue(RdString val, RdString field, FormErrors **err) {
	if (val.len == 0) {
		if (err) FormErrors_Push(err,
			(FormError) {
				.field = field,
				.msg   = $("empty")
			}
		);
	}

	return val;
}

#define DefineGetInt(type, full)                                           \
	type Field_Get##full(RdString val, RdString field, FormErrors **err) { \
		if (val.len == 0) {                                                \
			if (err) FormErrors_Push(err,                                  \
				(FormError) {                                              \
					.field = field,                                        \
					.msg   = $("empty")                                    \
				}                                                          \
			);                                                             \
		}                                                                  \
		type res = 0;                                                      \
		try {                                                              \
			res = full##_Parse(val);                                       \
		} catch(Integer, Underflow) {                                      \
			if (err) FormErrors_Push(err,                                  \
				(FormError) {                                              \
					.field = field,                                        \
					.msg   = $("too small")                                \
				}                                                          \
			);                                                             \
		} catch(Integer, Overflow) {                                       \
			if (err) FormErrors_Push(err,                                  \
				(FormError) {                                              \
					.field = field,                                        \
					.msg   = $("too large")                                \
				}                                                          \
			);                                                             \
		} finally {                                                        \
                                                                           \
		} tryEnd;                                                          \
                                                                           \
		return res;                                                        \
	}

DefineGetInt(s8,   Int8 );
DefineGetInt(u8,  UInt8 );
DefineGetInt(s32,  Int32);
DefineGetInt(u32, UInt32);

bool Field_IsSurroundedBySpaces(RdString s) {
	fwd(i, s.len) {
		if (Char_IsSpace(s.buf[i])) {
			return true;
		} else {
			break;
		}
	}

	bwd(i, s.len) {
		if (Char_IsSpace(s.buf[i])) {
			return true;
		} else {
			break;
		}
	}

	return false;
}

bool Field_IsValidEmail(RdString s) {
	ssize_t pos = String_Find(s, '@');

	if (pos == String_NotFound) {
		return false;
	}

	if (pos == 0 || (size_t) pos + 1 >= s.len) {
		return false;
	}

	RdString local  = String_Slice(s, 0, pos - 1);
	RdString domain = String_Slice(s, pos + 1);

	return local.len  > 0
		&& domain.len > 0;
}
