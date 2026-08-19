#ifndef I386_PAGING_H
#define I386_PAGING_H

#include <stdint.h>

void paging_initialize(void);
void paging_enable(void);
void paging_map_user_range(uint32_t start_addr, uint32_t end_addr);

#endif // I386_PAGING_H
