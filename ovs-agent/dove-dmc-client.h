#ifndef DOVE_DMC_CLIENT_H
#define DOVE_DMC_CLIENT_H 1

int dove_dmc_client_init(const char *dmcIP, char *switchIP);
void dove_dmc_client_cleanup();
int dove_dmc_get_leader(const char *dmcIP, struct in_addr *ip, short *port);
int dove_dmc_send_hb(const char *dmcIP);

#endif //DOVE_DMC_CLIENT_H
