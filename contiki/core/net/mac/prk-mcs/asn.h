#ifndef __ASN_H__
#define __ASN_H__

#include <stdint.h>

/* the ASN is an absolute slot number over 5 bytes */
struct asn_t {
    uint32_t ls4b;
    uint8_t ms1b;
};

/* for quick modulo operation on ASN */
struct asn_divisor_t {
	uint16_t val;
	uint16_t asn_ms1b_remainder;
};

/*-------------- Macros --------------*/
#define ASN_INIT(asn, ms1b_, ls4b_) do { \
    (asn).ms1b = (ms1b_); \
    (asn).ls4b = (ls4b_); \
} while(0);

#define ASN_INC(asn, inc) do { \
    uint32_t new_ls4b = (asn).ls4b + (inc); \
    if(new_ls4b < (asn).ls4b) { \
        (asn).ms1b++; \
    } \
    (asn).ls4b = new_ls4b; \
} while(0);

#define ASN_DEC(asn, dec) do { \
    uint32_t new_ls4b = (asn).ls4b - (dec); \
    if(new_ls4b > (asn).ls4b) { \
        (asn).ms1b--; \
    } \
    (asn).ls4b = new_ls4b; \
} while(0);

/* note that this only returns the 32-bit diff */
#define ASN_DIFF(asn1, asn2) ((asn1).ls4b - (asn2).ls4b)

#define ASN_DIVISOR_INIT(div, val_) do { \
    (div).val = (val_); \
    (div).asn_ms1b_remainder = ((0xffffffff % (val_)) + 1) % (val_); \
} while(0);

#define ASN_MOD(asn, div) \
    ((uint16_t)((asn).ls4b % (div).val) \
     + (uint16_t)((asn).ms1b * (div).asn_ms1b_remainder % (div).val)) \
    % (div).val 

#endif /* __ASN_H__ */
