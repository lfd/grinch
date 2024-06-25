#define VMGRINCH_SIGNATURE	"VMGRINCH"

struct vmgrinch_header {
	char signature[8];
	union {
		u64 __entry;
		void *entry;
	};
} __attribute__((packed));
