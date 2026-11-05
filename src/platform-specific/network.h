/*
  Network functionality
*/
#ifndef NETWORK_H
#define NETWORK_H

#include <fujinet-int.h>

uint8_t getResponse(const char *devicespec, void *buf, uint16_t len);

#endif /* NETWORK_H */
