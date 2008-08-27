#pragma once

int init_network();
int deinit_network();
int network_send(unsigned char *data, int length);
unsigned char *network_receive(int *length);
