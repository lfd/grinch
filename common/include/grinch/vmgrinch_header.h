#define VMGRINCH_SIGNATURE	"VMGRINCH"

struct vmgrinch_header {
	char signature[8];
	union {
		u64 __entry;
		void *entry;
	};
	union {
		u64 __gcov_info_head;
		void *gcov_info_head;
	};
} __attribute__((packed));
