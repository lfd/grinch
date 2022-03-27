/* IO mappers */
void *ioremap(paddr_t paddr, size_t size);
int iounmap(const void *vaddr, size_t size);
