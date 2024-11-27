#define GRINCH_SIGNATURE	":GRINCH:"

struct grinch_header {
	char signature[8];
	union {
		u64 __gcov_info_head;
		void *gcov_info_head;
	};
} __attribute__((packed));
